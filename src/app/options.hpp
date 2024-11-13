#pragma once
#include "nvim/nvim.hpp"
#include <cstdint>

struct Options {
  // TODO: load these options before connecting
  // and add commandline opts
  static inline bool multigrid = true;
  static inline bool interactiveShell = false;

  // window opts are used once at start, only one instance so static
  // so dont use them except in window creation (sdl_window constructor)
  static inline bool vsync = true;
  static inline bool highDpi = true;
  static inline bool borderless = false;
  static inline int blur = 0;

  int marginTop = 0;
  int marginBottom = 0;
  int marginLeft = 0;
  int marginRight = 0;

  bool macOptIsMeta = true;
  float cursorIdleTime = 10;
  float scrollSpeed = 1;

  uint32_t bgColor = 0x000000;
  float opacity = 1;
  float gamma = 1.7;

  float maxFps = 60;

  static std::future<Options> Load(Nvim& nvim, bool first);
};

