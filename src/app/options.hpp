#pragma once
#include <expected>

struct StartupOptions {
  bool interactive = true;
  bool multigrid = true;

  static std::expected<StartupOptions, int> LoadFromCommandLine(int argc, char** argv);
};


