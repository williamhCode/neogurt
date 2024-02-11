#pragma once

#include "editor/grid.hpp"

struct EditorState {
  GridManager gridManager;
  bool flush;

  void UpdateState(EditorState& source);
};

inline EditorState editorState;
inline EditorState renderState;  // only used by renderer

inline int numFlushes;
