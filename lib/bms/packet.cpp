#include "packet.h"

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

void Packet::recalculateCrcIfValid() {
  if (!valid_) {
    return;
  }
  uint16_t crc = 0;
  for (int i = 0; i < len_ - 2; i++) {
    crc += start_[i];
  }
  start_[len_ - 2] = (crc >> 8);
  start_[len_ - 1] = (crc & 0xFF);
}