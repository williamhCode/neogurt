#pragma once

#include "editor/grid.hpp"
#include "editor/highlight.hpp"
#include "gfx/font.hpp"
#include "gfx/instance.hpp"
#include "gfx/pipeline.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "webgpu/webgpu_cpp.h"
#include "webgpu_utils/webgpu.hpp"

template <class VertexType>
struct QuadRenderData {
  using Quad = std::array<VertexType, 4>;
  size_t quadCount;
  size_t vertexCount;
  size_t indexCount;
  std::vector<Quad> vertices;
  std::vector<uint32_t> indices;
  wgpu::Buffer vertexBuffer;
  wgpu::Buffer indexBuffer;

  void CreateBuffers(size_t size) {
    vertexBuffer = wgpu::utils::CreateVertexBuffer(ctx.device, sizeof(VertexType) * 4 * size);
    indexBuffer =
      wgpu::utils::CreateIndexBuffer(ctx.device, sizeof(uint32_t) * 6 * size);
  }

  void ResetCounts() {
    quadCount = 0;
    vertexCount = 0;
    indexCount = 0;
  }

  void ResizeBuffers(size_t size) {
    vertices.reserve(size);
    indices.reserve(size * 6);
  }

  void SetIndices() {
    indices[indexCount + 0] = vertexCount + 0;
    indices[indexCount + 1] = vertexCount + 1;
    indices[indexCount + 2] = vertexCount + 2;
    indices[indexCount + 3] = vertexCount + 2;
    indices[indexCount + 4] = vertexCount + 3;
    indices[indexCount + 5] = vertexCount + 0;
  }

  void IncrementCounts() {
    quadCount++;
    vertexCount += 4;
    indexCount += 6;
  }

  void WriteBuffers() {
    ctx.queue.WriteBuffer(vertexBuffer, 0, vertices.data(), sizeof(VertexType) * vertexCount);
    ctx.queue.WriteBuffer(
      indexBuffer, 0, indices.data(), sizeof(uint32_t) * indexCount
    );
  }
};

struct Renderer {
  wgpu::Color clearColor;
  wgpu::CommandEncoder commandEncoder;
  wgpu::TextureView nextTexture;

  // shared
  wgpu::Buffer viewProjBuffer;
  wgpu::BindGroup viewProjBG;

  // text
  QuadRenderData<TextQuadVertex> textData;
  wgpu::utils::RenderPassDescriptor textRenderPassDesc;

  // rect (background)
  QuadRenderData<RectQuadVertex> rectData;
  wgpu::utils::RenderPassDescriptor rectRenderPassDesc;

  Renderer(glm::uvec2 size);

  void Resize(glm::uvec2 size);

  void Begin();
  void RenderGrid(const Grid& grid, Font& font, const HlTable& hlTable);
  void End();
  void Present();
};
