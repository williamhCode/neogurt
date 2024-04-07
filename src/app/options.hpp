#pragma once

struct AppOptions {
  bool multigrid = true;
  bool vsync = true;
  // bool highDpi = true;
  bool macOptAsAlt = true;
};

extern AppOptions options;
