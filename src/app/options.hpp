#pragma once
#include <expected>

struct AppOptions {
  bool multigrid = true;
  bool interactiveShell = true;

  bool vsync = true;
  bool highDpi = true;
  bool borderless = false;
  int blur = 0;

  static std::expected<AppOptions, int> LoadFromCommandLine(int argc, char** argv);
};


