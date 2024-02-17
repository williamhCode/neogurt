#pragma once

#include "gfx/texture.hpp"
#include "nvim/parse.hpp"
#include "utils/ring_buffer.hpp"
#include "webgpu/webgpu_cpp.h"
#include <vector>

struct Grid {
  static const int fontSize = 30;  // TODO: make this configurable

  int width;
  int height;

  int cursorRow;
  int cursorCol;

  using Line = std::vector<std::string>;
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
