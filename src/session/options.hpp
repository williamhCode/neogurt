#pragma once
#include <string>

struct SessionOptions {
  int marginTop = 0;
  int marginBottom = 0;
  int marginLeft = 0;
  int marginRight = 0;

  std::string macosOptionIsMeta = "none";
  float cursorIdleTime = 10;
  float scrollSpeed = 1;

  uint32_t bgColor = 0x000000;
  float opacity = 1;

  float fps = 60;
};

