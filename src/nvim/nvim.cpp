#include "nvim.hpp"

#include "msgpack_rpc/client.hpp"
#include <fstream>
#include <future>
#include "utils/async.hpp"
#include "utils/logger.hpp"

using namespace std::chrono_literals;

std::future<bool> Nvim::ConnectStdio(const std::string& dir) {
  client = std::make_unique<rpc::Client>();

  // std::string luaInitPath = ROOT_DIR "/lua/init.lua";
  // std::string cmd = "nvim --embed --headless "
  //                 "--cmd \"set runtimepath+=" ROOT_DIR "\" "
  //                 "--cmd \"luafile " + luaInitPath + "\"";
  std::string cmd = "nvim --embed";

  if (!client->ConnectStdio(cmd, dir)) {
    co_return false;
  }

  co_await Setup();

  co_return true;
}

std::future<bool> Nvim::ConnectTcp(std::string_view host, uint16_t port) {
  client = std::make_unique<rpc::Client>();

  auto timeout = 500ms;
  auto elapsed = 0ms;
  auto delay = 50ms;
  while (elapsed < timeout) {
    if (client->ConnectTcp(host, port)) break;
    // if (client->Connect("data.cs.purdue.edu", port)) break;
    co_await AsyncSleep(delay);
    elapsed += delay;
  }

  if (!client->IsConnected()) co_return false;

  co_await Setup();

  // auto result = client->Call("nvim_get_api_info");
  // channelId = result->via.array.ptr[0].convert();
  // LOG_INFO("nvim_get_api_info: {}", channelId);
  co_return true;
}

std::future<void> Nvim::Setup() {
  std::stringstream buffer;
  co_await std::async(std::launch::async, [&buffer] {
    std::string luaInitPath = ROOT_DIR "/lua/init.lua";
    std::ifstream stream(luaInitPath);
    buffer << stream.rdbuf();
  });

  // get so exceptions get thrown
  co_await GetAll(
    SetVar("neogui", true),
    Command("runtime! ginit.vim"),
    SetClientInfo(
      "neogui",
      {
        {"major", 0},
        {"minor", 0},
        {"patch", 0},
      },
      "ui", {}, {}
    ),
    Command("set runtimepath+=" ROOT_DIR),
    ExecLua(buffer.str(), {})
  );
}

bool Nvim::IsConnected() {
  return client->IsConnected();
}

Nvim::Response Nvim::SetClientInfo(
  std::string_view name,
  MapRef version,
  std::string_view type,
  MapRef methods,
  MapRef attributes
) {
  return client->AsyncCall("nvim_set_client_info", name, version, type, methods, attributes);
}

Nvim::Response Nvim::UiAttach(int width, int height, MapRef options) {
  return client->AsyncCall("nvim_ui_attach", width, height, options);
}

Nvim::Response Nvim::UiDetach() {
  return client->AsyncCall("nvim_ui_detach");
}

Nvim::Response Nvim::UiTryResize(int width, int height) {
  return client->AsyncCall("nvim_ui_try_resize", width, height);
}

Nvim::Response Nvim::Input(std::string_view input) {
  return client->AsyncCall("nvim_input", input);
}

Nvim::Response Nvim::InputMouse(
  std::string_view button,
  std::string_view action,
  std::string_view modifier,
  int grid,
  int row,
  int col
) {
  return client->AsyncCall("nvim_input_mouse", button, action, modifier, grid, row, col);
}

Nvim::Response Nvim::ListUis() {
  return client->AsyncCall("nvim_list_uis");
}

Nvim::Response Nvim::GetOptionValue(std::string_view name, MapRef opts) {
  return client->AsyncCall("nvim_get_option_value", name, opts);
}

Nvim::Response Nvim::SetVar(std::string_view name, VariantRef value) {
  return client->AsyncCall("nvim_set_var", name, value);
}

Nvim::Response Nvim::GetVar(std::string_view name) {
  return client->AsyncCall("nvim_get_var", name);
}

Nvim::Response Nvim::ExecLua(std::string_view code, VectorRef args) {
  return client->AsyncCall("nvim_exec_lua", code, args);
}

Nvim::Response Nvim::Command(std::string_view command) {
  return client->AsyncCall("nvim_command", command);
}
