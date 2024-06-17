#pragma once

#include "glm/ext/vector_float2.hpp"
#include "webgpu/webgpu_cpp.h"

#include "gfx/quad.hpp"
#include "gfx/render_texture.hpp"
#include "nvim/events/parse.hpp"
#include "editor/grid.hpp"
#include "app/size.hpp"

#include <map>
#include <optional>

struct FMargins {
  float top = 0;
  float bottom = 0;
  float left = 0;
  float right = 0;
};

struct Margins {
  int top = 0;
  int bottom = 0;
  int left = 0;
  int right = 0;

  bool Empty() const;
  FMargins ToFloat(glm::vec2 size) const;
};

struct FloatData {
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

  // RenderTexture renderTexture;
  // RenderTexture prevRenderTexture;
  // bool hasPrevRender = false;

  ScrollableRenderTexture sRenderTexture;

  wgpu::TextureView maskTextureView;
  wgpu::Buffer maskPosBuffer;
  wgpu::BindGroup maskBG;

  QuadRenderData<RectQuadVertex> rectData;
  QuadRenderData<TextQuadVertex> textData;

  // FMargins fmargins; // margin size in pixels

  // bool scrolling;
  // float scrollDist;
  // float scrollCurr;
  // float scrollTime = 0.1; // transition time
  // float scrollElapsed;

  QuadRenderData<TextureQuadVertex> marginsData;
};

// for input handler
struct MouseInfo {
  int grid;
  int row;
  int col;
};

struct WinManager {
  GridManager* gridManager;
  const SizeHandler& sizes;
  bool dirty; // true if window pos updated from scrolling

  using ColorBytes = glm::vec<4, uint8_t>;
  ColorBytes clearColor;

  // not unordered because telescope float background overlaps text
  // so have to render floats in reverse order else text will be covered
  std::map<int, Win> windows;
  int msgWinId = -1;

  void InitRenderData(Win& win);
  void UpdateRenderData(Win& win);

  void Pos(const WinPos& e);
  void FloatPos(const WinFloatPos& e);
  void ExternalPos(const WinExternalPos& e);
  void Hide(const WinHide& e);
  void Close(const WinClose& e);
  void MsgSet(const MsgSetPos& e);
  void Viewport(const WinViewport& e);
  void UpdateScrolling(float dt);
  void ViewportMargins(const WinViewportMargins& e);
  void Extmark(const WinExtmark& e);

  int activeWinId = 0;
  Win* GetActiveWin();

  MouseInfo GetMouseInfo(glm::vec2 mousePos);
  MouseInfo GetMouseInfo(int grid, glm::vec2 mousePos);

  Win* GetMsgWin();
};
