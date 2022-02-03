#include <Arduino.h>

#include <algorithm>
#include <cmath>

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

int openCircuitSocFromVoltage(float voltageVolts) {
  // kindly provided by biell@ in https://github.com/lolwheel/Owie/issues/1
  return std::clamp((int)(99.9 / (0.8 + pow(1.24, (54 - voltageVolts))) - 10),
                    1, 100);
}

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
  relay.setCurrentRewriterCallback([](float amps, bool* shouldForward) {
    static unsigned long lastCurrentMessageMillis = 0;
    static float lastCurrent = 0;
    if (lastCurrentMessageMillis == 0) {
      lastCurrent = amps;
      lastCurrentMessageMillis = millis();
      return 0;
    }
    const unsigned long now = millis();
    const unsigned long millisElapsed = now - lastCurrentMessageMillis;
    lastCurrentMessageMillis = now;
    Settings.milliampseconds_till_empty -=
        (lastCurrent + amps) / 2 * millisElapsed;
    if (Settings.milliampseconds_till_empty <= 0) {
      Settings.milliampseconds_till_empty = 1;
    } else {
      Settings.real_board_capacity_mah =
          max(Settings.real_board_capacity_mah,
              (int)(Settings.milliampseconds_till_empty / 3600));
    }
    *shouldForward = false;
    return 0;
  });
  relay.setSocRewriterCallback([&](int8_t bmsSoc, bool* shouldForward) {
    if (relay.getTotalVoltageMillivolts() == 0) {
      return 99;
    }
    // Assume that the first time we boot the battery is very close to open
    // state and approximate the SOC just by voltage.
    if (Settings.milliampseconds_till_empty == 0) {
      if (relay.getTotalVoltageMillivolts() == 0) {
        return 99;
      }
      int8_t estimatedSoc =
          openCircuitSocFromVoltage(relay.getTotalVoltageMillivolts() / 1000.0);
      Settings.milliampseconds_till_empty =
          Settings.real_board_capacity_mah * estimatedSoc * 36.0;
    }
    return max(5, (int)(Settings.milliampseconds_till_empty / 36.0 /
                        Settings.real_board_capacity_mah));
  });
  relay.setPowerOffCallback([]() {
    Settings.graceful_shutdown_count++;
    saveSettings();
  });
  // An example serial override which defeats BMS pairing:
  // relay.setBMSSerialOverride(123456);
  setupWifi();
  setupWebServer(&relay);
  setupArduinoOTA();
  TaskQueue.postRecurringTask([&]() { relay.loop(); });
}
