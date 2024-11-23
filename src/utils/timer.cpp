#include "./timer.hpp"
#include <numeric>

Timer::Timer(size_t _bufferSize) : bufferSize(_bufferSize) {
}

void Timer::Start() {
  start = TimeNow();
}

void Timer::End() {
  auto end = TimeNow();
  durations.push_back(end - start);
  if (durations.size() > bufferSize) durations.pop_front();
}

void Timer::RegisterTime(std::chrono::nanoseconds time) {
  durations.push_back(time);
  if (durations.size() > bufferSize) durations.pop_front();
}

std::chrono::nanoseconds Timer::GetAverageDuration() {
  return std::accumulate(
           durations.begin(), durations.end(), std::chrono::nanoseconds(0)
         ) /
         durations.size();
}
