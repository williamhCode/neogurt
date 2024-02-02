#pragma once

#include "webgpu/webgpu_cpp.h"

// wrapper for texture to support high dpi
struct TextureHandle {
  wgpu::Texture texture;
  wgpu::TextureView view;
  int width;
  int height;

  void CreateView() {
    view = texture.CreateView();
  }

  void CreateView(const wgpu::TextureViewDescriptor& desc) {
    view = texture.CreateView(&desc);
  }
};
