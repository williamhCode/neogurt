#pragma once

#include "webgpu/webgpu_cpp.h"
#include "glm/ext/vector_float4.hpp"

glm::vec4 AdjustAlpha(const glm::vec4& color, float gamma);
glm::vec4 PremultiplyAlpha(const glm::vec4& color);

wgpu::Color ToWGPUColor(const glm::vec4& color);
glm::vec4 ToGlmColor(const wgpu::Color& color);

glm::vec4 IntToColor(uint32_t color);
// uint32_t ColorToInt(const glm::vec4& color); // rgba

glm::vec4 ToLinear(const glm::vec4& color, float gamma);
glm::vec4 ToSrgb(const glm::vec4& color, float gamma);
