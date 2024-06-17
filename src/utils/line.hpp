#pragma once

struct Line {
  float pos;
  float height;

  float Top() const {
    return pos;
  }

  float Bot() const {
    return pos + height;
  }

  bool Intersects(const Line& other) const {
    return Top() < other.Bot() && Bot() > other.Top();
  }
};
