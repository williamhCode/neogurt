#include "grid.hpp"
#include "utils/webgpu.hpp"
#include "gfx/instance.hpp"

using namespace wgpu;

void GridManager::Resize(GridResize& e) {
  auto& grid = grids[e.grid];
  grid.width = e.width;
  grid.height = e.height;
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
