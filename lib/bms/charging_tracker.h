#ifndef CHARGING_TRACKER_H
#define CHARGING_TRACKER_H

#include "bms_relay.h"

class ChargingTracker {
 public:
  ChargingTracker(BmsRelay* relay, uint32_t makeNewPointAfterMah);

  typedef struct {
      uint32_t millivoltsDelta;
      uint32_t mahDelta;
  } ChargingPoint_t;

  const std::vector<ChargingPoint_t>& getChargingPoints() { return chargingPoints_;}

 private:
  std::vector<ChargingPoint_t> chargingPoints_;
  const uint32_t makePointAfterMah_;
  uint32_t lastInsertedChargingPointTotalMah_;
  uint32_t lastInsertedChargingPointTotalVoltageMillivolts_;
  void insertChargingPoint(uint32_t totalVoltageMillivolts, uint32_t totalChargedMah);
};

#endif  // CHARGING_TRACKER_H