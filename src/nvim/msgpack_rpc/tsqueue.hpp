#include <mutex>
#include <queue>

template <typename T>
struct TSQueue {
private:
  std::queue<T> queue;
  std::mutex mutex;
  bool toExit = false;

public:
  void push(T& item) {
    std::scoped_lock<std::mutex> lock(mutex);
    queue.push(std::move(item));
  }

  void pop() {
    std::scoped_lock<std::mutex> lock(mutex);
    if (queue.empty()) {
      throw std::runtime_error("Cannot pop empty queue");
    }
    queue.pop();
  }

  T& front() {
    std::scoped_lock<std::mutex> lock(mutex);
    if (queue.empty()) {
      throw std::runtime_error("Cannot get front of empty queue");
    }
    return queue.front();
  }

  bool empty() {
    std::scoped_lock lock(mutex);
    return queue.empty();
  }

  size_t size() {
    std::scoped_lock lock(mutex);
    return queue.size();
  }
};
