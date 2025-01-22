#include "./grid.hpp"
#include "utils/logger.hpp"
#include <algorithm>

void GridManager::Resize(const event::GridResize& e) {
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

void GridManager::Clear(const event::GridClear& e) {
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

  grid.dirty = true;
}

void GridManager::Line(const event::GridLine& e) {
  auto it = grids.find(e.grid);
  if (it == grids.end()) {
    LOG_ERR("GridManager::Line: grid {} not found", e.grid);
    return;
  }
  auto& grid = it->second;

  auto& line = grid.lines[e.row];
  size_t col = e.colStart;
  int recentHlId = 0;

  for (size_t i = 0; i < e.cells.size(); i++) {
    const auto& eventCell = e.cells[i];
    if (eventCell.hlId) {
      recentHlId = *eventCell.hlId;
    }

    if (eventCell.repeat) {
      for (int j = 0; j < eventCell.repeat; j++) {
        auto& cell = line[col];
        cell.text = eventCell.text;
        cell.hlId = recentHlId;
        cell.doubleWidth = false;
        col++;
      }
    } else {
      auto& cell = line[col];
      cell.text = eventCell.text;
      cell.hlId = recentHlId;
      // "The right cell of a double-width char will be represented as the empty
      // string. Double-width chars never use repeat."
      cell.doubleWidth = i + 1 < e.cells.size() && e.cells[i + 1].text.empty();
      col++;
    }
  }

  grid.dirty = true;
}

void GridManager::Scroll(const event::GridScroll& e) {
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

void GridManager::Destroy(const event::GridDestroy& e) {
  auto removed = grids.erase(e.grid);
  if (removed == 0) {
    LOG_ERR("GridManager::Destroy: grid {} not found", e.grid);
  }
}
