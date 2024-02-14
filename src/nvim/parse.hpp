#pragma once

#include "msgpack_rpc/client.hpp"
#include <variant>

struct Flush {};
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
