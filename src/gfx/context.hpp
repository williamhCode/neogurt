#pragma once

#include "GLFW/glfw3.h"
#include "glm/ext/vector_uint2.hpp"
#include "webgpu/webgpu_cpp.h"

#include "gfx/pipeline.hpp"

extern const WGPUContext &ctx;

struct WGPUContext {
  wgpu::Instance instance;
  wgpu::Surface surface;
  wgpu::Adapter adapter;
  wgpu::Device device;
  wgpu::Queue queue;
  wgpu::SwapChain swapChain;

  wgpu::TextureFormat swapChainFormat;
  glm::uvec2 size;

  Pipeline pipeline;

  WGPUContext() = default;
  WGPUContext(GLFWwindow *window, glm::uvec2 size, wgpu::PresentMode presentMode);
};
