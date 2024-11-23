#pragma once

#include "glm/ext/vector_uint2.hpp"
#include "webgpu_tools/webgpu_utils.hpp"
#include "gfx/pipeline.hpp"

struct SDL_Window;

struct WGPUContext : wgpu::utils::DeviceWrapper {
  wgpu::Instance instance;
  wgpu::Surface surface;
  wgpu::Adapter adapter;
  wgpu::Device device;
  wgpu::Queue queue;

  // surface config stuff
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
