#pragma once

#include "glm/ext/vector_float2.hpp"
#include "nvim/nvim.hpp"
#include <optional>

struct InputHandler {
  Nvim& nvim;

  int mods = 0;
  std::optional<int> mouseButton;
  glm::vec2 cursorPos;
  glm::vec2 charSize;

  InputHandler(Nvim& nvim, glm::vec2 cursorPos, glm::vec2 charSize);

  void HandleKey(int key, int scancode, int action, int mods);
  void HandleChar(unsigned int codepoint);
  void HandleMouseButton(int button, int action, int mods);
  void HandleCursorPos(double xpos, double ypos);
  void HandleScroll(double xoffset, double yoffset);

  void HandleMouseButtonAndCursorPos(int action);
};
