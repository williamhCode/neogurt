#pragma once

#include "nvim/parse.hpp"
#include "utils/ring_buffer.hpp"
#include <vector>

struct Grid {
  static const int fontSize = 30; // TODO: make this configurable
  bool empty = true;  // temporary hack

  int width;
  int height;

  int cursorRow;
  int cursorCol;

  struct Cell {
    std::string text;
    int hlId = 0;
  };
  using Line = std::vector<Cell>;
  using Lines = RingBuffer<Line>;
  Lines lines;
};

struct GridManager {
  std::unordered_map<int, Grid> grids;

  void Resize(GridResize& e);
  void Clear(GridClear& e);
  void CursorGoto(GridCursorGoto& e);
  void Line(GridLine& e);
  void Scroll(GridScroll& e);
  void Destroy(GridDestroy& e);
};
