#pragma once

#include <unordered_map>

struct Grid {
  int width;
  int height;

  int cursorRow;
  int cursorCol;
};

struct GridManager {
  std::unordered_map<int, Grid> grids;

  void Resize(int grid, int width, int height) {
    auto &g = grids[grid];
    g.width = width;
    g.height = height;
  }

  void Clear(int grid) {
    // clear grid
  }

  void CursorGoto(int grid, int row, int col) {
    auto &g = grids[grid];
    g.cursorRow = row;
    g.cursorCol = col;
  }

  void Line(int grid, int row, int colStart, const std::string& cells) {
    auto &g = grids[grid];
    // draw line
  }

  void Scroll(int grid, int top, int bot, int left, int right, int rows, int cols) {
    auto &g = grids[grid];
    // scroll grid
  }

  void Destroy(int grid) {
    grids.erase(grid);
  }
};
