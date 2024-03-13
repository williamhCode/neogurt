#pragma once
#include "glm/ext/vector_float2.hpp"
#include <optional>

enum class CursorShape {
  Block,
  Horizontal,
  Vertical,
};

struct CursorMode {
  std::optional<CursorShape> cursorShape;
  std::optional<int> cellPercentage;
  std::optional<int> blinkwait;
  std::optional<int> blinkon;
  std::optional<int> blinkoff;
  std::optional<int> attrId;
};

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
