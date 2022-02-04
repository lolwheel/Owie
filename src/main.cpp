#include <Arduino.h>

#include "ESP8266WiFi.h"
#include "bms_main.h"
#include "dprint.h"
#include "recovery.h"
#include "settings.h"
#include "task_queue.h"

void resetQuickPowerCycleCount() {
  Settings->quick_power_cycle_count = 0;
  saveSettings();
  DPRINTF("Wrote QPC = %d\n", Settings->quick_power_cycle_count);
}

bool isInRecoveryMode() {
  DPRINTF("Read QPC = %d\n", Settings->quick_power_cycle_count);
  bool recovery = false;
  if (Settings->quick_power_cycle_count > 1) {
    recovery = true;
    Settings->quick_power_cycle_count = 0;
  } else {
    Settings->quick_power_cycle_count++;
    // Reset in 10 seconds;
    TaskQueue.postOneShotTask(resetQuickPowerCycleCount, 10000L);
  }
  saveSettings();
  DPRINTF("Wrote QPC = %d\n", Settings->quick_power_cycle_count);
  return recovery;
}

extern "C" void setup() {
  WiFi.persistent(false);
  loadSettings();

  if (isInRecoveryMode()) {
    recovery_setup();
  } else {
    bms_setup();
  }
}

extern "C" void loop() { TaskQueue.process(); }