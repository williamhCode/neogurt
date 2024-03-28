#pragma once

struct AppOptions {
  bool multigrid = true;
  bool vsync = false;
  // bool highDpi = true;
  bool macOptAsAlt = true;
};

extern AppOptions options;
