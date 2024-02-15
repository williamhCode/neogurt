#include "context.hpp"
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include "webgpu_utils/webgpu.hpp"

#include <iostream>

using namespace wgpu;

WGPUContext::WGPUContext(GLFWwindow* window, glm::uvec2 size, PresentMode presentMode)
    : size(size), presentMode(presentMode) {
  instance = CreateInstance();
  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    std::exit(1);
  }

  surface = glfw::CreateSurfaceForWindow(instance, window);

  RequestAdapterOptions adapterOpts{
    .compatibleSurface = surface,
    .powerPreference = PowerPreference::HighPerformance,
  };
  adapter = utils::RequestAdapter(instance, &adapterOpts);

  SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);
  // utils::PrintLimits(supportedLimits.limits);

  RequiredLimits requiredLimits{
    .limits = supportedLimits.limits,
  };

  DeviceDescriptor deviceDesc{
    .requiredLimits = &requiredLimits,
  };
  device = utils::RequestDevice(adapter, &deviceDesc);
  utils::SetUncapturedErrorCallback(device);

  queue = device.GetQueue();

  swapChainFormat = TextureFormat::BGRA8Unorm;

  SwapChainDescriptor swapChainDesc{
    .usage = TextureUsage::RenderAttachment,
    .format = swapChainFormat,
    .width = size.x,
    .height = size.y,
    .presentMode = presentMode,
  };
  swapChain = device.CreateSwapChain(surface, &swapChainDesc);

  pipeline = Pipeline(*this);
}

void WGPUContext::Resize(glm::uvec2 size) {
  this->size = size;

  SwapChainDescriptor swapChainDesc{
    .usage = TextureUsage::RenderAttachment,
    .format = swapChainFormat,
    .width = size.x,
    .height = size.y,
    .presentMode = presentMode,
  };
  swapChain = device.CreateSwapChain(surface, &swapChainDesc);
}
