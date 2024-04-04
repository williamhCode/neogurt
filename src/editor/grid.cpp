#include "grid.hpp"
#include "utils/logger.hpp"
#include <algorithm>

void GridManager::Resize(const GridResize& e) {
  auto [it, first] = grids.try_emplace(e.grid);
  auto& grid = it->second;

  if (first) {
    grid.lines = Grid::Lines(e.height, Grid::Line(e.width, Grid::Cell{" "}));
  } else {
    // TODO: perhaps add RingBuffer::Resize for cleaner code
    // copy old lines over when resizing
    int minWidth = std::min(grid.width, e.width);
    int minHeight = std::min(grid.height, e.height);

    auto oldLines(std::move(grid.lines));
    grid.lines = Grid::Lines(e.height, Grid::Line(e.width, Grid::Cell{" "}));

    for (int i = 0; i < minHeight; i++) {
      auto& oldLine = oldLines[i];
      auto& newLine = grid.lines[i];
      std::copy(oldLine.begin(), oldLine.begin() + minWidth, newLine.begin());
    }
  }

  grid.width = e.width;
  grid.height = e.height;

  grid.dirty = true;
}

void GridManager::Clear(const GridClear& e) {
  auto it = grids.find(e.grid);
  if (it == grids.end()) {
    LOG_ERR("GridManager::Clear: grid {} not found", e.grid);
    return;
  }
  auto& grid = it->second;

  for (size_t i = 0; i < grid.lines.Size(); i++) {
    auto& line = grid.lines[i];
    for (auto& cell : line) {
      cell = Grid::Cell{" "};
    }
  }
}

void GridManager::CursorGoto(const GridCursorGoto& e) {
  auto it = grids.find(e.grid);
  if (it == grids.end()) {
    LOG_ERR("GridManager::CursorGoto: grid {} not found", e.grid);
    return;
  }
  auto& grid = it->second;

  grid.cursorRow = e.row;
  grid.cursorCol = e.col;
}

void GridManager::Line(const GridLine& e) {
  auto it = grids.find(e.grid);
  if (it == grids.end()) {
    LOG_ERR("GridManager::Line: grid {} not found", e.grid);
    return;
  }
  auto& grid = it->second;

  auto& line = grid.lines[e.row];
  int col = e.colStart;
  for (const auto& cell : e.cells) {
    for (int i = 0; i < cell.repeat; i++) {
      auto& lineCell = line[col];
      lineCell.text = cell.text;
      lineCell.hlId = cell.hlId;
      col++;
    }
  }

  grid.dirty = true;
}

void GridManager::Scroll(const GridScroll& e) {
  auto it = grids.find(e.grid);
  if (it == grids.end()) {
    LOG_ERR("GridManager::Scroll: grid {} not found", e.grid);
    return;
  }
  auto& grid = it->second;

  if (e.top == 0 && e.bot == grid.height && e.left == 0 && e.right == grid.width && e.cols == 0) {
    grid.lines.Scroll(e.rows);
  } else {
    if (e.rows > 0) {
      // scrolling down, move lines up
      int top = e.top;
      int bot = e.bot - e.rows;
      for (int i = top; i < bot; i++) {
        auto& src = grid.lines[i + e.rows];
        auto& dest = grid.lines[i];
        std::copy(src.begin() + e.left, src.begin() + e.right, dest.begin() + e.left);
      }
    } else {
      // scrolling up, move lines down
      int rows = -e.rows;
      int top = e.top + rows;
      int bot = e.bot;
      for (int i = bot - 1; i >= top; i--) {
        auto& src = grid.lines[i - rows];
        auto& dest = grid.lines[i];
        std::copy(src.begin() + e.left, src.begin() + e.right, dest.begin() + e.left);
      }
    }
  }

  grid.dirty = true;
}

void GridManager::Destroy(const GridDestroy& e) {
  auto removed = grids.erase(e.grid);
  if (removed == 0) {
    LOG_ERR("GridManager::Destroy: grid {} not found", e.grid);
  }
}
