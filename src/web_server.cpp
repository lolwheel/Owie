#include "web_server.h"

#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>

#include "data.h"
#include "task_queue.h"

#define WIFI_NAME "<scrubbed>"
#define WIFI_PASS "<scrubbed>"

namespace {
DNSServer dnsServer;
AsyncWebServer webServer(80);
AsyncWebSocket ws("/rawdata");
}  // namespace

void setupWebServer() {
  WiFi.mode(WIFI_AP_STA);
  char apName[64];
  sprintf(apName, "Owie-%04X", ESP.getChipId() & 0xFFFF);
  WiFi.softAP(apName);
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  WiFi.hostname(apName);
  MDNS.begin("owie");
  webServer.addHandler(&ws);
  dnsServer.start(53, "*", WiFi.softAPIP());  // DNS spoofing.
  webServer.onNotFound([&](AsyncWebServerRequest *request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", TEMPLATE_HTML_PROGMEM_ARRAY,
                    TEMPLATE_HTML_SIZE, [](const String &var) -> String {
                      if (var == "OWIE_version") {
                        return "0.0.1";
                      } else if (var == "CONTENT") {
                        return INDEX_HTML;
                      }
                      return "UNKNOWN";
                    });
  });
  webServer.begin();
  TaskQueue.postRecurringTask([&]() {
    dnsServer.processNextRequest();
    MDNS.update();
  });
}

void streamBMSPacket(const char *buffer, size_t len) {
  ws.binaryAll(buffer, len);
}