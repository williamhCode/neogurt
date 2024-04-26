#include "nvim.hpp"
#include "parse.hpp"
#include "utils/logger.hpp"
#include <thread>

Nvim::Nvim(bool _debug) : debug(_debug) {
  // start nvim process
  uint16_t port = 6666;
  if (!debug) {
    sessionManager.LoadSessions(ROOT_DIR "/sessions.txt");
    port = sessionManager.GetOrCreateSession("default");
    LOG_INFO("Using port: {}", port);
  }

  using namespace std::chrono_literals;
  auto timeout = 500ms;
  auto elapsed = 0ms;
  auto delay = 50ms;
  while (elapsed < timeout) {
    if (client.Connect("localhost", port)) break;
    std::this_thread::sleep_for(delay);
    elapsed += delay;
  }

  if (client.IsConnected()) {
    // std::cout << "Connected to nvim" << std::endl;
    LOG_INFO("Connected to nvim");
  } else {
    LOG_ERR("Failed to connect to nvim");
  }

  SetClientInfo(
    "NeovimGui",
    {
      {"major", 0},
      {"minor", 1},
      {"patch", 0},
    },
    "ui", {}, {}
  );
}

Nvim::~Nvim() {
  client.Disconnect();
  if (!debug) {
    sessionManager.SaveSessions(ROOT_DIR "/sessions.txt");
  }
}

void Nvim::SetClientInfo(
  const std::string& name,
  const std::map<std::string, variant>& version,
  const std::string& type,
  const std::map<std::string, variant>& methods,
  const std::map<std::string, std::string>& attributes
) {
  client.Send("nvim_set_client_info", name, version, type, methods, attributes);
}

void Nvim::SetVar(const std::string& name, const variant& value) {
  client.Send("nvim_set_var", name, value);
}

void Nvim::UiAttach(
  int width, int height, const std::map<std::string, variant>& options
) {
  client.Send("nvim_ui_attach", width, height, options);
}

void Nvim::UiDetach() {
  client.Send("nvim_ui_detach");
}

void Nvim::UiTryResize(int width, int height) {
  client.Send("nvim_ui_try_resize", width, height);
}

void Nvim::Input(const std::string& input) {
  client.Send("nvim_input", input);
}

void Nvim::InputMouse(
  const std::string& button,
  const std::string& action,
  const std::string& modifier,
  int grid,
  int row,
  int col
) {
  client.Send("nvim_input_mouse", button, action, modifier, grid, row, col);
}

void Nvim::NvimListUis() {
  auto response = client.AsyncCall("nvim_list_uis");

  std::thread([response = std::move(response)]() mutable {
    auto result = response.get();
    LOG_INFO("nvim_list_uis: {}", ToString(result.get()));
  }).detach();
}

void Nvim::ParseRedrawEvents() {
  ::ParseRedrawEvents(client, redrawState);
}
