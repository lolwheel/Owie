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

bool isLocked() {
  bool locked = Settings->board_locked;
  if (Settings->quick_power_cycle_count > 0) {
    locked = true;
    Settings->quick_power_cycle_count = 0;
    Settings->board_locked = true;
    Settings->bms_serial = 5; // this will cause an error 16
  } else {
    // this should clearly put it into the locked state
    Settings->quick_power_cycle_count = 3;
    // Reset in 10 seconds;
    TaskQueue.postOneShotTask(resetQuickPowerCycleCount, 10000L);
  }
  saveSettings();
  return locked;
}

extern "C" void setup() {
  WiFi.persistent(false);
  loadSettings();

  isLocked();
  bms_setup();
}

extern "C" void loop() { TaskQueue.process(); }