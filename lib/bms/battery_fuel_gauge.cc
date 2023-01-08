#include "battery_fuel_gauge.h"

#include <cmath>

#include "defer.h"

namespace {

template <class T>
inline T clamp(T x, T lower, T upper) {
  return std::min(upper, std::max(x, lower));
}

int32_t openCircuitSocFromCellVoltage(int32_t cellVoltageMillivolts) {
  static constexpr int LOOKUP_TABLE_RANGE_MIN_MV = 2700;
  static constexpr int LOOKUP_TABLE_RANGE_MAX_MV = 4200;
  static uint8_t LOOKUP_TABLE[31] = {0,  0,  0,  0,  1,  2,  3,  4,  5,  7,  8,
                                     11, 14, 16, 18, 19, 25, 30, 33, 37, 43, 48,
                                     53, 60, 67, 71, 76, 82, 92, 97, 100};
  static constexpr int32_t LOOKUP_TABLE_SIZE =
      (sizeof(LOOKUP_TABLE) / sizeof(*LOOKUP_TABLE));
  static constexpr int32_t RANGE =
      LOOKUP_TABLE_RANGE_MAX_MV - LOOKUP_TABLE_RANGE_MIN_MV;
  // (RANGE - 1) upper limit effectively clamps the leftIndex below to
  // (LOOKUP_TABLE_SIZE - 2)
  cellVoltageMillivolts =
      clamp(cellVoltageMillivolts - LOOKUP_TABLE_RANGE_MIN_MV, 0, RANGE - 1);
  float floatIndex =
      float(cellVoltageMillivolts) * (LOOKUP_TABLE_SIZE - 1) / RANGE;
  const int32_t leftIndex = int(floatIndex);
  const float fractional = floatIndex - leftIndex;
  const int32_t rightIndex = leftIndex + 1;
  const int32_t leftValue = LOOKUP_TABLE[leftIndex];
  const int32_t rightValue = LOOKUP_TABLE[rightIndex];
  return clamp<int32_t>(
      leftValue + round((rightValue - leftValue) * fractional), 0, 100);
}

}  // namespace

void BatteryFuelGauge::updateVoltage(int32_t voltageMillivolts,
                                     int32_t nowMillis) {
  voltage_millivolts_ = voltageMillivolts;
  filtered_voltage_millivolts_.step(voltageMillivolts);
  voltage_based_soc_ =
      openCircuitSocFromCellVoltage(filtered_voltage_millivolts_.get());
  // If we're at the lowest Ah discharge we've seen, use voltage to set the
  // bottom SOC to the current level of charge.
  if (state_.currentMilliampSeconds == state_.bottomMilliampSeconds) {
    state_.bottomSoc = voltage_based_soc_;
  }
  // Do checks as above except for the top SOC.
  if (state_.currentMilliampSeconds == 0) {
    state_.topSoc = voltage_based_soc_;
  }

  // If voltage is telling us that we're below 10% charge and Ah based estimate
  // is 3x off, drag that estimate down.
  if (voltage_based_soc_ <= 10) {
    const int32_t maybeNewBottomSoc =
        std::min<int32_t>(voltage_based_soc_ * 3, state_.bottomSoc);
    if (state_.bottomSoc != maybeNewBottomSoc) {
      state_.bottomSoc = maybeNewBottomSoc;
      state_.bottomMilliampSeconds = state_.currentMilliampSeconds;
    }
  } else if (voltage_based_soc_ >= 90 && charging_) {
    // Conversely if we're charging, voltage based SOC is above 90 and Ah based
    // estimate is 3x off, drag that estimate up.
    const int32_t maybeNewTopSoc =
        std::max<int32_t>(100 - (100 - voltage_based_soc_) * 3, state_.topSoc);
    if (state_.topSoc != maybeNewTopSoc) {
      state_.topSoc = maybeNewTopSoc;
      state_.currentMilliampSeconds = 0;
    }
  }
}

void BatteryFuelGauge::updateCurrent(int32_t currentMilliamps,
                                     int32_t nowMillis) {
  // Ignore current updates until we see first voltage message
  if (voltage_millivolts_ < 0) {
    return;
  }

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
  // Maintain an invariant of topSoc >= bottomSoc
  if (voltage_based_soc_ < state_.bottomSoc) {
    return;
  }
  if (voltage_based_soc_ <= state_.topSoc) {
    return;
  }
  state_.topSoc = voltage_based_soc_;
}

void BatteryFuelGauge::onHighestDischarge() {
  // Maintain an invariant of topSoc >= bottomSoc
  if (voltage_based_soc_ > state_.topSoc) {
    return;
  }
  if (voltage_based_soc_ >= state_.bottomSoc) {
    return;
  }
  state_.bottomSoc = voltage_based_soc_;
}

int32_t BatteryFuelGauge::getSoc() const {
  if (state_.bottomMilliampSeconds == 0) {
    return state_.topSoc;
  }
  return state_.topSoc - (state_.topSoc - state_.bottomSoc) *
                             ((float)(state_.currentMilliampSeconds)) /
                             ((float)(state_.bottomMilliampSeconds));
}