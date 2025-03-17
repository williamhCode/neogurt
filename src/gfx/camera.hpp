#pragma once

#include "webgpu/webgpu_cpp.h"
#include "glm/ext/vector_float2.hpp"

struct Ortho2D {
  wgpu::Buffer viewProjBuffer;
  wgpu::BindGroup viewProjBG;

  Ortho2D();
  Ortho2D(glm::vec2 size);

  void Resize(glm::vec2 size, glm::vec2 pos = {0, 0});
};
