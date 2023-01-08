#ifndef BATTERY_FUEL_GAUGE_H
#define BATTERY_FUEL_GAUGE_H

#include <stdint.h>

#include "filter.h"

struct FuelGaugeState {
  // A positive number, mAs of the deepest discharge seen.
  int32_t bottomMilliampSeconds = 0;
  // A positive number, mAs currently discharged compared to the highest charge
  // seen.
  int32_t currentMilliampSeconds = 0;
  // Voltage-based SOC estimate corresponding to the top of the tracked SOC
  // range.
  int32_t topSoc = 0;
  // Voltage-based SOC estimate corresponding to the bottom of the tracked SOC
  // range.
  int32_t bottomSoc = 0;
};

/**
 * Assumes that the class gets to see all of the energy going into and out of
 * the battery.
 */
class BatteryFuelGauge {
 public:
  // restoreState must be called exatly once and before any other calls on this
  // object.
  void restoreState(const FuelGaugeState& from) { state_ = from; };
  const FuelGaugeState& getState() const { return state_; }

  // Takes a single cell voltage in millivolts.
  void updateVoltage(int32_t voltageMillivolts, int32_t nowMillis);
  void updateCurrent(int32_t currentMilliamps, int32_t nowMillis);
  // Only transition from true to false is expected.
  void updateChargingStatus(bool charging) { charging_ = charging; }
  int32_t getSoc() const;
  int32_t getVoltageBasedSoc() const { return voltage_based_soc_; }

  int32_t getMilliampSecondsDischarged() {
    return milliamp_seconds_discharged_;
  }
  int32_t getMilliampSecondsRecharged() { return milliamp_seconds_recharged_; }

 private:
  void onHighestCharge();
  void onHighestDischarge();

  int32_t voltage_millivolts_ = -1;
  LowPassFilter filtered_voltage_millivolts_;
  int32_t voltage_based_soc_ = -1;
  int32_t last_current_update_time_millis_ = -1;
  int32_t milliamp_seconds_discharged_ = 0;
  int32_t milliamp_seconds_recharged_ = 0;
  bool charging_ = false;
  FuelGaugeState state_;
};

#endif  // BATTERY_FUEL_GAUGE_H
