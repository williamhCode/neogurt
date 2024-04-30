#pragma once

#include "nvim/parse.hpp"
// #include "process.hpp"
// #include "msgpack/v3/adaptor/boost/msgpack_variant_decl.hpp"
#include "nvim/session.hpp"

struct Nvim {
  using variant_ref = msgpack::type::variant_ref;

  bool debug;
  SessionManager sessionManager;
  rpc::Client client;
  RedrawState redrawState;

  Nvim(bool debug);
  ~Nvim();
  Nvim(const Nvim&) = delete;
  Nvim& operator=(const Nvim&) = delete;

  void SetClientInfo(
    std::string_view name,
    const std::map<std::string_view, variant_ref>& version,
    std::string_view type,
    const std::map<std::string_view, variant_ref>& methods,
    const std::map<std::string_view, std::string_view>& attributes
  );
  void SetVar(std::string_view name, const variant_ref& value);
  void UiAttach(int width, int height, const std::map<std::string_view, variant_ref>& options);
  void UiDetach();
  void UiTryResize(int width, int height);
  void Input(std::string_view input);
  void InputMouse(
    std::string_view button,
    std::string_view action,
    std::string_view modifier,
    int grid,
    int row,
    int col
  );
  void NvimListUis();
  void ParseRedrawEvents();
};
