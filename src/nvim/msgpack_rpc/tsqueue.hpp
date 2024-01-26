#include <mutex>
#include <queue>

template <typename T>
struct TSQueue {
private:
  TSQueue& self = *this;
  std::queue<T> queue;
  std::mutex mutex;
  bool toExit = false;

public:
  void push(T& item) {
    std::scoped_lock<std::mutex> lock(self.mutex);
    self.queue.push(std::move(item));
  }

  void pop() {
    std::scoped_lock<std::mutex> lock(self.mutex);
    if (self.queue.empty()) {
      throw std::runtime_error("Cannot pop empty queue");
    }
    self.queue.pop();
  }

  T& front() {
    std::scoped_lock<std::mutex> lock(self.mutex);
    if (self.queue.empty()) {
      throw std::runtime_error("Cannot get front of empty queue");
    }
    return self.queue.front();
  }

  bool empty() {
    std::scoped_lock lock(self.mutex);
    return self.queue.empty();
  }

  size_t size() {
    std::scoped_lock lock(self.mutex);
    return self.queue.size();
  }
};
