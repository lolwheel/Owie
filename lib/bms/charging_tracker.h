#ifndef CHARGING_TRACKER_H
#define CHARGING_TRACKER_H

#include "bms_relay.h"

class ChargingTracker {
 public:
  ChargingTracker(BmsRelay* relay, uint32_t makeNewPointAfterMah);

  typedef struct {
      uint32_t millivolts;
      uint32_t totalMah;
  } ChargingPoint_t;

  const std::vector<ChargingPoint_t>& getChargingPoints() { return chargingPoints_;}
  uint8_t getTrackedCellIndex() { return tracked_cell_index_;}

 private:
  std::vector<ChargingPoint_t> chargingPoints_;
  const uint32_t makePointAfterMah_;
  uint8_t tracked_cell_index_ = 0;
  void insertChargingPoint(uint32_t totalVoltageMillivolts, uint32_t totalChargedMah);
};

#endif  // CHARGING_TRACKER_H