#pragma once

#include "nvim/msgpack_rpc/client.hpp"

struct Nvim {
  rpc::Client client;

  Nvim();
  void StartUi(int width, int height);
  void Input(std::string input);
};
