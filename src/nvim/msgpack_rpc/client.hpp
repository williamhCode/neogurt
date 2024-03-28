#pragma once

#include "asio.hpp"
#include "utils/logger.hpp"
#include "msgpack.hpp"

#include "tsqueue.hpp"
#include "messages.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <future>
#include <thread>

namespace rpc {

struct Client {
private:
  asio::io_context context;
  asio::ip::tcp::socket socket;
  std::thread contextThr;
  std::atomic_bool exit;
  std::unordered_map<int, std::promise<msgpack::object_handle>> responses;
  std::mutex responsesMutex;

public:
  struct NotificationData {
    std::string_view method;
    msgpack::object params;
    msgpack::unique_ptr<msgpack::zone> _zone; // holds the lifetime of the data
  };

  Client() : socket(context) {
  }

  ~Client() {
    if (socket.is_open()) socket.close();

    if (contextThr.joinable()) contextThr.join();
    context.stop();
  }

  bool Connect(const std::string& host, const uint16_t port) {
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
    contextThr = std::thread([this]() {
      context.run();
    });

    return true;
  }

  void Disconnect() {
    exit = true;
  }

  bool IsConnected() {
    return !exit;
  }

  msgpack::object_handle Call(const std::string& method, auto... args) {
    auto future = AsyncCall(method, args...);
    return future.get();
  }

  template <typename... Args>
  std::future<msgpack::object_handle>
  AsyncCall(const std::string& func_name, Args... args) {
    if (!IsConnected()) return {};

    // template required for Apple Clang
    Request<std::tuple<Args...>> msg{
      .msgid = Msgid(),
      .method = func_name,
      .params = std::tuple(args...),
    };
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, msg);
    Write(std::move(buffer));

    std::promise<msgpack::object_handle> promise;
    auto future = promise.get_future();
    {
      std::scoped_lock lock(responsesMutex);
      responses[msg.msgid] = std::move(promise);
    }

    return future;
  }

  template <typename... Args>
  void Send(const std::string& func_name, Args... args) {
    if (!IsConnected()) return;

    // template required for Apple Clang
    NotificationOut<std::tuple<Args...>> msg{
      .method = func_name,
      .params = std::tuple(args...),
    };
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, msg);
    Write(std::move(buffer));
  }

  // returns next notification at front of queue
  NotificationData PopNotification() {
    return msgsIn.Pop();
  }

  bool HasNotification() {
    return !msgsIn.Empty();
  }

private:
  msgpack::unpacker unpacker;
  static constexpr std::size_t readSize = 1024 << 10;
  TSQueue<NotificationData> msgsIn;
  TSQueue<msgpack::sbuffer> msgsOut;
  uint32_t currId = 0;

  uint32_t Msgid() {
    return currId++;
  }

  void GetData() {
    if (!IsConnected()) return;

    socket.async_read_some(
      asio::buffer(unpacker.buffer(), readSize),
      [&](asio::error_code ec, std::size_t length) {
        if (!ec) {
          unpacker.buffer_consumed(length);

          msgpack::object_handle handle;
          while (unpacker.next(handle)) {
            auto& obj = handle.get();
            if (obj.type != msgpack::type::ARRAY) {
              LOG_ERR("Client::GetData: Not an array");
              continue;
            }

            int type = obj.via.array.ptr[0].convert();
            if (type == MessageType::Response) {
              Response msg(obj.convert());

              auto it = responses.find(msg.msgid);
              if (it != responses.end()) {
                auto& promise = it->second;
                if (msg.error.is_nil()) {
                  promise.set_value(
                    msgpack::object_handle(msg.result, std::move(handle.zone()))
                  );

                } else {
                  // TODO: do something with error
                  // promise.set_exception(std::make_exception_ptr(
                  //   std::runtime_error(msg.error.via.array.ptr[1].as<std::string>())
                  // ));
                  std::cout << "ERROR_TYPE: " << msg.error.via.array.ptr[0] << "\n";
                  std::cout << "ERROR: " << msg.error.via.array.ptr[1] << "\n";
                  // promise.set_value(
                  //   msgpack::object_handle(msg.error, std::move(handle.zone()))
                  // );
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
            LOG_INFO("Client::GetData: {}", ec.message());
          }
        }
      }
    );
  }

  void Write(msgpack::sbuffer&& buffer) {
    msgsOut.Push(std::move(buffer));
    if (msgsOut.Size() > 1) return;
    DoWrite();
  }

  void DoWrite() {
    if (!IsConnected()) return;
    if (msgsOut.Empty() || exit) return;
    auto& buffer = msgsOut.Front();
    asio::async_write(
      socket, asio::buffer(buffer.data(), buffer.size()),
      [&](const asio::error_code& ec, size_t length) {
        (void)length;
        if (!ec) {
          if (msgsOut.Empty()) {
            LOG_WARN("Client::DoWrite: msgsOut is empty");
            return;
          }
          msgsOut.Pop();
          DoWrite();

        } else {
          LOG_INFO("Client::DoWrite: {}", ec.message());
        }
      }
    );
  }
};

} // namespace rpc
