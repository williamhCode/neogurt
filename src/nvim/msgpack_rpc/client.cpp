#include "client.hpp"

#include "boost/process/io.hpp"
#include "boost/process/start_dir.hpp"
#include "boost/asio/connect.hpp"
#include "messages.hpp"
#include "msgpack/v3/adaptor/nil_decl.hpp"
#include "msgpack/v3/object_fwd_decl.hpp"
#include "utils/logger.hpp"

namespace bp = boost::process;
namespace asio = boost::asio;

namespace rpc {

Client::~Client() {
  Disconnect();

  if (clientType == ClientType::Stdio) {
    if (readPipe->is_open()) readPipe->close();
    if (writePipe->is_open()) writePipe->close();
    if (process.running()) process.terminate();

  } else if (clientType == ClientType::Tcp) {
    if (socket->is_open()) socket->close();
  }

  if (contextThr.joinable()) contextThr.join();
  context.stop();
}

bool Client::ConnectStdio(const std::string& command, const std::string& dir) {
  clientType = ClientType::Stdio;
  readPipe = std::make_unique<bp::async_pipe>(context);
  writePipe = std::make_unique<bp::async_pipe>(context);

  // LOG_INFO("Starting process: {}, {}", command, dir);
  process = bp::child(
    command,
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

  unpacker.reserve_buffer(readSize);
  GetData();
  contextThr = std::thread([this]() { context.run(); });

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

  unpacker.reserve_buffer(readSize);
  GetData();
  contextThr = std::thread([this]() { context.run(); });

  return true;
}

void Client::Disconnect() {
  exit = true;
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

void Client::GetData() {
  if (!IsConnected()) return;

  auto buffer = asio::buffer(unpacker.buffer(), readSize);
  auto handler = [this](boost::system::error_code ec, std::size_t length) {
    if (!ec) {
      unpacker.buffer_consumed(length);

      msgpack::object_handle handle;
      while (unpacker.next(handle)) {
        const auto& obj = handle.get();
        if (obj.type != msgpack::type::ARRAY) {
          LOG_ERR("Client::GetData: Not an array");
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

            if (auto result = future.get()) {
              msg.msgid = msgid;
              msg.result = (*result).get();
            } else {
              msg.msgid = msgid;
              msg.error = result.error().get();
            }

            msgpack::sbuffer buffer;
            msgpack::pack(buffer, msg);
            Write(std::move(buffer));
          })
          .detach();

        } else if (type == MessageType::Response) {
          ResponseIn response(obj.convert());

          decltype(responses)::iterator it;
          bool found;
          {
            std::scoped_lock lock(responsesMutex);
            it = responses.find(response.msgid);
            found = it != responses.end();
          }
          if (found) {
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

            {
              std::scoped_lock lock(responsesMutex);
              responses.erase(it);
            }

          } else {
            LOG_ERR("Client::GetData: Response not found for msgid: {}", response.msgid);
          }

        } else if (type == MessageType::Notification) {
          NotificationIn notification(obj.convert());
          notifications.Push(Notification{
            .method = notification.method,
            .params = notification.params,
            ._zone = std::move(handle.zone()),
          });

        } else {
          LOG_WARN("Client::GetData: Unknown type: {}", type);
        }
      }

      if (unpacker.buffer_capacity() < readSize) {
        // LOG("Reserving extra buffer: {}", readSize);
        unpacker.reserve_buffer(readSize);
      }
      GetData();

    } else if (ec == asio::error::eof) {
      LOG_INFO("Client::GetData: The server closed the connection");
      Disconnect();

    } else {
      if (IsConnected()) {
        // std::cout << "Error code: " << ec << std::endl;
        // std::cout << "Error message: " << ec.message() << std::endl;
        LOG_ERR("Client::GetData: {}", ec.message());
      }
    }
  };

  if (clientType == ClientType::Stdio) {
    readPipe->async_read_some(buffer, handler);
  } else if (clientType == ClientType::Tcp) {
    socket->async_read_some(buffer, handler);
  }
}

void Client::Write(msgpack::sbuffer&& buffer) {
  msgsOut.Push(std::move(buffer));
  // ongoing write
  if (msgsOut.Size() > 1) return;
  DoWrite();
}

void Client::DoWrite() {
  // send all messages before exiting
  if (msgsOut.Empty()) return;

  auto& msgBuffer = msgsOut.Front();

  auto buffer = asio::buffer(msgBuffer.data(), msgBuffer.size());
  auto handler = [this](boost::system::error_code ec, size_t /* length */) {
    if (!ec) {
      if (msgsOut.Empty()) {
        LOG_WARN("Client::DoWrite: msgsOut is empty");
        return;
      }
      msgsOut.Pop();
      DoWrite();

    } else {
      LOG_ERR("Client::DoWrite: {}", ec.message());
    }
  };

  if (clientType == ClientType::Stdio) {
    writePipe->async_write_some(buffer, handler);
  } else if (clientType == ClientType::Tcp) {
    socket->async_write_some(buffer, handler);
  }
}

} // namespace rpc
