#pragma once

#include <chrono>
#include <deque>
#include <optional>

class Clock {
private:
  // clock sleep
  std::chrono::steady_clock::time_point lastFrame;
  std::chrono::steady_clock::time_point nextFrame;

  // calc delta and fps
  std::chrono::steady_clock::time_point prevTime;
  std::deque<std::chrono::nanoseconds> durations;

public:
  size_t bufferSize = 200;

  Clock();
  double Tick(std::optional<double> fps = std::nullopt);
  double GetFps();
};
