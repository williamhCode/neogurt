#include "cursor.hpp"
#include "glm/exponential.hpp"
#include "glm/geometric.hpp"

void Cursor::SetDestPos(glm::vec2 _destPos) {
  if (destPos == _destPos) return;

  destPos = _destPos;
  startPos = pos;
  elasped = 0.0f;
}

void Cursor::Update(float dt) {
  if (pos == destPos) return;

  elasped += dt;
  if (elasped >= jumpTime) {
    pos = destPos;
    elasped = 0.0f;
  } else {
    // use smoothstep
    float t = elasped / jumpTime;
    pos = glm::mix(startPos, destPos, glm::pow(t, 1 / 2.8));
  }
}
