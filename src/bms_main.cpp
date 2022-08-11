#include <Arduino.h>

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

HardwareSerial Serial(0);
}  // namespace

BmsRelay *relay;

void bms_setup() {

//If monitoring-only is enabled the Source function reads from serial and writes the byte straight out before returning it for OWIE to process it.
//The Sink source does nothing since data has already been written.
if (Settings->monitoring_only_enabled) {
  relay = new BmsRelay([]() { 
                            int byte = Serial.read(); 
                            Serial.write(byte);
                            return byte;
                          },
                      [](uint8_t b) {
                        //DO NOTHING, WE DO NOT WANT TO CHANGE THE BMS DATA FOR PINTX.
                      },
                      millis);
} else {
  relay = new BmsRelay([]() { return Serial.read(); },
                       [](uint8_t b) {
                         // This if statement is what implements locking.
                         if (!Settings->is_locked) {
                           Serial.write(b);
                         }
                       },
                       millis);
}
  
 
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

  relay->setBMSSerialOverride(0xFFABCDEF);

  setupWifi();
  setupWebServer(relay);
  TaskQueue.postRecurringTask([]() { relay->loop(); });
}
