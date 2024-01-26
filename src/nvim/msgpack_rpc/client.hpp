#pragma once

#include "asio.hpp"
#include "msgpack.hpp"

#include "tsqueue.hpp"

#include <iostream>
#include <string>
#include <vector>

struct Client {
private:
  Client& self = *this;

  template <typename T>
  struct RequestMsg {
    uint8_t type = 0;
    uint32_t msgid;
    std::string method;
    T params;
    MSGPACK_DEFINE(type, msgid, method, params);
  };

  template <typename T>
  struct NotificationMsg {
    uint8_t type = 0;
    std::string method;
    T params;
    MSGPACK_DEFINE(type, method, params);
  };

  struct RespondMsg {
    int8_t type;
    uint32_t msgid;
    msgpack::object error;
    msgpack::object result;
    MSGPACK_DEFINE(type, msgid, error, result);
  };

  asio::io_context context;
  asio::ip::tcp::socket socket;
  std::thread contextThr;
  // std::thread writeThr;
  bool exit = false;

public:
  Client(const std::string& host, const uint16_t port)
      : socket(self.context), getBuffer(1024 * 100) {
    asio::error_code ec;
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address(host), port);
    self.socket.connect(endpoint, ec);

    if (ec) {
      std::cerr << "Failed to connect to address:\n" << ec.message() << std::endl;
      self.socket.close();
      return;
    }

    asio::io_context::work idleWork(self.context);
    self.contextThr = std::thread([&]() { self.context.run(); });

    self.GetData();
  }

  ~Client() {
    self.Disconnect();
  }

  void Disconnect() {
    if (self.IsConnected()) self.socket.close();

    self.context.stop();
    if (self.contextThr.joinable()) self.contextThr.join();

    exit = true;
  }

  bool IsConnected() {
    return self.socket.is_open();
  }

  template <typename... Args>
  msgpack::object_handle Call(const std::string& method, Args... args) {
  }

  template <typename... Args>
  // std::future<msgpack::object_handle>
  void
  AsyncCall(const std::string& func_name, Args... args) {
    RequestMsg msg{
      .msgid = self.Msgid(),
      .method = func_name,
      .params = std::tuple(args...),
    };
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, msg);
    self.QueueWrite(buffer);
  }

  template <typename... Args>
  void Send(const std::string& func_name, Args... args);

private:
  std::vector<char> getBuffer;

  void GetData() {
    socket.async_read_some(
      asio::buffer(getBuffer.data(), getBuffer.size()),
      [&](std::error_code ec, std::size_t length) {
        if (!ec) {
          std::cout << "\n\nRead " << length << " bytes\n\n";
          auto handle = msgpack::unpack(getBuffer.data(), length);

          RespondMsg data = handle.get().convert();
          std::cout << "type: " << data.type << "\n";
          std::cout << "msgid: " << data.msgid << "\n";
          std::cout << "error: " << data.error << "\n";
          std::cout << "result: " << data.result << "\n";

          self.GetData();
        }
        std::cout << "---------------------- \n";
      }
    );
  }

  TSQueue<msgpack::sbuffer> msgsOut;
  uint32_t currId = 0;

  uint32_t Msgid() {
    return self.currId++;
  }

  // write loop, call in own thread
  void Write() {
    if (!self.IsConnected()) return;

    std::cout << "Size: " << self.msgsOut.size() << std::endl;

    auto& buffer = self.msgsOut.front();
    asio::async_write(
      self.socket, asio::buffer(buffer.data(), buffer.size()),
      [&](const asio::error_code& ec, size_t bytes) {
        (void)bytes;
        if (!ec) {
          self.msgsOut.pop();
          if (self.msgsOut.size() == 0 || exit) return;
          self.Write();
        } else {
          std::cerr << "Failed to write to socket:\n" << ec.message() << std::endl;
        }
      }
    );
  }

  void QueueWrite(msgpack::sbuffer& buffer) {
    self.msgsOut.push(buffer);
    if (self.msgsOut.size() > 1) return;
    self.Write();
  }
};
