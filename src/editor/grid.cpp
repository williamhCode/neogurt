#include "grid.hpp"
#include "editor/window.hpp"
#include "gfx/instance.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "utils/logger.hpp"
#include "webgpu_utils/webgpu.hpp"
#include <algorithm>

using namespace wgpu;

void GridManager::Resize(GridResize& e) {
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

  // rendering
  auto size = glm::vec2(grid.width, grid.height) * gridSize;
  auto view = glm::ortho<float>(0, size.x, size.y, 0, -1, 1);

  if (first) {
    grid.viewProjBuffer =
      utils::CreateUniformBuffer(ctx.device, sizeof(glm::mat4), &view);

    grid.viewProjBG = utils::MakeBindGroup(
      ctx.device, ctx.pipeline.viewProjBGL,
      {
        {0, grid.viewProjBuffer},
      }
    );
  } else {
    ctx.queue.WriteBuffer(grid.viewProjBuffer, 0, &view, sizeof(glm::mat4));
  }

  Extent3D textureSize{
    .width = static_cast<uint32_t>(size.x * dpiScale),
    .height = static_cast<uint32_t>(size.y * dpiScale),
  };
  grid.textureView =
    utils::CreateRenderTexture(ctx.device, textureSize, TextureFormat::BGRA8Unorm)
      .CreateView();

  // when creating new window, grid_resize is called first, so handle that in win_pos
  // but when resizing window, order is unknown, so we need to update
  // window's bind group after grid's texture view is recreated
  if (grid.win != nullptr) {
    auto& win = *grid.win;
    win.MakeTextureBG();

    // we also need to update floating windows because their width and height
    // depend on grid's width and height
    if (win.floatData.has_value()) {
      win.width = win.grid.width;
      win.height = win.grid.height;
      win.UpdateFloatPos();
      win.UpdateRenderData();
    }
  }

  const size_t maxTextQuads = grid.width * grid.height;
  grid.rectData.CreateBuffers(maxTextQuads);
  grid.textData.CreateBuffers(maxTextQuads);

  grid.rectData.ReserveVectors(maxTextQuads);
  grid.textData.ReserveVectors(maxTextQuads);
}

void GridManager::Clear(GridClear& e) {
  auto it = grids.find(e.grid);
  if (it == grids.end()) {
    LOG_WARN("GridManager::Clear: grid {} not found", e.grid);
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

void GridManager::CursorGoto(GridCursorGoto& e) {
  auto it = grids.find(e.grid);
  if (it == grids.end()) {
    LOG_WARN("GridManager::CursorGoto: grid {} not found", e.grid);
    return;
  }
  auto& grid = it->second;

  grid.cursorRow = e.row;
  grid.cursorCol = e.col;
}

void GridManager::Line(GridLine& e) {
  auto it = grids.find(e.grid);
  if (it == grids.end()) {
    LOG_WARN("GridManager::Line: grid {} not found", e.grid);
    return;
  }
  auto& grid = it->second;

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

  grid.dirty = true;
}

void GridManager::Scroll(GridScroll& e) {
  auto it = grids.find(e.grid);
  if (it == grids.end()) {
    LOG_WARN("GridManager::Scroll: grid {} not found", e.grid);
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
      e.rows = -e.rows;
      int top = e.top + e.rows;
      int bot = e.bot;
      for (int i = bot - 1; i >= top; i--) {
        auto& src = grid.lines[i - e.rows];
        auto& dest = grid.lines[i];
        std::copy(src.begin() + e.left, src.begin() + e.right, dest.begin() + e.left);
      }
    }
  }

  grid.dirty = true;
}

void GridManager::Destroy(GridDestroy& e) {
  auto removed = grids.erase(e.grid);
  if (removed == 0) {
    LOG_WARN("GridManager::Destroy: grid {} not found", e.grid);
  }
}
