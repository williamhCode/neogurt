#pragma once

#include "msgpack_rpc/client.hpp"
#include <variant>

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
struct Cell {
  std::string text;
  int hlId;
  int repeat = 1;
};
struct GridLine {
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

using RedrawEvent = std::variant<
  GridResize,
  GridClear,
  GridCursorGoto,
  GridLine,
  GridScroll,
  GridDestroy
>;

inline std::vector<RedrawEvent> redrawEvents;

void ParseNotifications(rpc::Client& client);
