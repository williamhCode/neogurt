#include "renderer.hpp"
#include "editor/grid.hpp"
#include "editor/window.hpp"
#include "gfx/instance.hpp"
#include "gfx/pipeline.hpp"
#include "utils/unicode.hpp"
#include <utility>

using namespace wgpu;

Renderer::Renderer(const SizeHandler& sizes) {
  clearColor = {0.0, 0.0, 0.0, 1.0};

  // shared
  camera = Ortho2D(sizes.size);

  finalRenderTexture = RenderTexture(
    sizes.offset, sizes.uiSize, sizes.dpiScale, TextureFormat::BGRA8Unorm
  );

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
      // .view = maskTextureView,
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .clearValue = {0.0, 0.0, 0.0, 0.0},
    },
  });

  // windows
  windowsRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .view = finalRenderTexture.textureView,
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });

  // final texture
  finalRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load, .storeOp = StoreOp::Store,
      // .clearValue = {1.0, 0.0, 0.0, 0.5},
    },
  });

  // cursor
  offset = sizes.offset;
  auto maskOffset = offset * sizes.dpiScale;
  offsetBuffer = utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec2), &maskOffset);
  maskOffsetBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.maskOffsetBGL,
    {
      {0, offsetBuffer},
    }
  );

  cursorData.CreateBuffers(1);

  cursorRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load, .storeOp = StoreOp::Store,
      // .loadOp = LoadOp::Clear,
      // .storeOp = StoreOp::Store,
      // .clearValue = {1.0, 0.7, 0.8, 0.5},
    },
  });
}

void Renderer::Resize(const SizeHandler& sizes) {
  camera.Resize(sizes.size);

  finalRenderTexture.Update(sizes.offset, sizes.uiSize, sizes.dpiScale);
  windowsRPD = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .view = finalRenderTexture.textureView,
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });

  offset = sizes.offset;
  auto maskOffset = offset * sizes.dpiScale;
  ctx.queue.WriteBuffer(offsetBuffer, 0, &maskOffset, sizeof(glm::vec2));
}

void Renderer::Begin() {
  commandEncoder = ctx.device.CreateCommandEncoder();
  nextTexture = ctx.swapChain.GetCurrentTexture();
  nextTextureView = nextTexture.CreateView();
}

void Renderer::RenderWindow(Win& win, Font& font, const HlTable& hlTable) {
  auto& textData = win.textData;
  auto& rectData = win.rectData;

  textData.ResetCounts();
  rectData.ResetCounts();

  glm::vec2 textOffset(0, 0);

  for (size_t i = 0; i < win.grid.lines.Size(); i++) {
    auto& line = win.grid.lines[i];
    textOffset.x = 0;

    for (auto& cell : line) {
      auto charcode = UTF8ToUnicode(cell.text);
      auto hl = hlTable.at(cell.hlId);

      // don't render background if default
      if (cell.hlId != 0 && hl.background.has_value() && hl.background != hlTable.at(0).background) {
        auto rectPositions = MakeRegion({0, 0}, font.charSize);

        auto background = *hl.background;
        for (size_t i = 0; i < 4; i++) {
          auto& vertex = rectData.quads[rectData.quadCount][i];
          vertex.position = textOffset + rectPositions[i];
          vertex.color = background;
        }

        rectData.Increment();
      }

      if (cell.text != " ") {
        auto& glyphInfo = font.GetGlyphInfoOrAdd(charcode);

        glm::vec2 textQuadPos{
          textOffset.x + glyphInfo.bearing.x,
          textOffset.y - glyphInfo.bearing.y + font.size,
        };

        auto foreground = GetForeground(hlTable, hl);
        for (size_t i = 0; i < 4; i++) {
          auto& vertex = textData.quads[textData.quadCount][i];
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

  textData.WriteBuffers();
  rectData.WriteBuffers();

  font.UpdateTexture();

  // background
  {
    rectRPD.cColorAttachments[0].view = win.renderTexture.textureView;
    rectRPD.cColorAttachments[0].clearValue = clearColor;
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&rectRPD);
    passEncoder.SetPipeline(ctx.pipeline.rectRPL);
    passEncoder.SetBindGroup(0, win.renderTexture.camera.viewProjBG);
    rectData.SetBuffers(passEncoder);
    passEncoder.DrawIndexed(rectData.indexCount);
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
    textData.SetBuffers(passEncoder);
    passEncoder.DrawIndexed(textData.indexCount);
    passEncoder.End();
  }
}

// jst playing with templates and concepts is a bit unnecessary
void Renderer::RenderWindows(const RangeOf<const Win*> auto& windows) {
  auto passEncoder = commandEncoder.BeginRenderPass(&windowsRPD);
  passEncoder.SetPipeline(ctx.pipeline.textureRPL);

  for (const Win* win : windows) {
    passEncoder.SetBindGroup(0, finalRenderTexture.camera.viewProjBG);
    passEncoder.SetBindGroup(1, win->renderTexture.textureBG);
    win->renderTexture.Render(passEncoder);
  }

  passEncoder.End();
}
// explicit template instantiations
template void Renderer::RenderWindows(const std::deque<const Win*>& windows);

void Renderer::RenderFinalTexture() {
  finalRPD.cColorAttachments[0].view = nextTextureView;
  auto passEncoder = commandEncoder.BeginRenderPass(&finalRPD);
  passEncoder.SetPipeline(ctx.pipeline.textureRPL);
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, finalRenderTexture.textureBG);
  finalRenderTexture.Render(passEncoder);
  passEncoder.End();
}

void Renderer::RenderCursor(
  const Cursor& cursor, const HlTable& hlTable, const wgpu::BindGroup& maskBG
) {
  if (cursor.modeInfo == nullptr) return;

  auto attrId = cursor.modeInfo->attrId;
  auto& hl = hlTable.at(attrId);
  auto foreground = GetForeground(hlTable, hl);
  auto background = GetBackground(hlTable, hl);
  if (attrId == 0) std::swap(foreground, background);

  cursorData.ResetCounts();
  for (size_t i = 0; i < 4; i++) {
    auto& vertex = cursorData.quads[0][i];
    vertex.position = offset + cursor.pos + cursor.corners[i];
    vertex.foreground = foreground;
    vertex.background = background;
  }
  cursorData.Increment();
  cursorData.WriteBuffers();

  cursorRPD.cColorAttachments[0].view = nextTextureView;
  RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&cursorRPD);
  passEncoder.SetPipeline(ctx.pipeline.cursorRPL);
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, maskBG);
  passEncoder.SetBindGroup(2, maskOffsetBG);
  cursorData.SetBuffers(passEncoder);
  passEncoder.DrawIndexed(cursorData.indexCount);
  passEncoder.End();
}

void Renderer::End() {
  auto commandBuffer = commandEncoder.Finish();
  ctx.queue.Submit(1, &commandBuffer);
}

void Renderer::Present() {
  ctx.swapChain.Present();
}
