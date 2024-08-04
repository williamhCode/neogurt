#pragma once

#include "msgpack.hpp"

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/process/async_pipe.hpp"
#include "boost/process/child.hpp"

#include "msgpack/v3/object_decl.hpp"
#include "nvim/msgpack_rpc/messages.hpp"
#include "tsqueue.hpp"

#include <atomic>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <future>
#include <thread>
#include <expected>

namespace rpc {

struct Notification {
  std::string_view method;
  msgpack::object params;
  msgpack::unique_ptr<msgpack::zone> _zone; // holds the lifetime of the data
};

using RequestValue = std::expected<msgpack::object_handle, msgpack::object_handle>;

struct Request {
  std::string_view method;
  msgpack::object params;
  msgpack::unique_ptr<msgpack::zone> _zone; // holds the lifetime of the data
  std::promise<RequestValue> promise;

  void SetValue(auto&& value) {
    auto zone = std::make_unique<msgpack::zone>();
    msgpack::object obj(value, *zone);
    promise.set_value(msgpack::object_handle(obj, std::move(zone)));
  }

  void SetError(auto&& error) {
    auto zone = std::make_unique<msgpack::zone>();
    msgpack::object obj(error, *zone);
    promise.set_value(std::unexpected(msgpack::object_handle(obj, std::move(zone))));
  }
};

enum class ClientType {
  Unknown,
  Stdio,
  Tcp,
};

struct Client {
private:
  boost::asio::io_context context;

  ClientType clientType = ClientType::Unknown;

  // stdio
  std::unique_ptr<boost::process::async_pipe> readPipe;
  std::unique_ptr<boost::process::async_pipe> writePipe;
  boost::process::child process;

  // tcp
  std::unique_ptr<boost::asio::ip::tcp::socket> socket;

  std::thread contextThr;
  std::atomic_bool exit;

  std::unordered_map<u_int32_t, std::promise<msgpack::object_handle>> responses;
  std::mutex responsesMutex;

  TSQueue<Request> requests;
  TSQueue<Notification> notifications;

public:
  Client() = default;
  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
  ~Client();

  bool ConnectStdio(const std::string& command, const std::string& dir = {});
  bool ConnectTcp(std::string_view host, uint16_t port);

  // public functions below are thread-safe
  void Disconnect();
  bool IsConnected();

  msgpack::object_handle Call(std::string_view method, auto... args);
  std::future<msgpack::object_handle> AsyncCall(std::string_view func_name, auto... args);
  void Send(std::string_view func_name, auto... args);

  Request PopRequest();
  bool HasRequest();

  Notification PopNotification();
  bool HasNotification();

private:
  msgpack::unpacker unpacker;
  static constexpr std::size_t readSize = 1024 << 10;
  TSQueue<msgpack::sbuffer> msgsOut;
  std::atomic_uint32_t currId = 0;

  uint32_t Msgid();
  void GetData();
  void Write(msgpack::sbuffer&& buffer);
  void DoWrite();
};

} // namespace rpc

namespace rpc {

msgpack::object_handle Client::Call(std::string_view method, auto... args) {
  auto future = AsyncCall(method, args...);
  return future.get();
}

std::future<msgpack::object_handle>
Client::AsyncCall(std::string_view func_name, auto... args) {
  if (!IsConnected()) return {};

  RequestOut msg{
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

void Client::Send(std::string_view func_name, auto... args) {
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
