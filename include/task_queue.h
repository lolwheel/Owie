#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <functional>
#include <utility>
#include <vector>

class TaskQueueType {
 public:
  typedef std::function<void(void)> Task;

  TaskQueueType(){};
  void postOneShotTask(const Task& task, unsigned long delayMs);
  void process();

 private:
  std::vector<std::pair<unsigned long, Task>> callbacks;
};

extern TaskQueueType TaskQueue;

#endif  // TASK_QUEUE_H