#pragma once

#include "asio.hpp"
#include "msgpack.hpp"

#include <string>

struct Client {
  asio::io_context context;
  std::thread thrContext;

  Client() = default;
  bool Connect(const std::string& address, const uint16_t port);

  template <typename... Args>
  msgpack::object_handle Call(const std::string& method, Args... args);

  template <typename... Args>
  std::future<msgpack::object_handle>
  AsyncCall(const std::string& func_name, Args... args);

  template <typename... Args>
  void Send(const std::string& func_name, Args... args);
};
