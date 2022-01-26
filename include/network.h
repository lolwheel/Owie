#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>

void setupWifi();
void setupWebServer();
void streamBMSPacket(const char* buffer, size_t len);

#endif  // NETWORK_H