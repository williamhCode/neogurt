#pragma once

#include "glm/ext/vector_float2.hpp"
#include "webgpu/webgpu_cpp.h"

// wrapper for texture to support high dpi
struct TextureHandle {
  wgpu::Texture t;
  wgpu::TextureView view;
  glm::vec2 size;
  glm::vec2 bufferSize;

  void CreateView() {
    view = t.CreateView();
  }

  void CreateView(const wgpu::TextureViewDescriptor& desc) {
    view = t.CreateView(&desc);
  }
};
