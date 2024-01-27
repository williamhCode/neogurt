#pragma once

#include "asio.hpp"
#include "msgpack.hpp"
#include "GLFW/glfw3.h"

#include "tsqueue.hpp"

#include <iostream>
#include <string>
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

    asio::io_context::work idleWork(context);
    contextThr = std::thread([&]() { context.run(); });

    GetData();
  }

  ~Client() {
    if (socket.is_open()) socket.close();

    context.stop();
    if (contextThr.joinable()) contextThr.join();
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
  // std::future<msgpack::object_handle>
  void AsyncCall(const std::string& func_name, Args... args) {
    if (!IsConnected()) return;

    RequestMessage msg{
      .msgid = Msgid(),
      .method = func_name,
      .params = std::tuple(args...),
    };
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, msg);
    QueueWrite(buffer);
  }

  template <typename... Args>
  void Send(const std::string& func_name, Args... args) {
    if (!IsConnected()) return;

    NotificationMessageOut msg{
      .method = func_name,
      .params = std::tuple(args...),
    };
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, msg);
    QueueWrite(buffer);
  }

private:
  msgpack::unpacker unpacker;
  static constexpr std::size_t readSize = 1024 * 100;
  TSQueue<msgpack::object_handle> msgsIn;
  TSQueue<msgpack::sbuffer> msgsOut;
  uint32_t currId = 0;

  void GetData() {
    socket.async_read_some(
      asio::buffer(unpacker.buffer(), readSize),
      [&](asio::error_code ec, std::size_t length) {
        if (!ec) {
          std::cout << "\n\n----------------------------------\n";
          std::cout << "Read " << length << " bytes\n";
          unpacker.buffer_consumed(length);

          msgpack::object_handle result;
          while (unpacker.next(result)) {
            std::cout << "----------------------------------\n";

            msgpack::object obj(result.get());
            int type = obj.via.array.ptr[0].convert();

            if (type == MessageType::Response) {
              RespondMessage msg = obj.convert();
              std::cout << "msgid: " << msg.msgid << "\n";
              std::cout << "error: " << msg.error << "\n";
              std::cout << "result: " << msg.result << "\n";
            } else if (type == MessageType::Notification) {
              NotificationMessageIn msg = obj.convert();
              std::cout << "method: " << msg.method << "\n";
              std::cout << "params: " << msg.params << "\n";
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

  uint32_t Msgid() {
    return currId++;
  }

  // write loop, call in own thread
  void Write() {
    if (!IsConnected()) return;

    auto& buffer = msgsOut.front();
    asio::async_write(
      socket, asio::buffer(buffer.data(), buffer.size()),
      [&](const asio::error_code& ec, size_t bytes) {
        (void)bytes;
        if (!ec) {
          msgsOut.pop();
          if (msgsOut.size() == 0 || exit) return;
          Write();
        } else {
          std::cerr << "Failed to write to socket:\n" << ec.message() << std::endl;
        }
      }
    );
  }

  void QueueWrite(msgpack::sbuffer& buffer) {
    msgsOut.push(buffer);
    if (msgsOut.size() > 1) return;
    Write();
  }
};
