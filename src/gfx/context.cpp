#include "./context.hpp"
#include "utils/logger.hpp"
#include "webgpu_utils/init.hpp"
#include "sdl3webgpu.hpp"
#include "dawn/webgpu_cpp_print.h"

using namespace wgpu;

WGPUContext::WGPUContext(SDL_Window* window, glm::uvec2 _size, PresentMode _presentMode)
    : size(_size), presentMode(_presentMode) {

  std::vector<std::string> enabledToggles = {
    "enable_immediate_error_handling", "allow_unsafe_apis"
  };

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
    .features = {.timedWaitAnyEnable = true},
  };
  instance = CreateInstance(&desc);
  if (!instance) {
    LOG_ERR("Could not create WebGPU instance!");
    std::exit(1);
  }

  surface = SDL_GetWGPUSurface(instance, window);
  Init();
}

void WGPUContext::Init() {
  adapter = utils::RequestAdapter(
    instance,
    {
      .compatibleSurface = surface,
      .powerPreference = PowerPreference::Undefined,
    }
  );

  // SupportedLimits supportedLimits;
  // adapter.GetLimits(&supportedLimits);
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

  std::vector<FeatureName> requiredFeatures{
    FeatureName::TimestampQuery,
  };

  DeviceDescriptor deviceDesc({
    .requiredFeatureCount = requiredFeatures.size(),
    .requiredFeatures = requiredFeatures.data(),
    // .requiredLimits = &requiredLimits,
  });

  deviceDesc.SetUncapturedErrorCallback(
    [](const Device& device, ErrorType type, const char* message) {
      std::ostringstream() << type;
      LOG_ERR("Device error: {} ({})", ToString(type), message);
    }
  );

  deviceDesc.SetDeviceLostCallback(
    CallbackMode::AllowSpontaneous,
    [](const Device& device, DeviceLostReason reason, const char* message) {
      if (reason != DeviceLostReason::Destroyed) {
        LOG_ERR("Device lost: {} ({})", ToString(reason), message);
      }
    }
  );

  device = utils::RequestDevice(adapter, deviceDesc);
  SetDevice(device); // utils

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
