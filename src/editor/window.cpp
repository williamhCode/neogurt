#include "window.hpp"
#include "gfx/instance.hpp"
#include "glm/common.hpp"
#include "glm/exponential.hpp"
#include "glm/ext/vector_float2.hpp"
#include "webgpu_tools/utils/webgpu.hpp"
#include <algorithm>
#include <cstdlib>
#include <utility>

using namespace wgpu;

void WinManager::InitRenderData(Win& win) {
  auto pos = glm::vec2(win.startCol, win.startRow) * sizes.charSize;
  auto size = glm::vec2(win.width, win.height) * sizes.charSize;

  win.renderTexture = RenderTexture(size, sizes.dpiScale, TextureFormat::BGRA8Unorm);
  win.renderTexture.UpdatePos(pos);
  win.prevRenderTexture =
    RenderTexture(size, sizes.dpiScale, TextureFormat::BGRA8Unorm);
  win.prevRenderTexture.UpdatePos(pos);

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

  win.marginsData.CreateBuffers(4);

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
    win.renderTexture = RenderTexture(size, sizes.dpiScale, TextureFormat::BGRA8Unorm);
    win.renderTexture.UpdatePos(pos);
    win.prevRenderTexture =
      RenderTexture(size, sizes.dpiScale, TextureFormat::BGRA8Unorm);
    win.prevRenderTexture.UpdatePos(pos);
  } else {
    win.renderTexture.UpdatePos(pos);
    win.prevRenderTexture.UpdatePos(pos);
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
  // auto it = windows.find(e.grid);
  // if (it == windows.end()) {
  //   LOG_ERR("WinManager::Hide: window {} not found", e.grid);
  //   return;
  // }
  // auto& win = it->second;
  // win.hidden = true;

  // save memory because tabs get hidden
  auto removed = windows.erase(e.grid);
  if (removed == 0) {
    LOG_ERR("WinManager::Hide: window {} not found", e.grid);
  }
}

void WinManager::Close(const WinClose& e) {
  auto removed = windows.erase(e.grid);
  if (removed == 0) {
    // see editor/state.cpp GridDestroy
    // LOG_WARN("WinManager::Close: window {} not found - ignore due to nvim bug",
    // e.grid);
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
  auto it = windows.find(e.grid);
  if (it == windows.end()) {
    LOG_ERR("WinManager::Viewport: window {} not found", e.grid);
    return;
  }
  auto& win = it->second;

  bool shouldScroll =              //
    std::abs(e.scrollDelta) > 0 && //
    std::abs(e.scrollDelta) <= win.height - (win.margins.top + win.margins.bottom);
  if (shouldScroll) {
    win.scrolling = true;
    win.scrollDist = e.scrollDelta * sizes.charSize.y;
    win.scrollElapsed = 0;

    std::swap(win.prevRenderTexture, win.renderTexture);
  }
}

void WinManager::UpdateScrolling(float dt) {
  for (auto& [id, win] : windows) {
    if (!win.scrolling) continue;

    win.scrollElapsed += dt;
    dirty = true;

    auto pos = win.pos;

    if (win.scrollElapsed >= win.scrollTime) {
      win.scrolling = false;
      win.scrollElapsed = 0;

      win.renderTexture.UpdatePos(pos);

      auto maskPos = pos * sizes.dpiScale;
      ctx.queue.WriteBuffer(win.maskPosBuffer, 0, &maskPos, sizeof(glm::vec2));

    } else {
      auto size = win.size;
      auto& margins = win.fmargins;

      float t = win.scrollElapsed / win.scrollTime;
      float x = glm::pow(t, 1 / 2.8f);
      float scrollCurrAbs = glm::mix(0.0f, glm::abs(win.scrollDist), x);
      win.scrollCurr = glm::sign(win.scrollDist) * scrollCurrAbs;
      auto scrollDistAbs = glm::abs(win.scrollDist);

      if (glm::sign(win.scrollDist) == 1) {
        pos.y -= scrollCurrAbs;

        RegionHandle prevRegion{
          .pos = {margins.left, margins.top + scrollCurrAbs},
          .size =
            {
              size.x - margins.left - margins.right,
              scrollDistAbs - scrollCurrAbs,
            },
        };
        win.prevRenderTexture.UpdatePos(pos + prevRegion.pos, prevRegion);

        RegionHandle region{
          .pos = {margins.left, margins.top},
          .size =
            {
              size.x - margins.left - margins.right,
              size.y - margins.top - margins.bottom - (scrollDistAbs - scrollCurrAbs),
            },
        };
        win.renderTexture.UpdatePos(
          pos + glm::vec2(0, scrollDistAbs) + region.pos, region
        );

      } else {
        pos.y += scrollCurrAbs;

        RegionHandle region{
          .pos = {margins.left, margins.top + (scrollDistAbs - scrollCurrAbs)},
          .size =
            {
              size.x - margins.left - margins.right,
              size.y - margins.top - margins.bottom - (scrollDistAbs - scrollCurrAbs),
            },
        };
        win.renderTexture.UpdatePos(
          pos + glm::vec2(0, -scrollDistAbs) + region.pos, region
        );

        RegionHandle prevRegion{
          .pos = {margins.left, size.y - scrollDistAbs - margins.bottom},
          .size =
            {
              size.x - margins.left - margins.right,
              scrollDistAbs - scrollCurrAbs,
            },
        };
        win.prevRenderTexture.UpdatePos(pos + prevRegion.pos, prevRegion);
      }

      auto maskPos = (pos + glm::vec2(0, win.scrollDist)) * sizes.dpiScale;
      ctx.queue.WriteBuffer(win.maskPosBuffer, 0, &maskPos, sizeof(glm::vec2));
    }
  }
}

void WinManager::ViewportMargins(const WinViewportMargins& e) {
  auto it = windows.find(e.grid);
  if (it == windows.end()) {
    // LOG_ERR("WinManager::ViewportMargins: window {} not found", e.grid);
    return;
  }
  auto& win = it->second;

  win.margins.top = e.top;
  win.margins.bottom = e.bottom;
  win.margins.left = e.left;
  win.margins.right = e.right;

  win.fmargins = win.margins.ToFloat(sizes.charSize);

  win.marginsData.ResetCounts();

  auto SetData = [&](glm::vec2 pos, glm::vec2 size) {
    auto positions = MakeRegion(pos, size);
    auto uvs = MakeRegion(pos, size / win.size);

    for (size_t i = 0; i < 4; i++) {
      auto& vertex = win.marginsData.CurrQuad()[i];
      vertex.position = win.pos + positions[i];
      vertex.uv = uvs[i];
    }
    win.marginsData.Increment();
  };

  if (win.margins.top != 0) {
    SetData({0, 0}, {win.size.x, win.fmargins.top});
  }
  if (win.margins.bottom != 0) {
    SetData({0, win.size.y - win.fmargins.bottom}, {win.size.x, win.fmargins.bottom});
  }
  if (win.margins.left != 0) {
    SetData(
      {0, win.fmargins.top},
      {win.fmargins.left, win.size.y - win.fmargins.top - win.fmargins.bottom}
    );
  }
  if (win.margins.right != 0) {
    SetData(
      {win.size.x - win.fmargins.right, win.fmargins.top},
      {win.fmargins.right, win.size.y - win.fmargins.top - win.fmargins.bottom}
    );
  }
  win.marginsData.WriteBuffers();
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

  std::vector<std::pair<int, const Win*>> sortedWins;
  for (auto& [id, win] : windows) {
    if (win.hidden || id == 1) continue;
    sortedWins.emplace_back(id, &win);
  }

  std::ranges::sort(sortedWins, [](const auto& a, const auto& b) {
    return a.second->floatData.value_or(FloatData{.zindex = 0}).zindex >
           b.second->floatData.value_or(FloatData{.zindex = 0}).zindex;
  });

  int grid = 1; // default grid number
  for (auto [id, win] : sortedWins) {
    if (win->hidden || id == 1 || (win->floatData && !win->floatData->focusable)) {
      continue;
    }

    int top = win->startRow;
    int bottom = win->startRow + win->height;
    int left = win->startCol;
    int right = win->startCol + win->width;

    if (globalRow >= top && globalRow < bottom && globalCol >= left && globalCol < right) {
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
