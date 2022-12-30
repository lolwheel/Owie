#ifndef BATTERY_FUEL_GAUGE_H
#define BATTERY_FUEL_GAUGE_H

#include <stdint.h>

#include "filter.h"

/**
 * Assumes that the class gets to see all of the energy going into and out of
 * the battery.
 */
class BatteryFuelGauge {
 public:
  // restoreState must be called exatly once and before any other calls on this
  // object.
  void restoreState();
  void saveState();

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
  LowPassFilter cell_voltage_filter_;
  int32_t known_range_top_voltage_millivolts_ = -1;
  int32_t known_range_bottom_voltage_millivolts_ = -1;
  int32_t battery_capacity_hint_milliamp_seconds_ = -1;
  int32_t spent_milliamps_second_ = -1;
  int32_t regenerated_milliamps_second_ = -1;
  int32_t last_current_update_time_millis_ = -1;
  int32_t milliamp_seconds_discharged_ = 0;
  int32_t milliamp_seconds_recharged_ = 0;
  bool charging_ = false;
};

#endif  // BATTERY_FUEL_GAUGE_H
