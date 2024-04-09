#pragma once

struct AppOptions {
  bool multigrid = true;
  bool vsync = true;
  bool highDpi = true;
  bool macOptAsAlt = true;

  struct Margins {
    int top = 0;
    int bottom = 0;
    int left = 0;
    int right = 0;
  } windowMargins;

  bool borderless = false;
};

extern AppOptions appOpts;
