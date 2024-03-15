#include "clock.hpp"
#include <numeric>
#include <thread>

using namespace std::chrono;

Clock::Clock() {
  lastFrame = steady_clock::now();
  nextFrame = lastFrame;

  prevTime = steady_clock::now();
}

double Clock::Tick(std::optional<double> fps) {
  if (fps) {
    std::this_thread::sleep_until(nextFrame);
    lastFrame = nextFrame;
    double deltaSecs = 1.0 / *fps;
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

