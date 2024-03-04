#include "grid.hpp"

void GridManager::Resize(GridResize& e) {
  auto& grid = grids[e.grid];

  grid.width = e.width;
  grid.height = e.height;

  grid.lines = Grid::Lines(e.height, Grid::Line(e.width));
  for (size_t i = 0; i < grid.lines.Size(); i++) {
    auto& line = grid.lines[i];
    for (auto& cell : line) {
      cell = Grid::Cell{" "};
    }
  }
}

void GridManager::Clear(GridClear& e) {
  auto& grid = grids[e.grid];
  grid.empty = false;
  for (size_t i = 0; i < grid.lines.Size(); i++) {
    auto& line = grid.lines[i];
    for (auto& cell : line) {
      cell = Grid::Cell{" "};
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
      auto& lineCell = line[col];
      lineCell.text = cell.text;
      lineCell.hlId = cell.hlId;
      col++;
    }
  }
}

void GridManager::Scroll(GridScroll& e) {
  auto& grid = grids[e.grid];

  if (e.top == 0 && e.bot == grid.height && e.left == 0 && e.right == grid.width && e.cols == 0) {
    grid.lines.Scroll(e.rows);
  } else {
    if (e.rows > 0) {
      // scrolling down, move lines up
      int top = e.top;
      int bot = e.bot - e.rows;
      for (int i = top; i < bot; i++) {
        grid.lines[i] = grid.lines[i + e.rows];
      }
    } else {
      // scrolling up, move lines down
      e.rows = -e.rows;
      int top = e.top + e.rows;
      int bot = e.bot;
      for (int i = bot - 1; i >= top; i--) {
        grid.lines[i] = grid.lines[i - e.rows];
      }
    }
  }
}

void GridManager::Destroy(GridDestroy& e) {
  grids.erase(e.grid);
}
