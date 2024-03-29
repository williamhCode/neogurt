#pragma once

#include "editor/window.hpp"
#include "glm/ext/vector_float2.hpp"
#include "nvim/nvim.hpp"
#include <optional>

struct InputHandler {
  Nvim& nvim;
  WinManager& winManager;

  // shared
  int mods = 0;

  // mouse related
  glm::vec2 cursorPos;
  std::optional<int> mouseButton;
  std::optional<int> currGrid;

  double yAccum = 0;
  double xAccum = 0;
  int scrollDir = 0;

  InputHandler(Nvim& nvim, WinManager& winManager, glm::vec2 cursorPos);

  void HandleKey(int key, int scancode, int action, int mods);
  void HandleChar(unsigned int codepoint);
  void HandleMouseButton(int button, int action, int mods);
  void HandleCursorPos(double xpos, double ypos);
  void HandleScroll(double xoffset, double yoffset);

  void HandleMouseButtonAndCursorPos(int action);
};
