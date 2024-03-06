#pragma once

#include <mutex>
#include <queue>

template <typename T>
struct TSQueue {
private:
  std::queue<T> queue;
  std::mutex mutex;

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

  T Pop() {
    std::scoped_lock lock(mutex);
    if (queue.empty()) {
      throw std::runtime_error("Cannot pop empty queue");
    }
    auto t = std::move(queue.front());
    queue.pop();
    return t;
  }

  void Push(const T& item) {
    std::scoped_lock lock(mutex);
    queue.push(item);
  }

  void Push(T&& item) {
    std::scoped_lock lock(mutex);
    queue.push(std::move(item));
  }

  bool Empty() {
    std::scoped_lock lock(mutex);
    return queue.empty();
  }

  size_t Size() {
    std::scoped_lock lock(mutex);
    return queue.size();
  }
};
