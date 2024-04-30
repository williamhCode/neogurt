#pragma once

#include "editor/cursor.hpp"
#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "editor/options.hpp"
#include "editor/window.hpp"
#include "nvim/events/parse.hpp"
#include <vector>

// All state information that gets parsed from ui events.
// Editor refers to neovim, so per neovim instance, there is an editor state.
struct EditorState {
  UiOptions uiOptions;
  GridManager gridManager;
  WinManager winManager;
  HlTable hlTable;
  Cursor cursor;
  std::vector<ModeInfo> modeInfoList;
  // std::map<int, std::string> hlGroupTable;
};

// returns true if there are events to process
bool ParseEditorState(UiEvents& uiEvents, EditorState& editorState);
