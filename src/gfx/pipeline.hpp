#pragma once

#include <webgpu/webgpu_cpp.h>

struct WGPUContext;

struct Pipeline {
  Pipeline() = default;
  Pipeline(WGPUContext &ctx);
};

