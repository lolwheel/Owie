#include "charging_tracker.h"

#include "packet.h"

ChargingTracker::ChargingTracker(BmsRelay* relay, int32_t makeNewPointAfterMah, const std::function<void(ChargingTracker*)>& newPointCallback)
    : makePointAfterMah_(makeNewPointAfterMah), newPointCallback_(newPointCallback) {
  // This takes advantage of the fact that the current packets are forwarded iff
  // the board is charging
  relay->addForwardedPacketCallback([this](BmsRelay* relay, Packet* p) {
    if (p->getType() != 5) {
      return;
    }
    // Can't do anything if we don't have voltage yet.
    if (relay->getTotalVoltageMillivolts() < 1) {
      return;
    }
    const int32_t totalChargedMah = relay->getRegeneratedChargeMah();

    // Initialize the first data point to absolute value of the voltage and
    // whatever mah we've recorded already.
    if (this->chargingPoints_.size() == 0) {
      auto cellMillivoltsArray = relay->getCellMillivolts();
      uint16_t minCellVoltage = 0xFFFF;
      for (int i = 0; i < 15; i++) {
        uint16_t voltage = cellMillivoltsArray[i];
        if (voltage < minCellVoltage) {
          minCellVoltage = voltage;
          this->tracked_cell_index_ = i;
        }
      }
      this->chargingPoints_.reserve(128);
      ChargingPoint_t initialPoint = {.millivolts = minCellVoltage,
                                      .totalMah = totalChargedMah};
      // Two data points inserted initially;
      this->chargingPoints_.push_back(initialPoint);
      this->chargingPoints_.push_back(initialPoint);
      return;
    }

    uint16_t voltageMillivolts =
        relay->getCellMillivolts()[this->tracked_cell_index_];
    ChargingPoint_t* lastPoint = &this->chargingPoints_.back();
    lastPoint->millivolts = voltageMillivolts;
    lastPoint->totalMah = totalChargedMah;

    // Create a new point if last two points are more than makePointAfterMah_
    // apart.
    if ((lastPoint->totalMah -
         this->chargingPoints_[this->chargingPoints_.size() - 2].totalMah) >=
        this->makePointAfterMah_) {
      this->chargingPoints_.push_back(*lastPoint);
      this->newPointCallback_(this);
    }
  });
}
