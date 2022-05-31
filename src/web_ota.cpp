#include "web_ota.h"

#include <cstddef>

#include "global_instances.h"
#include "AsyncElegantOTA.h"

void WebOta::begin(AsyncWebServer* server) { AsyncElegantOTA.begin(server); }
