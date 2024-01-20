#include "context.hpp"
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include "utils/webgpu.hpp"
#include <dawn/utils/TextureUtils.h>

#include <iostream>

using namespace wgpu;

WGPUContext::WGPUContext(GLFWwindow *window, glm::uvec2 size) : size(size) {
  // instance
  instance = CreateInstance();
  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    std::exit(1);
  }

  // surface
  surface = glfw::CreateSurfaceForWindow(instance, window);

  // adapter
  RequestAdapterOptions adapterOpts{
    .compatibleSurface = surface,
    .powerPreference = PowerPreference::HighPerformance,
  };
  adapter = utils::RequestAdapter(instance, &adapterOpts);

  // device limits
  SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);
  // utils::PrintLimits(supportedLimits.limits);

  RequiredLimits requiredLimits{
    .limits = supportedLimits.limits,
  };

  // device
  DeviceDescriptor deviceDesc{
    .requiredLimits = &requiredLimits,
  };
  device = utils::RequestDevice(adapter, &deviceDesc);
  utils::SetUncapturedErrorCallback(device);

  // queue
  queue = device.GetQueue();

  // swap chain format
  swapChainFormat = TextureFormat::BGRA8Unorm;

  // swap chain
  SwapChainDescriptor swapChainDesc{
    .usage = TextureUsage::RenderAttachment,
    .format = swapChainFormat,
    .width = size.x,
    .height = size.y,
    .presentMode = PresentMode::Fifo,
  };
  swapChain = device.CreateSwapChain(surface, &swapChainDesc);

  // pipeline = gfx::Pipeline(*this);
}

