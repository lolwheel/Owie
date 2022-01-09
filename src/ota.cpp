#include <AsyncElegantOTA.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define SSID_NAME "OWEnhancer-recovery"

namespace {
DNSServer dnsServer;
AsyncWebServer webServer(80);
}  // namespace

void ota_setup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID_NAME);
  dnsServer.start(53, "*", WiFi.softAPIP());  // DNS spoofing.
  webServer.onNotFound([&](AsyncWebServerRequest *request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/update");
  });
  AsyncElegantOTA.begin(&webServer);
  webServer.begin();
}

void ota_loop() { dnsServer.processNextRequest(); }