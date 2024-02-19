#include "grid.hpp"
#include "utils/ring_buffer.hpp"

using namespace wgpu;

void GridManager::Resize(GridResize& e) {
  auto& grid = grids[e.grid];

  grid.width = e.width;
  grid.height = e.height;

  grid.lines = Grid::Lines(e.height, Grid::Line(e.width));
  for (size_t i = 0; i < grid.lines.Size(); i++) {
    auto& line = grid.lines[i];
    for (auto& cell : line) {
      cell = " ";
    }
  }
}

void GridManager::Clear(GridClear& e) {
  auto& grid = grids[e.grid];
  for (size_t i = 0; i < grid.lines.Size(); i++) {
    auto& line = grid.lines[i];
    for (auto& cell : line) {
      cell = " ";
    }
  }
}

void GridManager::CursorGoto(GridCursorGoto& e) {
  auto& grid = grids[e.grid];
  grid.cursorRow = e.row;
  grid.cursorCol = e.col;
}

void GridManager::Line(GridLine& e) {
  auto& grid = grids[e.grid];

  auto& line = grid.lines[e.row];
  int col = e.colStart;
  for (auto& cell : e.cells) {
    for (int i = 0; i < cell.repeat; i++) {
      line[col] = cell.text;
      col++;
    }
  }
}

void GridManager::Scroll(GridScroll& e) {
  auto& grid = grids[e.grid];

  if (e.top == 0 && e.bot == grid.height && e.left == 0 && e.right == grid.width && e.cols == 0) {
    grid.lines.Scroll(e.rows);
  } else {
    // scroll
  }
}

void GridManager::Destroy(GridDestroy& e) {
  grids.erase(e.grid);
}
