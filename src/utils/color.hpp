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

inline auto IntToColor(uint32_t color) {
  return glm::vec4(
    ((color >> 16) & 0xff) / 255.0f,
    ((color >> 8) & 0xff) / 255.0f,
    (color & 0xff) / 255.0f,
    1.0f
  );
};
