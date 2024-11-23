#pragma once

#include <chrono>
#include <deque>

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
  // non positive fps means as fast as possible
  double Tick(double fps = 0);
  double GetFps();
};
