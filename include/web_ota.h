#ifndef WEB_OTA_H
#define WEB_OTA_H

class AsyncWebServer;

class WebOta {
 public:
  static void begin(AsyncWebServer* server);
};

#endif