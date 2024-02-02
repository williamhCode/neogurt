#pragma once

#include "msgpack.hpp"

namespace rpc {

struct MessageType {
  enum : int32_t {
    Request = 0,
    Response = 1,
    Notification = 2,
  };
};

template <typename T>
struct Request {
  int32_t type = MessageType::Request;
  uint32_t msgid;
  std::string method;
  T params;
  MSGPACK_DEFINE(type, msgid, method, params);
};

struct Response {
  int32_t type;
  uint32_t msgid;
  msgpack::object error;
  msgpack::object result;
  MSGPACK_DEFINE(type, msgid, error, result);
};

template <typename T>
struct NotificationOut {
  int32_t type = MessageType::Notification;
  std::string method;
  T params;
  MSGPACK_DEFINE(type, method, params);
};

struct NotificationIn {
  int32_t type;
  std::string method;
  msgpack::object params;
  MSGPACK_DEFINE(type, method, params);
};

} // namespace rpc
