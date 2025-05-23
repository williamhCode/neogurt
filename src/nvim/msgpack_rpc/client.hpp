#pragma once

#include "./message_internal.hpp"
#include "./message.hpp"

#include <type_traits>
#include "msgpack.hpp"

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/process/v1/async_pipe.hpp"
#include "boost/process/v1/child.hpp"

#include "msgpack/v3/object_decl.hpp"
#include "utils/thread.hpp"

#include <atomic>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <future>
#include <thread>
#include <expected>
#include <queue>

namespace rpc {

namespace bp = boost::process::v1;
namespace asio = boost::asio;

enum class ClientType {
  Unknown,
  Stdio,
  Tcp,
};

struct Client : std::enable_shared_from_this<Client> {
private:
  asio::io_context context;

  ClientType clientType = ClientType::Unknown;

  // stdio
  std::unique_ptr<bp::async_pipe> readPipe;
  std::unique_ptr<bp::async_pipe> writePipe;
  bp::child process;

  // tcp
  std::unique_ptr<asio::ip::tcp::socket> socket;

  std::vector<std::jthread> rwThreads;
  std::atomic_bool exit;

  std::unordered_map<u_int32_t, std::promise<msgpack::object_handle>> responses;
  std::mutex responsesMutex;

  Sync<std::queue<Message>> messages;

public:
  Client() = default;
  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
  ~Client();

  bool ConnectStdio(
    const std::string& command, bool interactive, const std::string& dir = {}
  );
  bool ConnectTcp(std::string_view host, uint16_t port);

  // public functions below are thread-safe
  void TryDisconnect();
  bool IsConnected();

  std::future<msgpack::object_handle> Call(std::string_view func_name, auto... args);
  void Send(std::string_view func_name, auto... args);

  bool HasMessage() { return !messages.lock()->empty(); }
  Message& FrontMessage() { return messages.lock()->front(); }
  void PopMessage() { messages.lock()->pop(); }

private:
  msgpack::unpacker unpacker;
  static constexpr std::size_t readSize = 1024 << 10;
  Sync<std::queue<msgpack::sbuffer>> msgsOut;
  std::condition_variable msgsOutCv;
  std::atomic_uint32_t currId = 0;

  uint32_t Msgid();
  void DoRead();
  void Write(msgpack::sbuffer&& buffer);
  void DoWrite();
};

std::future<msgpack::object_handle>
Client::Call(std::string_view func_name, auto... args) {
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
