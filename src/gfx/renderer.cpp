#include "renderer.hpp"
#include "gfx/instance.hpp"
#include "gfx/pipeline.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"

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

  // text
  auto maxTextQuads = 10000; // * 4 dont exceed 65535 + 1 cuz uint16_t
  textVertexBuffer =
    utils::CreateVertexBuffer(ctx.device, sizeof(TextQuadVertex) * 4 * maxTextQuads);
  textIndexBuffer =
    utils::CreateIndexBuffer(ctx.device, sizeof(uint16_t) * 6 * maxTextQuads);

  textRenderPassDesc = utils::RenderPassDescriptor({
    RenderPassColorAttachment{
      .loadOp = LoadOp::Clear,
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

void Renderer::RenderGrid(const Grid& grid, const Font& font, const HlTable& hlTable) {
  using TextQuad = std::array<TextQuadVertex, 4>;
  std::vector<TextQuad> textVertices(grid.width * grid.height);
  std::vector<uint16_t> textIndices(grid.width * grid.height * 6);

  size_t quadCount = 0;
  glm::vec2 textureSize(font.texture.width, font.texture.height);
  glm::vec2 currOffset(0, 0);
  for (size_t i = 0; i < grid.lines.Size(); i++) {
    auto& line = grid.lines[i];
    currOffset.x = 0;
    for (auto& cell : line) {
      char c = cell.text[0]; // only support char for now, add unicode support ltr
      auto hl = hlTable.at(cell.hlId);
      if (c == ' ') {
        currOffset.x += font.charWidth;
        continue;
      }
      auto glyphIndex = FT_Get_Char_Index(font.face, c);

      auto it = font.glyphInfoMap.find(glyphIndex);
      // if (it == font.glyphInfoMap.end()) it = font.glyphInfoMap.find(32);
      auto& glyphInfo = it->second;

      glm::vec2 quadPos{
        currOffset.x + glyphInfo.bearing.x,
        currOffset.y - glyphInfo.bearing.y + font.size,
      };
      currOffset.x += glyphInfo.advance;

      for (size_t i = 0; i < 4; i++) {
        auto& vertex = textVertices[quadCount][i];
        vertex.position = quadPos + glyphInfo.positions[i];
        vertex.uv = glyphInfo.texCoords[i];
        vertex.foreground = GetForeground(hlTable, hl);
      }

      auto indexCount = quadCount * 6;
      auto vertexCount = quadCount * 4;
      textIndices[indexCount + 0] = vertexCount + 0;
      textIndices[indexCount + 1] = vertexCount + 1;
      textIndices[indexCount + 2] = vertexCount + 2;
      textIndices[indexCount + 3] = vertexCount + 0;
      textIndices[indexCount + 4] = vertexCount + 2;
      textIndices[indexCount + 5] = vertexCount + 3;

      quadCount++;
    }
    currOffset.y += font.charHeight;
  }

  auto vertexBufferSize = quadCount * sizeof(TextQuad);
  auto indexBufferSize = quadCount * 6 * sizeof(uint16_t);
  ctx.queue.WriteBuffer(textVertexBuffer, 0, textVertices.data(), vertexBufferSize);
  ctx.queue.WriteBuffer(textIndexBuffer, 0, textIndices.data(), indexBufferSize);

  textRenderPassDesc.cColorAttachments[0].view = nextTexture;
  textRenderPassDesc.cColorAttachments[0].clearValue = clearColor;
  RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textRenderPassDesc);
  passEncoder.SetPipeline(ctx.pipeline.textRPL);
  passEncoder.SetBindGroup(0, viewProjBG);
  passEncoder.SetBindGroup(1, font.fontTextureBG);
  passEncoder.SetVertexBuffer(0, textVertexBuffer, 0, vertexBufferSize);
  passEncoder.SetIndexBuffer(textIndexBuffer, IndexFormat::Uint16, 0, indexBufferSize);
  passEncoder.DrawIndexed(quadCount * 6);
  passEncoder.End();
}

void Renderer::End() {
  auto commandBuffer = commandEncoder.Finish();
  ctx.queue.Submit(1, &commandBuffer);
}

void Renderer::Present() {
  ctx.swapChain.Present();
}
