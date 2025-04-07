#pragma once

#include <cmath>

struct Spring {
  float zeta;
  float omega;

  float velocity = 0;

  float Update(float position, float target, float dt) {
    float displacement = position - target;
    float accel = -omega * omega * displacement - 2.0f * zeta * omega * velocity;

    velocity += accel * dt;
    return velocity * dt;
  }

  bool AtRest(float margin) {
    return std::abs(velocity) < margin;
  }

  void Reset() {
    velocity = 0.0f;
  }
};
