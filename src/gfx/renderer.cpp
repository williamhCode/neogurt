#include "renderer.hpp"
#include "gfx/instance.hpp"
#include "gfx/pipeline.hpp"
#include "dawn/utils/WGPUHelpers.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"

using namespace wgpu;

Renderer::Renderer(glm::uvec2 size) {
  clearColor = {0.0, 0.0, 0.0, 1.0};

  // shared
  auto view = glm::ortho<float>(0, size.x, size.y, 0, -1, 1);
  viewProjBuffer = utils::CreateUniformBuffer(ctx.device, sizeof(glm::mat4), &view);
  viewProjBG = dawn::utils::MakeBindGroup(
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
    {
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
    },
  });
}

void Renderer::Begin() {
  commandEncoder = ctx.device.CreateCommandEncoder();
  nextTexture = ctx.swapChain.GetCurrentTextureView();
}

void Renderer::RenderGrid(const Grid& grid, const Font& font) {
  using TextQuad = std::array<TextQuadVertex, 4>;
  std::vector<TextQuad> textVertices(grid.width * grid.height);
  std::vector<uint16_t> textIndices(grid.width * grid.height * 6);

  size_t quadIndex = 0;
  glm::vec2 textureSize(font.texture.width, font.texture.height);
  glm::vec2 offset(0, 0);
  for (size_t i = 0; i < grid.lines.Size(); i++) {
    auto& line = grid.lines[i];
    offset.x = 0;
    for (auto& cell : line) {
      char c = cell[0]; // only support char for now, add unicode support ltr
      auto glyphIndex = FT_Get_Char_Index(font.face, c);

      auto it = font.glyphInfoMap.find(glyphIndex);
      if (it == font.glyphInfoMap.end()) it = font.glyphInfoMap.find(32);
      auto& glyphInfo = it->second;

      // region to be drawn in context of the entire font texture
      glm::vec4 region{
        glyphInfo.pos.x,
        glyphInfo.pos.y,
        font.size,
        font.size,
      };

      // offset to be added to the position of the quad
      glm::vec2 offsetPos{
        offset.x + glyphInfo.bearing.x,
        offset.y - glyphInfo.bearing.y + font.size,
      };
      offset.x += glyphInfo.advance;

      // winding order is clockwise starting from top left
      // region = x, y, width, height
      std::array<glm::vec2, 4> localPositions{
        glm::vec2(0, 0),
        glm::vec2(region.z, 0),
        glm::vec2(region.z, region.w),
        glm::vec2(0, region.w),
      };

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

      for (size_t i = 0; i < 4; i++) {
        auto& vertex = textVertices[quadIndex][i];
        vertex.position = {localPositions[i] + offsetPos};
        vertex.uv = texCoords[i];
        vertex.foreground = {1, 1, 1, 1};
        vertex.background = {0, 0, 0, 1};
      }

      auto indexCount = quadIndex * 6;
      auto vertexCount = quadIndex * 4;
      textIndices[indexCount + 0] = vertexCount + 0;
      textIndices[indexCount + 1] = vertexCount + 1;
      textIndices[indexCount + 2] = vertexCount + 2;
      textIndices[indexCount + 3] = vertexCount + 0;
      textIndices[indexCount + 4] = vertexCount + 2;
      textIndices[indexCount + 5] = vertexCount + 3;

      quadIndex++;
    }
    offset.y += font.size;
  }

  auto vertexBufferSize = quadIndex * sizeof(TextQuad);
  auto indexBufferSize = quadIndex * 6 * sizeof(uint16_t);
  ctx.queue.WriteBuffer(
    textVertexBuffer, 0, textVertices.data(), vertexBufferSize
  );
  ctx.queue.WriteBuffer(
    textIndexBuffer, 0, textIndices.data(), indexBufferSize
  );

  textRenderPassDesc.cColorAttachments[0].view = nextTexture;
  textRenderPassDesc.cColorAttachments[0].clearValue = clearColor;
  RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&textRenderPassDesc);
  passEncoder.SetPipeline(ctx.pipeline.textRPL);
  passEncoder.SetBindGroup(0, viewProjBG);
  passEncoder.SetBindGroup(1, font.fontTextureBG);
  passEncoder.SetVertexBuffer(0, textVertexBuffer, 0, vertexBufferSize);
  passEncoder.SetIndexBuffer(textIndexBuffer, IndexFormat::Uint16, 0, indexBufferSize);
  passEncoder.DrawIndexed(quadIndex * 6);
  passEncoder.End();
}

void Renderer::End() {
  auto commandBuffer = commandEncoder.Finish();
  ctx.queue.Submit(1, &commandBuffer);
}

void Renderer::Present() {
  ctx.swapChain.Present();
}
