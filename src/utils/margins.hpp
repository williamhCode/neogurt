#pragma once

#include "glm/ext/vector_float2.hpp"

struct FMargins {
  float top = 0;
  float bottom = 0;
  float left = 0;
  float right = 0;
};

struct Margins {
  int top = 0;
  int bottom = 0;
  int left = 0;
  int right = 0;

  bool Empty() const;
  FMargins ToFloat(glm::vec2 size) const;
};

inline bool Margins::Empty() const {
  return top == 0 && bottom == 0 && left == 0 && right == 0;
}

inline FMargins Margins::ToFloat(glm::vec2 size) const {
  return {
    top * size.y,
    bottom * size.y,
    left * size.x,
    right * size.x,
  };
}
