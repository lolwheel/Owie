#include <Arduino.h>

#include "ESP8266WiFi.h"
#include "bms_main.h"
#include "dprint.h"
#include "ota.h"
#include "recovery.h"
#include "task_queue.h"

int isRecovery = false;

extern "C" void setup() {
  WiFi.persistent(false);
  WiFi.setOutputPower(0);
  isRecovery = isInRecoveryMode();
  if (isRecovery) {
    ota_setup();
  } else {
    bms_setup();
  }
}

extern "C" void loop() {
  Serial.getRxBufferSize();
  if (isRecovery) {
    ota_loop();
  } else {
    bms_loop();
  }
  TaskQueue.process();
}