#include <Arduino.h>

#include "arduino_ota.h"
#include "bms_relay.h"
#include "charging_tracker.h"
#include "global_instances.h"
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

BmsRelay *relay;
ChargingTracker *chargingTracker;

void maybeSaveCharingDataToSettings(ChargingTracker *) {
  static_assert(sizeof(ChargingDataMsg::voltage_offsets) ==
                sizeof(ChargingDataMsg::mah_offsets));
  const auto &chargingPoints = chargingTracker->getChargingPoints();

  if (chargingPoints.size() < 2 ||
      (Settings->has_charging_data &&
       Settings->charging_data.voltage_offsets_count > chargingPoints.size())) {
    return;
  }
  ChargingDataMsg &proto = Settings->charging_data;
  Settings->has_charging_data = true;
  const uint32_t numPoints = min<uint32_t>(
      (sizeof(proto.voltage_offsets) / sizeof(*proto.voltage_offsets)),
      chargingPoints.size());
  proto.mah_offsets_count = proto.voltage_offsets_count = numPoints;
  proto.mah_offsets[0] = chargingPoints[0].totalMah;
  proto.voltage_offsets[0] = chargingPoints[0].millivolts;
  for (unsigned int i = 1; i < numPoints; i++) {
    proto.mah_offsets[i] =
        chargingPoints[i].totalMah - chargingPoints[i - 1].totalMah;
    proto.voltage_offsets[i] =
        chargingPoints[i].millivolts - chargingPoints[i - 1].millivolts;
  }
  proto.tracked_cell_index = chargingTracker->getTrackedCellIndex();
  saveSettings();
}

void bms_setup() {
  relay = new BmsRelay([]() { return Serial.read(); },
                       [](uint8_t b) {
                         // This if statement is what implements locking.
                         if (!Settings->is_locked) {
                           Serial.write(b);
                         }
                       },
                       millis);
  chargingTracker = new ChargingTracker(
      relay, 50, /** make new datapoint after every 50 mah of charge*/
      maybeSaveCharingDataToSettings);
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

  relay->addReceivedPacketCallback([](BmsRelay *, Packet *packet) {
    static uint8_t ledState = 0;
    digitalWrite(LED_BUILTIN, ledState);
    ledState = 1 - ledState;
    streamBMSPacket(packet->start(), packet->len());
  });
  relay->setUnknownDataCallback([](uint8_t b) {
    static std::vector<uint8_t> unknownData = {0};
    if (unknownData.size() > 128) {
      return;
    }
    unknownData.push_back(b);
    streamBMSPacket(&unknownData[0], unknownData.size());
  });

  relay->setPowerOffCallback([]() {
    Settings->graceful_shutdown_count++;
    saveSettings();
  });

  if (Settings->bms_serial != 0) {
    relay->setBMSSerialOverride(Settings->bms_serial);
  }

  setupWifi();
  setupWebServer(relay);
  setupArduinoOTA();
  TaskQueue.postRecurringTask([]() { relay->loop(); });
}
