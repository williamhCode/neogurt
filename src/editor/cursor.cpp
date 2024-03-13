#include "cursor.hpp"
#include "glm/common.hpp"
#include "glm/exponential.hpp"
#include "utils/logger.hpp"

void Cursor::SetDestPos(glm::vec2 _destPos) {
  if (_destPos == destPos) return;

  destPos = _destPos;
  startPos = pos;
  jumpElasped = 0.0;

  if (blink) {
    blinkState = BlinkState::Wait;
    blinkElasped = 0.0;
  }
}

void Cursor::SetMode(ModeInfo* _modeInfo) {
  modeInfo = _modeInfo;
  // LOG("SetMode: {}", modeInfo->ToString());

  float ratio = modeInfo->cellPercentage / 100.0;
  glm::vec2 size = fullSize;
  glm::vec2 offset(0);
  switch (modeInfo->cursorShape) {
    case CursorShape::Block:
      break;
    case CursorShape::Horizontal:
      size.y *= ratio;
      offset.y = fullSize.y * (1 - ratio);
      break;
    case CursorShape::Vertical:
      size.x *= ratio;
      break;
    case CursorShape::None:
      assert(false);
      break;
  }
  destCorners = {
    glm::vec2(0, 0),
    glm::vec2(size.x, 0),
    glm::vec2(size.x, size.y),
    glm::vec2(0, size.y),
  };
  for (auto& corner : destCorners) {
    corner += offset;
  }
  startCorners = corners;
  cornerElasped = 0.0;

  if (blink) {
    blinkState = BlinkState::Wait;
    blinkElasped = 0.0;
  }
  blink = modeInfo->blinkwait != 0 && modeInfo->blinkon != 0 && modeInfo->blinkoff != 0;
}

void Cursor::Update(float dt) {
  // position
  if (pos != destPos) {
    jumpElasped += dt;
    if (jumpElasped >= jumpTime) {
      pos = destPos;
      jumpElasped = 0.0;
    } else {
      // use smoothstep
      float t = jumpElasped / jumpTime;
      float x = glm::pow(t, 1 / 2.8);
      pos = glm::mix(startPos, destPos, x);
    }
  }

  // Shape transition
  if (corners != destCorners) {
    cornerElasped += dt;
    if (cornerElasped >= cornerTime) {
      corners = destCorners;
      cornerElasped = 0.0;
    } else {
      float t = cornerElasped / cornerTime;
      for (size_t i = 0; i < 4; i++) {
        float x = glm::pow(t, 1 / 2.8);
        corners[i] = glm::mix(startCorners[i], destCorners[i], x);
      }
    }
  }

  // blink
  if (blink) {
    blinkElasped += dt * 1000; // in milliseconds
    switch (blinkState) {
      case BlinkState::Wait:
        if (blinkElasped >= modeInfo->blinkwait) {
          blinkState = BlinkState::On;
          blinkElasped = 0.0;
        }
        break;
      case BlinkState::On:
        if (blinkElasped >= modeInfo->blinkon) {
          blinkState = BlinkState::Off;
          blinkElasped = 0.0;
        }
        break;
      case BlinkState::Off:
        if (blinkElasped >= modeInfo->blinkoff) {
          blinkState = BlinkState::On;
          blinkElasped = 0.0;
        }
        break;
    }
  }
}
