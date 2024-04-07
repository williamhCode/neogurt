#pragma once

#include "nvim/parse.hpp"
#include "process.hpp"
#include "msgpack/v3/adaptor/boost/msgpack_variant_decl.hpp"

struct Nvim {
  using variant = msgpack::type::variant;

  rpc::Client client;
  std::unique_ptr<TinyProcessLib::Process> nvimProcess;

  RedrawState redrawState;

  Nvim(bool debug);
  ~Nvim();
  Nvim(const Nvim&) = delete;
  Nvim& operator=(const Nvim&) = delete;

  void SetClientInfo(
    const std::string& name,
    const std::map<std::string, variant>& version,
    const std::string& type,
    const std::map<std::string, variant>& methods,
    const std::map<std::string, std::string>& attributes
  );
  void SetVar(const std::string& name, const variant& value);
  void Input(const std::string& input);
  void InputMouse(
    const std::string& button,
    const std::string& action,
    const std::string& modifier,
    int grid,
    int row,
    int col
  );
  void UiAttach(int width, int height, const std::map<std::string, variant>& options);
  void UiTryResize(int width, int height);
  void ParseRedrawEvents();
};
