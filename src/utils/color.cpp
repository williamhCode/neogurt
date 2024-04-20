#include "color.hpp"

glm::vec4 PremultiplyAlpha(const glm::vec4& color) {
  return {
    color.r * color.a,
    color.g * color.a,
    color.b * color.a,
    color.a,
  };
}

wgpu::Color ToWGPUColor(const glm::vec4& color) {
  return {
    color.r,
    color.g,
    color.b,
    color.a,
  };
}

// rgb no alpha
glm::vec4 IntToColor(uint32_t color) {
  return {
    ((color >> 16) & 0xff) / 255.0f,
    ((color >> 8) & 0xff) / 255.0f,
    (color & 0xff) / 255.0f,
    1.0f
  };
};

// rgba
// uint32_t ColorToInt(const glm::vec4& color) {
//   return
//     ((static_cast<uint32_t>(color.r * 255.0f) & 0xff) << 24) |
//     ((static_cast<uint32_t>(color.g * 255.0f) & 0xff) << 16) |
//     ((static_cast<uint32_t>(color.b * 255.0f) & 0xff) << 8) |
//     (static_cast<uint32_t>(color.a * 255.0f) & 0xff);
// }

