#ifndef TASK_QUEUE_TYPE_H
#define TASK_QUEUE_TYPE_H

#include <functional>
#include <utility>
#include <vector>

class TaskQueueType {
 public:
  typedef std::function<void(void)> Task;

  TaskQueueType(const std::function<unsigned long()>& millis)
      : millis_(millis){};
  void postOneShotTask(const Task& task, unsigned long delayMs);
  void postRecurringTask(const Task& task);
  void process();

 private:
  std::function<unsigned long()> millis_;
  std::vector<Task> recurringTasks_;
  std::vector<std::pair<unsigned long, Task>> timedTasks_;
};

#endif  // TASK_QUEUE_TYPE_H