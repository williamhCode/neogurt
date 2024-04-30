#pragma once

#include "msgpack_rpc/client.hpp"
#include "nvim/ui_events.hpp"

// Nvim client that wraps the rpc client.
struct Nvim {
  rpc::Client client;
  UiEvents uiEvents;
  int channelId;

  Nvim() = default;
  Nvim(uint16_t port);
  Nvim(const Nvim&) = delete;
  Nvim& operator=(const Nvim&) = delete;
  ~Nvim();

  bool IsConnected();

  using variant_ref = msgpack::type::variant_ref;
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
  void ListUis();
};
