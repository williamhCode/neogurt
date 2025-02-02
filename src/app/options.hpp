#pragma once
#include "nvim/nvim.hpp"
#include <cstdint>

struct Options {
  // startup options
  inline static bool multigrid = true;
  inline static bool interactiveShell = true;

  // return optional exit code
  static std::optional<int> LoadFromCommandLine(int argc, char** argv);

  // window opts are used once at start, only one instance so static
  // so dont use them except in window creation (sdl_window constructor)
  inline static bool vsync = true;
  inline static bool highDpi = true;
  inline static bool borderless = false;
  inline static int blur = 0;

  int marginTop = 0;
  int marginBottom = 0;
  int marginLeft = 0;
  int marginRight = 0;
  std::string macosOptionIsMeta = "none";
  float cursorIdleTime = 10;
  float scrollSpeed = 1;

  uint32_t bgColor = 0x000000;
  float opacity = 1;
  float gamma = 1.7;

  float maxFps = 60;

  static std::future<Options> Load(Nvim& nvim, bool first);
};

