#include "context.hpp"
#include "utils/logger.hpp"
#include "webgpu_tools/utils/webgpu.hpp"
#include "webgpu_tools/utils/sdl3webgpu.h"

#define MAGIC_ENUM_RANGE_MAX 1300
#include "magic_enum.hpp"

using namespace wgpu;

WGPUContext::WGPUContext(SDL_Window* window, glm::uvec2 _size, PresentMode _presentMode)
    : size(_size), presentMode(_presentMode) {

  std::vector<std::string> enabledToggles = {"enable_immediate_error_handling"};

  std::vector<const char*> enabledToggleNames;
  for (const std::string& toggle : enabledToggles) {
    enabledToggleNames.push_back(toggle.c_str());
  }

  wgpu::DawnTogglesDescriptor toggles({
    .enabledToggleCount = enabledToggleNames.size(),
    .enabledToggles = enabledToggleNames.data(),
  });

  InstanceDescriptor desc{
    .nextInChain = &toggles,
  };
  instance = CreateInstance(&desc);
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
    .powerPreference = PowerPreference::Undefined,
  };
  adapter = utils::RequestAdapter(instance, &adapterOpts);

  SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);
  // utils::PrintLimits(supportedLimits.limits);

  // FeatureName features[256];
  // size_t featureCount = adapter.EnumerateFeatures(features);
  // for (size_t i = 0; i < featureCount; i++) {
  //   LOG_INFO("feature: {}", magic_enum::enum_name(features[i]));
  // }

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
  // LOG_INFO("preferred format: {}", magic_enum::enum_name(prefFormat));
  surfaceFormat = TextureFormat::BGRA8Unorm;

  // apple doesn't support unpremultiplied alpha
  alphaMode = CompositeAlphaMode::Premultiplied;

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
