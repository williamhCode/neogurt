#include "context.hpp"
#include <webgpu/webgpu_cpp.h>
#include "utils/logger.hpp"
#include "webgpu_tools/utils/webgpu.hpp"
#include "webgpu_tools/utils/sdl3webgpu.h"

using namespace wgpu;

WGPUContext::WGPUContext(SDL_Window* window, glm::uvec2 _size, PresentMode _presentMode)
    : size(_size), presentMode(_presentMode) {
  instance = CreateInstance();
  if (!instance) {
    LOG_ERR("Could not initialize WebGPU!");
    std::exit(1);
  }

  surface = Surface::Acquire(SDL_GetWGPUSurface(instance.Get(), window));
  Init();
}

void WGPUContext::Init() {
  RequestAdapterOptions adapterOpts{
    .compatibleSurface = surface,
    .powerPreference = PowerPreference::LowPower,
  };
  adapter = utils::RequestAdapter(instance, &adapterOpts);

  SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);
  // utils::PrintLimits(supportedLimits.limits);

  RequiredLimits requiredLimits{
    .limits = supportedLimits.limits,
  };

  // std::vector<FeatureName> requiredFeatures{
  //   FeatureName::SurfaceCapabilities,
  // };

  DeviceDescriptor deviceDesc{
    // .requiredFeatureCount = requiredFeatures.size(),
    // .requiredFeatures = requiredFeatures.data(),
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

void WGPUContext::Resize(glm::uvec2 _size) {
  size = _size;

  SwapChainDescriptor swapChainDesc{
    .usage = TextureUsage::RenderAttachment,
    .format = swapChainFormat,
    .width = size.x,
    .height = size.y,
    .presentMode = presentMode,
  };
  swapChain = device.CreateSwapChain(surface, &swapChainDesc);
}
