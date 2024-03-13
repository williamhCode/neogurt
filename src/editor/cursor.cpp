#include "cursor.hpp"
#include "glm/common.hpp"
#include "glm/exponential.hpp"
#include "utils/logger.hpp"

void Cursor::SetDestPos(glm::vec2 _destPos) {
  if (destPos == _destPos) return;

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
  float width;
  float height;
  glm::vec2 offset;
  switch (modeInfo->cursorShape) {
    case CursorShape::Block:
      width = size.x;
      height = size.y;
      offset = glm::vec2(0, 0);
      break;
    case CursorShape::Horizontal:
      width = size.x;
      height = size.y * ratio;
      offset = glm::vec2(0, size.y * (1 - ratio));
      break;
    case CursorShape::Vertical:
      width = size.x * ratio;
      height = size.y;
      offset = glm::vec2(0, 0);
      break;
    case CursorShape::None:
      assert(false);
      break;
  }
  positions = {
    glm::vec2(0, 0),
    glm::vec2(width, 0),
    glm::vec2(width, height),
    glm::vec2(0, height),
  };
  for (auto& pos : positions) {
    pos += offset;
  }

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
      pos = glm::mix(startPos, destPos, glm::pow(t, 1 / 2.8));
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
