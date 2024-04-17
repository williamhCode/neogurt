#include "webgpu/webgpu_cpp.h"
#include "glm/ext/vector_float4.hpp"

inline glm::vec4 PremultiplyAlpha(const glm::vec4& color) {
  return {
    color.r * color.a,
    color.g * color.a,
    color.b * color.a,
    color.a,
  };
}

inline wgpu::Color ToWGPUColor(const glm::vec4& color) {
  return {
    color.r,
    color.g,
    color.b,
    color.a,
  };
}
