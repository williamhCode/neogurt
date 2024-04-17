#pragma once

#include "SDL3/SDL.h"
#include "glm/ext/vector_uint2.hpp"
#include "webgpu/webgpu_cpp.h"

#include "gfx/pipeline.hpp"

struct WGPUContext {
  wgpu::Instance instance;
  wgpu::Surface surface;
  wgpu::Adapter adapter;
  wgpu::Device device;
  wgpu::Queue queue;

  glm::uvec2 size;
  wgpu::TextureFormat surfaceFormat;
  wgpu::CompositeAlphaMode alphaMode;
  wgpu::PresentMode presentMode;

  Pipeline pipeline;

  WGPUContext() = default;
  WGPUContext(SDL_Window* window, glm::uvec2 size, wgpu::PresentMode presentMode);
  void Init();
  void Resize(glm::uvec2 size);
};
