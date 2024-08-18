#pragma once

#include <coroutine>
#include <exception>
#include <future>
#include <thread>
#include <type_traits>
#include <chrono>

// Enable the use of std::future<T> as a coroutine type
// by using a std::promise<T> as the promise type.
template <typename T, typename... Args>
  requires(!std::is_void_v<T> && !std::is_reference_v<T>)
struct std::coroutine_traits<std::future<T>, Args...> {
  struct promise_type : std::promise<T> {
    std::future<T> get_return_object() noexcept {
      return this->get_future();
    }

    std::suspend_never initial_suspend() const noexcept { return {}; }
    std::suspend_never final_suspend() const noexcept { return {};
    }

    void return_value(const T& value)
      noexcept(std::is_nothrow_copy_constructible_v<T>) {
      this->set_value(value);
    }

    void return_value(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>) {
      this->set_value(std::move(value));
    }

    void unhandled_exception() noexcept {
      this->set_exception(std::current_exception());
    }
  };
};

// Same for std::future<void>.
template <typename... Args>
struct std::coroutine_traits<std::future<void>, Args...> {
  struct promise_type : std::promise<void> {
    std::future<void> get_return_object() noexcept {
      return this->get_future();
    }

    std::suspend_never initial_suspend() const noexcept { return {}; }
    std::suspend_never final_suspend() const noexcept { return {}; }

    void return_void() noexcept {
      this->set_value();
    }

    void unhandled_exception() noexcept {
      this->set_exception(std::current_exception());
    }
  };
};

// Allow co_await'ing std::future<T> and std::future<void>
// by naively spawning a new thread for each co_await.
template <typename T>
inline auto operator co_await(std::future<T> future) noexcept
  requires(!std::is_reference_v<T>)
{
  struct awaiter : std::future<T> {
    bool await_ready() const noexcept {
      using namespace std::chrono_literals;
      return this->wait_for(0s) != std::future_status::timeout;
    }

    void await_suspend(std::coroutine_handle<> cont) const {
      std::thread([this, cont] {
        this->wait();
        cont();
      })
      .detach();
    }

    T await_resume() {
      return this->get();
    }
  };

  return awaiter{std::move(future)};
}

// technically could use as normal sleep if discarding return, but should prob just use std::this_thread::sleep_for(duration) for that
[[nodiscard]] inline std::future<void> AsyncSleep(const std::chrono::nanoseconds& duration) {
  return std::async(std::launch::async, [duration]() {
    std::this_thread::sleep_for(duration);
  });
}

// wait for all futures to complete in parallel
template <typename... Futures>
auto WhenAll(Futures&&... futures) -> std::future<std::tuple<std::decay_t<Futures>...>> {
  return std::async(
    std::launch::async,
    [futures = std::make_tuple(std::forward<Futures>(futures)...)]() mutable {
      std::apply([](auto&&... futures) { (futures.wait(), ...); }, futures);
      return std::move(futures);
    }
  );
}

auto FutureGet(auto&& future) {
  if constexpr (std::is_same_v<decltype(future.get()), void>) {
    return std::monostate{};
  } else {
    return future.get();
  }
}

// returns all future results when all futures are ready in parallel
// returns std::monoate if value type is void
template <typename... Futures>
auto GetAll(Futures&&... futures) -> std::future<std::tuple<decltype(FutureGet(futures))...>> {
  return std::async(
    std::launch::async,
    [futures = std::make_tuple(std::forward<Futures>(futures)...)]() mutable {
      return std::apply(
        [](auto&&... futures) {
          return std::make_tuple(FutureGet(std::forward<Futures>(futures))...);
        },
        futures
      );
    }
  );
}
