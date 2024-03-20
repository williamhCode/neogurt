#include "window.hpp"
#include "gfx/instance.hpp"
#include "glm/ext/vector_float2.hpp"
#include "utils/region.hpp"
#include "webgpu_utils/webgpu.hpp"

using namespace wgpu;

Win::Win(Grid& grid) : grid(grid) {
  // when creating new window, grid_resize is called first, so we can create
  // bind group with grid's textureview when initializing window first time

  // but when resizing window, order is unknown, so we update from grid_resize
  MakeTextureBG();

  renderData.CreateBuffers(1);
  renderData.ReserveVectors(1);
}

void Win::MakeTextureBG() {
  auto textureSampler = ctx.device.CreateSampler(ToPtr(SamplerDescriptor{
    .addressModeU = AddressMode::ClampToEdge,
    .addressModeV = AddressMode::ClampToEdge,
    .magFilter = FilterMode::Nearest,
    .minFilter = FilterMode::Nearest,
  }));

  textureBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.textureBGL,
    {
      {0, grid.textureView},
      {1, textureSampler},
    }
  );
}

void Win::UpdateRenderData() {
  renderData.ResetCounts();

  auto positions = MakeRegion(startCol, startRow, width, height);
  auto uvs = MakeRegion(0, 0, 1, 1);

  for (size_t i = 0; i < 4; i++) {
    auto& vertex = renderData.quads[0][i];
    vertex.position = positions[i] * gridSize;
    vertex.uv = uvs[i];
  }

  renderData.SetIndices();
  renderData.IncrementCounts();

  renderData.WriteBuffers();
}

void Win::UpdateFloatPos() {
  auto& anchorWin = *floatData->anchorWin;
  auto& e = *floatData;

  auto north = anchorWin.startRow + e.anchorRow;
  auto south = anchorWin.startRow + e.anchorRow - height;
  auto west = anchorWin.startCol + e.anchorCol;
  auto east = anchorWin.startCol + e.anchorCol - width;
  switch (e.anchor) {
    case Anchor::NW:
      startRow = north;
      startCol = west;
      break;
    case Anchor::NE:
      startRow = north;
      startCol = east;
      break;
    case Anchor::SW:
      startRow = south;
      startCol = west;
      break;
    case Anchor::SE:
      startRow = south;
      startCol = east;
      break;
    default: assert(false);
  }
}

void WinManager::Pos(WinPos& e) {
  auto [it, first] = windows.try_emplace(e.grid, Win(gridManager->grids.at(e.grid)));
  auto& win = it->second;

  win.grid.win = &win;

  win.startRow = e.startRow;
  win.startCol = e.startCol;

  win.width = e.width;
  win.height = e.height;

  win.hidden = false;

  win.gridSize = gridSize;

  win.UpdateRenderData();
}

void WinManager::FloatPos(WinFloatPos& e) {
  auto [winIt, first] = windows.try_emplace(e.grid, Win(gridManager->grids.at(e.grid)));
  if (!first) return; // assume float can't be moved after creation
  auto& win = winIt->second;

  win.grid.win = &win;

  win.width = win.grid.width;
  win.height = win.grid.height;

  win.hidden = false;

  win.gridSize = gridSize;

  auto anchorIt = windows.find(e.anchorGrid);
  if (anchorIt == windows.end()) {
    LOG_ERR("WinManager::FloatPos: anchor grid {} not found", e.anchorGrid);
    return;
  }
  win.floatData = FloatData{
    .anchorWin = &anchorIt->second,
    .anchor =
      [&] {
        if (e.anchor == "NW") return Anchor::NW;
        if (e.anchor == "NE") return Anchor::NE;
        if (e.anchor == "SW") return Anchor::SW;
        if (e.anchor == "SE") return Anchor::SE;
        LOG_WARN("WinManager::FloatPos: unknown anchor {}", e.anchor);
      }(),
    .anchorGrid = e.anchorGrid,
    .anchorRow = e.anchorRow,
    .anchorCol = e.anchorCol,
    .focusable = e.focusable,
    .zindex = e.zindex,
  };

  win.UpdateFloatPos();
  win.UpdateRenderData();
}

void WinManager::ExternalPos(WinExternalPos& e) {
}

void WinManager::Hide(WinHide& e) {
  // NOTE: temporary to fix nvim issue
  if (auto it = windows.find(e.grid); it != windows.end()) {
    auto& win = it->second;
    win.hidden = true;
  } else {
  }
}

void WinManager::Close(WinClose& e) {
  auto removed = windows.erase(e.grid);
  if (removed == 0) {
    LOG_WARN("WinManager::Close: window {} not found", e.grid);
  }
}

void WinManager::Viewport(WinViewport& e) {
}

void WinManager::Extmark(WinExtmark& e) {
}
