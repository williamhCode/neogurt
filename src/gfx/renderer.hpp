#pragma once

#include "app/size.hpp"
#include "editor/cursor.hpp"
#include "editor/font.hpp"
#include "editor/highlight.hpp"
#include "editor/grid.hpp"
#include "editor/window.hpp"
#include "gfx/camera.hpp"
#include "gfx/pipeline.hpp"
#include "gfx/quad.hpp"
#include "gfx/render_texture.hpp"
#include "gfx/timestamp.hpp"
#include "webgpu_tools/utils/webgpu.hpp"
#include <span>

struct Renderer {
  TimestampHelper timestamp;

  // color stuff
  wgpu::Color clearColor{};
  wgpu::Color linearClearColor{};
  wgpu::Color premultClearColor{};

  float gamma{};
  wgpu::Buffer gammaBuffer;
  wgpu::BindGroup gammaBG;

  glm::vec4 linearColor{};
  wgpu::Buffer linearColorBuffer;
  wgpu::BindGroup defaultColorBG;

  // shared
  wgpu::CommandEncoder commandEncoder;
  wgpu::Texture nextTexture;
  wgpu::TextureView nextTextureView;

  Ortho2D camera;
  RenderTexture finalRenderTexture;
  // double buffer, so resizing doesn't flicker
  RenderTexture prevFinalRenderTexture;

  // rect (background)
  wgpu::utils::RenderPassDescriptor rectRPD;
  wgpu::utils::RenderPassDescriptor rectNoClearRPD;

  // text and shapes
  wgpu::utils::RenderPassDescriptor textRPD;
  wgpu::utils::RenderPassDescriptor shapesRPD;

  // text mask
  QuadRenderData<TextMaskQuadVertex> textMaskData;
  wgpu::utils::RenderPassDescriptor textMaskRPD;

  // windows
  wgpu::utils::RenderPassDescriptor windowsRPD;

  // final texture
  wgpu::utils::RenderPassDescriptor finalRPD;

  // cursor
  QuadRenderData<CursorQuadVertex> cursorData;
  wgpu::utils::RenderPassDescriptor cursorRPD;

  Renderer() = default;
  Renderer(const SizeHandler& sizes);

  void Resize(const SizeHandler& sizes);
  void SetColors(const glm::vec4& color, float gamma);

  void Begin();
  // void RenderShapes(FontFamily& fontFamily);
  void RenderToWindow(Win& win, FontFamily& fontFamily, HlTable& hlTable);
  void RenderCursorMask(
    const Win& win, const Cursor& cursor, FontFamily& fontFamily, HlTable& hlTable
  );
  void RenderWindows(const Win* msgWin, std::span<const Win*> windows, std::span<const Win*> floatWindows);
  void RenderFinalTexture();
  void RenderCursor(const Cursor& cursor, HlTable& hlTable);
  void End();
};
