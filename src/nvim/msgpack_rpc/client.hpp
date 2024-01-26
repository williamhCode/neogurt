#pragma once

#include "asio.hpp"
#include "msgpack.hpp"

#include <exception>
#include <iostream>
#include <string>

struct Client {
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

  // asio::error_code ec;
  asio::io_context context;
  std::thread thrContext;
  asio::ip::tcp::socket socket;

  Client(const std::string& host, const uint16_t port) : socket(self.context) {
    try {
      asio::ip::tcp::resolver resolver(self.context);
      auto endpoints = resolver.resolve(host, std::to_string(port));
      // throw std::exception();

      // asio::async_connect(
      //   self.socket, endpoints,
      //   [this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
      //     if (!ec) {
      //     }
      //   }
      // );

      // asio::error_code ec;
      // asio::ip::tcp::endpoint endpoint(asio::ip::make_address("localhost", ec), port);
      // self.socket.connect(endpoint, ec);
      // if (ec) {
      //   std::cerr << "Failed to connect to address:\n" << ec.message() << std::endl;
      //   self.socket.close();
      // }

      // self.thrContext = std::thread([&]() { self.context.run(); });

    } catch (std::exception& e) {
      std::cerr << "Client Exception: " << e.what() << "\n";
    }
  }

  ~Client() {
    // self.Disconnect();
  }

  void Disconnect() {
    // if (self.IsConnected()) asio::post(self.context, [this]() { self.socket.close(); });
    self.context.stop();
    // if (self.thrContext.joinable()) self.thrContext.join();
  }

  bool IsConnected() {
    return self.socket.is_open();
  }

  template <typename... Args>
  msgpack::object_handle Call(const std::string& method, Args... args);

  template <typename... Args>
  std::future<msgpack::object_handle>
  AsyncCall(const std::string& func_name, Args... args);

  template <typename... Args>
  void Send(const std::string& func_name, Args... args);
};
