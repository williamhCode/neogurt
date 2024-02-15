#pragma once

#include "gfx/texture.hpp"
#include "nvim/parse.hpp"
#include "webgpu/webgpu_cpp.h"

struct Grid {
  static const int fontSize = 30;  // TODO: make this configurable

  int width;
  int height;

  int cursorRow;
  int cursorCol;
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
