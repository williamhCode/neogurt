#include "./context.hpp"
#include "app/path.hpp"
#include "utils/logger.hpp"
#include "webgpu_utils/init.hpp"
#include "sdl3webgpu.hpp"
#include "dawn/webgpu_cpp_print.h"

using namespace wgpu;

WGPUContext::WGPUContext(SDL_Window* window, glm::uvec2 size, bool vsync, float gamma) {
  // instance -----------------------------
  std::vector<std::string> enabledToggles = {
    "enable_immediate_error_handling", "allow_unsafe_apis"
  };

  std::vector<const char*> enabledToggleNames;
  enabledToggleNames.reserve(enabledToggles.size());
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

  // adapter ------------------------------------
  adapter = utils::RequestAdapter(
    instance,
    {
      .compatibleSurface = surface,
      .powerPreference = PowerPreference::Undefined,
    }
  );

  SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);
  limits = supportedLimits.limits;
  // utils::PrintLimits(limits);

  // FeatureName features[256];
  // size_t featureCount = adapter.EnumerateFeatures(features);
  // for (size_t i = 0; i < featureCount; i++) {
  //   LOG_INFO("feature: {}", magic_enum::enum_name(features[i]));
  // }

  RequiredLimits requiredLimits{
    .limits = {.maxTextureDimension2D = limits.maxTextureDimension2D}
  };

  // SurfaceCapabilities surfaceCaps;
  // surface.GetCapabilities(adapter, &surfaceCaps);
  // utils::PrintSurfaceCapabilities(surfaceCaps);

  // device -------------------------------------
  std::vector<FeatureName> requiredFeatures{
    FeatureName::TimestampQuery,
  };

  DeviceDescriptor deviceDesc({
    .requiredFeatureCount = requiredFeatures.size(),
    .requiredFeatures = requiredFeatures.data(),
    .requiredLimits = &requiredLimits,
  });

  deviceDesc.SetUncapturedErrorCallback(
    [](const Device& _, ErrorType type, const char* message) {
      std::ostringstream() << type;
      LOG_ERR("Device error: {} ({})", ToString(type), message);
      // LOG_TRACE();
    }
  );

  deviceDesc.SetDeviceLostCallback(
    CallbackMode::AllowSpontaneous,
    [](const Device& _, DeviceLostReason reason, const char* message) {
      if (reason != DeviceLostReason::Destroyed) {
        LOG_ERR("Device lost: {} ({})", ToString(reason), message);
      }
    }
  );

  device = utils::RequestDevice(instance, adapter, deviceDesc);
  DeviceWrapper::device = device;

  queue = device.GetQueue();

  // surface --------------------------------
  Resize(size, vsync);

  // pipeline --------------------------------
  slang = SlangContext(fs::path(resourcesDir) / "shaders");
  pipeline = Pipeline(*this, slang, gamma);
}

void WGPUContext::Resize(glm::uvec2 size, bool vsync) {
  SurfaceConfiguration surfaceConfig{
    .device = device,
    .format = TextureFormat::BGRA8Unorm,
    .alphaMode = CompositeAlphaMode::Premultiplied,
    .width = size.x,
    .height = size.y,
    .presentMode = vsync ? PresentMode::Fifo : PresentMode::Immediate,
  };
  surface.Configure(&surfaceConfig);
}
