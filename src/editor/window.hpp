#pragma once

#include "gfx/quad.hpp"
#include "glm/ext/vector_float2.hpp"
#include "nvim/parse.hpp"
#include "editor/grid.hpp"
#include "webgpu/webgpu_cpp.h"
#include <map>

enum class Anchor { NW, NE, SW, SE };

struct FloatData {
  Win* anchorWin;

  Anchor anchor;
  int anchorGrid;
  float anchorRow;
  float anchorCol;

  bool focusable;
  int zindex;
};

struct Win {
  Grid& grid;

  int startRow;
  int startCol;

  int width;
  int height;

  int hidden;

  // exists only if window is floating
  std::optional<FloatData> floatData;

  // rendering data
  glm::vec2 gridSize;

  wgpu::BindGroup textureBG;
  QuadRenderData<TextureQuadVertex> renderData;

  Win(Grid& grid);
  void MakeTextureBG();
  void UpdateRenderData();
  void UpdateFloatPos();
};

struct WinManager {
  GridManager* gridManager;
  glm::vec2 gridSize;

  // not unordered because telescope float background overlaps text
  // so have to render in reverse order else text will be covered
  std::map<int, Win> windows;

  void Pos(WinPos& e);
  void FloatPos(WinFloatPos& e);
  void ExternalPos(WinExternalPos& e);
  void Hide(WinHide& e);
  void Close(WinClose& e);
  // void MsgSetPos(MsgSetPos& e);
  void Viewport(WinViewport& e);
  void Extmark(WinExtmark& e);
};
