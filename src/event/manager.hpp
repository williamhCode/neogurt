#pragma once

#include "session/forward.hpp"
#include "utils/thread.hpp"
#include <queue>
#include <functional>
#include <thread>
#include <future>

struct EventManager {
private:
  SessionManager& sessionManager;
  Sync<std::queue<std::function<void()>>> events;

public:
  EventManager(SessionManager& sessionManager);

  // tasks to execute on render thread
  template <typename T>
  void AddTask(T&& func) {
    events.lock()->push(std::forward<T>(func));
  }
  void ExecuteTasks();

  void ProcessSessionEvents(SessionHandle& session);

private:
  void SetImeHighlight(SessionHandle& session);

  // executes the function onReady on render thread when the future is ready
  template <typename T, typename Func, typename... Args>
  void DispatchWhenReady(std::future<T> future, Func onReady, Args... args);
};

template <typename T, typename Func, typename... Args>
void EventManager::DispatchWhenReady(
  std::future<T> future, Func onReady, Args... args
) {
  std::thread([future = std::move(future), onReady = std::move(onReady),
               ... args = std::move(args), this]() mutable {
    try {
      T result = future.get();
      auto resultPtr = std::make_shared<T>(std::move(result));

      AddTask([onReady = std::move(onReady), resultPtr = std::move(resultPtr),
               ... args = std::move(args), this]() mutable {
        onReady(*resultPtr, args...);
      });

    } catch (...) {
      // Future broken (session destroyed), safe to ignore
    }
  }).detach();
}
