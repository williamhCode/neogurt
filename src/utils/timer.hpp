#pragma once

#include <chrono>
#include <deque>

struct Timer {
  size_t bufferSize;
  std::chrono::time_point<std::chrono::steady_clock> start;
  std::deque<std::chrono::nanoseconds> durations;

  Timer(size_t bufferSize = 20);

  void Start();
  void End();
  std::chrono::nanoseconds GetAverageDuration();
};
