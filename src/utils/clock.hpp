#pragma once

#include <chrono>
#include <deque>
#include <optional>

struct Clock {
  // clock sleep
  std::chrono::steady_clock::time_point lastFrame;
  std::chrono::steady_clock::time_point nextFrame;

  // calc delta and fps
  std::chrono::steady_clock::time_point prevTime;
  std::deque<std::chrono::nanoseconds> durations;
  size_t bufferSize = 500;

  Clock();
  double Tick(std::optional<int> fps = std::nullopt);
  double GetFps();
};
