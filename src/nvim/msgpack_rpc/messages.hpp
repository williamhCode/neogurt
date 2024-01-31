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
  struct Request {
    int type = MessageType::Request;
    uint32_t msgid;
    std::string method;
    T params;
    MSGPACK_DEFINE(type, msgid, method, params);
  };

  struct Response {
    int type;
    uint32_t msgid;
    msgpack::object error;
    msgpack::object result;
    MSGPACK_DEFINE(type, msgid, error, result);
  };

  template <typename T>
  struct NotificationOut {
    int type = MessageType::Notification;
    std::string method;
    T params;
    MSGPACK_DEFINE(type, method, params);
  };

  struct NotificationIn {
    int type;
    std::string method;
    msgpack::object params;
    MSGPACK_DEFINE(type, method, params);
  };
}
