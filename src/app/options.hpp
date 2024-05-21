#pragma once
#include "nvim/nvim.hpp"
#include <cstdint>

struct Options {
  struct Window {
    bool vsync;
    bool highDpi;
    bool borderless;
    int blur;
  } window;

  struct Margins {
    int top;
    int bottom;
    int left;
    int right;
  } margins;

  bool multigrid;
  bool macOptAsAlt;
  float cursorIdleTime;

  // rgb
  uint32_t bgColor;
  float transparency;

  void Load(Nvim& nvim);
};
