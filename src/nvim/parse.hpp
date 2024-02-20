#pragma once

#include "msgpack_rpc/client.hpp"
#include <map>
#include <variant>

struct SetTitle {
  std::string title;
  MSGPACK_DEFINE(title);
};
struct SetIcon {
  std::string icon;
  MSGPACK_DEFINE(icon);
};
struct ModeInfoSet {
  bool enabled; // cursor style enabled
  using ModePropertyMap = std::map<std::string, msgpack::type::variant>;
  std::vector<ModePropertyMap> modeInfo;
  MSGPACK_DEFINE(enabled, modeInfo);
};
struct OptionSet {
  std::string name;
  msgpack::type::variant value;
  MSGPACK_DEFINE(name, value);
};
struct ModeChange {
  std::string mode;
  int modeIdx;
  MSGPACK_DEFINE(mode, modeIdx);
};
struct MouseOn {};
struct MouseOff {};
struct BusyStart {};
struct BusyStop {};
struct UpdateMenu {};
struct Flush {};
struct DefaultColorsSet {
  int rgbFg;
  int rgbBg;
  int rgbSp;
  int ctermFg;
  int ctermBg;
  MSGPACK_DEFINE(rgbFg, rgbBg, rgbSp, ctermFg, ctermBg);
};
struct HlAttrDefine {
  int id;
  using HlAttrMap = std::map<std::string, msgpack::type::variant>;
  HlAttrMap rgbAttrs;
  HlAttrMap ctermAttrs;
  std::vector<HlAttrMap> info;
  MSGPACK_DEFINE(id, rgbAttrs, ctermAttrs, info);
};
struct HlGroupSet {
  std::string name;
  int id;
  MSGPACK_DEFINE(name, id);
};
struct GridResize {
  int grid;
  int width;
  int height;
  MSGPACK_DEFINE(grid, width, height);
};
struct GridClear {
  int grid;
  MSGPACK_DEFINE(grid);
};
struct GridCursorGoto {
  int grid;
  int row;
  int col;
  MSGPACK_DEFINE(grid, row, col);
};
struct GridLine {
  struct Cell {
    std::string text;
    int hlId;
    int repeat = 1;
  };
  int grid;
  int row;
  int colStart;
  std::vector<Cell> cells;
};
struct GridScroll {
  int grid;
  int top;
  int bot;
  int left;
  int right;
  int rows;
  int cols;
  MSGPACK_DEFINE(grid, top, bot, left, right, rows, cols);
};
struct GridDestroy {
  int grid;
  MSGPACK_DEFINE(grid);
};

// clang-format off
using RedrawEvent = std::variant<
  SetTitle,
  SetIcon,
  ModeInfoSet,
  OptionSet,
  ModeChange,
  MouseOn,
  MouseOff,
  BusyStart,
  BusyStop,
  UpdateMenu,
  DefaultColorsSet,
  HlAttrDefine,
  HlGroupSet,
  Flush,
  GridResize,
  GridClear,
  GridCursorGoto,
  GridLine,
  GridScroll,
  GridDestroy
>;
// clang-format on

struct RedrawState {
  int numFlushes = 0;
  std::deque<std::vector<RedrawEvent>> eventsQueue;

  RedrawState() {
    eventsQueue.emplace_back();
  }

  auto& currEvents() {
    return eventsQueue.back();
  }
};

void ParseNotifications(rpc::Client& client, RedrawState& state);
