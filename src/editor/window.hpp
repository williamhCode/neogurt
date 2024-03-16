#pragma once

#include "gfx/quad.hpp"
#include "glm/ext/vector_float2.hpp"
#include "nvim/parse.hpp"
#include "editor/grid.hpp"
#include "webgpu/webgpu_cpp.h"
#include <unordered_map>

struct Window {
  Grid* grid;

  int startRow;
  int startCol;

  int width;
  int height;

  int hidden;

  // rendering data
  wgpu::BindGroup textureBG;
  QuadRenderData<TextureQuadVertex> renderData;
};

struct WindowManager {
  GridManager* gridManager;
  glm::vec2 gridSize;

  std::unordered_map<int, Window> windows;

  void Pos(WinPos& e);
  void FloatPos(WinFloatPos& e);
  void ExternalPos(WinExternalPos& e);
  void Hide(WinHide& e);
  void Close(WinClose& e);
  // void MsgSetPos(MsgSetPos& e);
  void Viewport(WinViewport& e);
  void Extmark(WinExtmark& e);
};
