#pragma once

#include "gfx/instance.hpp"
#include <algorithm>
#include <vector>
#include <array>
#include <cassert>

// https://stackoverflow.com/questions/21028299/is-this-behavior-of-vectorresizesize-type-n-under-c11-and-boost-container/21028912#21028912
// Allocator adaptor that interposes construct() calls to
// convert value initialization into default initialization.
template <typename T, typename A = std::allocator<T>>
class default_init_allocator : public A {
  typedef std::allocator_traits<A> a_t;

public:
  template <typename U>
  struct rebind {
    using other = default_init_allocator<U, typename a_t::template rebind_alloc<U>>;
  };

  using A::A;

  template <typename U>
  void construct(U* ptr) noexcept(std::is_nothrow_default_constructible<U>::value) {
    ::new (static_cast<void*>(ptr)) U;
  }

  template <typename U, typename... Args>
  void construct(U* ptr, Args&&... args) {
    a_t::construct(static_cast<A&>(*this), ptr, std::forward<Args>(args)...);
  }
};

// Helper for rendering quads with an optional dynamic resizing behavior
template <class VertexType, bool Dynamic = false>
struct QuadRenderData {
  using Quad = std::array<VertexType, 4>;
  size_t quadCount = 0;
  size_t vertexCount = 0;
  size_t indexCount = 0;
  std::vector<Quad, default_init_allocator<Quad>> quads;
  std::vector<uint32_t, default_init_allocator<uint32_t>> indices;
  wgpu::Buffer vertexBuffer;
  wgpu::Buffer indexBuffer;

  QuadRenderData() = default;
  QuadRenderData(size_t numQuads) {
    CreateBuffers(numQuads);
  }

  void CreateBuffers(size_t numQuads) {
    quads.resize(numQuads);
    indices.resize(numQuads * 6);

    vertexBuffer = ctx.CreateVertexBuffer(sizeof(VertexType) * 4 * numQuads);
    indexBuffer = ctx.CreateIndexBuffer(sizeof(uint32_t) * 6 * numQuads);
  }

  void ResetCounts() {
    quadCount = 0;
    vertexCount = 0;
    indexCount = 0;
  }

  Quad& NextQuad() {
    if constexpr (Dynamic) {
      // Dynamic version resizes the vector if needed
      if (quadCount >= quads.size()) {
        auto newSize = std::max<size_t>(quads.size() * 2, 1);
        quads.resize(newSize);
        indices.resize(newSize * 6);
      }
    } else {
      assert(quadCount < quads.size());
    }

    auto& quad = quads[quadCount];

    indices[indexCount + 0] = vertexCount + 0;
    indices[indexCount + 1] = vertexCount + 1;
    indices[indexCount + 2] = vertexCount + 2;
    indices[indexCount + 3] = vertexCount + 2;
    indices[indexCount + 4] = vertexCount + 3;
    indices[indexCount + 5] = vertexCount + 0;

    quadCount++;
    vertexCount += 4;
    indexCount += 6;

    return quad;
  }

  void WriteBuffers() {
    if constexpr (Dynamic) {
      size_t bufferQuadCount = vertexBuffer.GetSize() / (sizeof(VertexType) * 4);
      if (bufferQuadCount != quads.size()) {
        // LOG_INFO("Resizing vertex buffer from {} to {}", bufferQuadCount, quads.size());
        vertexBuffer = ctx.CreateVertexBuffer(sizeof(VertexType) * 4 * quads.size());
        indexBuffer = ctx.CreateIndexBuffer(sizeof(uint32_t) * 6 * quads.size());
      }
    }

    ctx.queue.WriteBuffer(
      vertexBuffer, 0, quads.data(), sizeof(VertexType) * vertexCount
    );
    ctx.queue.WriteBuffer(
      indexBuffer, 0, indices.data(), sizeof(uint32_t) * indexCount
    );
  }

  void Render(
    const wgpu::RenderPassEncoder& passEncoder, uint64_t offset = 0, uint64_t size = 0
  ) const {
    assert(offset <= quadCount);
    assert(size <= quadCount);
    if (size == 0) size = quadCount;

    passEncoder.SetVertexBuffer(0, vertexBuffer);

    auto indexStride = sizeof(uint32_t) * 6;
    passEncoder.SetIndexBuffer(
      indexBuffer, wgpu::IndexFormat::Uint32, offset * indexStride, size * indexStride
    );

    passEncoder.DrawIndexed(size * 6);
  }
};
