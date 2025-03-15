#pragma once
#include <mutex>
#include <shared_mutex>

template <
  class T,
  class M = std::mutex,
  template <class...> class WL = std::unique_lock,
  template <class...> class RL = std::unique_lock>
struct MutexGuarded {
  MutexGuarded() = default;
  
  MutexGuarded(T in) : t(std::move(in)) {
  }

  auto read(auto f) const {
    auto l = lock();
    return f(t);
  }

  auto write(auto f) {
    auto l = lock();
    return f(t);
  }

private:
  mutable M m;
  T t;

  auto lock() const {
    return RL<M>(m);
  }

  auto lock() {
    return WL<M>(m);
  }
};

template <class T>
using SharedGuarded =
  MutexGuarded<T, std::shared_mutex, std::shared_lock, std::unique_lock>;
