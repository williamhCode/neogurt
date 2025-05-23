#pragma once

#include "app/sdl_window.hpp"
#include "glm/ext/vector_float2.hpp"
#include "session/options.hpp"

struct SizeHandler {
  glm::vec2 size;
  glm::vec2 fbSize;
  glm::vec2 uiSize;
  glm::vec2 uiFbSize;
  glm::vec2 charSize;
  glm::vec2 charFbSize;
  int uiWidth;
  int uiHeight;
  float dpiScale;

  glm::vec2 offset;
  glm::vec2 fbOffset;

  void UpdateSizes(
    const sdl::Window& window, glm::vec2 charSize, const GlobalOptions& globalOpts
  );
};
