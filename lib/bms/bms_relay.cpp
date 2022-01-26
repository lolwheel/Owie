#include "bms_relay.h"

#include <cstring>

#include "packet.h"
#include "packet_parsers.h"

namespace {
const std::vector<uint8_t> PREAMBLE = {0xFF, 0x55, 0xAA};
// Sanity check
const uint8_t MAX_PACKET_SIZE = 128;
}  // namespace

BmsRelay::BmsRelay(const Source& source, const Sink& sink)
    : source_(source), sink_(sink) {
  sourceBuffer_.reserve(128);
  addPacketCallback(bmsSerialParser);
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

// Called with every new byte.
void BmsRelay::processNextByte() {
  // If up to first three bytes of the sourceBuffer don't match expected
  // preamble, flush the data unchanged.
  for (unsigned int i = 0; i < std::min(PREAMBLE.size(), sourceBuffer_.size());
       i++) {
    if (sourceBuffer_[i] != PREAMBLE[i]) {
      for (uint8_t b : sourceBuffer_) {
        sink_(b);
      }
      sourceBuffer_.clear();
      return;
    }
  }
  // Look for the next preamble to determine if we have a full packet at our
  // hands. Minimum size of a packet is 3 (preamble) + 1 type (data) + 2 (crc) =
  // 6 Scan for the next packet only if we already have 6 + 3 (next packet
  // preamble) = 9 bytes
  if (sourceBuffer_.size() < 9) {
    return;
  }
  // This function is called with every new byte so we only need to look at the
  // last three bytes of the buffer.
  if (std::memcmp(PREAMBLE.data(),
                  sourceBuffer_.data() + sourceBuffer_.size() - PREAMBLE.size(),
                  PREAMBLE.size())) {
    // No preamble found, let's do sanity check on accumulated size of the data
    // and flush it if it's over the threshold.
    if (sourceBuffer_.size() >= MAX_PACKET_SIZE) {
      // An edge case that I'm too lazy to handle is that we might flush first
      // one or two first bytes of the preamble. In that case we'll miss one
      // extra packet.
      for (uint8_t b : sourceBuffer_) {
        sink_(b);
      }
      sourceBuffer_.clear();
    }
    return;
  }
  // We did find the next packet preamble, treat the data up until the last
  // three bytes (preamble) as a packet and perform the CRC check.
  Packet p(sourceBuffer_.data(), sourceBuffer_.size() - PREAMBLE.size());
  for (auto callback : packetCallbacks_) {
    callback(this, &p);
  }
  p.recalculateCrcIfValid();
  for (int i = 0; i < p.len(); i++) {
    sink_(p.start()[i]);
  }
  sourceBuffer_.resize(PREAMBLE.size());
  std::copy(PREAMBLE.begin(), PREAMBLE.end(), sourceBuffer_.begin());
}