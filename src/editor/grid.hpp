#pragma once

#include "gfx/quad.hpp"
#include "glm/ext/vector_float2.hpp"
#include "nvim/parse.hpp"
#include "utils/ring_buffer.hpp"
#include "webgpu/webgpu_cpp.h"
#include <vector>

struct Window; // forward decl

struct Grid {
  Window* win;

  int width;
  int height;

  int cursorRow;
  int cursorCol;

  struct Cell {
    std::string text;
    int hlId = 0;
  };
  using Line = std::vector<Cell>;
  using Lines = RingBuffer<Line>;
  Lines lines;

  // rendering
  bool dirty;

  wgpu::Buffer viewProjBuffer;
  wgpu::BindGroup viewProjBG;
  wgpu::TextureView textureView;

  QuadRenderData<RectQuadVertex> rectData;
  QuadRenderData<TextQuadVertex> textData;
};

struct GridManager {
  glm::vec2 gridSize;
  float dpiScale;

  std::unordered_map<int, Grid> grids;

  void Resize(GridResize& e);
  void Clear(GridClear& e);
  void CursorGoto(GridCursorGoto& e);
  void Line(GridLine& e);
  void Scroll(GridScroll& e);
  void Destroy(GridDestroy& e);
};
