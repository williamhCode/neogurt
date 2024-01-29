#include "nvim.hpp"

Nvim::Nvim() {
  // start nvim process


  bool connected = client.Connect("localhost", 6666);
  if (!connected) {
    std::cout << "Failed to connect to nvim" << std::endl;
    exit(1);
  }
}

void Nvim::StartUi(int width, int height) {
  std::map<std::string, std::string> options;

  client.Send("nvim_ui_attach", width, height, options);
}

void Nvim::Input(std::string input) {
  client.Send("nvim_input", input);
}
