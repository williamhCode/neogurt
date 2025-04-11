#pragma once

#include <mutex>
#include <utility>

template <typename T>
class Synchronized {
public:
  template <typename... Args>
  explicit Synchronized(Args&&... args)
      : value_(std::forward<Args>(args)...) {}

  // Access wrapper that locks and unlocks
  class Access {
  public:
    Access(T& value, std::mutex& mtx)
        : lock_(mtx), value_(value) {}

    T& operator*() { return value_; }
    T* operator->() { return &value_; }

    auto& get_lock() { return lock_; }

  private:
    std::unique_lock<std::mutex> lock_;
    T& value_;
  };

  Access lock() { return Access(value_, mtx_); }

private:
  T value_;
  mutable std::mutex mtx_;
};

