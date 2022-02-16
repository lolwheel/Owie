#include "web_ota.h"

#include <cstddef>

#include "AsyncElegantOTA.h"

void WebOta::begin(AsyncWebServer* server) { AsyncElegantOTA.begin(server); }
