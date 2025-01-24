#pragma once

#include "SDL3/SDL_events.h"
#include "editor/state.hpp"

struct ImeHandler {
  EditorState* editorState;

  static const int imeGrid = -1;
  bool active = false;

  std::string text;
  int32_t start = 0;
  int32_t length = 0;

  void Clear();
  void HandleTextEditing(SDL_TextEditingEvent& event);
  void Update();
};
