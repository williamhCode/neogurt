#include "grid.hpp"
#include "utils/ring_buffer.hpp"

using namespace wgpu;

void GridManager::Resize(GridResize& e) {
  auto& grid = grids[e.grid];
  grid.width = e.width;
  grid.height = e.height;

  grid.lines = Grid::Lines(e.height);
  for (size_t i = 0; i < grid.lines.size(); i++) {
    auto& line = grid.lines[i];
    line.resize(e.width);
  }
}

void GridManager::Clear(GridClear& e) {
  auto& grid = grids[e.grid];
  // clear texture
}

void GridManager::CursorGoto(GridCursorGoto& e) {
  auto& grid = grids[e.grid];
  grid.cursorRow = e.row;
  grid.cursorCol = e.col;
}

void GridManager::Line(GridLine& e) {
  auto& grid = grids[e.grid];
  // draw line

}

void GridManager::Scroll(GridScroll& e) {
  auto& grid = grids[e.grid];
  // scroll
}

void GridManager::Destroy(GridDestroy& e) {
  grids.erase(e.grid);
}
