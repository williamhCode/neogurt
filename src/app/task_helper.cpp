#include "./task_helper.hpp"
#include "utils/logger.hpp"
#include "utils/tsqueue.hpp"
#include <cassert>

static TSQueue<std::function<void()>> taskQueue;

void DeferToMainThread(std::function<void()>&& task) {
  taskQueue.Push(std::move(task));

  SDL_Event event;
  SDL_zero(event);
  event.type = EVENT_DEFERRED_TASK;
  SDL_PushEvent(&event);
}

void ProcessNextMainThreadTask() {
  assert(!taskQueue.Empty());

  auto& task = taskQueue.Front();
  task();
  taskQueue.Pop();
}
