#pragma once
#include "editor/cursor.hpp"
#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "nvim/parse.hpp"

struct EditorState {
  GridManager gridManager;
  HlTable hlTable;
  Cursor cursor;
};

inline EditorState editorState;

void ProcessRedrawEvents(RedrawState& redrawState, EditorState& editorState);
