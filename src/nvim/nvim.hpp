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
  Nvim(const Nvim&) = delete;
  Nvim& operator=(const Nvim&) = delete;
  ~Nvim();

  bool ConnectStdio();
  bool ConnectTcp(std::string_view host, uint16_t port);
  void Setup();
  bool IsConnected();

  using Variant = msgpack::type::variant;
  using VariantRef = msgpack::type::variant_ref;
  using MapRef = const std::map<std::string_view, VariantRef>&;
  using VectorRef = const std::vector<VariantRef>&;

  using Msg = std::future<msgpack::object_handle>;

  Msg SetClientInfo(
    std::string_view name,
    MapRef version,
    std::string_view type,
    MapRef methods,
    MapRef attributes
  );
  Msg SetVar(std::string_view name, VariantRef value);
  Msg UiAttach(int width, int height, MapRef options);
  Msg UiDetach();
  Msg UiTryResize(int width, int height);
  Msg Input(std::string_view input);
  Msg InputMouse(
    std::string_view button,
    std::string_view action,
    std::string_view modifier,
    int grid,
    int row,
    int col
  );
  void ListUis();
  Msg GetOptionValue(std::string_view name, MapRef opts);
  Msg GetVar(std::string_view name);
  Msg ExecLua(std::string_view code, VectorRef args);
  Msg Command(std::string_view command);
};
