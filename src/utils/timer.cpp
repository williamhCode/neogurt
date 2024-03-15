#include "timer.hpp"

#include <numeric>

Timer::Timer(size_t bufferSize) : bufferSize(bufferSize) {
}

void Timer::Start() {
  start = std::chrono::steady_clock::now();
}

void Timer::End() {
  auto end = std::chrono::steady_clock::now();
  durations.push_back(end - start);
  if (durations.size() > bufferSize) durations.pop_front();
}

std::chrono::nanoseconds Timer::GetAverageDuration() {
  return std::accumulate(
           durations.begin(), durations.end(), std::chrono::nanoseconds(0)
         ) /
         durations.size();
}
