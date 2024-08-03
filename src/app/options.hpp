#pragma once
#include "nvim/nvim.hpp"
#include <cstdint>

struct Options {
  struct Window {
    bool vsync = true;
    bool highDpi = true;
    bool borderless = false;
    int blur = 0;
  } window;

  struct Margins {
    int top = 0;
    int bottom = 0;
    int left = 0;
    int right = 0;
  } margins;

  bool multigrid = true;
  bool macOptIsMeta = true;
  float cursorIdleTime = 10;
  float scrollSpeed = 1;

  // rgb
  uint32_t bgColor = 0x000000;
  float opacity = 1;

  float maxFps = 60;
};

std::future<Options> LoadOptions(Nvim& nvim);
