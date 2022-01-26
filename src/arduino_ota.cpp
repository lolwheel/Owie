#include "arduino_ota.h"

#include <ArduinoOTA.h>

#include "settings.h"
#include "task_queue.h"

void setupArduinoOTA() {
  ArduinoOTA.setRebootOnSuccess(false);
  ArduinoOTA.begin(false /* useMDNS */);
  ArduinoOTA.onEnd([]() {
    disableFlashPageRotation();
    saveSettingsAndRestartSoon();
  });
  TaskQueue.postRecurringTask([]() { ArduinoOTA.handle(); });
}