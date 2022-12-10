#ifndef ASYNC_OTA_H
#define ASYNC_OTA_H
#include <functional>

// Based on AsyncElegantOTA.

class AsyncWebServer;
class AsyncWebServerRequest;

class AsyncOtaClass {
 private:
  const uint8_t* landingPage_ = nullptr;
  size_t landingPageLen_ = 0;
  const uint8_t* updateSuccessfuleResponse_ = nullptr;
  size_t updateSuccessfuleResponseLen_ = 0;
  const uint8_t* updateFailedTemplate_ = nullptr;
  size_t updateFailedTemplateLen_ = 0;

  std::function<void()> startCallback_;
  std::function<void()> endCallback_;

  void respondToOtaPostRequest(AsyncWebServerRequest* request);

 public:
  AsyncOtaClass(const uint8_t* landingPage, size_t landingPageLen,
                const uint8_t* updateSuccessfuleResponse,
                size_t updateSuccessfuleResponseLen,
                const uint8_t* updateFailedTemplate,
                size_t updateFailedTemplateLen,
                const std::function<void()>& startCallback,
                const std::function<void()>& endCallback)
      : landingPage_(landingPage),
        landingPageLen_(landingPageLen),
        updateSuccessfuleResponse_(updateSuccessfuleResponse),
        updateSuccessfuleResponseLen_(updateSuccessfuleResponseLen),
        updateFailedTemplate_(updateFailedTemplate),
        updateFailedTemplateLen_(updateFailedTemplateLen),
        startCallback_(startCallback),
        endCallback_(endCallback){};

  void listen(AsyncWebServer* server);
};

extern AsyncOtaClass AsyncOta;

#endif