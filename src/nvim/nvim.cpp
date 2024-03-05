#include "nvim.hpp"
#include "msgpack/v3/adaptor/boost/msgpack_variant_decl.hpp"
#include "parse.hpp"
#include <vector>

Nvim::Nvim(bool debug) {
  // start nvim process
  if (!debug) {
    std::string command = "nvim --listen localhost:6666 --headless $HOME/.config/nvim";
    // std::string command = "nvim --listen localhost:6666 --headless";
    nvimProcess = std::make_unique<TinyProcessLib::Process>(command, "", nullptr);
  }

  using namespace std::chrono_literals;
  auto timeout = 500ms;
  auto elapsed = 0ms;
  auto delay = 50ms;
  while (elapsed < timeout) {
    if (client.Connect("localhost", 6666)) break;
    std::this_thread::sleep_for(delay);
    elapsed += delay;
  }

  if (client.IsConnected()) {
    std::cout << "Connected to nvim" << std::endl;
  } else {
    std::cout << "Failed to connect to nvim" << std::endl;
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
  if (nvimProcess) nvimProcess->kill(true);
}

void Nvim::SetClientInfo(
  const std::string& name,
  const std::map<std::string, msgpack::type::variant>& version,
  const std::string& type,
  const std::map<std::string, msgpack::type::variant>& methods,
  const std::map<std::string, std::string>& attributes
) {
  client.Send("nvim_set_client_info", name, version, type, methods, attributes);
}

void Nvim::UiAttach(int width, int height) {
  std::map<std::string, bool> options{
    {"rgb", true},
    // {"ext_hlstate", true},
    {"ext_multigrid", true},
    {"ext_linegrid", true},
  };

  client.Send("nvim_ui_attach", width, height, options);
}

void Nvim::UiTryResize(int width, int height) {
  client.Send("nvim_ui_try_resize", width, height);
}

void Nvim::Input(const std::string& input) {
  client.Send("nvim_input", input);
}

void Nvim::ParseEvents() {
  ParseRedrawEvents(client, redrawState);
}
