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
    // Reset in 5 seconds;
    TaskQueue.postOneShotTask(resetQuickPowerCycleCount, 5000L);
  }
  saveSettings();
  DPRINTF("Wrote QPC = %d\n", Settings->quick_power_cycle_count);
  return (!Settings->is_locked) && recovery;
}

/**
 * @brief After running this function, Settings->board_locked is an
 * authoritative source of whether or not the board is locked.
 */
void maybeLockOnStartup() {
  // Make sure we unlock the board and disable locking if the AP password was
  // reset.
  if (strlen(Settings->ap_self_password) < 8) {
    Settings->is_locked = false;
    Settings->locking_enabled = false;
    return;
  }
  if (Settings->is_locked || !Settings->locking_enabled) {
    return;
  }
  if (Settings->quick_power_cycle_count > 0) {
    Settings->is_locked = true;
  }
}

extern "C" void setup() {
  WiFi.persistent(false);
  loadSettings();
  // It is important to do this *BEFORE* calling isInRecoveryMode()
  maybeLockOnStartup();

  if (isInRecoveryMode()) {
    recovery_setup();
  } else {
    bms_setup();
  }
}

extern "C" void loop() { TaskQueue.process(); }