#include "./task_helper.hpp"
#include "utils/thread.hpp"
#include <cassert>
#include <queue>

static Sync<std::queue<std::function<void()>>> taskQueue;

void ExecuteOnMainThread(std::function<void()>&& task) {
  taskQueue.lock()->push(std::move(task));

  SDL_Event event;
  SDL_zero(event);
  event.type = EVENT_DEFERRED_TASK;
  SDL_PushEvent(&event);
}

void ProcessNextMainThreadTask() {
  assert(!taskQueue.lock()->empty());
  auto& task = taskQueue.lock()->front();
  task();
  taskQueue.lock()->pop();
}

static Sync<std::queue<SessionHandle>> sessionsQueue;

void PushSessionToMainThread(SessionHandle session) {
  sessionsQueue.lock()->push(std::move(session));

  SDL_Event event;
  SDL_zero(event);
  event.type = EVENT_SWITCH_SESSION;
  SDL_PushEvent(&event);
}

SessionHandle PopSessionFromMainThread() {
  SessionHandle session;
  {
    auto access = sessionsQueue.lock();
    assert(!access->empty());
    session = std::move(access->front());
    access->pop();
  }
  return session;
}
