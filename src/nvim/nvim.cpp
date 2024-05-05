#include "nvim.hpp"
#include "utils/logger.hpp"
#include <thread>

Nvim::Nvim(std::string_view host, uint16_t port) {
  using namespace std::chrono_literals;
  auto timeout = 500ms;
  auto elapsed = 0ms;
  auto delay = 50ms;
  while (elapsed < timeout) {
    if (client.Connect(host, port)) break;
    // if (client.Connect("data.cs.purdue.edu", port)) break;
    std::this_thread::sleep_for(delay);
    elapsed += delay;
  }

  if (client.IsConnected()) {
    // std::cout << "Connected to nvim" << std::endl;
    LOG_INFO("Connected to nvim");
  } else {
    throw std::runtime_error("Failed to connect to port " + std::to_string(port));
  }

  SetClientInfo(
    "neogui",
    {
      {"major", 0},
      {"minor", 0},
      {"patch", 0},
    },
    "ui", {}, {}
  );

  // auto result = client.Call("nvim_get_api_info");
  // channelId = result->via.array.ptr[0].convert();
  // LOG_INFO("nvim_get_api_info: {}", channelId);
}

Nvim::~Nvim() {
  client.Disconnect();
}

bool Nvim::IsConnected() {
  return client.IsConnected();
}

void Nvim::SetClientInfo(
  std::string_view name,
  MapRef version,
  std::string_view type,
  MapRef methods,
  MapRef attributes
) {
  client.Send("nvim_set_client_info", name, version, type, methods, attributes);
}

void Nvim::SetVar(std::string_view name, VariantRef value) {
  client.Send("nvim_set_var", name, value);
}

void Nvim::UiAttach(int width, int height, MapRef options) {
  client.Send("nvim_ui_attach", width, height, options);
}

void Nvim::UiDetach() {
  client.Send("nvim_ui_detach");
}

void Nvim::UiTryResize(int width, int height) {
  client.Send("nvim_ui_try_resize", width, height);
}

void Nvim::Input(std::string_view input) {
  client.Send("nvim_input", input);
}

void Nvim::InputMouse(
  std::string_view button,
  std::string_view action,
  std::string_view modifier,
  int grid,
  int row,
  int col
) {
  client.Send("nvim_input_mouse", button, action, modifier, grid, row, col);
}

void Nvim::ListUis() {
  auto response = client.AsyncCall("nvim_list_uis");

  std::thread([response = std::move(response)]() mutable {
    auto result = response.get();
    LOG_INFO("nvim_list_uis: {}", ToString(result.get()));
  }).detach();
}

Nvim::Variant Nvim::GetOptionValue(std::string_view name, MapRef opts) {
  return client.Call("nvim_get_option_value", name, opts)->convert();
}
