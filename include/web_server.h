#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stddef.h>

void setupWebServer();
void streamBMSPacket(const char* buffer, size_t len);

#endif  // WEB_SERVER_H