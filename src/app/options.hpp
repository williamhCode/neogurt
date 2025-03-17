#pragma once
#include <expected>

struct AppOptions {
  bool interactive = true;
  bool multigrid = true;

  bool vsync = true;
  bool highDpi = true;
  bool borderless = false;
  int blur = 0;

  static std::expected<AppOptions, int> LoadFromCommandLine(int argc, char** argv);
};


