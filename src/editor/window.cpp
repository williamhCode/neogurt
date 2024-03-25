#include "window.hpp"
#include "gfx/instance.hpp"
#include "glm/ext/vector_float2.hpp"
#include "utils/region.hpp"
#include "webgpu_utils/webgpu.hpp"

using namespace wgpu;

Win::Win(Grid& grid) : grid(grid) {
  MakeTextureBG();
  renderData.CreateBuffers(1);
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
  renderData.Increment();

  renderData.WriteBuffers();
}

void WinManager::Pos(const WinPos& e) {
  auto [it, first] = windows.try_emplace(e.grid, Win(gridManager->grids.at(e.grid)));
  auto& win = it->second;

  win.startRow = e.startRow;
  win.startCol = e.startCol;

  win.width = e.width;
  win.height = e.height;

  win.hidden = false;

  win.gridSize = gridSize;

  win.MakeTextureBG();
  win.UpdateRenderData();
}

void WinManager::FloatPos(const WinFloatPos& e) {
  auto [winIt, first] = windows.try_emplace(e.grid, Win(gridManager->grids.at(e.grid)));
  auto& win = winIt->second;

  win.width = win.grid.width;
  win.height = win.grid.height;

  win.hidden = false;

  win.gridSize = gridSize;

  auto anchorIt = windows.find(e.anchorGrid);
  if (anchorIt == windows.end()) {
    LOG_ERR("WinManager::FloatPos: anchor grid {} not found", e.anchorGrid);
    return;
  }
  auto& anchorWin = anchorIt->second;

  auto north = anchorWin.startRow + e.anchorRow;
  auto south = anchorWin.startRow + e.anchorRow - win.height;
  auto west = anchorWin.startCol + e.anchorCol;
  auto east = anchorWin.startCol + e.anchorCol - win.width;
  if (e.anchor == "NW") {
    win.startRow = north;
    win.startCol = west;
  } else if (e.anchor == "NE") {
    win.startRow = north;
    win.startCol = east;
  } else if (e.anchor == "SW") {
    win.startRow = south;
    win.startCol = west;
  } else if (e.anchor == "SE") {
    win.startRow = south;
    win.startCol = east;
  } else {
    LOG_WARN("WinManager::FloatPos: unknown anchor {}", e.anchor);
  }

  win.floatData = FloatData{
    .focusable = e.focusable,
    .zindex = e.zindex,
  };

  win.MakeTextureBG();
  win.UpdateRenderData();
}

void WinManager::ExternalPos(const WinExternalPos& e) {
}

void WinManager::Hide(const WinHide& e) {
  // NOTE: temporary to fix nvim issue
  if (auto it = windows.find(e.grid); it != windows.end()) {
    auto& win = it->second;
    win.hidden = true;
  } else {
  }
}

void WinManager::Close(const WinClose& e) {
  auto removed = windows.erase(e.grid);
  if (removed == 0) {
    // see editor/state.cpp GridDestroy
    LOG_WARN("WinManager::Close: window {} not found - ignore due to nvim bug", e.grid);
  }
}

void WinManager::MsgSet(const MsgSetPos& e) {
  auto [winIt, first] = windows.try_emplace(e.grid, Win(gridManager->grids.at(e.grid)));
  auto& win = winIt->second;

  win.startRow = e.row;
  win.startCol = 0;

  win.width = win.grid.width;
  win.height = win.grid.height;

  win.hidden = false;

  win.gridSize = gridSize;

  win.MakeTextureBG();
  win.UpdateRenderData();
}

void WinManager::Viewport(const WinViewport& e) {
}

void WinManager::Extmark(const WinExtmark& e) {
}
