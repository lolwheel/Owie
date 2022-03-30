#include "bms_relay.h"

#include <cstring>

#include "packet.h"

namespace {
const uint8_t PREAMBLE[] = {0xFF, 0x55, 0xAA};

const int8_t packetLengths[] = {7, -1, 38, 7, 11, 8,  10, 13, 7,
                                7, -1, 8,  8, 9,  -1, 11, 16, 10};
}  // namespace

BmsRelay::BmsRelay(const Source& source, const Sink& sink, const Millis& millis)
    : source_(source), sink_(sink), millis_(millis) {
  sourceBuffer_.reserve(64);
}

void BmsRelay::loop() {
  while (true) {
    int byte = source_();
    if (byte < 0) {
      return;
    }
    sourceBuffer_.push_back(byte);
    processNextByte();
  }
}

void BmsRelay::purgeUnknownData() {
  for (uint8_t b : sourceBuffer_) {
    sink_(b);
  }
  if (unknownDataCallback_) {
    for (uint8_t b : sourceBuffer_) {
      unknownDataCallback_(b);
    }
  }
  sourceBuffer_.clear();
}

// Called with every new byte.
void BmsRelay::processNextByte() {
  // If up to first three bytes of the sourceBuffer don't match expected
  // preamble, flush the data unchanged.
  for (unsigned int i = 0; i < std::min(sizeof(PREAMBLE), sourceBuffer_.size());
       i++) {
    if (sourceBuffer_[i] != PREAMBLE[i]) {
      purgeUnknownData();
      return;
    }
  }
  // Check if we have the message type.
  if (sourceBuffer_.size() < 4) {
    return;
  }
  uint8_t type = sourceBuffer_[3];
  if (type >= sizeof(packetLengths) || packetLengths[type] < 0) {
    purgeUnknownData();
    return;
  }
  uint8_t len = packetLengths[type];
  if (sourceBuffer_.size() < len) {
    return;
  }
  Packet p(sourceBuffer_.data(), len);
  for (auto& callback : receivedPacketCallbacks_) {
    callback(this, &p);
  };
  bmsStatusParser(p);
  bmsSerialParser(p);
  currentParser(p);
  batteryPercentageParser(p);
  cellVoltageParser(p);
  temperatureParser(p);
  powerOffParser(p);
  //  Recalculate CRC so that logging callbacks see the correct CRCs
  p.recalculateCrcIfValid();
  if (p.shouldForward()) {
    for (auto& callback : forwardedPacketCallbacks_) {
      callback(this, &p);
    }
  }
  p.recalculateCrcIfValid();
  if (p.shouldForward()) {
    for (int i = 0; i < p.len(); i++) {
      sink_(p.start()[i]);
    }
  }
  sourceBuffer_.clear();
}