#include "cursor.hpp"
#include "glm/geometric.hpp"

void Cursor::Update(float dt) {
  if (first) {
    pos = destPos;
    first = false;
    return;
  }

  auto dir = glm::normalize(destPos - pos);
  auto delta = dir * speed * dt;
  if (glm::length(delta) > glm::length(destPos - pos)) {
    pos = destPos;
  } else {
    pos += delta;
  }
}
