#pragma once

#include "SDL3/SDL_events.h"
#include "editor/state.hpp"
#include <atomic>

struct ImeHandler {
  EditorState* editorState;

  static constexpr int imeGrid = -1;

  inline static int imeNormalHlId = 0;
  inline static int imeSelectedHlId = 0;

  std::atomic_bool active = false;

  std::string text;
  int start = -1;
  int length = -1;

  void Clear();
  void HandleTextEditing(SDL_TextEditingEvent& event);

  void Update();
};
