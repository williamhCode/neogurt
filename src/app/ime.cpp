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
    auto cells =
      SplitUTF8(text) | std::views::transform([](auto& text) {
        return event::GridLine::Cell{
          .text = text,
          .hlId = imeNormalHlId,
        };
      }) |
      std::ranges::to<std::vector>();

    if (start != -1) {
      int end = start + (length != -1 ? length : 0);
      LOG_INFO("--------------");
      LOG_INFO("text: {}", text);
      LOG_INFO("start: {}, length: {}", start, length);

      // actual column from start cuz double width stuff
      int col = 0;
      for (int i = 0; i < end; i++) {
        bool isDouble = col + 1 < (int)cells.size() && cells[col + 1].text.empty();

        // set selected text highlight
        if (i >= start && i < end) {
          cells[col].hlId = imeSelectedHlId;
          if (isDouble) cells[col + 1].hlId = imeSelectedHlId;
        }

        col += isDouble ? 2 : 1;
        // counteract issue
        // https://github.com/libsdl-org/SDL/issues/12344
        col = std::min(col, (int)cells.size() - 1);
      }
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
