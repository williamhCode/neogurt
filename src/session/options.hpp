#pragma once
#include <string>

// global options (shared across all sessions)
struct GlobalOptions {
  bool showTitle = true;
  std::string titlebar = "default"; // "default", "transparent", "none"
  int blur = 0;
  float gamma = 1.7;
  bool vsync = true;
  float fps = 60;
};

// session specific options
struct SessionOptions {
  int marginTop = 0;
  int marginBottom = 0;
  int marginLeft = 0;
  int marginRight = 0;

  std::string macosOptionIsMeta = "none"; // "none", "only_left", "only_right", "both"
  float cursorIdleTime = 10;
  float scrollSpeed = 1;

  uint32_t bgColor = 0x000000;
  float opacity = 1;
};

