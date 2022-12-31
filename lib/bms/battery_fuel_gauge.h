#ifndef BATTERY_FUEL_GAUGE_H
#define BATTERY_FUEL_GAUGE_H

#include <stdint.h>

#include "filter.h"

struct FuelGaugeState {
  int32_t bottomMilliampSeconds;
  int32_t currentMilliampSeconds;
  int8_t topSoc;
  int8_t bottomSoc;
};

/**
 * Assumes that the class gets to see all of the energy going into and out of
 * the battery.
 */
class BatteryFuelGauge {
 public:
  // restoreState must be called exatly once and before any other calls on this
  // object.
  void restoreState(const FuelGaugeState& from);
  void saveState(FuelGaugeState& to);

  // Takes a single cell voltage in millivolts.
  void updateVoltage(int32_t voltageMillivolts, int32_t nowMillis);
  void updateCurrent(int32_t currentMilliamps, int32_t nowMillis);
  // Only transition from true to false is expected.
  void updateChargingStatus(bool charging) { charging_ = charging; }
  int32_t getBatteryPercentage();

  int32_t getMilliampSecondsDischarged() {
    return milliamp_seconds_discharged_;
  }
  int32_t getMilliampSecondsRecharged() { return milliamp_seconds_recharged_; }

 private:
  void onHighestCharge();
  void onHighestDischarge();

  int32_t voltage_millivolts_ = -1;
  LowPassFilter filtered_voltage_millivolts_;
  int32_t last_current_update_time_millis_ = -1;
  int32_t milliamp_seconds_discharged_ = 0;
  int32_t milliamp_seconds_recharged_ = 0;
  bool charging_ = false;
  FuelGaugeState state_;
};

#endif  // BATTERY_FUEL_GAUGE_H
