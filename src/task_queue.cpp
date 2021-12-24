#include "task_queue.h"

#include <Arduino.h>

#include <algorithm>
#include <utility>

#include "dprint.h"

namespace {
struct {
  bool operator()(const std::pair<unsigned long, TaskQueueType::Task>& a,
                  std::pair<unsigned long, TaskQueueType::Task>& b) const {
    return a.first > b.first;
  }
} comparator;
}  // namespace

void TaskQueueType::postOneShotTask(const TaskQueueType::Task& task,
                                    unsigned long delayMs) {
  unsigned long currentMillis = millis();
  callbacks.push_back(std::make_pair(currentMillis + delayMs, task));
  std::push_heap(callbacks.begin(), callbacks.end(), comparator);
}

void TaskQueueType::process() {
  if (!callbacks.empty() && millis() > callbacks.back().first) {
    callbacks.back().second();
    callbacks.pop_back();
    std::pop_heap(callbacks.begin(), callbacks.end(), comparator);
  }
}

TaskQueueType TaskQueue;