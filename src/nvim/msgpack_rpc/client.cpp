#include "client.hpp"

#include "asio/connect.hpp"
#include "asio/write.hpp"
#include "msgpack.hpp"

#include "messages.hpp"
#include "utils/logger.hpp"
#include <thread>

namespace rpc {

Client::~Client() {
  if (socket.is_open()) socket.close();

  if (contextThr.joinable()) contextThr.join();
  context.stop();
}

bool Client::Connect(std::string_view host, uint16_t port) {
  asio::error_code ec;
  asio::ip::tcp::resolver resolver(context);
  auto endpoints = resolver.resolve(host, std::to_string(port), ec);
  asio::connect(socket, endpoints, ec);

  if (ec) {
    socket.close();
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

// returns next notification at front of queue
Client::NotificationData Client::PopNotification() {
  auto msg = std::move(msgsIn.Front());
  msgsIn.Pop();
  return msg;
}

bool Client::HasNotification() {
  return !msgsIn.Empty();
}

uint32_t Client::Msgid() {
  return currId++;
}

void Client::GetData() {
  if (!IsConnected()) return;

  socket.async_read_some(
    asio::buffer(unpacker.buffer(), readSize),
    [this](asio::error_code ec, std::size_t length) {
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
          if (type == MessageType::Response) {
            ResponseIn msg(obj.convert());

            decltype(responses)::iterator it;
            bool found;
            {
              std::scoped_lock lock(responsesMutex);
              it = responses.find(msg.msgid);
              found = it != responses.end();
            }
            if (found) {
              auto& promise = it->second;
              if (msg.error.is_nil()) {
                promise.set_value(
                  msgpack::object_handle(msg.result, std::move(handle.zone()))
                );

              } else {
                promise.set_exception(std::make_exception_ptr(std::runtime_error(
                  "rpc::Client response error: " +
                  msg.error.via.array.ptr[1].as<std::string>()
                )));
              }

              {
                std::scoped_lock lock(responsesMutex);
                responses.erase(it);
              }

            } else {
              LOG_WARN("Client::GetData: Response not found for msgid: {}", msg.msgid);
            }

          } else if (type == MessageType::Notification) {
            NotificationIn msg(obj.convert());
            msgsIn.Push(NotificationData{
              .method = msg.method,
              .params = msg.params,
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
    }
  );
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

  auto& buffer = msgsOut.Front();
  socket.async_write_some(
    asio::buffer(buffer.data(), buffer.size()),
    [this](const asio::error_code& ec, size_t /* length */) {
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
    }
  );
}

} // namespace rpc
