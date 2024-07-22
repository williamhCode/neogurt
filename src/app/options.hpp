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
  bool macOptIsMeta;
  float cursorIdleTime;
  float scrollSpeed;

  // rgb
  uint32_t bgColor;
  float transparency;

  float maxFps;

  void Load(Nvim& nvim);
};
