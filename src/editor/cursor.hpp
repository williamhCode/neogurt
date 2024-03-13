#pragma once
#include "glm/ext/vector_float2.hpp"
#include <optional>
#include <string>

enum class CursorShape {
  Block,
  Horizontal,
  Vertical,
};

struct ModeInfo {
  std::optional<CursorShape> cursorShape;
  std::optional<int> cellPercentage;
  // 0 = no blink
  int blinkwait = 0;
  int blinkon = 0;
  int blinkoff = 0;
  std::optional<int> attrId;

  std::string ToString() {
    std::string str = "ModeInfo: ";
    if (cursorShape.has_value()) {
      str += "cursorShape: " + std::to_string(int(cursorShape.value())) + ", ";
    }
    if (cellPercentage.has_value()) {
      str += "cellPercentage: " + std::to_string(cellPercentage.value()) + ", ";
    }
    str += "blinkwait: " + std::to_string(blinkwait) + ", ";
    str += "blinkon: " + std::to_string(blinkon) + ", ";
    str += "blinkoff: " + std::to_string(blinkoff) + ", ";
    if (attrId.has_value()) {
      str += "attrId: " + std::to_string(attrId.value()) + ", ";
    }
    return str;
  }
};

enum class BlinkState { Wait, On, Off };

struct Cursor {
  glm::vec2 size;

  ModeInfo* modeInfo = nullptr;
  bool blink = false;
  float blinkElasped;
  BlinkState blinkState;

  glm::vec2 startPos;
  glm::vec2 destPos;
  glm::vec2 pos;
  float jumpTime = 0.06;
  float jumpElasped;

  void SetDestPos(glm::vec2 destPos);
  void SetMode(ModeInfo* modeInfo);
  void Update(float dt);
};
