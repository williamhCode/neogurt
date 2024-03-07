#include "renderer.hpp"
#include "gfx/instance.hpp"
#include "gfx/pipeline.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "utils/unicode.hpp"

using namespace wgpu;

Renderer::Renderer(glm::uvec2 size) {
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

  // rect
  // Todo: make maxTextQuads resize to best fit
  const size_t maxTextQuads = 30000;
  rectData.CreateBuffers(maxTextQuads);
  rectRenderPassDesc = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
    },
  });

  // text
  textData.CreateBuffers(maxTextQuads);
  textRenderPassDesc = utils::RenderPassDescriptor({
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

void Renderer::Begin() {
  commandEncoder = ctx.device.CreateCommandEncoder();
  nextTexture = ctx.swapChain.GetCurrentTextureView();
}

void Renderer::RenderGrid(const Grid& grid, Font& font, const HlTable& hlTable) {
  textData.ResetCounts();
  textData.ResizeBuffers(grid.width * grid.height);

  rectData.ResetCounts();
  rectData.ResizeBuffers(grid.width * grid.height);

  glm::vec2 textureSize(font.texture.width, font.texture.height);
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
          glm::vec2(font.charWidth, 0),
          glm::vec2(font.charWidth, font.charHeight),
          glm::vec2(0, font.charHeight),
        };

        auto background = hl.background.value();
        for (size_t i = 0; i < 4; i++) {
          auto& vertex = rectData.vertices[rectData.quadCount][i];
          vertex.position = textOffset + rectPositions[i];
          vertex.color = background;
        }

        rectData.SetIndices();
        rectData.IncrementCounts();
      }

      if (cell.text != " ") {
        auto& glyphInfo = font.GetGlyphInfo(charcode);

        glm::vec2 textQuadPos{
          textOffset.x + glyphInfo.bearing.x,
          textOffset.y - glyphInfo.bearing.y + font.size,
        };

        // region that holds current glyph in context of the entire font texture
        // region = x, y, width, height
        glm::vec4 region{glyphInfo.pos.x, glyphInfo.pos.y, font.size, font.size};

        float left = region.x / textureSize.x;
        float right = (region.x + region.z) / textureSize.x;
        float top = region.y / textureSize.y;
        float bottom = (region.y + region.w) / textureSize.y;

        std::array<glm::vec2, 4> texCoords{
          glm::vec2(left, top),
          glm::vec2(right, top),
          glm::vec2(right, bottom),
          glm::vec2(left, bottom),
        };

        auto foreground = GetForeground(hlTable, hl);
        for (size_t i = 0; i < 4; i++) {
          auto& vertex = textData.vertices[textData.quadCount][i];
          vertex.position = textQuadPos + glyphInfo.positions[i];
          vertex.uv = texCoords[i];
          vertex.foreground = foreground;
        }

        textData.SetIndices();
        textData.IncrementCounts();
      }

      textOffset.x += font.charWidth;
    }

    textOffset.y += font.charHeight;
  }

  font.UpdateTexture();
  textData.WriteBuffers();
  rectData.WriteBuffers();

  // background
  {
    rectRenderPassDesc.cColorAttachments[0].view = nextTexture;
    rectRenderPassDesc.cColorAttachments[0].clearValue = clearColor;
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&rectRenderPassDesc);
    passEncoder.SetPipeline(ctx.pipeline.rectRPL);
    passEncoder.SetBindGroup(0, viewProjBG);
    passEncoder.SetVertexBuffer(
      0, rectData.vertexBuffer, 0, rectData.vertexCount * sizeof(RectQuadVertex)
    );
    passEncoder.SetIndexBuffer(
      rectData.indexBuffer, IndexFormat::Uint32, 0,
      rectData.indexCount * sizeof(uint32_t)
    );
    passEncoder.DrawIndexed(rectData.indexCount);
    passEncoder.End();
  }
  // text
  {
    textRenderPassDesc.cColorAttachments[0].view = nextTexture;
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textRenderPassDesc);
    passEncoder.SetPipeline(ctx.pipeline.textRPL);
    passEncoder.SetBindGroup(0, viewProjBG);
    passEncoder.SetBindGroup(1, font.fontTextureBG);
    passEncoder.SetVertexBuffer(
      0, textData.vertexBuffer, 0, textData.vertexCount * sizeof(TextQuadVertex)
    );
    passEncoder.SetIndexBuffer(
      textData.indexBuffer, IndexFormat::Uint32, 0,
      textData.indexCount * sizeof(uint32_t)
    );
    passEncoder.DrawIndexed(textData.indexCount);
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
