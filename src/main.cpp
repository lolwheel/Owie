#include <Arduino.h>

#include "bms_mirror.h"
#include "dprint.h"
#include "ota.h"
#include "recovery.h"
#include "task_queue.h"

int isRecovery = false;

extern "C" void setup() {
#ifdef DEBUG_ESP_PORT
  DEBUG_ESP_PORT.begin(2000000);
#endif
  isRecovery = isInRecoveryMode();
  if (isRecovery) {
    ota_setup();
  } else {
    bms_setup();
  }
}

extern "C" void loop() {
  if (isRecovery) {
    ota_loop();
  } else {
    bms_loop();
  }
  TaskQueue.process();
}