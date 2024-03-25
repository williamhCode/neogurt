#pragma once

#include "webgpu/webgpu_cpp.h"
#include "glm/ext/vector_uint2.hpp"

struct Ortho2D {
  wgpu::Buffer viewProjBuffer;
  wgpu::BindGroup viewProjBG;

  Ortho2D() = default;
  Ortho2D(glm::uvec2 size);
  void Resize(glm::uvec2 size);
};
