#pragma once

#include <chrono>
#include <deque>
#include <optional>

class Clock {
private:
  // clock sleep
  std::chrono::steady_clock::time_point nextFrame;

  // calc delta and fps
  std::chrono::steady_clock::time_point prevTime;
  std::deque<std::chrono::nanoseconds> durations;

  size_t bufferSize = 10;

public:
  Clock();
  double Tick(std::optional<double> fps = std::nullopt);
  double GetFps();
};
