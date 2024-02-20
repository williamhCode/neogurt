#pragma once

#include "nvim/msgpack_rpc/client.hpp"
#include "nvim/parse.hpp"
#include "process.hpp"

struct Nvim {
  rpc::Client client;
  std::unique_ptr<TinyProcessLib::Process> nvimProcess;

  RedrawState redrawState;

  Nvim(bool debug);
  ~Nvim();
  void UiAttach(int width, int height);
  void UiTryResize(int width, int height);
  void Input(std::string input);
  void ParseEvents();
};
