#include "./client.hpp"
#include "boost/asio/use_awaitable.hpp"
#include "boost/asio/redirect_error.hpp"
#include "boost/asio/co_spawn.hpp"
#include "boost/asio/connect.hpp"
#include "boost/asio/detached.hpp"
#include "boost/process/io.hpp"
#include "boost/process/start_dir.hpp"
#include "boost/process/search_path.hpp"
#include "messages.hpp"
#include "msgpack/v3/object_fwd_decl.hpp"
#include "utils/logger.hpp"

namespace rpc {

Client::~Client() {
  Disconnect();

  if (clientType == ClientType::Stdio) {
    if (process.running()) process.terminate();

  } else if (clientType == ClientType::Tcp) {
    if (socket->is_open()) socket->close();
  }

  for (auto& thread : contextThreads) {
    thread.join();
  }
}

bool Client::ConnectStdio(
  const std::string& command, bool interactive, const std::string& dir
) {
  clientType = ClientType::Stdio;
  readPipe = std::make_unique<bp::async_pipe>(context);
  writePipe = std::make_unique<bp::async_pipe>(context);

  auto GetEnv = [](const char* name) -> std::string {
    char* value = std::getenv(name);
    return value ? value : "";
  };

  std::string shell = GetEnv("SHELL");
  if (shell.empty()) {
    shell = bp::search_path("zsh").string();
  }

  std::vector<std::string> flags;
  if (GetEnv("TERM").empty()) {
    flags.emplace_back("-l");
  }
  if (interactive) {
    flags.emplace_back("-i");
  }
  flags.emplace_back("-c");

  process = bp::child(
    shell, flags, command,
    bp::start_dir = dir,
    bp::std_out > *readPipe,
    bp::std_in < *writePipe
  );

  if (!process.running()) {
    LOG_ERR("Failed to start process: {}", command);
    exit = true;
    return false;
  }
  exit = false;

  co_spawn(context, DoRead(), asio::detached);
  co_spawn(context, DoWrite(), asio::detached);

  contextThreads.emplace_back([this]() { context.run(); });
  contextThreads.emplace_back([this]() { context.run(); });

  return true;
}

bool Client::ConnectTcp(std::string_view host, uint16_t port) {
  clientType = ClientType::Tcp;
  socket = std::make_unique<asio::ip::tcp::socket>(context);

  boost::system::error_code ec;
  asio::ip::tcp::resolver resolver(context);
  auto endpoints = resolver.resolve(host, std::to_string(port), ec);
  asio::connect(*socket, endpoints, ec);

  if (ec) {
    socket->close();
    exit = true;
    return false;
  }
  exit = false;

  co_spawn(context, DoRead(), asio::detached);
  co_spawn(context, DoWrite(), asio::detached);

  contextThreads.emplace_back([this]() { context.run(); });
  contextThreads.emplace_back([this]() { context.run(); });

  return true;
}

void Client::Disconnect() {
  exit = true;
  msgsOut.Exit();
}

bool Client::IsConnected() {
  return !exit;
}

Request Client::PopRequest() {
  auto req = std::move(requests.Front());
  requests.Pop();
  return req;
}

bool Client::HasRequest() {
  return !requests.Empty();
}

Notification Client::PopNotification() {
  auto msg = std::move(notifications.Front());
  notifications.Pop();
  return msg;
}

bool Client::HasNotification() {
  return !notifications.Empty();
}

uint32_t Client::Msgid() {
  return currId++;
}

asio::awaitable<void> Client::DoRead() {
  unpacker.reserve_buffer(readSize);

  while (IsConnected()) {
    auto buffer = asio::buffer(unpacker.buffer(), readSize);

    boost::system::error_code ec;
    std::size_t length;

    if (clientType == ClientType::Stdio) {
      length = co_await readPipe->async_read_some(
        buffer, asio::redirect_error(asio::use_awaitable, ec)
      );
    } else if (clientType == ClientType::Tcp) {
      length = co_await socket->async_read_some(
        buffer, asio::redirect_error(asio::use_awaitable, ec)
      );
    }

    if (ec) {
      if (ec == asio::error::eof) {
        LOG_INFO("Client::DoRead: The server closed the connection");
        Disconnect();
        co_return;

      } else {
        LOG_ERR("Client::DoRead: Error - {}", ec.message());
        continue;
      }
    }

    unpacker.buffer_consumed(length);

    msgpack::object_handle handle;
    while (unpacker.next(handle)) {
      const auto& obj = handle.get();
      if (obj.type != msgpack::type::ARRAY) {
        LOG_ERR("Client::DoRead: Not an array");
        continue;
      }

      int type = obj.via.array.ptr[0].convert();
      if (type == MessageType::Request) {
        RequestIn request(obj.convert());

        std::promise<RequestValue> promise;
        auto future = promise.get_future();

        requests.Push(Request{
          .method = request.method,
          .params = request.params,
          ._zone = std::move(handle.zone()),
          .promise = std::move(promise),
        });

        std::thread([this, msgid = request.msgid, future = std::move(future)] mutable {
          ResponseOut msg;

          auto result = future.get();
          if (result) {
            msg.msgid = msgid;
            msg.result = (*result).get();
          } else {
            msg.msgid = msgid;
            msg.error = result.error().get();
          }

          msgpack::sbuffer buffer;
          msgpack::pack(buffer, msg);
          Write(std::move(buffer));
        }).detach();

      } else if (type == MessageType::Response) {
        ResponseIn response(obj.convert());
        {
          std::scoped_lock lock(responsesMutex);

          if (auto it = responses.find(response.msgid); it != responses.end()) {
            auto& promise = it->second;
            if (response.error.is_nil()) {
              promise.set_value(
                msgpack::object_handle(response.result, std::move(handle.zone()))
              );
            } else {
              promise.set_exception(std::make_exception_ptr(std::runtime_error(
                "rpc::Client response error: " +
                response.error.via.array.ptr[1].as<std::string>()
              )));
            }
            responses.erase(it);

          } else {
            LOG_ERR("Client::DoRead: Response not found for msgid: {}", response.msgid);
          }
        }

      } else if (type == MessageType::Notification) {
        NotificationIn notification(obj.convert());
        notifications.Push(Notification{
          .method = notification.method,
          .params = notification.params,
          ._zone = std::move(handle.zone()),
        });

      } else {
        LOG_WARN("Client::DoRead: Unknown type: {}", type);
      }
    }

    if (unpacker.buffer_capacity() < readSize) {
      unpacker.reserve_buffer(readSize);
    }
  }

  co_return;
}

void Client::Write(msgpack::sbuffer&& buffer) {
  msgsOut.Push(std::move(buffer));
}

asio::awaitable<void> Client::DoWrite() {
  while (msgsOut.Wait()) {
    auto& msgBuffer = msgsOut.Front();
    auto buffer = asio::buffer(msgBuffer.data(), msgBuffer.size());

    boost::system::error_code ec;

    if (clientType == ClientType::Stdio) {
      co_await writePipe->async_write_some(
        buffer, asio::redirect_error(asio::use_awaitable, ec)
      );
    } else if (clientType == ClientType::Tcp) {
      co_await socket->async_write_some(
        buffer, asio::redirect_error(asio::use_awaitable, ec)
      );
    }

    if (ec) {
      LOG_ERR("Client::DoWrite: {}", ec.message());
    }

    msgsOut.Pop();
  }

  co_return;
}

} // namespace rpc
