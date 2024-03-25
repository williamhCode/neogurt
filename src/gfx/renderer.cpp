#include "renderer.hpp"
#include "editor/grid.hpp"
#include "editor/window.hpp"
#include "gfx/instance.hpp"
#include "gfx/pipeline.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "utils/unicode.hpp"
#include <utility>

using namespace wgpu;

Renderer::Renderer(const SizeHandler& sizes) {
  clearColor = {0.0, 0.0, 0.0, 1.0};

  // shared
  camera = Ortho2D(sizes.size);

  Extent3D maskSize{
    static_cast<uint32_t>(sizes.fbSize.x), static_cast<uint32_t>(sizes.fbSize.y)
  };
  maskTextureView =
    utils::CreateRenderTexture(ctx.device, maskSize, TextureFormat::R8Unorm)
      .CreateView();

  windowsRenderTexture =
    RenderTexture({0, 0}, sizes.uiSize, sizes.dpiScale, TextureFormat::BGRA8Unorm);

  // rect
  rectRenderPassDesc = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
    },
  });

  // text
  textRenderPassDesc = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
    // RenderPassColorAttachment{
    //   .view = maskTextureView,
    //   .loadOp = LoadOp::Clear,
    //   .storeOp = StoreOp::Store,
    //   .clearValue = {0.0, 0.0, 0.0, 0.0},
    // },
  });

  // windows
  windowRenderPassDesc = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .view = windowsRenderTexture.textureView,
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });

  // all windows
  windowsRenderPassDesc = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });

  // cursor
  cursorData.CreateBuffers(1);

  cursorBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.cursorBGL,
    {
      {0, maskTextureView},
    }
  );

  cursorRenderPassDesc = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });
}

void Renderer::Resize(const SizeHandler& sizes) {
  camera.Resize(sizes.size);

  windowsRenderTexture.Resize({0, 0}, sizes.uiSize, sizes.dpiScale);
  windowRenderPassDesc = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .view = windowsRenderTexture.textureView,
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });
}

// void Renderer::FbResize(glm::uvec2 fbSize) {
// maskTextureView =
//   utils::CreateRenderTexture(ctx.device, {fbSize.x, fbSize.y},
//   TextureFormat::R8Unorm)
//     .CreateView();

// maskBG = utils::MakeBindGroup(
//   ctx.device, ctx.pipeline.maskBGL,
//   {
//     {0, maskTextureView},
//   }
// );

// textRenderPassDesc.cColorAttachments[1].view = maskTextureView;
// }

void Renderer::Begin() {
  commandEncoder = ctx.device.CreateCommandEncoder();
  nextTexture = ctx.swapChain.GetCurrentTextureView();
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
    rectRenderPassDesc.cColorAttachments[0].view = win.renderTexture.textureView;
    rectRenderPassDesc.cColorAttachments[0].clearValue = clearColor;
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&rectRenderPassDesc);
    passEncoder.SetPipeline(ctx.pipeline.rectRPL);
    passEncoder.SetBindGroup(0, win.renderTexture.camera.viewProjBG);
    rectData.SetBuffers(passEncoder);
    passEncoder.DrawIndexed(rectData.indexCount);
    passEncoder.End();
  }
  // text
  {
    textRenderPassDesc.cColorAttachments[0].view = win.renderTexture.textureView;
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textRenderPassDesc);
    passEncoder.SetPipeline(ctx.pipeline.textRPL);
    passEncoder.SetBindGroup(0, win.renderTexture.camera.viewProjBG);
    passEncoder.SetBindGroup(1, font.fontTextureBG);
    textData.SetBuffers(passEncoder);
    passEncoder.DrawIndexed(textData.indexCount);
    passEncoder.End();
  }
}

void Renderer::RenderWindows(const std::vector<const Win*>& windows) {
  auto passEncoder = commandEncoder.BeginRenderPass(&windowRenderPassDesc);
  passEncoder.SetPipeline(ctx.pipeline.textureRPL);

  for (auto win : windows) {
    passEncoder.SetBindGroup(0, windowsRenderTexture.camera.viewProjBG);
    passEncoder.SetBindGroup(1, win->renderTexture.textureBG);
    win->renderTexture.renderData.SetBuffers(passEncoder);
    passEncoder.DrawIndexed(win->renderTexture.renderData.indexCount);
  }

  passEncoder.End();
}

void Renderer::RenderWindowsTexture() {
  windowsRenderPassDesc.cColorAttachments[0].view = nextTexture;
  auto passEncoder = commandEncoder.BeginRenderPass(&windowsRenderPassDesc);
  passEncoder.SetPipeline(ctx.pipeline.textureRPL);
  passEncoder.SetBindGroup(0, camera.viewProjBG);
  passEncoder.SetBindGroup(1, windowsRenderTexture.textureBG);
  windowsRenderTexture.renderData.SetBuffers(passEncoder);
  passEncoder.DrawIndexed(windowsRenderTexture.renderData.indexCount);
  passEncoder.End();
}

void Renderer::RenderCursor(const Cursor& cursor, const HlTable& hlTable) {
  if (cursor.modeInfo == nullptr) return;

  cursorData.ResetCounts();

  auto attrId = cursor.modeInfo->attrId;
  auto& hl = hlTable.at(attrId);
  auto foreground = GetForeground(hlTable, hl);
  auto background = GetBackground(hlTable, hl);
  if (attrId == 0) {
    std::swap(foreground, background);
  }

  for (size_t i = 0; i < 4; i++) {
    auto& vertex = cursorData.quads[0][i];
    vertex.position = cursor.pos + cursor.corners[i];
    vertex.foreground = foreground;
    vertex.background = background;
  }

  cursorData.Increment();
  cursorData.WriteBuffers();

  {
    cursorRenderPassDesc.cColorAttachments[0].view = nextTexture;
    RenderPassEncoder passEncoder =
      commandEncoder.BeginRenderPass(&cursorRenderPassDesc);
    passEncoder.SetPipeline(ctx.pipeline.cursorRPL);
    passEncoder.SetBindGroup(0, camera.viewProjBG);
    passEncoder.SetBindGroup(1, cursorBG);
    cursorData.SetBuffers(passEncoder);
    passEncoder.DrawIndexed(cursorData.indexCount);
    passEncoder.End();
  }
}

void Renderer::End() {
  auto commandBuffer = commandEncoder.Finish();
  ctx.queue.Submit(1, &commandBuffer);
}

void Renderer::Present() {
  ctx.swapChain.Present();
}
