#pragma once

#include "gfx/pipeline.hpp"
#include "gfx/quad.hpp"
#include "gfx/render_texture.hpp"

#include "utils/margins.hpp"
#include "editor/grid.hpp"
#include "app/size.hpp"

#include "glm/ext/vector_float2.hpp"
#include <unordered_map>
#include <optional>
#include <deque>

struct FloatData {
  int anchorGrid;
  bool focusable;
  int zindex;
};

struct Win {
  int id;
  Grid& grid;

  int startRow;
  int startCol;

  int width;
  int height;

  bool hidden;

  Margins margins;

  // exists only if window is floating
  std::optional<FloatData> floatData;
  bool IsFloating() const {
    return floatData.has_value();
  }

  // rendering data
  glm::vec2 pos;
  glm::vec2 size;

  QuadRenderData<RectQuadVertex, true> rectData;
  QuadRenderData<TextQuadVertex, true> textData;
  QuadRenderData<ShapeQuadVertex, true> shapeData;

  ScrollableRenderTexture sRenderTexture;
};

// for input handler
struct MouseInfo {
  int grid;
  int row;
  int col;
};

struct WinManager {
  static const int defaultGridId = 1;

  GridManager* gridManager;
  SizeHandler sizes;
  bool dirty; // true if window pos updated from scrolling

  std::deque<Win*> windowsOrder;
  std::unordered_map<int, Win> windows;
  int msgWinId = -1;

  // added to public functions called in main thread that reads (main thread doesn't write)
  // added to public functions called in render thread that writes
  mutable std::mutex windowsMutex;

private:
  int CalcMaxTexPerPage(const Win& win);
  void InitRenderData(Win& win);
  void UpdateRenderData(Win& win);

public:
  // updates dpi scale for all windows if changed
  void TryChangeDpiScale(float dpiScale);

  void Pos(const event::WinPos& e);
  void FloatPos(int grid);
  void FloatPos(const event::WinFloatPos& e);
  void ExternalPos(const event::WinExternalPos& e);
  void Hide(const event::WinHide& e);
  void Close(const event::WinClose& e);
  void MsgSetPos(const event::MsgSetPos& e);
  void Viewport(const event::WinViewport& e);
  void UpdateScrolling(float dt);
  bool ViewportMargins(const event::WinViewportMargins& e);
  void Extmark(const event::WinExtmark& e);

  MouseInfo GetMouseInfo(glm::vec2 mousePos) const;
  MouseInfo GetMouseInfo(int grid, glm::vec2 mousePos) const;

  const Win* GetWin(int id) const;
  const Win* GetMsgWin() const;
};
