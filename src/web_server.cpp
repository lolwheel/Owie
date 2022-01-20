#include "web_server.h"

#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

#include "data.h"
#include "task_queue.h"

namespace {
DNSServer dnsServer;
AsyncWebServer webServer(80);
AsyncWebSocket ws("/rawdata");
}  // namespace

void setupWebServer() {
  WiFi.mode(WIFI_AP);
  char apName[64];
  sprintf(apName, "Owie-%04X", ESP.getChipId() & 0xFFFF);
  WiFi.softAP(apName);
  webServer.addHandler(&ws);
  dnsServer.start(53, "*", WiFi.softAPIP());  // DNS spoofing.
  webServer.onNotFound([&](AsyncWebServerRequest *request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", INDEX_HTML, sizeof(INDEX_HTML),
                    [](const String &var) {
                      if (var == "OWIE_version") {
                        return "0.0.1";
                      }
                      return "UNKNOWN";
                    });
  });
  webServer.begin();
  TaskQueue.postRecurringTask([&]() { dnsServer.processNextRequest(); });
}

void streamBMSPacket(const char* buffer, size_t len) {
    ws.binaryAll(buffer, len);
}