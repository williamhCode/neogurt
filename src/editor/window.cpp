#include "window.hpp"
#include "gfx/instance.hpp"
#include "glm/ext/vector_float2.hpp"
#include "webgpu_utils/webgpu.hpp"

using namespace wgpu;

void WinManager::InitRenderData(Win& win) {
  auto pos = glm::vec2(win.startCol, win.startRow) * sizes.charSize;
  auto size = glm::vec2(win.width, win.height) * sizes.charSize;

  win.renderTexture =
    RenderTexture(pos, size, sizes.dpiScale, TextureFormat::BGRA8Unorm);

  auto fbSize = size * sizes.dpiScale;
  Extent3D maskSize{(uint32_t)fbSize.x, (uint32_t)fbSize.y};
  win.maskTextureView =
    utils::CreateRenderTexture(ctx.device, maskSize, TextureFormat::R8Unorm)
      .CreateView();

  auto maskPos = pos * sizes.dpiScale;
  win.maskPosBuffer =
    utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec2), &maskPos);

  win.maskBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.maskBGL,
    {
      {0, win.maskTextureView},
      {1, win.maskPosBuffer},
    }
  );

  const size_t maxTextQuads = win.width * win.height;
  win.rectData.CreateBuffers(maxTextQuads);
  win.textData.CreateBuffers(maxTextQuads);

  win.grid.dirty = true;

  win.pos = pos;
  win.size = size;
}

void WinManager::UpdateRenderData(Win& win) {
  auto pos = glm::vec2(win.startCol, win.startRow) * sizes.charSize;
  auto size = glm::vec2(win.width, win.height) * sizes.charSize;

  bool posChanged = pos != win.pos;
  bool sizeChanged = size != win.size;
  if (!posChanged && !sizeChanged) {
    return;
  }

  if (sizeChanged) {
    win.renderTexture.Update(pos, size, sizes.dpiScale);
  } else {
    win.renderTexture.UpdateRegion(pos, size);
  }

  if (sizeChanged) {
    auto fbSize = size * sizes.dpiScale;
    Extent3D maskSize{(uint32_t)fbSize.x, (uint32_t)fbSize.y};
    win.maskTextureView =
      utils::CreateRenderTexture(ctx.device, maskSize, TextureFormat::R8Unorm)
        .CreateView();

    win.maskBG = utils::MakeBindGroup(
      ctx.device, ctx.pipeline.maskBGL,
      {
        {0, win.maskTextureView},
        {1, win.maskPosBuffer},
      }
    );

    win.grid.dirty = true;
  }

  if (posChanged) {
    auto maskPos = pos * sizes.dpiScale;
    ctx.queue.WriteBuffer(win.maskPosBuffer, 0, &maskPos, sizeof(glm::vec2));
  }

  if (sizeChanged) {
    const size_t maxTextQuads = win.width * win.height;
    win.rectData.CreateBuffers(maxTextQuads);
    win.textData.CreateBuffers(maxTextQuads);
  }

  win.pos = pos;
  win.size = size;
}

void WinManager::Pos(const WinPos& e) {
  auto [it, first] =
    windows.try_emplace(e.grid, Win{.grid = gridManager->grids.at(e.grid)});
  auto& win = it->second;

  win.startRow = e.startRow;
  win.startCol = e.startCol;

  win.width = e.width;
  win.height = e.height;

  win.hidden = false;

  if (first) {
    InitRenderData(win);
  } else {
    UpdateRenderData(win);
  }
}

void WinManager::FloatPos(const WinFloatPos& e) {
  auto [winIt, first] =
    windows.try_emplace(e.grid, Win{.grid = gridManager->grids.at(e.grid)});
  auto& win = winIt->second;

  win.width = win.grid.width;
  win.height = win.grid.height;

  win.hidden = false;

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

  if (first) {
    InitRenderData(win);
  } else {
    UpdateRenderData(win);
  }
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
  auto [winIt, first] =
    windows.try_emplace(e.grid, Win{.grid = gridManager->grids.at(e.grid)});
  auto& win = winIt->second;

  win.startRow = e.row;
  win.startCol = 0;

  win.width = win.grid.width;
  win.height = win.grid.height;

  win.hidden = false;

  if (first) {
    InitRenderData(win);
  } else {
    UpdateRenderData(win);
  }
}

void WinManager::Viewport(const WinViewport& e) {
}

void WinManager::Extmark(const WinExtmark& e) {
}

Win* WinManager::GetActiveWin() {
  auto it = windows.find(activeWinId);
  if (it == windows.end()) return nullptr;
  return &it->second;
}

MouseInfo WinManager::GetMouseInfo(glm::vec2 cursorPos) {
  cursorPos -= sizes.offset;
  int globalRow = cursorPos.y / sizes.charSize.y;
  int globalCol = cursorPos.x / sizes.charSize.x;

  int grid = 1; // default grid number
  for (auto& [id, win] : windows) {
    if (win.hidden || id == 1) continue;

    int top = win.startRow;
    int bottom = win.startRow + win.height;
    int left = win.startCol;
    int right = win.startCol + win.width;

    if (globalRow >= top && globalRow < bottom &&
        globalCol >= left && globalCol < right) {
      grid = id;
      break;
    }
  }

  auto& win = windows.at(grid);
  int row = std::max(globalRow - win.startRow, 0);
  int col = std::max(globalCol - win.startCol, 0);

  return {grid, row, col};
}

MouseInfo WinManager::GetMouseInfo(int grid, glm::vec2 cursorPos) {
  auto& win = windows.at(grid);

  cursorPos -= sizes.offset;
  int globalRow = cursorPos.y / sizes.charSize.y;
  int globalCol = cursorPos.x / sizes.charSize.x;

  int row = std::max(globalRow - win.startRow, 0);
  int col = std::max(globalCol - win.startCol, 0);

  return {grid, row, col};
}
