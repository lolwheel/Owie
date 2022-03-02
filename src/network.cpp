#include "network.h"

#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>

#include <cstring>

#include "bms_relay.h"
#include "data.h"
#include "settings.h"
#include "task_queue.h"
#include "web_ota.h"

namespace {
DNSServer dnsServer;
AsyncWebServer webServer(80);
AsyncWebSocket ws("/rawdata");

const String defaultPass("****");
BmsRelay *relay;

inline String uptimeString() {
  const unsigned long nowSecs = millis() / 1000;
  const int hrs = nowSecs / 3600;
  const int mins = (nowSecs % 3600) / 60;
  const int secs = nowSecs % 60;
  String ret;
  if (hrs) {
    ret.concat(hrs);
    ret.concat('h');
  }
  ret.concat(mins);
  ret.concat('m');
  ret.concat(secs);
  ret.concat('s');
  return ret;
}

String templateProcessor(const String &var) {
  if (var == "TOTAL_VOLTAGE") {
    return String(relay->getTotalVoltageMillivolts() / 1000.0,
                  /* decimalPlaces = */ 2);
  } else if (var == "CURRENT_AMPS") {
    return String(relay->getCurrentInAmps(),
                  /* decimalPlaces = */ 1);
  } else if (var == "BMS_SOC") {
    return String(relay->getBmsReportedSOC());
  } else if (var == "OVERRIDDEN_SOC") {
    return String(relay->getOverriddenSOC());
  } else if (var == "USED_CHARGE_MAH") {
    return String(relay->getUsedChargeMah());
  } else if (var == "REGENERATED_CHARGE_MAH") {
    return String(relay->getRegeneratedChargeMah());
  } else if (var == "OWIE_version") {
    return "0.0.1";
  } else if (var == "SSID") {
    return Settings->ap_name;
  } else if (var == "PASS") {
    if (strlen(Settings->ap_password) > 0) {
      return defaultPass;
    }
    return "";
  } else if (var == "GRACEFUL_SHUTDOWN_COUNT") {
    return String(Settings->graceful_shutdown_count);
  } else if (var == "UPTIME") {
    return uptimeString();
  } else if (var == "LOCK_STATUS") {
    return Settings->board_locked ? "locked" : "unlocked";
  } else if (var == "BOARD_LOCK_ARMED") {
    return Settings->board_lock_armed ? "arm" : "disarm";
  } else if (var == "BOARD_LOCK_ARMED_INV") {
    return Settings->board_lock_armed ? "Disarm" : "Arm";
  } else if (var == "CELL_VOLTAGE_TABLE") {
    const uint16_t *cellMillivolts = relay->getCellMillivolts();
    String out;
    out.reserve(256);
    for (int i = 0; i < 3; i++) {
      out.concat("<tr>");
      for (int j = 0; j < 5; j++) {
        out.concat("<td>");
        out.concat(cellMillivolts[i * 5 + j] / 1000.0);
        out.concat("</td>");
      }
      out.concat("<tr>");
    }
    return out;
  } else if (var == "BMS_SERIAL_OVERRIDE") {
    // so it doesn't return 0 when no override is set
    if (Settings->bms_serial == 0) {
      return "";
    } else {
      return String(Settings->bms_serial);
    }
  } else if (var == "AP_PASSWORD") {
    return Settings->ap_self_password;
  }
  return "<script>alert('UNKNOWN PLACEHOLDER')</script>";
}

} // namespace

void setupWifi() {
  WiFi.setOutputPower(9);
  bool stationMode = (strlen(Settings->ap_name) > 0);
  WiFi.mode(stationMode ? WIFI_AP_STA : WIFI_AP);
  char apName[64];
  // sprintf isn't causing the issue of bungled SSID anymore (can't reproduce)
  // but snprintf should be safer, so trying that now
  // 9 bytes should be sufficient
  snprintf(apName, sizeof(apName), "Owie-%04X", ESP.getChipId() & 0xFFFF);
  WiFi.softAP(apName, Settings->ap_self_password);
  if (stationMode) {
    WiFi.begin(Settings->ap_name, Settings->ap_password);
    WiFi.hostname(apName);
  }
  MDNS.begin("owie");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP()); // DNS spoofing.
  TaskQueue.postRecurringTask([]() {
    dnsServer.processNextRequest();
    MDNS.update();
  });
}

void setupWebServer(BmsRelay *bmsRelay) {
  relay = bmsRelay;
  WebOta::begin(&webServer);
  webServer.addHandler(&ws);
  webServer.onNotFound([](AsyncWebServerRequest *request) {
    if (request->host().indexOf("owie.local") >= 0) {
      request->send(404);
      return;
    }
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", INDEX_HTML_PROGMEM_ARRAY, INDEX_HTML_SIZE,
                    templateProcessor);
  });
  webServer.on("/wifi", HTTP_ANY, [](AsyncWebServerRequest *request) {
    switch (request->method()) {
      case HTTP_GET:
        request->send_P(200, "text/html", WIFI_HTML_PROGMEM_ARRAY,
                        WIFI_HTML_SIZE, templateProcessor);
        return;
      case HTTP_POST:
        const auto ssidParam = request->getParam("s", true);
        const auto passwordParam = request->getParam("p", true);
        if (ssidParam == nullptr || passwordParam == nullptr ||
            ssidParam->value().length() > sizeof(Settings->ap_name) ||
            passwordParam->value().length() > sizeof(Settings->ap_password)) {
          request->send(200, "text/html", "Invalid SSID or Password.");
          return;
        }
        std::strncpy(Settings->ap_name, ssidParam->value().c_str(),
                     sizeof(Settings->ap_name));
        std::strncpy(Settings->ap_password, passwordParam->value().c_str(),
                     sizeof(Settings->ap_password));
        saveSettingsAndRestartSoon();
        request->send(200, "text/html", "WiFi settings saved, restarting...");
        return;
    }
    request->send(404);
  });
  webServer.on("/monitor", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", MONITOR_HTML_PROGMEM_ARRAY,
                    MONITOR_HTML_SIZE, templateProcessor);
  });
  webServer.on("/settings", HTTP_ANY, [](AsyncWebServerRequest *request) {
    switch (request->method()) {
    case HTTP_GET:
      request->send_P(200, "text/html", SETTINGS_HTML_PROGMEM_ARRAY,
                      SETTINGS_HTML_SIZE, templateProcessor);
      return;
    case HTTP_POST:
      const auto bmsSerialParam = request->getParam("bs", true);
      const auto apSelfPassword = request->getParam("pw", true);
      if (bmsSerialParam == nullptr) {
        request->send(400, "text/html", "Invalid BMS Serial number.");
        return;
      }
      if (apSelfPassword == nullptr ||
          apSelfPassword->value().length() >
              sizeof(Settings->ap_self_password) ||
          (apSelfPassword->value().length() < 8 &&
           apSelfPassword->value().length() >
               0)) { // this check is necessary so the user can't set a too
                     // small password and thus the network wont' show up
        request->send(400, "text/html", "Invalid AP password.");
        return;
      }
      // allows user to leave bms serial field blank instead of having to put 0
      if (bmsSerialParam->value().length() == 0) {
        Settings->bms_serial = 0;
      } else {
        Settings->bms_serial = bmsSerialParam->value().toInt();
      }
      std::strncpy(Settings->ap_self_password, apSelfPassword->value().c_str(),
                   sizeof(Settings->ap_self_password));
      saveSettingsAndRestartSoon();
      request->send(200, "text/html", "Settings saved, restarting...");
      return;
    }
    request->send(404);
  });
  webServer.on("/lock", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("unlock")) {
      Settings->board_locked = false; // unlock the board
      saveSettingsAndRestartSoon();
      request->send(
          200, "text/html",
          "Board unlocked, restarting Owie. Please wait a few seconds "
          "and reboot your board.");
      return;
    } else if (request->hasParam("toggleArm")) {
      Settings->board_lock_armed = !Settings->board_lock_armed;
      Settings->board_locked = false; // unlock the board
      String retval = Settings->board_lock_armed ? "Disarm" : "Arm";
      saveSettingsAndRestartSoon();
      request->send(200, "text/html", retval);
      return;
    }
    request->send(404);
  });

  webServer.begin();
}

void streamBMSPacket(uint8_t *const data, size_t len) {
  ws.binaryAll((char *const)data, len);
}