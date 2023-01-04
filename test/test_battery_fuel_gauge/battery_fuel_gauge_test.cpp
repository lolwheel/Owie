#include "battery_fuel_gauge.h"

#include <unity.h>

#include <memory>

std::unique_ptr<BatteryFuelGauge> gauge;

void setUp(void) { gauge.reset(new BatteryFuelGauge()); }

void testUninitializedGauge() {
  TEST_ASSERT_EQUAL(0, gauge->getSoc());
  TEST_ASSERT_EQUAL(0, gauge->getMilliampSecondsDischarged());
  TEST_ASSERT_EQUAL(0, gauge->getMilliampSecondsRecharged());
}

void testCurrentRideSpentAndRegeneratedStats() {
  gauge->updateCurrent(6666, 0);  // The first current is disregarded
  gauge->updateCurrent(1000, 123);
  TEST_ASSERT_EQUAL(0, gauge->getMilliampSecondsRecharged());
  TEST_ASSERT_EQUAL(123, gauge->getMilliampSecondsDischarged());
  gauge->updateCurrent(-1000, 456);
  TEST_ASSERT_EQUAL(333, gauge->getMilliampSecondsRecharged());
  TEST_ASSERT_EQUAL(123, gauge->getMilliampSecondsDischarged());
}

void testSocAfterFirstVoltageMessageAndStateSaving() {
  gauge->updateVoltage(3800, 0);
  TEST_ASSERT_EQUAL(53, gauge->getSoc());
  auto state = gauge->getState();
  gauge.reset(new BatteryFuelGauge());
  gauge->restoreState(state);
  TEST_ASSERT_EQUAL(53, gauge->getSoc());
}

/**
 * Emulates the following scenario:
 * 1. Turns on the board in chargin mode at 18% SOC based on voltage
 * 2. Charges the board with 2 amps for an hour. Emulates the target voltage
 * based SOC of 68%
 * 3. Turns off the board.
 * 4. Turns on and discharges at 6 amps for 10 minutes.
 */
void testSimpleChargeAndHalfwayDischarge() {
  gauge->updateVoltage(3400, 0);
  TEST_ASSERT_EQUAL(18, gauge->getSoc());
  for (int32_t time = 0; time <= 3600 * 1000; time += 100) {
    gauge->updateCurrent(-2000, time);
    // Also update the voltage with the target voltage so that
    // we saturate the low pass filter
    gauge->updateVoltage(3908, time);
  }
  TEST_ASSERT_EQUAL(68, gauge->getSoc());
  TEST_ASSERT_EQUAL(2 * 3600 * 1000, gauge->getMilliampSecondsRecharged());
  TEST_ASSERT_EQUAL(0, gauge->getMilliampSecondsDischarged());

  // Recreate based on state
  auto state = gauge->getState();
  gauge.reset(new BatteryFuelGauge());
  gauge->restoreState(state);
  TEST_ASSERT_EQUAL(68, gauge->getSoc());

  for (int32_t time = 0; time <= 10 * 60 * 1000; time += 100) {
    gauge->updateCurrent(6000, time);
    // Note that we're updating voltage with a value that is off.
    // The fuel gauge must correctly go based solely off Ah calculations.
    gauge->updateVoltage(3908, time);
  }
  state = gauge->getState();
  TEST_ASSERT_EQUAL(43, gauge->getSoc());
  TEST_ASSERT_EQUAL(6 * 10 * 60 * 1000, gauge->getMilliampSecondsDischarged());
  TEST_ASSERT_EQUAL(0, gauge->getMilliampSecondsRecharged());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(testUninitializedGauge);
  RUN_TEST(testCurrentRideSpentAndRegeneratedStats);
  RUN_TEST(testSocAfterFirstVoltageMessageAndStateSaving);
  RUN_TEST(testSimpleChargeAndHalfwayDischarge);
  UNITY_END();

  return 0;
}