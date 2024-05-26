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


glm::vec4 ToLinear(const glm::vec4& color) {
  return {
    std::pow(color.r, 2.2f),
    std::pow(color.g, 2.2f),
    std::pow(color.b, 2.2f),
    color.a,
  };
}

glm::vec4 ToSrgb(const glm::vec4& color) {
  return {
    std::pow(color.r, 1.0f / 2.2f),
    std::pow(color.g, 1.0f / 2.2f),
    std::pow(color.b, 1.0f / 2.2f),
    color.a,
  };
}
