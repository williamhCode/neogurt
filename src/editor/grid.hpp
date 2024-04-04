#pragma once

#include "nvim/parse.hpp"
#include "utils/ring_buffer.hpp"
#include <vector>

struct Win; // forward decl

struct Grid {
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

  bool dirty;
};

struct GridManager {
  std::unordered_map<int, Grid> grids;

  void Resize(const GridResize& e);
  void Clear(const GridClear& e);
  void CursorGoto(const GridCursorGoto& e);
  void Line(const GridLine& e);
  void Scroll(const GridScroll& e);
  void Destroy(const GridDestroy& e);
};
