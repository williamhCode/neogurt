#pragma once

#include "events/ui_parse.hpp"
#include <future>
#include <memory>
#include <string_view>

// Forward declaration
// (reduce compile time cuz nvim.cpp calls a lot of templated functions)
namespace rpc { struct Client; }

// Nvim client that wraps the rpc client.
struct Nvim {
  std::shared_ptr<rpc::Client> client;
  UiEvents uiEvents;
  // int channelId;

  bool ConnectStdio(bool interactive, const std::string& dir = {});
  std::future<bool> ConnectTcp(std::string_view host, uint16_t port);
  void GuiSetup(bool multigrid);
  bool IsConnected();

  using Variant = msgpack::type::variant;
  using VariantRef = msgpack::type::variant_ref;
  using MapRef = const std::map<std::string_view, VariantRef>&;
  using VectorRef = const std::vector<VariantRef>&;

  using Response = std::future<msgpack::object_handle>;

  Response SetClientInfo(
    std::string_view name,
    MapRef version,
    std::string_view type,
    MapRef methods,
    MapRef attributes
  );
  Response UiAttach(int width, int height, MapRef options);
  Response UiDetach();
  Response UiTryResize(int width, int height);
  Response Input(std::string_view input);
  Response InputMouse(
    std::string_view button,
    std::string_view action,
    std::string_view modifier,
    int grid,
    int row,
    int col
  );
  Response ListUis();
  Response GetOptionValue(std::string_view name, MapRef opts);
  Response SetVar(std::string_view name, VariantRef value);
  Response GetVar(std::string_view name);
  Response ExecLua(std::string_view code, VectorRef args);
  Response Command(std::string_view command);
  Response GetHl(int nsId, MapRef opts);
};
