#include "grid.hpp"
#include "util/logger.hpp"

#include "msgpack.hpp"

void Grid::UpdateState(Grid& source) {
  // update this
  width = source.width;
  height = source.height;

  cursorRow = source.cursorRow;
  cursorCol = source.cursorCol;

  newCells = std::move(source.newCells);

  resize = source.resize;
  clear = source.clear;
  cursorGoto = source.cursorGoto;
  destroy = source.destroy;

  // reset source
  source.resize = false;
  source.clear = false;
  source.cursorGoto = false;
  // don't reset destroy
}

void GridManager::UpdateState(GridManager& source) {
  // grids.clear();
  for (auto& [sourceId, sourceGrid] : source.grids) {
    grids[sourceId].UpdateState(sourceGrid);

    // keep grid in this because renderer needs to clear resources
    // remember to remove grid from this after renderer has cleared resources
    if (sourceGrid.destroy) {
      source.grids.erase(sourceId);
    }
  }
}

void GridManager::Resize(const msgpack::object& args) {
  auto [grid, width, height] = args.as<std::tuple<int, int, int>>();
  LOG("grid_resize: {} {} {}", grid, width, height);

  auto& g = grids[grid];
  g.width = width;
  g.height = height;
  g.resize = true;
}

void GridManager::Clear(const msgpack::object& args) {
  auto [grid] = args.as<std::tuple<int>>();
  LOG("grid_clear: {}", grid);

  auto& g = grids[grid];
  g.clear = true;
}

void GridManager::CursorGoto(const msgpack::object& args) {
  auto [grid, row, col] = args.as<std::tuple<int, int, int>>();
  LOG("grid_cursor_goto: {} {} {}", grid, row, col);

  auto& g = grids[grid];
  g.cursorRow = row;
  g.cursorCol = col;
  g.cursorGoto = true;
}

void GridManager::Line(const msgpack::object& args) {
  auto [grid, row, col_start, cells] =
    args.as<std::tuple<int, int, int, msgpack::object>>();
  LOG("grid_line: {} {} {} {}", grid, row, col_start, ToString(cells));

  std::span<const msgpack::object> cellsList(cells.via.array);
  auto& g = grids[grid];
  int curr_hl_id = -1;
  for (const auto& cell : cellsList) {
    int length = cell.via.array.size;
    switch (length) {
      case 3: {
        auto [text, hl_id, repeat] = cell.as<std::tuple<std::string, int, int>>();
        curr_hl_id = hl_id;
        g.newCells.push_back({text, hl_id, repeat});
        break;
      }
      case 2: {
        auto [text, hl_id] = cell.as<std::tuple<std::string, int>>();
        curr_hl_id = hl_id;
        g.newCells.push_back({text, hl_id});
        break;
      }
      case 1: {
        auto [text] = cell.as<std::tuple<std::string>>();
        g.newCells.push_back({text, curr_hl_id});
        break;
      }
    }
  }
}

void GridManager::Scroll(const msgpack::object& args) {
  auto [grid, top, bot, left, right, rows, cols] =
    args.as<std::tuple<int, int, int, int, int, int, int>>();
  LOG("grid_scroll: {} {} {} {} {} {} {}", grid, top, bot, left, right, rows, cols);

  auto& g = grids[grid];
}

void GridManager::Destroy(const msgpack::object& args) {
  auto [grid] = args.as<std::tuple<int>>();
  LOG("grid_destroy: {}", grid);

  auto& g = grids[grid];
  g.destroy = true;
}
