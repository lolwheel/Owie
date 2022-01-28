#include <cstdint>

#include "bms_relay.h"
#include "packet.h"

namespace {
inline int16_t int16FromNetworkOrder(const void* const p) {
  uint8_t* const charPointer = (uint8_t* const)p;
  return ((uint16_t)(*charPointer)) << 8 | *(charPointer + 1);
}
}  // namespace

void BmsRelay::batteryPercentageParser(Packet& p) {
  if (p.getType() != 3) {
    return;
  }
  // 0x3 message is just one byte containing battery percentage.
  if (p.dataLength() != 1) {
    return;
  }
  battery_percentage_ = *(int8_t*)p.data();
}

void BmsRelay::currentParser(Packet& p) {
  if (p.getType() != 5) {
    return;
  }
  // 0x5 message encodes current as signed int16.
  // The scaling factor (tested on a Pint) seems to be 0.05
  // i.e. 1 in the data message below corresponds to 0.05 Amps.
  if (p.dataLength() != 2) {
    return;
  }
  int16_t current = int16FromNetworkOrder(p.data());
  current_ = current * 0.05;
}

void BmsRelay::bmsSerialParser(Packet& p) {
  if (p.getType() != 6) {
    return;
  }
  // 0x6 message has the BMS serial number encoded inside of it.
  // It is the last seven digits from the sticker on the back of the BMS.
  if (p.dataLength() != 4) {
    return;
  }
  if (captured_serial_ == 0) {
    for (int i = 0; i < 4; i++) {
      captured_serial_ |= p.data()[i] << (8 * (3 - i));
    }
  }
  if (serial_override_ == 0) {
    return;
  }
  uint32_t serial_override_copy = serial_override_;
  for (int i = 3; i >= 0; i--) {
    p.data()[i] = serial_override_copy & 0xFF;
    serial_override_copy >>= 8;
  }
}

void BmsRelay::cellVoltageParser(Packet& p) {
  if (p.getType() != 2) {
    return;
  }
  // The data in this packet is 16 int16-s. First 15 of them is
  // individual cell voltages in millivolts. The last value is mysterious.
  if (p.dataLength() != 32) {
    return;
  }
  const uint8_t* const data = p.data();
  uint16_t total_voltage = 0;
  for (int i = 0; i < 15; i++) {
    uint16_t cellVoltage = int16FromNetworkOrder(data + (i << 1));
    total_voltage += cellVoltage;
    cell_millivolts_[i] = cellVoltage;
  }
  total_voltage_millivolts_ = total_voltage;
}

void BmsRelay::temperatureParser(Packet& p) {
  if (p.getType() != 4) {
    return;
  }
  // The data seems to encode 5 uint8_t temperature readings in celsius
  if (p.dataLength() != 5) {
    return;
  }
  int8_t* const temperatures = (int8_t*)p.data();
  int16_t temperature_sum = 0;
  for (int i = 0; i < 5; i++) {
    int8_t temp = temperatures[i];
    temperature_sum += temp;
    temperatures_celsius_[i] = temp;
  }
  avg_temperature_celsius_ = temperature_sum / 5;
}

void BmsRelay::powerOffParser(Packet& p) {
  if (p.getType() != 11) {
    return;
  }
  // We're about to shut down.
}