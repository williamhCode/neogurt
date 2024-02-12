#pragma once

#include "nvim/parse.hpp"
#include <deque>

struct EditorState {
  int numFlushes;
  std::deque<std::vector<RedrawEvent>> redrawEventsQueue;

  EditorState() : numFlushes(0) {
    redrawEventsQueue.emplace_back();
  }

  auto& currEvents() {
    return redrawEventsQueue.back();
  }
};

inline EditorState editorState;
