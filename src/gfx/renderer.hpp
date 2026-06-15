#pragma once

#include "webgpu_utils/render_pass.hpp"
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
#include <span>

struct Renderer {
  TimestampHelper timestamp;

  // color stuff
  wgpu::Color clearColor{};
  wgpu::Color linearClearColor{};
  wgpu::Color premultClearColor{};

  glm::vec4 defaultBgLinear{};
  wgpu::Buffer defaultBgLinearBuffer;
  wgpu::BindGroup defaultBgLinearBG;

  // shared
  wgpu::CommandEncoder commandEncoder;
  wgpu::Texture nextTexture;
  wgpu::TextureView nextTextureView;

  Ortho2D camera;

  bool resize = false;
  bool postProcessing = false;

  int currTextureIndex = 0;
  std::array<RenderTexture, 2> finalRenderTextures;
  void SwapFinalRenderTexture() {
    currTextureIndex = (currTextureIndex + 1) % 2;
  }
  auto& CurrFinalRenderTexture() {
    return finalRenderTextures[currTextureIndex];
  }
  auto& OtherFinalRenderTexture() {
    return finalRenderTextures[(currTextureIndex + 1) % 2];
  }

  // rect (background)
  wgpu::utils::RenderPassDescriptor rectRPD;
  wgpu::utils::RenderPassDescriptor rectNoClearRPD;

  // text
  wgpu::utils::RenderPassDescriptor textRPD;

  // text mask
  QuadRenderData<TextMaskQuadVertex, true> textMaskData;
  wgpu::utils::RenderPassDescriptor textMaskRPD;

  // windows
  wgpu::utils::RenderPassDescriptor windowsRPD;

  // final texture
  wgpu::utils::RenderPassDescriptor finalRPD;

  // pre-effects: cursor + windows composited here before post-processing pass
  RenderTexture preEffectsTexture;

  // post-processing pass (reads preEffectsTexture, writes to surface)
  wgpu::utils::RenderPassDescriptor postFxRPD;
  wgpu::Buffer postFxTimeBuffer;
  wgpu::BindGroup postFxTimeBG;

  // cursor
  QuadRenderData<CursorQuadVertex> cursorData;
  wgpu::utils::RenderPassDescriptor cursorRPD;

  // emoji cursor overlay
  QuadRenderData<TextQuadVertex, true> cursorEmojiOverlayData;
  wgpu::utils::RenderPassDescriptor cursorEmojiOverlayRPD;

  Renderer();

  void Resize(const SizeHandler& sizes);
  void SetColors(const glm::vec4& color, float gamma);

  bool GetNextTexture();
  void Begin();
  void RenderToWindow(Win& win, FontFamily& fontFamily, HlManager& hlManager);
  void RenderCursorMask(
    const Win& win, Cursor& cursor, FontFamily& fontFamily, HlManager& hlManager
  );
  void RenderWindows(std::span<const Win*> windows, std::span<const Win*> floatWindows);
  void RenderFinalTexture();
  void RenderCursor(const Cursor& cursor, HlManager& hlManager);
  void RenderCursorEmoji(
    const Win& win, const Cursor& cursor, FontFamily& fontFamily, HlManager& hlManager
  );
  void RenderPostFx();
  void End();

private:
  wgpu::TextureView& EffectsTarget() {
    return postProcessing ? preEffectsTexture.textureView : nextTextureView;
  }
};
