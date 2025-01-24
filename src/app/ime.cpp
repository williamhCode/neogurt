#include "./ime.hpp"

#include "nvim/events/ui.hpp"
#include "utils/logger.hpp"
#include "utils/unicode.hpp"
#include "SDL3/SDL_stdinc.h"
#include <ranges>

void ImeHandler::Clear() {
  text = "";
  Update();
}

void ImeHandler::HandleTextEditing(SDL_TextEditingEvent& event) {
  text = event.text;
  start = event.start;
  length = event.length;
  Update();
  SDL_free((char*)event.text);
}


void ImeHandler::Update() {
  if (text == "") {
    if (active) {
      editorState->gridManager.Clear({.grid = imeGrid});
      editorState->winManager.Hide({.grid = imeGrid});
      editorState->cursor.ImeClear();
      active = false;
    }

  } else {
    auto cells =
      SplitUTF8(text) |
      std::views::transform([](auto& text) { return event::GridLine::Cell{text}; }) |
      std::ranges::to<std::vector>();

    if (start != -1) {
      // get actual column from start cuz double width stuff
      int col = 0;
      int end = start;
      if (length != -1) end += length;
      for (int i = 0; i < end; i++) {
        if (col + 1 < (int)cells.size() && cells[col + 1].text.empty()) {
          col++;
        }
        col++;
      }
      editorState->cursor.ImeGoto({.grid = imeGrid, .row = 0, .col = col});
    }

    editorState->gridManager.Resize({
      .grid = imeGrid,
      .width = (int)cells.size() + 1, // +1 width for cursor
      .height = 1,
    });
    editorState->gridManager.Clear({.grid = imeGrid});
    editorState->gridManager.Line({
      .grid = imeGrid,
      .row = 0,
      .colStart = 0,
      .cells = std::move(cells),
      .wrap = false,
    });
    editorState->winManager.FloatPos({
      .grid = imeGrid,
      .anchor = "NW",
      .anchorGrid = editorState->cursor.cursorGoto.grid,
      .anchorRow = (float)editorState->cursor.cursorGoto.row,
      .anchorCol = (float)editorState->cursor.cursorGoto.col,
      .focusable = false,
      .zindex = std::numeric_limits<int>::max(),
    });

    active = true;
  }
}
