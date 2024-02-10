#pragma once

#include "asio.hpp"
#include "msgpack.hpp"
// #include "GLFW/glfw3.h"

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
    msgpack::object_handle params;
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
    contextThr = std::thread([this]() { context.run(); });

    return true;
  }

  void Disconnect() {
    exit = true;
    // glfwPostEmptyEvent();
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
  // TODO: make size dynamic if needed
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
          // std::cout << "\n\n----------------------------------\n";
          // std::cout << "Read " << length << " bytes\n";
          unpacker.buffer_consumed(length);

          static int count = 0;

          msgpack::object_handle obj;
          while (unpacker.next(obj)) {
            // sometimes doesn't get array for some reason probably nvim issue
            if (obj.get().type != msgpack::type::ARRAY) {
              std::cout << "Not an array" << std::endl;
              continue;
            }
            // std::cout << "----------------------------------\n";
            int type = obj->via.array.ptr[0].convert();

            if (type == MessageType::Response) {
              Response msg(obj->convert());

              // std::cout << "\n\n--------------------------------- " << count << std::endl;
              // std::cout << "msgid: " << msg.msgid << std::endl;
              // std::cout << "error: " << msg.error << std::endl;
              // std::cout << "result: " << msg.result << std::endl;

              auto it = responses.find(msg.msgid);
              if (it != responses.end()) {
                if (msg.error.is_nil()) {
                  it->second.set_value(
                    msgpack::object_handle(msg.result, std::move(obj.zone()))
                  );

                } else {
                  // TODO: do something with error
                  // it->second.set_exception(std::make_exception_ptr(
                  //   std::runtime_error(msg.error.via.array.ptr[1].as<std::string>())
                  // ));
                  // std::cout << "ERROR_TYPE: " << msg.error.via.array.ptr[0] <<
                  // "\n"; std::cout << "ERROR: " << msg.error.via.array.ptr[1] <<
                  // "\n";
                }
                {
                  std::scoped_lock lock(responsesMutex);
                  responses.erase(it);
                }

              } else {
                std::cout << "Unknown msgid: " << msg.msgid << "\n";
              }

            } else if (type == MessageType::Notification) {
              NotificationIn msg(obj->convert());

              // std::cout << "\n\n--------------------------------- " << count << std::endl;
              // std::cout << "method: " << msg.method << std::endl;
              // std::cout << "params: " << msg.params << std::endl;

              msgsIn.Push(NotificationData{
                .method = msg.method,
                .params = msgpack::object_handle(msg.params, std::move(obj.zone())),
              });
            } else {
              std::cout << "Unknown type: " << type << "\n";
            }

            count++;
          }
          GetData();

        } else if (ec == asio::error::eof) {
          std::cout << "The server closed the connection\n";
          Disconnect();

        } else {
          if (IsConnected()) {
            std::cout << "Error code: " << ec << std::endl;
            std::cout << "Error message: " << ec.message() << std::endl;
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

    if (msgsOut.Size() == 0 || exit) return;
    auto& buffer = msgsOut.Front();
    asio::async_write(
      socket, asio::buffer(buffer.data(), buffer.size()),
      [&](const asio::error_code& ec, size_t length) {
        (void)length;
        if (!ec) {
          msgsOut.Pop();
          DoWrite();

        } else {
          std::cerr << "Failed to write to socket:\n" << ec.message() << std::endl;
        }
      }
    );
  }
};

} // namespace rpc
