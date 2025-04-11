#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
struct TsQueue {
private:
  std::queue<T> queue;
  std::mutex mutex;
  std::condition_variable cv;
  bool exit = false;

public:
  T& Front() {
    std::scoped_lock lock(mutex);
    if (queue.empty()) {
      throw std::runtime_error("Cannot get front of empty queue");
    }
    return queue.front();
  }

  T& Back() {
    std::scoped_lock lock(mutex);
    if (queue.empty()) {
      throw std::runtime_error("Cannot get back of empty queue");
    }
    return queue.back();
  }

  void Pop() {
    std::scoped_lock lock(mutex);
    if (queue.empty()) {
      throw std::runtime_error("Cannot pop empty queue");
    }
    queue.pop();
  }

  template <typename U>
  void Push(U&& item) {
    {
      std::scoped_lock lock(mutex);
      queue.push(std::forward<U>(item));
    }
    cv.notify_all();
  }

  // implement emplace
  template <typename... Args>
  decltype(auto) Emplace(Args&&... args) {
    {
      std::scoped_lock lock(mutex);
      queue.emplace(std::forward<Args>(args)...);
    }
    cv.notify_all();
  }

  bool Empty() {
    std::scoped_lock lock(mutex);
    return queue.empty();
  }

  size_t Size() {
    std::scoped_lock lock(mutex);
    return queue.size();
  }

  bool Wait() {
    std::unique_lock lock(mutex);
    cv.wait(lock, [this]() { return !queue.empty() || exit; });
    return !exit;
  }

  void Exit() {
    {
      std::scoped_lock lock(mutex);
      exit = true;
    }
    cv.notify_all();
  }
};
