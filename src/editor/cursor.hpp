#pragma once
#include "glm/ext/vector_float2.hpp"

struct Cursor {
  glm::vec2 pos = {0, 0};

  glm::vec2 startPos = {0, 0};
  glm::vec2 destPos = {0, 0};
  // bool first = true;

  float jumpTime = 0.06;
  float elasped = 0.0;

  void SetDestPos(glm::vec2 destPos);
  void Update(float dt);
};
