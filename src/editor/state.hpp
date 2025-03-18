#pragma once

#include "editor/ui_options.hpp"
#include "editor/grid.hpp"
#include "editor/window.hpp"
#include "editor/highlight.hpp"
#include "editor/cursor.hpp"
#include "editor/font.hpp"

// All state information that gets parsed from ui events.
// Editor refers to neovim, so per neovim instance, there is an editor state.
struct EditorState {
  std::string currDir;
  UiOptions uiOptions;
  GridManager gridManager;
  WinManager winManager;
  HlManager hlManager;
  Cursor cursor;
  FontFamily fontFamily;
};
