#pragma once

#include "webgpu/webgpu_cpp.h"

struct ColorState{
  float gamma = 1.7;
  wgpu::Buffer gammaBuffer;
  wgpu::BindGroup gammaBG;

  void Init();
  void SetGamma(float value);
};

