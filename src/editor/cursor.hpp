#pragma once
#include "glm/ext/vector_float2.hpp"

struct Cursor {
  glm::vec2 destPos;
  glm::vec2 pos;
  bool first = true;

  static constexpr float speed = 100;

  void Update(float dt);
};
