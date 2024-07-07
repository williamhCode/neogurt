#include "context.hpp"
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

  surface = SDL_GetWGPUSurface(instance, window);
  Init();
}

void WGPUContext::Init() {
  RequestAdapterOptions adapterOpts{
    .compatibleSurface = surface,
    .powerPreference = PowerPreference::HighPerformance,
  };
  adapter = utils::RequestAdapter(instance, &adapterOpts);

  SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);
  // utils::PrintLimits(supportedLimits.limits);

  // RequiredLimits requiredLimits{
  //   .limits = supportedLimits.limits,
  // };

  // SurfaceCapabilities surfaceCaps;
  // surface.GetCapabilities(adapter, &surfaceCaps);
  // utils::PrintSurfaceCapabilities(surfaceCaps);

  // std::vector<FeatureName> requiredFeatures{
  //   FeatureName::SurfaceCapabilities,
  // };

  DeviceDescriptor deviceDesc{
    // .requiredFeatureCount = requiredFeatures.size(),
    // .requiredFeatures = requiredFeatures.data(),
    // .requiredLimits = &requiredLimits,
  };
  device = utils::RequestDevice(adapter, &deviceDesc);
  utils::SetUncapturedErrorCallback(device);

  queue = device.GetQueue();

  // auto prefFormat = surface.GetPreferredFormat(adapter);
  // LOG_INFO("preferred format: {}", (int)prefFormat);
  surfaceFormat = TextureFormat::BGRA8Unorm;

  // apple doesn't support unpremultiplied alpha
  alphaMode = CompositeAlphaMode::Auto;

  SurfaceConfiguration surfaceConfig{
    .device = device,
    .format = surfaceFormat,
    .alphaMode = alphaMode,
    .width = size.x,
    .height = size.y,
    .presentMode = presentMode,
  };
  surface.Configure(&surfaceConfig);

  pipeline = Pipeline(*this);
}

void WGPUContext::Resize(glm::uvec2 _size) {
  size = _size;

  SurfaceConfiguration surfaceConfig{
    .device = device,
    .format = surfaceFormat,
    .alphaMode = alphaMode,
    .width = size.x,
    .height = size.y,
    .presentMode = presentMode,
  };
  surface.Configure(&surfaceConfig);
}
