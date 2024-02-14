#pragma once

#include "webgpu/webgpu_cpp.h"

// wrapper for texture to support high dpi
struct TextureHandle {
  wgpu::Texture t;
  wgpu::TextureView view;
  int width;
  int height;

  void CreateView() {
    view = t.CreateView();
  }

  void CreateView(const wgpu::TextureViewDescriptor& desc) {
    view = t.CreateView(&desc);
  }
};
