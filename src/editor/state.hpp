#pragma once
#include "editor/cursor.hpp"
#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "nvim/parse.hpp"
#include <vector>

struct EditorState {
  GridManager gridManager;
  HlTable hlTable;
  Cursor cursor;
  std::vector<CursorMode> cursorModes;
  int modeIdx = 0;
};

inline EditorState editorState;

void ProcessRedrawEvents(RedrawState& redrawState, EditorState& editorState);
