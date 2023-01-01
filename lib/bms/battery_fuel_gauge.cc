#include "battery_fuel_gauge.h"

#include <cmath>

#include "defer.h"

namespace {

template <class T>
inline T clamp(T x, T lower, T upper) {
  return std::min(upper, std::max(x, lower));
}

int openCircuitSocFromCellVoltage(int cellVoltageMillivolts) {
  static constexpr int LOOKUP_TABLE_RANGE_MIN_MV = 2700;
  static constexpr int LOOKUP_TABLE_RANGE_MAX_MV = 4200;
  static uint8_t LOOKUP_TABLE[31] = {0,  0,  0,  0,  1,  2,  3,  4,  5,  7,  8,
                                     11, 14, 16, 18, 19, 25, 30, 33, 37, 43, 48,
                                     53, 60, 67, 71, 76, 82, 92, 97, 100};
  static constexpr int LOOKUP_TABLE_SIZE =
      (sizeof(LOOKUP_TABLE) / sizeof(*LOOKUP_TABLE));
  static constexpr int RANGE =
      LOOKUP_TABLE_RANGE_MAX_MV - LOOKUP_TABLE_RANGE_MIN_MV;
  // (RANGE - 1) upper limit effectively clamps the leftIndex below to
  // (LOOKUP_TABLE_SIZE - 2)
  cellVoltageMillivolts =
      clamp(cellVoltageMillivolts - LOOKUP_TABLE_RANGE_MIN_MV, 0, RANGE - 1);
  float floatIndex =
      float(cellVoltageMillivolts) * (LOOKUP_TABLE_SIZE - 1) / RANGE;
  const int leftIndex = int(floatIndex);
  const float fractional = floatIndex - leftIndex;
  const int rightIndex = leftIndex + 1;
  const int leftValue = LOOKUP_TABLE[leftIndex];
  const int rightValue = LOOKUP_TABLE[rightIndex];
  return clamp<int>(leftValue + round((rightValue - leftValue) * fractional), 0,
                    100);
}

}  // namespace

void BatteryFuelGauge::updateVoltage(int32_t voltageMillivolts,
                                     int32_t nowMillis) {
  voltage_millivolts_ = voltageMillivolts;
  filtered_voltage_millivolts_.step(voltageMillivolts);
  if (state_.currentMilliampSeconds == state_.bottomMilliampSeconds) {
    state_.bottomSoc =
        openCircuitSocFromCellVoltage(filtered_voltage_millivolts_.get());
  }
  if (state_.currentMilliampSeconds == 0) {
    state_.topSoc =
        openCircuitSocFromCellVoltage(filtered_voltage_millivolts_.get());
  }
}

void BatteryFuelGauge::updateCurrent(int32_t currentMilliamps,
                                     int32_t nowMillis) {
  defer { last_current_update_time_millis_ = nowMillis; };
  if (last_current_update_time_millis_ < 0) {
    return;
  }
  const int32_t millisSinceLastUpdate =
      nowMillis - last_current_update_time_millis_;
  const int32_t milliampSecondsDelta =
      millisSinceLastUpdate * currentMilliamps / 1000;
  if (milliampSecondsDelta > 0) {
    milliamp_seconds_discharged_ += milliampSecondsDelta;
  } else {
    milliamp_seconds_recharged_ -= milliampSecondsDelta;
  }

  // State update:
  state_.currentMilliampSeconds += milliampSecondsDelta;
  // If we just charged past the point that was known to us, the current Ah
  // count is the new top. This means we move the bottom by the amount we
  // overshot the current top.
  if (state_.currentMilliampSeconds < 0) {
    state_.bottomMilliampSeconds -= state_.currentMilliampSeconds;
    state_.currentMilliampSeconds = 0;
    onHighestCharge();

    // Otherwise, if we discharged further than we've ever seen, move the bottom
    // further down.
  } else if (state_.currentMilliampSeconds > state_.bottomMilliampSeconds) {
    state_.bottomMilliampSeconds = state_.currentMilliampSeconds;
    onHighestDischarge();
  }
}

void BatteryFuelGauge::onHighestCharge() {
  const int voltageBasedSocEstimate =
      openCircuitSocFromCellVoltage(filtered_voltage_millivolts_.get());
  // Maintain an invariant of topSoc >= bottomSoc
  if (voltageBasedSocEstimate < state_.bottomSoc) {
    return;
  }
  if (voltageBasedSocEstimate <= state_.topSoc) {
    return;
  }
  state_.topSoc = voltageBasedSocEstimate;
}

void BatteryFuelGauge::onHighestDischarge() {
  const int voltageBasedSocEstimate =
      openCircuitSocFromCellVoltage(filtered_voltage_millivolts_.get());
  // Maintain an invariant of topSoc >= bottomSoc
  if (voltageBasedSocEstimate > state_.topSoc) {
    return;
  }
  if (voltageBasedSocEstimate >= state_.bottomSoc) {
    return;
  }
  state_.bottomSoc = voltageBasedSocEstimate;
}

int32_t BatteryFuelGauge::getBatteryPercentage() {
  if (state_.bottomMilliampSeconds == 0) {
    return state_.topSoc;
  }
  return state_.topSoc - (state_.topSoc - state_.bottomSoc) *
                             ((float)(state_.currentMilliampSeconds)) /
                             ((float)(state_.bottomMilliampSeconds));
}