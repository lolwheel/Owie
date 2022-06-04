#include "charging_tracker.h"

#include "packet.h"

ChargingTracker::ChargingTracker(BmsRelay* relay, uint32_t makeNewPointAfterMah)
    : makePointAfterMah_(makeNewPointAfterMah) {
  // This takes advantage of the fact that the current packets are forwarded iff
  // the board is charging
  relay->addForwardedPacketCallback([this](BmsRelay* relay, Packet* p) {
    if (p->getType() != 5) {
      return;
    }
    const uint32_t voltageMillivolts = relay->getTotalVoltageMillivolts();
    // Can't do anything if we don't have voltage yet.
    if (voltageMillivolts < 1) {
      return;
    }
    const uint32_t totalChargedMah = relay->getRegeneratedChargeMah();

    // Initialize the first data point to absolute value of the voltage and
    // whatever mah we've recorded already.
    if (this->chargingPoints_.size() == 0) {
      this->chargingPoints_.reserve(128);
      this->chargingPoints_.push_back(ChargingPoint_t{
          .millivolts = voltageMillivolts, .mah = totalChargedMah});
      this->lastInsertedChargingPointTotalMah_ = totalChargedMah;
      return;
    }

    if (totalChargedMah - this->lastInsertedChargingPointTotalMah_ <
        this->makePointAfterMah_) {
      return;
    }

    this->chargingPoints_.push_back(ChargingPoint_t{
        .millivolts = voltageMillivolts,
        .mah = totalChargedMah - this->lastInsertedChargingPointTotalMah_});

    this->lastInsertedChargingPointTotalMah_ = totalChargedMah;
  });
}
