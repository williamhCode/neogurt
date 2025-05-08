#pragma once

#include <type_traits>
#include "msgpack.hpp"
#include <expected>
#include <future>

namespace rpc {

using RequestResult = std::expected<msgpack::object_handle, msgpack::object_handle>;

struct Request {
  std::string_view method;
  msgpack::object params;
  msgpack::unique_ptr<msgpack::zone> _zone; // holds the lifetime of the data
  std::promise<RequestResult> promise;

  void SetResult(const auto& result) {
    auto zone = std::make_unique<msgpack::zone>();
    msgpack::object obj(result, *zone);
    promise.set_value(msgpack::object_handle(obj, std::move(zone)));
  }

  void SetError(const auto& error) {
    auto zone = std::make_unique<msgpack::zone>();
    msgpack::object obj(error, *zone);
    promise.set_value(std::unexpected(msgpack::object_handle(obj, std::move(zone))));
  }
};

struct Notification {
  std::string_view method;
  msgpack::object params;
  msgpack::unique_ptr<msgpack::zone> _zone; // holds the lifetime of the data
};

using Message = std::variant<Request, Notification>;

}
