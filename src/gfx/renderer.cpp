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

Renderer::Renderer(glm::uvec2 size, glm::uvec2 fbSize) {
  clearColor = {0.0, 0.0, 0.0, 1.0};

  // shared
  auto view = glm::ortho<float>(0, size.x, size.y, 0, -1, 1);
  viewProjBuffer = utils::CreateUniformBuffer(ctx.device, sizeof(glm::mat4), &view);
  viewProjBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.viewProjBGL,
    {
      {0, viewProjBuffer},
    }
  );

  maskTextureView =
    utils::CreateRenderTexture(ctx.device, {fbSize.x, fbSize.y}, TextureFormat::R8Unorm)
      .CreateView();

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

  // texture
  textureRenderPassDesc = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    },
  });

  // cursor
  cursorData.CreateBuffers(1);
  cursorData.ReserveVectors(1);

  maskBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.maskBGL,
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

void Renderer::Resize(glm::uvec2 size) {
  auto view = glm::ortho<float>(0, size.x, size.y, 0, -1, 1);
  ctx.queue.WriteBuffer(viewProjBuffer, 0, &view, sizeof(glm::mat4));
}

void Renderer::FbResize(glm::uvec2 fbSize) {
  // maskTextureView =
  //   utils::CreateRenderTexture(ctx.device, {fbSize.x, fbSize.y}, TextureFormat::R8Unorm)
  //     .CreateView();

  // maskBG = utils::MakeBindGroup(
  //   ctx.device, ctx.pipeline.maskBGL,
  //   {
  //     {0, maskTextureView},
  //   }
  // );

  // textRenderPassDesc.cColorAttachments[1].view = maskTextureView;
}

void Renderer::Begin() {
  commandEncoder = ctx.device.CreateCommandEncoder();
  nextTexture = ctx.swapChain.GetCurrentTextureView();
}

void Renderer::RenderGrid(
  Grid& grid, Font& font, const HlTable& hlTable
) {
  auto& textData = grid.textData;
  auto& rectData = grid.rectData;

  textData.ResetCounts();
  rectData.ResetCounts();

  glm::vec2 textOffset(0, 0);

  for (size_t i = 0; i < grid.lines.Size(); i++) {
    auto& line = grid.lines[i];
    textOffset.x = 0;

    for (auto& cell : line) {
      auto charcode = UTF8ToUnicode(cell.text);
      auto hl = hlTable.at(cell.hlId);

      // don't render background if default
      if (cell.hlId != 0 && hl.background.has_value()) {
        static std::array<glm::vec2, 4> rectPositions{
          glm::vec2(0, 0),
          glm::vec2(font.charSize.x, 0),
          glm::vec2(font.charSize.x, font.charSize.y),
          glm::vec2(0, font.charSize.y),
        };

        auto background = *hl.background;
        for (size_t i = 0; i < 4; i++) {
          auto& vertex = rectData.quads[rectData.quadCount][i];
          vertex.position = textOffset + rectPositions[i];
          vertex.color = background;
        }

        rectData.SetIndices();
        rectData.IncrementCounts();
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

        textData.SetIndices();
        textData.IncrementCounts();
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
    rectRenderPassDesc.cColorAttachments[0].view = grid.textureView;
    rectRenderPassDesc.cColorAttachments[0].clearValue = clearColor;
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&rectRenderPassDesc);
    passEncoder.SetPipeline(ctx.pipeline.rectRPL);
    passEncoder.SetBindGroup(0, grid.viewProjBG);
    rectData.SetBuffers(passEncoder);
    passEncoder.DrawIndexed(rectData.indexCount);
    passEncoder.End();
  }
  // text
  {
    textRenderPassDesc.cColorAttachments[0].view = grid.textureView;
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textRenderPassDesc);
    passEncoder.SetPipeline(ctx.pipeline.textRPL);
    passEncoder.SetBindGroup(0, grid.viewProjBG);
    passEncoder.SetBindGroup(1, font.fontTextureBG);
    textData.SetBuffers(passEncoder);
    passEncoder.DrawIndexed(textData.indexCount);
    passEncoder.End();
  }
}

void Renderer::RenderWindows(const std::vector<const Win*>& windows) {
  textureRenderPassDesc.cColorAttachments[0].view = nextTexture;
  textureRenderPassDesc.cColorAttachments[0].clearValue = clearColor;
  auto passEncoder = commandEncoder.BeginRenderPass(&textureRenderPassDesc);
  passEncoder.SetPipeline(ctx.pipeline.textureRPL);

  for (auto win : windows) {
    passEncoder.SetBindGroup(0, viewProjBG);
    passEncoder.SetBindGroup(1, win->textureBG);
    win->renderData.SetBuffers(passEncoder);
    passEncoder.DrawIndexed(win->renderData.indexCount);
  }

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
    auto& vertex = cursorData.quads[cursorData.quadCount][i];
    vertex.position = cursor.pos + cursor.corners[i];
    vertex.foreground = foreground;
    vertex.background = background;
  }

  cursorData.SetIndices();
  cursorData.IncrementCounts();

  cursorData.WriteBuffers();

  {
    cursorRenderPassDesc.cColorAttachments[0].view = nextTexture;
    RenderPassEncoder passEncoder =
      commandEncoder.BeginRenderPass(&cursorRenderPassDesc);
    passEncoder.SetPipeline(ctx.pipeline.cursorRPL);
    passEncoder.SetBindGroup(0, viewProjBG);
    passEncoder.SetBindGroup(1, maskBG);
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
