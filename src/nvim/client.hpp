#pragma once

#include "asio.hpp"
#include <string>

struct Client {
  struct Result {
    std::string data;
    std::string error;
  };

  Client() = default;
  Client(const std::string& host, uint16_t port);
  Result Call(const std::string& method, const std::string& args);
  Result AsyncCall(const std::string& method, const std::string& args);
};
