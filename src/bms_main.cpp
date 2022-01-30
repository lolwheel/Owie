#include <Arduino.h>

#include "arduino_ota.h"
#include "bms_relay.h"
#include "network.h"
#include "packet.h"
#include "settings.h"
#include "task_queue.h"

// UART RX is connected to the *BMS* White line
// UART TX is connected to the *MB* White line
// TX_INPUT_PIN must be soldered to the UART TX
#define TX_INPUT_PIN 4
// Connected to the MB B line
#define TX_INVERSE_OUT_PIN 5

namespace {

// Emulate the RS485 B line by bitbanging the inverse
// of the TX A line.
void IRAM_ATTR txPinRiseInterrupt() { digitalWrite(TX_INVERSE_OUT_PIN, 0); }
void IRAM_ATTR txPinFallInterrupt() { digitalWrite(TX_INVERSE_OUT_PIN, 1); }
}  // namespace

BmsRelay relay([]() { return Serial.read(); },
               [](uint8_t b) { Serial.write(b); });

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
  relay.addPacketCallback([](BmsRelay*, Packet* packet) {
    static uint8_t ledState = 0;
    digitalWrite(LED_BUILTIN, ledState);
    ledState = 1 - ledState;
    streamBMSPacket(packet->start(), packet->len());
  });
  relay.setUnknownDataCallback([](uint8_t b) {
    static std::vector<uint8_t> unknownData = {0};
    if (unknownData.size() > 128) {
      return;
    }
    unknownData.push_back(b);
    streamBMSPacket(&unknownData[0], unknownData.size());
  });
  relay.setPowerOffCallback([]() { saveSettings(); });
  // Returning flat out -1 amps throws incompatible error 23 in a couple of
  // minutes on Pint 5314/5050
  //
  // TODO(everyone): Experiment heavily on what is the smallest value accepted
  // by different boards without throwing the error.
  relay.setCurrentRewriterCallback([](float amps) { return amps * 0.5; });
  relay.setPowerOffCallback([]() {
    Settings.graceful_shutdown_count++;
    saveSettings();
  });
  // An example serial override which defeats BMS pairing:
  // relay.setBMSSerialOverride(123456);
  setupWifi();
  setupWebServer();
  setupArduinoOTA();
  TaskQueue.postRecurringTask([&]() { relay.loop(); });
}
