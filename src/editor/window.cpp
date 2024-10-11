#include "window.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/gtx/string_cast.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <cstdlib>
#include <utility>

using namespace wgpu;

void WinManager::InitRenderData(Win& win) {
  auto pos = glm::vec2(win.startCol, win.startRow) * sizes.charSize;
  auto size = glm::vec2(win.width, win.height) * sizes.charSize;

  auto numQuads = win.height * std::min(win.width, 80);
  win.rectData.CreateBuffers(numQuads);
  win.textData.CreateBuffers(numQuads);
  win.shapeData.CreateBuffers(numQuads);

  int maxTexPerPage = win.id == defaultGridId ? 1 : 2;
  win.sRenderTexture =
    ScrollableRenderTexture(size, sizes.dpiScale, sizes.charSize, maxTexPerPage);
  win.sRenderTexture.UpdatePos(pos);

  win.grid.dirty = true;

  win.pos = pos;
  win.size = size;
}

void WinManager::UpdateRenderData(Win& win) {
  auto pos = glm::vec2(win.startCol, win.startRow) * sizes.charSize;
  auto size = glm::vec2(win.width, win.height) * sizes.charSize;

  bool posChanged = pos != win.pos;
  // sometimes size can be equal, but charSize different
  // for hidden window, win.sRenderTexture.charSize will always be vec2(0, 0)
  bool sizeChanged = size != win.size || sizes.charSize != win.sRenderTexture.charSize;
  if (!posChanged && !sizeChanged) {
    return;
  }

  if (sizeChanged) {
    auto numQuads = win.height * std::min(win.width, 80);
    win.rectData.CreateBuffers(numQuads);
    win.textData.CreateBuffers(numQuads);
    win.shapeData.CreateBuffers(numQuads);

    int maxTexPerPage = win.id == defaultGridId ? 1 : 2;
    win.sRenderTexture =
      ScrollableRenderTexture(size, sizes.dpiScale, sizes.charSize, maxTexPerPage);
  }
  win.sRenderTexture.UpdatePos(pos);

  if (sizeChanged) {
    win.sRenderTexture.UpdateMargins(win.margins);
  }

  win.grid.dirty = true;

  win.pos = pos;
  win.size = size;
}

void WinManager::Pos(const event::WinPos& e) {
  std::lock_guard lock(windowsMutex);
  auto gridIt = gridManager->grids.find(e.grid);
  if (gridIt == gridManager->grids.end()) {
    LOG_ERR("WinManager::Pos: grid {} not found", e.grid);
    return;
  }
  auto [it, first] = windows.try_emplace(
    e.grid, Win{.id = e.grid, .grid = gridIt->second}
  );
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

// TODO: find a more robust way to handle
// grid and win events not syncing up
void WinManager::FloatPos(int grid) {
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

  if (!win.IsFloating()) return;

  win.width = win.grid.width;
  win.height = win.grid.height;

  win.hidden = false;

  UpdateRenderData(win);
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
  // sometimes the float window is outside the screen
  win.startCol = std::max(win.startCol, 0);

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
  auto removed = windows.erase(e.grid);
  if (removed == 0) {
    // see editor/state.cpp GridDestroy
    // LOG_WARN("WinManager::Close: window {} not found - ignore due to nvim bug",
    // e.grid);
  }
}

void WinManager::MsgSetPos(const event::MsgSetPos& e) {
  std::lock_guard lock(windowsMutex);
  auto gridIt = gridManager->grids.find(e.grid);
  if (gridIt == gridManager->grids.end()) {
    LOG_ERR("WinManager::MsgSet: grid {} not found", e.grid);
    return;
  }
  auto [winIt, first] = windows.try_emplace(
    e.grid, Win{.id = e.grid, .grid = gridIt->second}
  );
  auto& win = winIt->second;

  win.startRow = e.row;
  win.startCol = 0;

  win.width = win.grid.width;
  win.height = win.grid.height;

  win.hidden = false;

  if (msgWinId != -1 && msgWinId != e.grid) {
    Close({.grid = msgWinId});
    gridManager->Destroy({.grid = msgWinId});
  }
  msgWinId = e.grid;

  if (first) {
    InitRenderData(win);
  } else {
    UpdateRenderData(win);
  }
}

void WinManager::Viewport(const event::WinViewport& e) {
  std::lock_guard lock(windowsMutex);
  auto it = windows.find(e.grid);
  if (it == windows.end()) {
    // LOG_ERR("WinManager::Viewport: window {} not found", e.grid);
    return;
  }
  auto& win = it->second;

  bool shouldScroll =              //
    std::abs(e.scrollDelta) > 0 && //
    std::abs(e.scrollDelta) <= win.height - (win.margins.top + win.margins.bottom);

  // LOG_INFO("WinManager::Viewport: grid {} scrollDelta {} shouldScroll {}", e.grid,

  if (!shouldScroll) return;
  float scrollDist = e.scrollDelta * sizes.charSize.y;
  win.sRenderTexture.UpdateViewport(scrollDist);
}

void WinManager::UpdateScrolling(float dt) {
  std::lock_guard lock(windowsMutex);
  for (auto& [id, win] : windows) {
    if (!win.sRenderTexture.scrolling) continue;
    win.sRenderTexture.UpdateScrolling(dt);
    dirty = true;
  }
}

bool WinManager::ViewportMargins(const event::WinViewportMargins& e) {
  std::lock_guard lock(windowsMutex);
  auto it = windows.find(e.grid);
  if (it == windows.end()) {
    // LOG_ERR("WinManager::ViewportMargins: window {} not found", e.grid);
    return false;
  }
  auto& win = it->second;

  win.margins.top = e.top;
  win.margins.bottom = e.bottom;
  win.margins.left = e.left;
  win.margins.right = e.right;

  win.sRenderTexture.UpdateMargins(win.margins);

  return true;
}

void WinManager::Extmark(const event::WinExtmark& e) {
}

MouseInfo WinManager::GetMouseInfo(glm::vec2 mousePos) const {
  std::lock_guard lock(windowsMutex);
  mousePos -= sizes.offset;
  int globalRow = mousePos.y / sizes.charSize.y;
  int globalCol = mousePos.x / sizes.charSize.x;

  std::vector<std::pair<int, const Win*>> sortedWins;
  for (const auto& [id, win] : windows) {
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

    if (globalRow >= top && globalRow < bottom && globalCol >= left &&
        globalCol < right) {
      grid = id;
      break;
    }
  }

  auto it = windows.find(grid);
  assert(it != windows.end() && "Windows changed mid function, data race occured");
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
