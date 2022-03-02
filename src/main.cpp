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

bool isLocked() {
  // no password set or board isn't armed, won't lock
  if (strcmp(Settings->ap_self_password, "") == 0 ||
      !Settings->board_lock_armed) {
    // making sure the board can't go into a locked state
    Settings->board_locked = false;
    return false;
  }
  if (Settings->quick_power_cycle_count > 0) {
    Settings->quick_power_cycle_count = 0;
    Settings->board_locked = true;
  } else {
    Settings->quick_power_cycle_count++;
    // Reset in 10 seconds;
    TaskQueue.postOneShotTask(resetQuickPowerCycleCount, 10000L);
  }
  saveSettings();
  return Settings->board_locked;
}

extern "C" void setup() {
  WiFi.persistent(false);
  loadSettings();

  isLocked();
  bms_setup();
}

extern "C" void loop() { TaskQueue.process(); }