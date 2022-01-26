#ifndef PACKET_PARSERS_H
#define PACKET_PARSERS_H

#include "bms_relay.h"

void bmsSerialParser(BmsRelay* relay, Packet* p);
void currentParser(BmsRelay* relay, Packet* p);
void batteryPercentageParser(BmsRelay* relay, Packet* p);
void batteryVoltageParser(BmsRelay* relay, Packet* p);
void batteryTemperature(BmsRelay* relay, Packet* p);

#endif  // PACKET_PARSERS_H