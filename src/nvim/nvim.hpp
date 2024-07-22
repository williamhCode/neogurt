#pragma once

#include "msgpack_rpc/client.hpp"
#include "nvim/events/parse.hpp"
#include <string_view>

// Nvim client that wraps the rpc client.
struct Nvim {
  rpc::Client client;
  UiEvents uiEvents;
  // int channelId;

  Nvim() = default;
  bool Connect(std::string_view host, uint16_t port);
  Nvim(const Nvim&) = delete;
  Nvim& operator=(const Nvim&) = delete;
  ~Nvim();

  bool IsConnected();

  using Variant = msgpack::type::variant;
  using VariantRef = msgpack::type::variant_ref;
  using MapRef = const std::map<std::string_view, VariantRef>&;
  using VectorRef = const std::vector<VariantRef>&;
  void SetClientInfo(
    std::string_view name,
    MapRef version,
    std::string_view type,
    MapRef methods,
    MapRef attributes
  );
  void SetVar(std::string_view name, VariantRef value);
  void UiAttach(int width, int height, MapRef options);
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
  msgpack::object_handle GetOptionValue(std::string_view name, MapRef opts);
  msgpack::object_handle GetVar(std::string_view name);
  msgpack::object_handle ExecLua(std::string_view code, VectorRef args);
};
