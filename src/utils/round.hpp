#pragma once
#include <cmath>

inline float RoundToPixel(float val, float scale) {
  return std::round(val * scale) / scale;
}
