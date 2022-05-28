#include "task_queue_type.h"

#include <unity.h>

#include <memory>
#include <string>

unsigned long millis;
std::vector<std::string> tasksExecuted;
std::unique_ptr<TaskQueueType> TaskQueue;

TaskQueueType::Task taskNamed(const std::string& name) {
  return [=]() { tasksExecuted.push_back(name); };
}

void expectTasksExecuted(const std::vector<std::string>& expected) {
  TEST_ASSERT_EQUAL(expected.size(), tasksExecuted.size());
  for (int i = 0; i < expected.size(); i++) {
    TEST_ASSERT_EQUAL_STRING(expected[i].c_str(), tasksExecuted[i].c_str());
  }
}

void setUp(void) {
  millis = 0;
  tasksExecuted.clear();
  TaskQueue.reset(new TaskQueueType([&]() { return millis; }));
}

void testOneshotTaskExecutionOrder() {
  TaskQueue->postOneShotTask(taskNamed("three"), 3);
  TaskQueue->postOneShotTask(taskNamed("four"), 4);
  TaskQueue->postOneShotTask(taskNamed("two"), 2);
  TaskQueue->postOneShotTask(taskNamed("one"), 1);
  // millis = 0; nothing should execute;
  TaskQueue->process();
  expectTasksExecuted({});
  millis = 1;
  TaskQueue->process();
  expectTasksExecuted({});
  millis = 4;
  TaskQueue->process();
  expectTasksExecuted({"one", "two", "three"});
  millis = 5;
  TaskQueue->process();
  expectTasksExecuted({"one", "two", "three", "four"});
}

void testRecurringAndOneshotInterleaving() {
  TaskQueue->postOneShotTask(taskNamed("oneshot"), 1);
  TaskQueue->postRecurringTask(taskNamed("recurring"));
  TaskQueue->process();
  expectTasksExecuted({"recurring"});
  millis = 4;
  TaskQueue->process();
  expectTasksExecuted({"recurring", "oneshot", "recurring"});
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(testOneshotTaskExecutionOrder);
  RUN_TEST(testRecurringAndOneshotInterleaving);
  UNITY_END();

  return 0;
}