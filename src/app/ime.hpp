#pragma once

#include "SDL3/SDL_events.h"
#include "editor/state.hpp"
#include <atomic>

struct ImeHandler {
  EditorState* editorState;

  static constexpr int imeGrid = -1;

  static constexpr int imeNormalHlId = -1;
  static constexpr int imeSelectedHlId = -2;

  std::atomic_bool active = false;

  std::string text;
  int32_t start = -1;
  int32_t length = -1;

  void Clear();
  void HandleTextEditing(SDL_TextEditingEvent& event);

  void Update();
};
