#include <algorithm>
#include <cmath>
#include <cstdint>

#include "bms_relay.h"
#include "packet.h"

namespace {

const float CURRENT_SCALER = 0.055;

inline int16_t int16FromNetworkOrder(const void* const p) {
  uint8_t* const charPointer = (uint8_t* const)p;
  return ((uint16_t)(*charPointer)) << 8 | *(charPointer + 1);
}

int openCircuitSocFromVoltage(float voltageVolts) {
  // kindly provided by biell@ in https://github.com/lolwheel/Owie/issues/1
  return std::clamp((int)(99.9 / (0.8 + pow(1.24, (54 - voltageVolts))) - 10),
                    1, 100);
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
  bmsSocPercent_ = *(int8_t*)p.data();
  if (getTotalVoltageMillivolts() == 0) {
    // If we don't have voltage, swallow the packet
    p.setShouldForward(false);
    return;
  }
  // Assume that the first time we run the battery is very close to open
  // state and approximate the SOC just by voltage.
  if (!milliamp_seconds_till_empty_initialized_) {
    int8_t estimatedSoc =
        openCircuitSocFromVoltage(getTotalVoltageMillivolts() / 1000.0);
    milliamp_seconds_till_empty_ +=
        battery_capacity_override_ * estimatedSoc * 36.0;
    milliamp_seconds_till_empty_initialized_ = true;
  }
  p.data()[0] = std::max(5, (int)getOverriddenSoc());
  ;
}

void BmsRelay::currentParser(Packet& p) {
  if (p.getType() != 5) {
    return;
  }
  // 0x5 message encodes current as signed int16.
  // The scaling factor (tested on a Pint) seems to be 0.055
  // i.e. 1 in the data message below corresponds to 0.055 Amps.
  if (p.dataLength() != 2) {
    return;
  }
  current_ = int16FromNetworkOrder(p.data()) * CURRENT_SCALER;

  if (last_current_message_millis_ == 0) {
    last_current_ = current_;
    last_current_message_millis_ = millis_();
  }
  const unsigned long now = millis_();
  const unsigned long millisElapsed = now - last_current_message_millis_;
  last_current_message_millis_ = now;
  milliamp_seconds_till_empty_ -=
      (last_current_ + current_) / 2 * millisElapsed;
  p.setShouldForward(false);
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
  if (powerOffCallback_) {
    powerOffCallback_();
  }
}

void BmsRelay::chargingStatusParser(Packet& p) {
  if (p.getType() != 0) {
    return;
  }
  int8_t status = p.data()[0];
  // This one bit seems to indicate charging, block every other message.
  // Otherwise my Pint 5059 throws Error 23.
  if ((status & 0x20) == 0) {
    p.setShouldForward(false);
  }
  // Interesting observation is that if I swallow chargin packets too, the board
  // assumes riding state even with the charger plugged in. This is potentially
  // enabling Charge-n-Ride setups though I haven't tested this(and have no
  // plans to). It's likely that the BMS will shut down when regen
  // current exceeds the charging current limit, at least on
  // Pints. This will be a very interesting experiment to run on XRs where the
  // BMS charging port isn't utilised and the charging current probably goes
  // through the main HV bus from the controller board.
}
