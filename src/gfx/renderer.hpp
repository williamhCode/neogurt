#pragma once

#include "editor/cursor.hpp"
#include "editor/highlight.hpp"
#include "editor/grid.hpp"
#include "editor/window.hpp"
#include "gfx/font.hpp"
#include "gfx/pipeline.hpp"
#include "gfx/quad.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "webgpu/webgpu_cpp.h"
#include "webgpu_utils/webgpu.hpp"

// forward decl
struct Renderer {
  wgpu::Color clearColor;
  wgpu::CommandEncoder commandEncoder;
  wgpu::TextureView nextTexture;

  // shared
  wgpu::Buffer viewProjBuffer;
  wgpu::BindGroup viewProjBG;
  wgpu::TextureView maskTextureView;

  // rect (background)
  wgpu::utils::RenderPassDescriptor rectRenderPassDesc;

  // text
  wgpu::utils::RenderPassDescriptor textRenderPassDesc;

  // texture
  wgpu::utils::RenderPassDescriptor textureRenderPassDesc;

  // cursor
  QuadRenderData<CursorQuadVertex> cursorData;
  wgpu::BindGroup maskBG;
  wgpu::utils::RenderPassDescriptor cursorRenderPassDesc;

  Renderer(glm::uvec2 size, glm::uvec2 fbSize);

  void Resize(glm::uvec2 size);
  void FbResize(glm::uvec2 fbSize);

  void Begin();
  void RenderGrid(Grid& grid, Font& font, const HlTable& hlTable);
  void RenderWindows(const std::vector<const Win*>& windows);
  void RenderCursor(const Cursor& cursor, const HlTable& hlTable);
  void End();
  void Present();
};
