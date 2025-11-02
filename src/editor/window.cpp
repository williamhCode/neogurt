#include "./window.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/gtx/string_cast.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <ranges>

using namespace wgpu;

int WinManager::CalcMaxTexPerPage(const Win& win) {
  if (win.id == defaultGridId || win.id == msgWinId) {
    return 1;
  }
  return 3;
}

void WinManager::UpdateWinAttributes(Win& win) {
  auto pos = glm::vec2(win.startCol, win.startRow) * sizes.charSize;
  auto size = glm::vec2(win.width, win.height) * sizes.charSize;

  bool posChanged = win.pos != pos;
  bool sizeChanged =
    (win.size != size) || (win.sRenderTexture.charSize != sizes.charSize);
  bool dpiChanged = win.sRenderTexture.dpiScale != sizes.dpiScale;

  win.posDirty |= posChanged;
  win.sizeDirty |= sizeChanged || dpiChanged;

  win.pos = pos;
  win.size = size;
}

void WinManager::UpdateRenderData() {
  std::lock_guard lock(windowsMutex);
  for (auto& [id, win] : windows) {
    if (win.hidden) continue;

    if (win.sizeDirty) {
      auto numQuads = win.height * std::min(win.width, 80);
      win.rectData.CreateBuffers(numQuads);
      win.textData.CreateBuffers(numQuads);
      win.emojiData.CreateBuffers(numQuads);

      win.sRenderTexture = ScrollableRenderTexture(
        win.size, sizes.dpiScale, sizes.charSize, CalcMaxTexPerPage(win)
      );
      win.grid.dirty = true;
    }

    if (win.sizeDirty || win.posDirty) {
      win.sRenderTexture.UpdatePos(win.pos);
    }

    if (win.sizeDirty || win.updateMargins) {
      win.sRenderTexture.UpdateMargins(win.margins);
    }

    if (win.scrollDist) {
      win.sRenderTexture.UpdateViewport(*win.scrollDist);
    }

    // reset dirty flags
    win.sizeDirty = false;
    win.posDirty = false;
    win.updateMargins = false;
    win.scrollDist.reset();
  }
}

void WinManager::UpdateScrolling(std::span<float> steps) {
  std::lock_guard lock(windowsMutex);
  for (auto& [id, win] : windows) {
    if (win.sRenderTexture.scrolling) {
      win.sRenderTexture.UpdateScrolling(steps);
      dirty = true;
    }
  }
}

void WinManager::Pos(const event::WinPos& e) {
  std::lock_guard lock(windowsMutex);
  auto gridIt = gridManager->grids.find(e.grid);
  if (gridIt == gridManager->grids.end()) {
    LOG_ERR("WinManager::Pos: grid {} not found", e.grid);
    return;
  }
  auto [winIt, first] = windows.try_emplace(
    e.grid, Win{.id = e.grid, .grid = gridIt->second}
  );
  auto& win = winIt->second;
  if (first) windowsOrder.emplace_front(&win);

  win.startRow = e.startRow;
  win.startCol = e.startCol;

  win.width = e.width;
  win.height = e.height;

  win.hidden = false;

  UpdateWinAttributes(win);
}

// TODO: find a more robust way to handle grid and win events not syncing up
void WinManager::SyncResize(int grid) {
  std::lock_guard lock(windowsMutex);
  auto gridIt = gridManager->grids.find(grid);
  if (gridIt == gridManager->grids.end()) {
    LOG_ERR("WinManager::FloatPos: grid {} not found", grid);
    return;
  }

  auto winIt = windows.find(grid);
  if (winIt == windows.end()) {
    return;
  }
  auto& win = winIt->second;

  win.width = win.grid.width;
  win.height = win.grid.height;

  win.hidden = false;

  UpdateWinAttributes(win);
}

void WinManager::FloatPos(const event::WinFloatPos& e) {
  std::lock_guard lock(windowsMutex);
  auto gridIt = gridManager->grids.find(e.grid);
  if (gridIt == gridManager->grids.end()) {
    LOG_ERR("WinManager::FloatPos: grid {} not found", e.grid);
    return;
  }
  auto [winIt, first] = windows.try_emplace(
    e.grid, Win{.id = e.grid, .grid = gridIt->second}
  );
  auto& win = winIt->second;
  if (first) windowsOrder.emplace_front(&win);

  win.width = win.grid.width;
  win.height = win.grid.height;

  win.hidden = false;

  int startRow = 0;
  int startCol = 0;
  auto anchorIt = windows.find(e.anchorGrid);
  if (anchorIt != windows.end()) {
    auto& anchorWin = anchorIt->second;
    startRow = anchorWin.startRow;
    startCol = anchorWin.startCol;
  } else {
    LOG_ERR("WinManager::FloatPos: anchor grid {} not found", e.anchorGrid);
  }

  int north = startRow + e.anchorRow;
  int south = startRow + e.anchorRow - win.height;
  int west = startCol + e.anchorCol;
  int east = startCol + e.anchorCol - win.width;
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
  // sometimes the float window is outside the screen
  win.startCol = std::min(win.startCol, sizes.uiWidth - win.width);
  win.startCol = std::max(win.startCol, 0);

  win.floatData = FloatData{
    .anchorGrid = e.anchorGrid,
    .mouseEnabled = e.mouseEnabled,
    .zindex = e.zindex,
    .compindex = e.compindex,
  };

  UpdateWinAttributes(win);
}

void WinManager::ExternalPos(const event::WinExternalPos& e) {
}

void WinManager::Hide(const event::WinHide& e) {
  std::lock_guard lock(windowsMutex);
  auto it = windows.find(e.grid);
  if (it == windows.end()) {
    LOG_ERR("WinManager::Hide: window {} not found", e.grid);
    return;
  }
  auto& win = it->second;
  win.hidden = true;

  // save memory when window gets hidden (e.g. switching tabs)
  win.sRenderTexture = {};
}

void WinManager::Close(const event::WinClose& e) {
  std::lock_guard lock(windowsMutex);
  std::erase_if(windowsOrder, [id = e.grid](const Win* win) {
    return win->id == id;
  });
  auto removed = windows.erase(e.grid);
  if (removed == 1) {

  } else {
    // see editor/state.cpp GridDestroy
    // LOG_WARN("WinManager::Close: window {} not found - ignore due to nvim bug",
    // e.grid);
  }
}

void WinManager::MsgSetPos(const event::MsgSetPos& e) {
  std::lock_guard lock(windowsMutex);
  auto gridIt = gridManager->grids.find(e.grid);
  if (gridIt == gridManager->grids.end()) {
    LOG_ERR("WinManager::MsgSetPos: grid {} not found", e.grid);
    return;
  }
  auto [winIt, first] = windows.try_emplace(
    e.grid, Win{.id = e.grid, .grid = gridIt->second}
  );
  auto& win = winIt->second;
  if (first) windowsOrder.emplace_front(&win);

  win.startRow = e.row;
  win.startCol = 0;

  win.width = win.grid.width;
  win.height = win.grid.height;

  win.hidden = false;

  // act as floating window if scrolled
  // NOTE: e.zindex != 0 for compatibility with nvim -v < 0.12, for 0.12 onward should have zindex == 200
  if (e.scrolled && e.zindex != 0) {
    win.floatData = FloatData{
      .zindex = e.zindex,
      .compindex = e.compindex,
    };
  } else {
    win.floatData.reset();
  }

  if (msgWinId != -1 && msgWinId != e.grid) {
    Close({.grid = msgWinId});
  }
  msgWinId = e.grid;

  UpdateWinAttributes(win);
}

void WinManager::Viewport(const event::WinViewport& e) {
  std::lock_guard lock(windowsMutex);
  auto it = windows.find(e.grid);
  if (it == windows.end()) {
    // LOG_ERR("WinManager::Viewport: window {} not found", e.grid);
    return;
  }
  auto& win = it->second;

  bool shouldScroll =
    std::abs(e.scrollDelta) > 0 &&
    std::abs(e.scrollDelta) <= win.height - (win.margins.top + win.margins.bottom);

  // LOG_INFO("WinManager::Viewport: grid {} scrollDelta {} shouldScroll {}", e.grid,

  if (!shouldScroll) return;
  float scrollDist = e.scrollDelta * sizes.charSize.y;
  if (win.scrollDist) {
    *win.scrollDist += scrollDist;
  } else {
    win.scrollDist = scrollDist;
  }
}

void WinManager::ViewportMargins(const event::WinViewportMargins& e) {
  std::lock_guard lock(windowsMutex);
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

  win.updateMargins = true;
}

void WinManager::Extmark(const event::WinExtmark& e) {
}

MouseInfo WinManager::GetMouseInfo(glm::vec2 mousePos) const {
  std::lock_guard lock(windowsMutex);
  mousePos -= sizes.offset;
  int globalRow = mousePos.y / sizes.charSize.y;
  int globalCol = mousePos.x / sizes.charSize.x;

  std::vector<const Win*> sortedWins;
  for (const auto* win : windowsOrder) {
    if (win->hidden || win->id == 1 || (win->floatData && !win->floatData->mouseEnabled)) {
      continue;
    }
    sortedWins.push_back(win);
  }

  // NOTE: sort by zindex for backward compatable, nvim 0.12 onward can use compindex
  std::ranges::stable_sort(sortedWins, [](const auto& a, const auto& b) {
    float za = a->floatData.value_or(FloatData{.zindex = -1}).zindex;
    float zb = b->floatData.value_or(FloatData{.zindex = -1}).zindex;

    if (za > zb) return true;
    if (za < zb) return false;

    return a->floatData.value_or(FloatData{.compindex = -1}).compindex >
           b->floatData.value_or(FloatData{.compindex = -1}).compindex;
  });

  int grid = 1; // default grid number
  for (const auto* win : sortedWins) {
    int top = win->startRow;
    int bottom = win->startRow + win->height;
    int left = win->startCol;
    int right = win->startCol + win->width;

    if (globalRow >= top && globalRow < bottom && globalCol >= left &&
        globalCol < right) {
      grid = win->id;
      break;
    }
  }

  auto it = windows.find(grid);

  // grid events not sent yet, no windows
  if (it == windows.end()) {
    return {grid, globalRow, globalCol};
  }

  const auto& win = it->second;
  int row = std::max(globalRow - win.startRow, 0);
  int col = std::max(globalCol - win.startCol, 0);

  return {grid, row, col};
}

MouseInfo WinManager::GetMouseInfo(int grid, glm::vec2 mousePos) const {
  {
    std::lock_guard lock(windowsMutex);
    auto it = windows.find(grid);
    if (it != windows.end()) {
      const auto& win = it->second;

      mousePos -= sizes.offset;
      int globalRow = mousePos.y / sizes.charSize.y;
      int globalCol = mousePos.x / sizes.charSize.x;

      int row = std::max(globalRow - win.startRow, 0);
      int col = std::max(globalCol - win.startCol, 0);

      return {grid, row, col};
    }
  }
  return GetMouseInfo(mousePos);
}

const Win* WinManager::GetWin(int id) const {
  auto it = windows.find(id);
  if (it == windows.end()) return nullptr;
  return &it->second;
}

const Win* WinManager::GetMsgWin() const {
  return GetWin(msgWinId);
}
