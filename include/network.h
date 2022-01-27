#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>

#include <cstdint>

void setupWifi();
void setupWebServer();
void streamBMSPacket(uint8_t* const buffer, size_t len);

#endif  // NETWORK_H