#include "nvim.hpp"

Nvim::Nvim(bool debug) {
  // start nvim process
  if (!debug) {
    std::string command = "nvim --listen localhost:6666 --headless";
    nvimProcess = std::make_unique<TinyProcessLib::Process>(command, "", nullptr);
  }

  using namespace std::chrono_literals;
  auto timeout = 500ms;
  auto elapsed = 0ms;
  auto delay = 50ms;
  while (elapsed < timeout) {
    bool connected = client.Connect("localhost", 6666);
    if (connected) {
      break;
    }
    std::this_thread::sleep_for(delay);
    elapsed += delay;
  }

  if (client.IsConnected()) {
    std::cout << "Connected to nvim" << std::endl;
  } else {
    std::cout << "Failed to connect to nvim" << std::endl;
  }
}

Nvim::~Nvim() {
  if (nvimProcess) nvimProcess->kill(true);
}

void Nvim::StartUi(int width, int height) {
  std::map<std::string, bool> options{
    {"rgb", true},
    {"ext_hlstate", true},
    {"ext_linegrid", true},
    {"ext_multigrid", true},
  };

  client.Send("nvim_ui_attach", width, height, options);
}

void Nvim::Input(std::string input) {
  client.Send("nvim_input", input);
}
