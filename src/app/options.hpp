#pragma once

struct AppOptions {
  bool multigrid = false;
  bool vsync = false;
  bool highDpi = true;
  bool macOptAsAlt = true;
};

extern AppOptions options;
