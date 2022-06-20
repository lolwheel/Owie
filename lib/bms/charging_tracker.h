#ifndef CHARGING_TRACKER_H
#define CHARGING_TRACKER_H

#include "bms_relay.h"

class ChargingTracker {
 public:
  ChargingTracker(BmsRelay* relay, int32_t makeNewPointAfterMah, const std::function<void(ChargingTracker*)>& newPointCallback);

  typedef struct {
      int32_t millivolts;
      int32_t totalMah;
  } ChargingPoint_t;

  const std::vector<ChargingPoint_t>& getChargingPoints() { return chargingPoints_;}
  uint8_t getTrackedCellIndex() { return tracked_cell_index_;}

 private:
  std::vector<ChargingPoint_t> chargingPoints_;
  const int32_t makePointAfterMah_;
  std::function<void(ChargingTracker*)> newPointCallback_;
  uint8_t tracked_cell_index_ = 0;
};

#endif  // CHARGING_TRACKER_H