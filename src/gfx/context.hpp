#pragma once

#include "webgpu_utils/device.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "gfx/pipeline.hpp"

struct SDL_Window;

struct WGPUContext : wgpu::utils::DeviceWrapper {
  wgpu::Instance instance;
  wgpu::Surface surface;
  wgpu::Adapter adapter;
  wgpu::Device device;
  wgpu::Queue queue;

  wgpu::Limits limits;

  Pipeline pipeline;

  WGPUContext() = default;
  WGPUContext(SDL_Window* window, glm::uvec2 size, bool vsync);
  void Resize(glm::uvec2 size, bool vsync);
};
