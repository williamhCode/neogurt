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
