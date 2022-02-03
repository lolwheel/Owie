#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>

#include <cstdint>

class BmsRelay;

void setupWifi();
void setupWebServer(BmsRelay* bmsRelay);
void streamBMSPacket(uint8_t* const buffer, size_t len);

#endif  // NETWORK_H