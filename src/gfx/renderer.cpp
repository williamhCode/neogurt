#include "renderer.hpp"
#include "editor/grid.hpp"
#include "editor/window.hpp"
#include "gfx/instance.hpp"
#include "gfx/pipeline.hpp"
#include "utils/region.hpp"
#include "utils/unicode.hpp"
#include "utils/color.hpp"
#include "webgpu/webgpu_cpp.h"
#include "webgpu_tools/utils/webgpu.hpp"
#include <utility>

using namespace wgpu;

Renderer::Renderer(const SizeHandler& sizes) {
  clearColor = {0.0, 0.0, 0.0, 1.0};

  // shared
  camera = Ortho2D(sizes.size);

  finalRenderTexture =
    RenderTexture(sizes.uiSize, sizes.dpiScale, TextureFormat::RGBA8Unorm);
  finalRenderTexture.UpdatePos(sizes.offset);

  // rect
  rectRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
    },
  });

  // text
  textRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
    RenderPassColorAttachment{
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .clearValue = {0.0, 0.0, 0.0, 0.0},
    },
  });

  // windows
  auto stencilTextureView =
    utils::CreateRenderTexture(
      ctx.device, Extent3D(sizes.uiFbSize.x, sizes.uiFbSize.y), TextureFormat::Stencil8
    )
      .CreateView();

  windowsRPD = utils::RenderPassDescriptor(
    {
      RenderPassColorAttachment{
        .view = finalRenderTexture.textureView,
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = {0.0, 0.0, 0.0, 0.0},
        // blending to color (0, 0, 0, 0) results in premultiplied texture
      },
    },
    RenderPassDepthStencilAttachment{
      .view = stencilTextureView,
      .stencilLoadOp = LoadOp::Clear,
      .stencilStoreOp = StoreOp::Store,
      .stencilClearValue = 0,
    }
  );

  // final texture
  finalRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
    },
  });

  // cursor
  maskOffsetBuffer =
    utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec2), &sizes.fbOffset);
  maskOffsetBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.maskOffsetBGL,
    {
      {0, maskOffsetBuffer},
    }
  );

  cursorData.CreateBuffers(1);

  cursorRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });
}

void Renderer::Resize(const SizeHandler& sizes) {
  camera.Resize(sizes.size);

  finalRenderTexture =
    RenderTexture(sizes.uiSize, sizes.dpiScale, TextureFormat::RGBA8Unorm);
  finalRenderTexture.UpdatePos(sizes.offset);
  windowsRPD.cColorAttachments[0].view = finalRenderTexture.textureView;

  auto stencilTextureView =
    utils::CreateRenderTexture(
      ctx.device, Extent3D(sizes.uiFbSize.x, sizes.uiFbSize.y), TextureFormat::Stencil8
    )
      .CreateView();
  windowsRPD.cDepthStencilAttachmentInfo.view = stencilTextureView;

  ctx.queue.WriteBuffer(maskOffsetBuffer, 0, &sizes.fbOffset, sizeof(glm::vec2));
}

void Renderer::SetClearColor(glm::vec4 color) {
  clearColor = ToWGPUColor(color);
  premultClearColor = ToWGPUColor(PremultiplyAlpha(color));
}

void Renderer::Begin() {
  commandEncoder = ctx.device.CreateCommandEncoder();
  SurfaceTexture surfaceTexture;
  ctx.surface.GetCurrentTexture(&surfaceTexture);
  nextTexture = surfaceTexture.texture;
  nextTextureView = nextTexture.CreateView();
}

void Renderer::RenderWindow(Win& win, Font& font, const HlTable& hlTable) {
  auto& rectData = win.rectData;
  auto& textData = win.textData;

  rectData.ResetCounts();
  textData.ResetCounts();

  glm::vec2 textOffset(0, 0);

  for (size_t row = 0; row < win.grid.lines.Size(); row++) {
    auto& line = win.grid.lines[row];
    textOffset.x = 0;

    for (auto& cell : line) {
      auto charcode = UTF8ToUnicode(cell.text);
      Highlight hl = hlTable.at(cell.hlId);

      // don't render background if default
      if (cell.hlId != 0 && hl.background.has_value() &&
          hl.background != hlTable.at(0).background) {
        auto rectPositions = MakeRegion({0, 0}, font.charSize);

        auto background = *hl.background;
        background.a = hl.bgAlpha;
        for (size_t i = 0; i < 4; i++) {
          auto& vertex = rectData.CurrQuad()[i];
          vertex.position = textOffset + rectPositions[i];
          vertex.color = background;
        }

        rectData.Increment();
      }

      if (cell.text != " ") {
        const auto& glyphInfo = *font.GetGlyphInfo(charcode);

        glm::vec2 textQuadPos{
          textOffset.x + glyphInfo.bearing.x,
          textOffset.y - glyphInfo.bearing.y + font.size,
        };

        glm::vec4 foreground = GetForeground(hlTable, hl);
        for (size_t i = 0; i < 4; i++) {
          auto& vertex = textData.CurrQuad()[i];
          vertex.position = textQuadPos + font.positions[i];
          vertex.regionCoords = glyphInfo.region[i];
          vertex.foreground = foreground;
        }

        textData.Increment();
      }

      textOffset.x += font.charSize.x;
    }

    textOffset.y += font.charSize.y;
  }

  rectData.WriteBuffers();
  textData.WriteBuffers();

  // updates texture if new glyphs added, old font texture from previous renders
  // is not owned by font anymore but still referenced by command encoder
  font.UpdateTexture();

  // background
  {
    rectRPD.cColorAttachments[0].view = win.renderTexture.textureView;
    rectRPD.cColorAttachments[0].clearValue = clearColor;
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&rectRPD);
    passEncoder.SetPipeline(ctx.pipeline.rectRPL);
    passEncoder.SetBindGroup(0, win.renderTexture.camera.viewProjBG);
    rectData.Render(passEncoder);
    passEncoder.End();
  }
  // text
  {
    textRPD.cColorAttachments[0].view = win.renderTexture.textureView;
    textRPD.cColorAttachments[1].view = win.maskTextureView;
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textRPD);
    passEncoder.SetPipeline(ctx.pipeline.textRPL);
    passEncoder.SetBindGroup(0, win.renderTexture.camera.viewProjBG);
    passEncoder.SetBindGroup(1, font.fontTextureBG);
    textData.Render(passEncoder);
    passEncoder.End();
  }
}

// jst playing with templates and concepts is a bit unnecessary
void Renderer::RenderWindows(
  const RangeOf<const Win*> auto& windows, const RangeOf<const Win*> auto& floatWindows
) {
  auto passEncoder = commandEncoder.BeginRenderPass(&windowsRPD);
  passEncoder.SetPipeline(ctx.pipeline.textureRPL);
  passEncoder.SetBindGroup(0, finalRenderTexture.camera.viewProjBG);

  auto renderWin = [&](const Win* win) {
    passEncoder.SetBindGroup(1, win->renderTexture.textureBG);
    win->renderTexture.renderData.Render(passEncoder);
    if (win->scrolling) {
      // if (win->hasPrevRender) {
      passEncoder.SetBindGroup(1, win->prevRenderTexture.textureBG);
      win->prevRenderTexture.renderData.Render(passEncoder);
      // }

      // draw window borders
      if (!win->margins.Empty()) {
        passEncoder.SetBindGroup(1, win->renderTexture.textureBG);
        win->marginsData.Render(passEncoder);
      }
    }
  };

  passEncoder.SetStencilReference(1);
  for (const Win* win : windows) {
    renderWin(win);
  }

  passEncoder.SetStencilReference(0);
  for (const Win* win : floatWindows) {
    renderWin(win);
  }

  passEncoder.End();
}
// explicit template instantiations
template void Renderer::RenderWindows(
  const std::deque<const Win*>& windows, const std::vector<const Win*>& floatWindows
);

void Renderer::RenderFinalTexture() {
  finalRPD.cColorAttachments[0].view = nextTextureView;
  finalRPD.cColorAttachments[0].clearValue = premultClearColor;
  auto passEncoder = commandEncoder.BeginRenderPass(&finalRPD);
  passEncoder.SetPipeline(ctx.pipeline.finalTextureRPL);
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, finalRenderTexture.textureBG);
  finalRenderTexture.renderData.Render(passEncoder);
  passEncoder.End();
}

void Renderer::RenderCursor(const Cursor& cursor, const HlTable& hlTable) {
  auto attrId = cursor.modeInfo->attrId;
  const auto& hl = hlTable.at(attrId);
  auto foreground = GetForeground(hlTable, hl);
  auto background = GetBackground(hlTable, hl);
  if (attrId == 0) std::swap(foreground, background);

  cursorData.ResetCounts();
  for (size_t i = 0; i < 4; i++) {
    auto& vertex = cursorData.CurrQuad()[i];
    vertex.position = cursor.pos + cursor.winScrollOffset + cursor.corners[i];
    vertex.foreground = foreground;
    vertex.background = background;
  }
  cursorData.Increment();
  cursorData.WriteBuffers();

  cursorRPD.cColorAttachments[0].view = nextTextureView;
  RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&cursorRPD);
  passEncoder.SetPipeline(ctx.pipeline.cursorRPL);
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, cursor.currMaskBG);
  passEncoder.SetBindGroup(2, maskOffsetBG);
  cursorData.Render(passEncoder);
  passEncoder.End();
}

void Renderer::End() {
  auto commandBuffer = commandEncoder.Finish();
  ctx.queue.Submit(1, &commandBuffer);
}
