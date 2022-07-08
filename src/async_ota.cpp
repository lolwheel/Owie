#include "async_ota.h"

#include <Arduino.h>

#include "ESPAsyncWebServer.h"
#include "data.h"
#include "flash_hal.h"
#include "settings.h"

namespace {
class StringPrint : public Print {
 private:
  String s;

 public:
  String getString() const { return s; }
  virtual size_t write(uint8_t c) override {
    s.concat((char)c);
    return 1;
  }
};

String getUpdateError() {
  StringPrint p;
  Update.printError(p);
  return p.getString();
}

}  // namespace

void AsyncOtaClass::respondToOtaPostRequest(AsyncWebServerRequest *request) {
  // the request handler is triggered after the upload has finished
  int responseCode;
  const uint8_t *reponse;
  size_t responseLen;
  boolean error = Update.hasError();
  if (error) {
    responseCode = 500;
    reponse = this->updateFailedTemplate_;
    responseLen = this->updateFailedTemplateLen_;
  } else {
    responseCode = 200;
    reponse = this->updateSuccessfuleResponse_;
    responseLen = this->updateSuccessfuleResponseLen_;
  }

  AsyncWebServerResponse *response =
      request->beginResponse_P(responseCode, "text/html", reponse, responseLen,
                               [&](const String &varName) {
                                 if (varName == "UPDATE_ERROR") {
                                   return getUpdateError();
                                 }
                                 return String("wat");
                               });
  response->addHeader("Connection", "close");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
  if (!error) {
    this->endCallback_();
  }
}

void AsyncOtaClass::listen(AsyncWebServer *server) {
  server->on("/update", HTTP_GET, [&](AsyncWebServerRequest *request) {
    request->send(request->beginResponse_P(200, "text/html", this->landingPage_,
                                           this->landingPageLen_));
  });
  server->on(
      "/update", HTTP_POST,
      [&](AsyncWebServerRequest *request) {
        this->respondToOtaPostRequest(request);
      },
      [&](AsyncWebServerRequest *request, String filename, size_t index,
          uint8_t *data, size_t len, bool final) {
        // Upload handles chunks in data
        if (!index) {
          if (request->hasParam("MD5", true) &&
              !Update.setMD5(request->getParam("MD5", true)->value().c_str())) {
            return request->send(400, "text/plain", "MD5 parameter invalid");
          }

          Update.runAsync(true);
          uint32_t maxSketchSpace =
              (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
          if (!Update.begin(maxSketchSpace,
                            U_FLASH)) {  // Start with max available size
            respondToOtaPostRequest(request);
          }
          this->startCallback_();
        }

        // Write chunked data to the free sketch space
        if (len && Update.write(data, len) != len) {
          respondToOtaPostRequest(request);
        }

        if (final) {  // if the final flag is set then this is the last frame of
                      // data
          if (!Update.end(
                  true)) {  // true to set the size to the current progress
            respondToOtaPostRequest(request);
          }
        }
      });
}

AsyncOtaClass AsyncOta(
    UPDATE_HTML_PROGMEM_ARRAY, UPDATE_HTML_SIZE,
    UPDATE_SUCCESSFUL_RESPONSE_HTML_PROGMEM_ARRAY,
    UPDATE_SUCCESSFUL_RESPONSE_HTML_SIZE,
    UPDATE_FAILED_TEMPLATE_HTML_PROGMEM_ARRAY, UPDATE_FAILED_TEMPLATE_HTML_SIZE,
    []() {
      disableFlashPageRotation();
      saveSettings();
    },
    saveSettingsAndRestartSoon);
