#pragma once

#include "webgpu/webgpu_cpp.h"
#include "glm/ext/vector_float4.hpp"

glm::vec4 PremultiplyAlpha(const glm::vec4& color);
wgpu::Color ToWGPUColor(const glm::vec4& color);
glm::vec4 IntToColor(uint32_t color);             // rgb no alpha
// uint32_t ColorToInt(const glm::vec4& color); // rgba
