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
  void RegisterTime(std::chrono::nanoseconds time);
  std::chrono::nanoseconds GetAverageDuration();
};

inline auto TimeNow() {
  return std::chrono::steady_clock::now();
}

inline auto TimeToUs(std::chrono::nanoseconds time) {
  return std::chrono::duration<double, std::micro>(time);
}

inline auto TimeToMs(std::chrono::nanoseconds time) {
  return std::chrono::duration<double, std::milli>(time);
}
