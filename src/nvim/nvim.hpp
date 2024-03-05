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
  void SetClientInfo(
    const std::string& name,
    const std::map<std::string, msgpack::type::variant>& version,
    const std::string& type,
    const std::map<std::string, msgpack::type::variant>& methods,
    const std::map<std::string, std::string>& attributes
  );
  void UiAttach(int width, int height);
  void UiTryResize(int width, int height);
  void Input(const std::string& input);
  void ParseEvents();
};
