#pragma once

#include "SDL3/SDL_events.h"
#include "editor/window.hpp"
#include "glm/ext/vector_float2.hpp"
#include "nvim/nvim.hpp"
#include <optional>

struct InputHandler {
  Nvim* nvim;
  WinManager* winManager;

  // options
  bool macOptIsMeta;
  bool multigrid;
  float scrollSpeed;

  // mouse related
  std::optional<int> mouseButton;
  std::optional<int> currGrid;

  double yAccum = 0;
  double xAccum = 0;
  int scrollDir = 0;

  InputHandler() = default;
  InputHandler(
    Nvim* nvim,
    WinManager* winManager,
    bool macOptAsAlt,
    bool multigrid,
    float scrollSpeed
  );

  void HandleKeyboard(const SDL_KeyboardEvent& event);
  void HandleTextInput(const SDL_TextInputEvent& event);
  void HandleMouseButton(const SDL_MouseButtonEvent& event);
  void HandleMouseMotion(const SDL_MouseMotionEvent& event);
  void HandleMouseButtonAndMotion(int state, glm::vec2 pos);
  void HandleMouseWheel(const SDL_MouseWheelEvent& event);
};
