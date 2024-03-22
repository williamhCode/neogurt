#pragma once

#include "app/size.hpp"
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
  wgpu::TextureView windowsTextureView;

  // rect (background)
  wgpu::utils::RenderPassDescriptor rectRenderPassDesc;

  // text
  wgpu::utils::RenderPassDescriptor textRenderPassDesc;

  // windows
  wgpu::Buffer uiViewProjBuffer;
  wgpu::BindGroup uiViewProjBG;
  wgpu::utils::RenderPassDescriptor windowRenderPassDesc;
  
  // final texture
  QuadRenderData<TextureQuadVertex> windowsTextureData;
  wgpu::BindGroup windowsTextureBG;
  wgpu::utils::RenderPassDescriptor windowsRenderPassDesc;

  // cursor
  QuadRenderData<CursorQuadVertex> cursorData;
  wgpu::BindGroup cursorBG;
  wgpu::utils::RenderPassDescriptor cursorRenderPassDesc;

  Renderer(const SizeHandler& sizes);

  void Resize(const SizeHandler& sizes);

  void Begin();
  void RenderGrid(Grid& grid, Font& font, const HlTable& hlTable);
  void RenderWindows(const std::vector<const Win*>& windows);
  void RenderWindowsTexture();
  void RenderCursor(const Cursor& cursor, const HlTable& hlTable);
  void End();
  void Present();
};
