#pragma once

#include "editor/cursor.hpp"
#include "editor/font.hpp"
#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "editor/ui_options.hpp"
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
  std::vector<CursorMode> cursorModes;
  FontFamily fontFamily;
  // std::map<int, std::string> hlGroupTable;

  void Init(const SizeHandler& sizes) {
    winManager.sizes = &sizes;
    winManager.gridManager = &gridManager;
    cursor.Init(sizes.charSize, sizes.dpiScale);
  }
};

// returns true if there were events processed
bool ParseEditorState(UiEvents& uiEvents, EditorState& editorState);
