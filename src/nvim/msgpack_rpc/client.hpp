#pragma once

#include "asio.hpp"
#include "msgpack.hpp"
#include "GLFW/glfw3.h"

#include "tsqueue.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

struct Client {
private:
  struct MessageType {
    enum : int {
      Request = 0,
      Response = 1,
      Notification = 2,
    };
  };

  template <typename T>
  struct RequestMessage {
    int type = MessageType::Request;
    uint32_t msgid;
    std::string method;
    T params;
    MSGPACK_DEFINE(type, msgid, method, params);
  };

  struct RespondMessage {
    int type;
    uint32_t msgid;
    msgpack::object error;
    msgpack::object result;
    MSGPACK_DEFINE(type, msgid, error, result);
  };

  template <typename T>
  struct NotificationMessageOut {
    int type = MessageType::Notification;
    std::string method;
    T params;
    MSGPACK_DEFINE(type, method, params);
  };

  struct NotificationMessageIn {
    int type;
    std::string method;
    msgpack::object params;
    MSGPACK_DEFINE(type, method, params);
  };

  asio::io_context context;
  asio::ip::tcp::socket socket;
  std::thread contextThr;
  std::atomic_bool exit = false;
  std::unordered_map<int, std::promise<msgpack::object_handle>> promises;

public:
  Client(const std::string& host, const uint16_t port) : socket(context) {
    asio::error_code ec;
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address(host, ec), port);
    socket.connect(endpoint, ec);
    if (ec) {
      std::cerr << "Can't connect to server: " << ec.message() << std::endl;
      exit = true;
      return;
    }

    std::cout << "Connected to " << socket.remote_endpoint() << std::endl;

    GetData();

    contextThr = std::thread([&]() { context.run(); });
  }

  ~Client() {
    if (socket.is_open()) socket.close();

    if (contextThr.joinable()) contextThr.join();
    context.stop();
  }

  void Disconnect() {
    exit = true;
    glfwPostEmptyEvent();
  }

  bool IsConnected() {
    return !exit;
  }

  template <typename... Args>
  msgpack::object_handle Call(const std::string& method, Args... args) {
  }

  template <typename... Args>
  std::future<msgpack::object_handle>
  AsyncCall(const std::string& func_name, Args... args) {
    if (!IsConnected()) return {};

    RequestMessage<std::tuple<Args...>> msg{
      .msgid = Msgid(),
      .method = func_name,
      .params = std::tuple(args...),
    };
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, msg);
    Write(buffer);

    std::promise<msgpack::object_handle> promise;
    auto future = promise.get_future();
    promises[msg.msgid] = std::move(promise);

    return future;
  }

  bool ran = false;
  template <typename... Args>
  void Send(const std::string& func_name, Args... args) {
    if (!IsConnected()) return;

    NotificationMessageOut<std::tuple<Args...>> msg{
      .method = func_name,
      .params = std::tuple(args...),
    };
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, msg);
    Write(buffer);
  }

private:
  uint32_t Msgid() {
    return currId++;
  }

  msgpack::unpacker unpacker;
  static constexpr std::size_t readSize = 1024 * 10;
  TSQueue<msgpack::object_handle> msgsIn;
  TSQueue<msgpack::sbuffer> msgsOut;
  uint32_t currId = 0;

  void GetData() {
    if (!IsConnected()) return;

    socket.async_read_some(
      asio::buffer(unpacker.buffer(), readSize),
      [&](asio::error_code ec, std::size_t length) {
        if (!ec) {
          std::cout << "\n\n----------------------------------\n";
          std::cout << "Read " << length << " bytes\n";
          unpacker.buffer_consumed(length);

          msgpack::object_handle obj;
          while (unpacker.next(obj)) {
            std::cout << "----------------------------------\n";
            int type = obj->via.array.ptr[0].convert();

            if (type == MessageType::Response) {
              RespondMessage msg = obj->convert();

              auto it = promises.find(msg.msgid);
              if (it != promises.end()) {
                if (msg.error.is_nil()) {
                  it->second.set_value(
                    msgpack::object_handle(msg.result, std::move(obj.zone()))
                  );

                } else {
                  it->second.set_exception(std::make_exception_ptr(
                    std::runtime_error(msg.error.via.array.ptr[1].as<std::string>())
                  ));
                  // std::string errStr = msg.error.via.array.ptr[1].convert();
                  // std::cout << "ERROR: " << msg.error.via.array.ptr[1] << "\n";
                  // std::cout << "ERROR TYPE: " << msg.error.via.array.ptr[1].type <<
                  // "\n";
                }
                promises.erase(it);

              } else {
                std::cout << "Unknown msgid: " << msg.msgid << "\n";
              }

              std::cout << "msgid: " << msg.msgid << "\n";
              // std::cout << "error: " << msg.error << "\n";
              std::cout << "result: " << msg.result << "\n";

            } else if (type == MessageType::Notification) {
              NotificationMessageIn msg = obj->convert();

              std::cout << "method: " << msg.method << "\n";
              std::cout << "result: " << msg.params << "\n";

            } else {
              std::cout << "Unknown type: " << type << "\n";
            }
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

  void Write(msgpack::sbuffer& buffer) {
    msgsOut.push(buffer);
    if (msgsOut.size() > 1) return;
    DoWrite();
  }

  // write loop, call in own thread
  void DoWrite() {
    if (!IsConnected()) return;

    auto& buffer = msgsOut.front();
    asio::async_write(
      socket, asio::buffer(buffer.data(), buffer.size()),
      [&](const asio::error_code& ec, size_t length) {
        (void)length;
        if (!ec) {
          msgsOut.pop();
          if (msgsOut.size() == 0 || exit) return;
          DoWrite();

        } else {
          std::cerr << "Failed to write to socket:\n" << ec.message() << std::endl;
        }
      }
    );
  }
};
