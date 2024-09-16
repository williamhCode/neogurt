#pragma once
#include "nvim/nvim.hpp"
#include <cstdint>

struct Options {
  // window opts are used once at start
  // so dont use them except in window creation (sdl_window constructor)
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
  static inline float titlebarHeight = 0;

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
