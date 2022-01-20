#include <Arduino.h>

#include "ESP8266WiFi.h"
#include "bms_main.h"
#include "dprint.h"
#include "ota.h"
#include "recovery.h"
#include "task_queue.h"

extern "C" void setup() {
  WiFi.persistent(false);
  WiFi.setOutputPower(0);
  if (isInRecoveryMode()) {
    ota_setup();
  } else {
    bms_setup();
  }
}

extern "C" void loop() { TaskQueue.process(); }