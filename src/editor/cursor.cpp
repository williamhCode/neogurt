#include "cursor.hpp"
#include "glm/common.hpp"
#include "glm/exponential.hpp"

void Cursor::SetDestPos(glm::vec2 _destPos) {
  if (destPos == _destPos) return;

  destPos = _destPos;
  startPos = pos;
  elasped = 0.0;
}

void Cursor::Update(float dt) {
  if (pos == destPos) return;

  elasped += dt;
  if (elasped >= jumpTime) {
    pos = destPos;
    elasped = 0.0;
  } else {
    // use smoothstep
    float t = elasped / jumpTime;
    pos = glm::mix(startPos, destPos, glm::pow(t, 1 / 2.8));
  }
}
