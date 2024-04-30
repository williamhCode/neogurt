#pragma once

#include "msgpack.hpp"
#include "asio/io_context.hpp"
#include "asio/ip/tcp.hpp"

#include "nvim/msgpack_rpc/messages.hpp"
#include "tsqueue.hpp"

#include <string_view>
#include <unordered_map>
#include <future>

namespace rpc {

struct Client {
private:
  asio::io_context context;
  asio::ip::tcp::socket socket{context};
  std::thread contextThr;
  std::atomic_bool exit;
  std::unordered_map<u_int32_t, std::promise<msgpack::object_handle>> responses;
  std::mutex responsesMutex;

public:
  struct NotificationData {
    std::string_view method;
    msgpack::object params;
    msgpack::unique_ptr<msgpack::zone> _zone; // holds the lifetime of the data
  };

  Client() = default;
  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
  ~Client();

  bool Connect(std::string_view host, uint16_t port);
  void Disconnect();
  bool IsConnected();

  msgpack::object_handle Call(std::string_view method, auto&&... args);
  std::future<msgpack::object_handle>
  AsyncCall(std::string_view func_name, auto&&... args);
  void Send(std::string_view func_name, auto&&... args);

  // returns next notification at front of queue
  NotificationData PopNotification();
  bool HasNotification();

private:
  msgpack::unpacker unpacker;
  static constexpr std::size_t readSize = 1024 << 10;
  TSQueue<NotificationData> msgsIn;
  TSQueue<msgpack::sbuffer> msgsOut;
  uint32_t currId = 0;

  uint32_t Msgid();
  void GetData();
  void Write(msgpack::sbuffer&& buffer);
  void DoWrite();
};

} // namespace rpc

namespace rpc {

msgpack::object_handle Client::Call(std::string_view method, auto&&... args) {
  auto future = AsyncCall(method, args...);
  return future.get();
}

std::future<msgpack::object_handle>
Client::AsyncCall(std::string_view func_name, auto&&... args) {
  if (!IsConnected()) return {};

  Request msg{
    .msgid = Msgid(),
    .method = func_name,
    .params = std::tuple(args...),
  };
  msgpack::sbuffer buffer;
  msgpack::pack(buffer, msg);
  Write(std::move(buffer));

  std::promise<msgpack::object_handle> promise;
  auto future = promise.get_future();
  {
    std::scoped_lock lock(responsesMutex);
    responses[msg.msgid] = std::move(promise);
  }

  return future;
}

void Client::Send(std::string_view func_name, auto&&... args) {
  if (!IsConnected()) return;

  // template required for Apple Clang
  NotificationOut msg{
    .method = func_name,
    .params = std::tuple(args...),
  };
  msgpack::sbuffer buffer;
  msgpack::pack(buffer, msg);
  Write(std::move(buffer));
}

} // namespace rpc
