#pragma once

#include "webgpu_utils/webgpu.hpp"
#include "gfx/instance.hpp"

template <class VertexType>
struct QuadRenderData {
  using Quad = std::array<VertexType, 4>;
  size_t quadCount;
  size_t vertexCount;
  size_t indexCount;
  std::vector<Quad> quads;
  std::vector<uint32_t> indices;
  wgpu::Buffer vertexBuffer;
  wgpu::Buffer indexBuffer;

  void CreateBuffers(size_t numQuads) {
    quads.reserve(numQuads);
    indices.reserve(numQuads * 6);

    vertexBuffer =
      wgpu::utils::CreateVertexBuffer(ctx.device, sizeof(VertexType) * 4 * numQuads);
    indexBuffer =
      wgpu::utils::CreateIndexBuffer(ctx.device, sizeof(uint32_t) * 6 * numQuads);
  }

  void ResetCounts() {
    quadCount = 0;
    vertexCount = 0;
    indexCount = 0;
  }

  Quad& CurrQuad() {
    return quads[quadCount];
  }

  void Increment() {
    indices[indexCount + 0] = vertexCount + 0;
    indices[indexCount + 1] = vertexCount + 1;
    indices[indexCount + 2] = vertexCount + 2;
    indices[indexCount + 3] = vertexCount + 2;
    indices[indexCount + 4] = vertexCount + 3;
    indices[indexCount + 5] = vertexCount + 0;

    quadCount++;
    vertexCount += 4;
    indexCount += 6;
  }

  void WriteBuffers() {
    ctx.queue.WriteBuffer(
      vertexBuffer, 0, quads.data(), sizeof(VertexType) * vertexCount
    );
    ctx.queue.WriteBuffer(
      indexBuffer, 0, indices.data(), sizeof(uint32_t) * indexCount
    );
  }

  void Render(const wgpu::RenderPassEncoder& passEncoder) const {
    passEncoder.SetVertexBuffer(0, vertexBuffer, 0, sizeof(VertexType) * vertexCount);
    passEncoder.SetIndexBuffer(
      indexBuffer, wgpu::IndexFormat::Uint32, 0, indexCount * sizeof(uint32_t)
    );
    passEncoder.DrawIndexed(indexCount);
  }
};
