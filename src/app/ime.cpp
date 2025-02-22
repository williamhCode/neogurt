#include "./ime.hpp"

#include "nvim/events/ui.hpp"
#include "utils/logger.hpp"
#include "utils/unicode.hpp"
#include "SDL3/SDL_stdinc.h"
#include <algorithm>
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
    text.append(" "); // extra space for cursor

    int col = -1;
    int colIdx = 0;

    // NOTE: for macos, start and length are utf32 lengths
    // was a bug cuz it was utf16, but SDL abi stuff so changed to utf32
    // use utf8 lengths for other platforms
    int u32idx = 0;
    int end = start + (length != -1 ? length : 0);

    auto cells =
      SplitByGraphemes(text) |
      std::views::transform([&, this](auto& info) {
        int hlId = imeNormalHlId;
        if (start != -1) {
          if (u32idx >= start && u32idx < end) {
            hlId = imeSelectedHlId;
          }
          if (u32idx == end) {
            col = colIdx;
          }
        }
        u32idx += info.u32Len;
        colIdx++;

        return event::GridLine::Cell{
          .text = info.str,
          .hlId = hlId,
        };
      }) |
      std::ranges::to<std::vector>();

    if (col != -1) {
      editorState->cursor.ImeGoto({.grid = imeGrid, .row = 0, .col = col});
    }

    editorState->gridManager.Resize({
      .grid = imeGrid,
      .width = (int)cells.size(),
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
