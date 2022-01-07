#include "bms_relay.h"

#include <cstring>

namespace {
const std::vector<uint8_t> PREAMBLE = {0xFF, 0x55, 0xAA};
// Sanity check
const uint8_t MAX_PACKET_SIZE = 128;
}  // namespace

void Packet::validate() {
  valid_ = false;
  if (len_ < 6) {
    return;
  }
  uint16_t crc = ((uint16_t)(start_[len_ - 2])) << 8 | start_[len_ - 1];
  for (int i = 0; i < len_ - 2; i++) {
    crc -= start_[i];
  }
  if (crc == 0) {
    valid_ = true;
  }
}

void Packet::doneUpdatingData() {
  uint16_t crc = 0;
  for (int i = 0; i < len_ - 2; i++) {
    crc += start_[i];
  }
  start_[len_ - 2] = (crc >> 8);
  start_[len_ - 1] = (crc & 0xFF);
}

void BmsRelay::loop() {
  while (true) {
    int byte = source_();
    if (byte < 0) {
      return;
    }
    sourceBuffer_.push_back(byte);
    processNextByte();
    if (byteReceivedCallback_) {
      byteReceivedCallback_();
    }
  }
}

bool BmsRelay::shouldForward(Packet& p) {
  if (p.getType() == 6) {
    // 0x6 message has the BMS serial number encoded inside of it.
    // It is the last seven digits from the sticker on the back of the BMS.
    if (p.dataLength() == 4) {
      if (captured_serial_ == 0) {
        for (int i = 0; i < 4; i++) {
          captured_serial_ |= p.data()[i] << (8 * (3-i));
        }
      }
      if (serial_override_ != 0) {
        uint32_t serial_override_copy = serial_override_;
        for (int i = 3; i >= 0; i--) {
          p.data()[i] = serial_override_copy & 0xFF;
          serial_override_copy >>= 8;
        }
        p.doneUpdatingData();
      }
    }
  }
  return true;
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
  if (!p.isValid() || shouldForward(p)) {
    for (int i = 0; i < p.len(); i++) {
      sink_(p.start()[i]);
    }
  }
  sourceBuffer_.resize(PREAMBLE.size());
  std::copy(PREAMBLE.begin(), PREAMBLE.end(), sourceBuffer_.begin());
}