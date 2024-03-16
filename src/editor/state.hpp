#pragma once

#include "editor/cursor.hpp"
#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "editor/window.hpp"
#include "nvim/parse.hpp"
#include <vector>

struct EditorState {
  GridManager gridManager;
  WindowManager windowManager;
  HlTable hlTable;
  Cursor cursor;
  std::vector<ModeInfo> modeInfoList;
  int modeIdx;
};

inline EditorState editorState;

void ProcessRedrawEvents(RedrawState& redrawState, EditorState& editorState);
