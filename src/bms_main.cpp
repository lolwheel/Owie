#include <Arduino.h>

#include "bms_relay.h"

// UART RX is connected to the BMS A line
// UART TX is connected to the MB A line
// TX_INPUT_PIN must be soldered to the UART RX
#define TX_INPUT_PIN 4
// Connected to the MB B line
#define TX_INVERSE_OUT_PIN 5

namespace {

// Emulate the RS485 B line by bitbanging the inverse
// of the A line.
void IRAM_ATTR txPinRiseInterrupt() { digitalWrite(TX_INVERSE_OUT_PIN, 0); }
void IRAM_ATTR txPinFallInterrupt() { digitalWrite(TX_INVERSE_OUT_PIN, 1); }
}  // namespace

BmsRelay relay(&Serial, &Serial);

void bms_setup() {
  Serial.begin(115200);

  // The B line idle is 0
  digitalWrite(TX_INVERSE_OUT_PIN, 0);
  pinMode(TX_INVERSE_OUT_PIN, OUTPUT);
  pinMode(TX_INPUT_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(TX_INPUT_PIN), txPinRiseInterrupt,
                  RISING);
  attachInterrupt(digitalPinToInterrupt(TX_INPUT_PIN), txPinFallInterrupt,
                  FALLING);
  relay.setByteReceivedCallback(
      [] { digitalWrite(LED_BUILTIN, 1 - digitalRead(LED_BUILTIN)); });
}

void bms_loop() { relay.loop(); }