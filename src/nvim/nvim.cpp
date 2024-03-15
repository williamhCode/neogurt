#include "nvim.hpp"
#include "parse.hpp"

Nvim::Nvim(bool debug) {
  // start nvim process
  if (!debug) {
    // std::string command = "nvim --listen localhost:6666 --headless "
    //                       "--cmd \"let g:neovim_gui = 1\"";
    std::string command = "nvim --listen localhost:6666 --headless "
                          "--cmd \"let g:neovim_gui = 1\" $HOME/.config/nvim";
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

void Nvim::ParseEvents() {
  ParseRedrawEvents(client, redrawState);
}
