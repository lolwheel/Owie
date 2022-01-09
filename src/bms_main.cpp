#include <Arduino.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

#include "bms_relay.h"
#include "data.h"

// UART RX is connected to the *BMS* White line
// UART TX is connected to the *MB* White line
// TX_INPUT_PIN must be soldered to the UART TX
#define TX_INPUT_PIN 4
// Connected to the MB B line
#define TX_INVERSE_OUT_PIN 5

namespace {

DNSServer dnsServer;
AsyncWebServer webServer(80);

// Emulate the RS485 B line by bitbanging the inverse
// of the TX A line.
void IRAM_ATTR txPinRiseInterrupt() { digitalWrite(TX_INVERSE_OUT_PIN, 0); }
void IRAM_ATTR txPinFallInterrupt() { digitalWrite(TX_INVERSE_OUT_PIN, 1); }
}  // namespace

BmsRelay relay([]() { return Serial.read(); },
               [](uint8_t b) { return Serial.write(b); });

void setupWebServer() {
  WiFi.mode(WIFI_AP);
  char apName[64];
  sprintf(apName, "OWEnhancer-%04X", ESP.getChipId() & 0xFFFF);
  WiFi.softAP(apName);
  dnsServer.start(53, "*", WiFi.softAPIP());  // DNS spoofing.
  webServer.onNotFound([&](AsyncWebServerRequest *request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", INDEX_HTML, sizeof(INDEX_HTML),
                    [](const String &var) {
                      if (var == "OWE_version") {
                        return "0.0.1";
                      }
                      return "UNKNOWN";
                    });
  });
  webServer.begin();
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
  relay.setByteReceivedCallback(
      [] { digitalWrite(LED_BUILTIN, 1 - digitalRead(LED_BUILTIN)); });
  // An example serial override which defeats BMS pairing:
  // relay.setBMSSerialOverride(123456);
  setupWebServer();
}

void bms_loop() {
  relay.loop();
  dnsServer.processNextRequest();
}
