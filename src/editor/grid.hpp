#pragma once

#include "utils/ring_buffer.hpp"
#include "event/ui_parse.hpp"

struct Win; // forward decl

struct Grid {
  int width;
  int height;

  struct Cell {
    std::string text;
    int hlId;
    bool doubleWidth;
  };
  using Line = std::vector<Cell>;
  using Lines = RingBuffer<Line>;
  Lines lines;

  bool dirty;

  bool ValidCoords(int row, int col) const {
    return row >= 0 && row < height && col >= 0 && col < width;
  }
};

struct GridManager {
  std::unordered_map<int, Grid> grids;

  void Resize(const event::GridResize& e);
  void Clear(const event::GridClear& e);
  void Line(const event::GridLine& e);
  void Scroll(const event::GridScroll& e);
  void Destroy(const event::GridDestroy& e);
};
