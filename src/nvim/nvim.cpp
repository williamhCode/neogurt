#include "nvim.hpp"

Nvim::Nvim() {
  // start nvim process
  std::string command = "nvim --listen localhost:6666 --headless";
  nvimProcess = std::make_unique<TinyProcessLib::Process>(command, "", nullptr);

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

  if (!client.IsConnected()) {
    std::cout << "Failed to connect to nvim" << std::endl;
  }
}

Nvim::~Nvim() {
  std::cout << "Nvim::~Nvim()" << std::endl;
  nvimProcess->kill(true);
}

void Nvim::StartUi(int width, int height) {
  std::map<std::string, std::string> options;

  client.Send("nvim_ui_attach", width, height, options);
}

void Nvim::Input(std::string input) {
  client.Send("nvim_input", input);
}
