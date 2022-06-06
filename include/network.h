#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>
#include <functional>

#include <cstdint>

class BmsRelay;

void setupWifi();
void setupWebServer(BmsRelay* bmsRelay, const std::function<void()>& syncChargingDataToSettingsCallback);
void streamBMSPacket(uint8_t* const buffer, size_t len);

#endif  // NETWORK_H