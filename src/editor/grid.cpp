#include "grid.hpp"
#include "editor/window.hpp"
#include "gfx/instance.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "webgpu_utils/webgpu.hpp"
#include <algorithm>

using namespace wgpu;

void GridManager::Resize(GridResize& e) {
  bool exists = grids.contains(e.grid);
  auto& grid = grids[e.grid];

  // TODO: perhaps add RingBuffer::Resize for cleaner code
  if (exists) {
    // copy lines over
    int minWidth = std::min(grid.width, e.width);
    int minHeight = std::min(grid.height, e.height);

    auto oldLines(std::move(grid.lines));
    grid.lines = Grid::Lines(e.height, Grid::Line(e.width, Grid::Cell{" "}));

    for (int i = 0; i < minHeight; i++) {
      auto& oldLine = oldLines[i];
      auto& newLine = grid.lines[i];
      std::copy(oldLine.begin(), oldLine.begin() + minWidth, newLine.begin());
    }
  } else {
    grid.lines = Grid::Lines(e.height, Grid::Line(e.width, Grid::Cell{" "}));
  }

  grid.width = e.width;
  grid.height = e.height;

  grid.dirty = true;

  // rendering
  auto size = glm::vec2(grid.width, grid.height) * gridSize;
  auto view = glm::ortho<float>(0, size.x, size.y, 0, -1, 1);
  grid.viewProjBuffer =
    utils::CreateUniformBuffer(ctx.device, sizeof(glm::mat4), &view);

  grid.viewProjBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.viewProjBGL,
    {
      {0, grid.viewProjBuffer},
    }
  );

  Extent3D textureSize{
    .width = static_cast<uint32_t>(size.x * dpiScale),
    .height = static_cast<uint32_t>(size.y * dpiScale),
  };
  grid.textureView =
    utils::CreateRenderTexture(ctx.device, textureSize, TextureFormat::BGRA8Unorm)
      .CreateView();
  // grid.textureView.SetLabel(std::format("grid texture {}", e.grid).c_str());

  if (grid.win) {
    auto textureSampler = ctx.device.CreateSampler(ToPtr(SamplerDescriptor{
      .addressModeU = AddressMode::ClampToEdge,
      .addressModeV = AddressMode::ClampToEdge,
      .magFilter = FilterMode::Nearest,
      .minFilter = FilterMode::Nearest,
    }));

    grid.win->textureBG = utils::MakeBindGroup(
      ctx.device, ctx.pipeline.textureBGL,
      {
        {0, grid.textureView},
        {1, textureSampler},
      }
    );
  }

  const size_t maxTextQuads = grid.width * grid.height;
  grid.rectData.CreateBuffers(maxTextQuads);
  grid.textData.CreateBuffers(maxTextQuads);

  grid.rectData.ReserveVectors(maxTextQuads);
  grid.textData.ReserveVectors(maxTextQuads);
}

void GridManager::Clear(GridClear& e) {
  auto& grid = grids.at(e.grid);
  for (size_t i = 0; i < grid.lines.Size(); i++) {
    auto& line = grid.lines[i];
    for (auto& cell : line) {
      cell = Grid::Cell{" "};
    }
  }

  grid.dirty = true;
}

void GridManager::CursorGoto(GridCursorGoto& e) {
  auto& grid = grids.at(e.grid);
  grid.cursorRow = e.row;
  grid.cursorCol = e.col;
}

void GridManager::Line(GridLine& e) {
  auto& grid = grids.at(e.grid);

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
  auto& grid = grids.at(e.grid);

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
  grids.erase(e.grid);
}
