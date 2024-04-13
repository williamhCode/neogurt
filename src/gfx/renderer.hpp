#pragma once

#include "app/size.hpp"
#include "editor/cursor.hpp"
#include "editor/highlight.hpp"
#include "editor/grid.hpp"
#include "editor/window.hpp"
#include "gfx/camera.hpp"
#include "gfx/font.hpp"
#include "gfx/pipeline.hpp"
#include "gfx/quad.hpp"
#include "webgpu/webgpu_cpp.h"
#include "webgpu_tools/utils/webgpu.hpp"
#include <ranges>

template <typename R, typename V>
concept RangeOf =
  std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, V>;

struct Renderer {
  wgpu::Color clearColor;
  wgpu::CommandEncoder commandEncoder;
  wgpu::Texture nextTexture;
  wgpu::TextureView nextTextureView;

  // shared
  Ortho2D camera;
  wgpu::TextureView maskTextureView;
  RenderTexture finalRenderTexture;

  // rect (background)
  wgpu::utils::RenderPassDescriptor rectRPD;

  // text
  wgpu::utils::RenderPassDescriptor textRPD;

  // windows
  wgpu::utils::RenderPassDescriptor windowsRPD;

  // final texture
  wgpu::utils::RenderPassDescriptor finalRPD;

  // cursor
  wgpu::Buffer maskOffsetBuffer;
  wgpu::BindGroup maskOffsetBG;
  QuadRenderData<CursorQuadVertex> cursorData;
  wgpu::utils::RenderPassDescriptor cursorRPD;

  Renderer() = default;
  Renderer(const SizeHandler& sizes);

  void Resize(const SizeHandler& sizes);
  void SetClearColor(glm::vec4 color);

  void Begin();
  void RenderWindow(Win& win, Font& font, const HlTable& hlTable);
  void RenderWindows(const RangeOf<const Win*> auto& windows);
  void RenderFinalTexture();
  void RenderCursor(const Cursor& cursor, const HlTable& hlTable);
  void End();
};
