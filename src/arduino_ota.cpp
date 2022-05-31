#include "arduino_ota.h"

#include "global_instances.h"
#include "settings.h"
#include "task_queue.h"

void setupArduinoOTA() {
  ArduinoOTA.begin(false /* useMDNS */);
  ArduinoOTA.onStart([]() {
    disableFlashPageRotation();
    saveSettings();
  });
  TaskQueue.postRecurringTask([]() { ArduinoOTA.handle(); });
}