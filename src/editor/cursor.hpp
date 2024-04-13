#pragma once

#include "webgpu/webgpu_cpp.h"
#include "glm/ext/vector_float2.hpp"
#include "utils/region.hpp"
#include <string>

enum class CursorShape {
  Block,
  Horizontal,
  Vertical,
  None,
};

struct ModeInfo {
  CursorShape cursorShape = CursorShape::None;

  int cellPercentage = 0;
  int blinkwait = 0;
  int blinkon = 0;
  int blinkoff = 0;
  int attrId = 0;

  std::string ToString() {
    std::string str = "ModeInfo: ";
    str += "cursorShape: " + std::to_string(int(cursorShape)) + ", ";
    str += "cellPercentage: " + std::to_string(cellPercentage) + ", ";
    str += "blinkwait: " + std::to_string(blinkwait) + ", ";
    str += "blinkon: " + std::to_string(blinkon) + ", ";
    str += "blinkoff: " + std::to_string(blinkoff) + ", ";
    str += "attrId: " + std::to_string(attrId) + ", ";
    return str;
  }
};

enum class BlinkState { Wait, On, Off };

struct Cursor {
  glm::vec2 winScrollOffset; // window smooth scroll offset

  glm::vec2 startPos;
  glm::vec2 destPos;
  glm::vec2 pos;
  float jumpTime = 0.06;
  float jumpElasped;

  ModeInfo* modeInfo;

  glm::vec2 fullSize;
  Region startCorners;
  Region destCorners;
  Region corners;
  float cornerTime = 0.06; // transition time
  float cornerElasped;

  bool blink;
  float blinkElasped; // in milliseconds
  BlinkState blinkState;

  wgpu::BindGroup currMaskBG;

  bool SetDestPos(glm::vec2 destPos);
  void SetMode(ModeInfo* modeInfo);
  void Update(float dt);
  bool ShouldRender();
};
