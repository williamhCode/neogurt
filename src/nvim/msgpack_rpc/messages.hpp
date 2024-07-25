#pragma once

#include "msgpack.hpp"

namespace rpc {

struct MessageType {
  enum : int {
    Request = 0,
    Response = 1,
    Notification = 2,
  };
};

template <typename T>
struct RequestOut {
  int type = MessageType::Request;
  uint32_t msgid;
  std::string_view method;
  T params;
  MSGPACK_DEFINE(type, msgid, method, params);
};

struct ResponseOut {
  int type = MessageType::Response;
  uint32_t msgid;
  msgpack::type::variant error;
  msgpack::type::variant result;
  MSGPACK_DEFINE(type, msgid, error, result);
};

template <typename T>
struct NotificationOut {
  int type = MessageType::Notification;
  std::string_view method;
  T params;
  MSGPACK_DEFINE(type, method, params);
};

struct RequestIn {
  int type;
  uint32_t msgid;
  std::string_view method;
  msgpack::object params;
  MSGPACK_DEFINE(type, msgid, method, params);
};

struct ResponseIn {
  int type;
  uint32_t msgid;
  msgpack::object error;
  msgpack::object result;
  MSGPACK_DEFINE(type, msgid, error, result);
};

struct NotificationIn {
  int type;
  std::string_view method;
  msgpack::object params;
  MSGPACK_DEFINE(type, method, params);
};

} // namespace rpc
