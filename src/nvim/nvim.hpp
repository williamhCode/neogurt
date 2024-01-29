#pragma once

#include "nvim/msgpack_rpc/client.hpp"
#include "process.hpp"

struct Nvim {
  rpc::Client client;
  std::unique_ptr<TinyProcessLib::Process> nvimProcess;

  Nvim();
  ~Nvim();
  void StartUi(int width, int height);
  void Input(std::string input);
};
