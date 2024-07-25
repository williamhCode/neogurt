#include "clock.hpp"
#include <numeric>
#include <thread>

using namespace std::chrono;

Clock::Clock() {
  nextFrame = steady_clock::now();
  prevTime = steady_clock::now();
}

double Clock::Tick(double fps) {
  if (fps > 0) {
    // nextFrame not catching up when app is paused for a while
    auto now = steady_clock::now();
    if (nextFrame < now) {
      nextFrame = now;
    }

    std::this_thread::sleep_until(nextFrame);
    double deltaSecs = 1.0 / fps;
    auto delta = nanoseconds(static_cast<long long>(deltaSecs * 1e9));
    nextFrame += delta;
  }

  auto currTime = steady_clock::now();
  auto dt = currTime - prevTime;
  prevTime = currTime;

  durations.push_back(dt);
  if (durations.size() > bufferSize) durations.pop_front();

  return duration_cast<duration<double>>(dt).count();
}

double Clock::GetFps() {
  auto sum = std::accumulate(durations.begin(), durations.end(), nanoseconds(0));
  auto sumSecs = duration_cast<duration<double>>(sum).count();
  return durations.size() / sumSecs;
}

