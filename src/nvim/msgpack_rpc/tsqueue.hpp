#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
struct TSQueue {
private:
  TSQueue& self = *this;
  std::queue<T> queue;
  std::mutex mutex;
  std::condition_variable cond;
  bool toExit = false;

public:
  void push(T& item) {
    std::scoped_lock<std::mutex> lock(mutex);
    queue.push(std::move(item));
    cond.notify_one();
  }

  void pop() {
    std::scoped_lock<std::mutex> lock(self.mutex);
    if (queue.empty()) {
      throw std::runtime_error("Cannot pop empty queue");
    }
    self.queue.pop();
  }

  T& front() {
    std::scoped_lock<std::mutex> lock(self.mutex);
    if (queue.empty()) {
      throw std::runtime_error("Cannot get front of empty queue");
    }
    return self.queue.front();
  }

  bool empty() {
    std::scoped_lock lock(mutex);
    return queue.empty();
  }

  size_t size() {
    std::scoped_lock lock(mutex);
    return queue.size();
  }

  void exit() {
    std::scoped_lock lock(mutex);
    self.toExit = true;
    cond.notify_all();
  }

  // returns if exit was called
  bool wait() {
    std::unique_lock<std::mutex> lock(self.mutex);
    self.cond.wait(lock, [&]() { return !self.queue.empty() || self.toExit; });
    if (toExit) {
      return true;
    }
    return false;
  }
};
