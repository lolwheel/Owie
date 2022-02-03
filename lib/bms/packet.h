#ifndef PACKET_H
#define PACKET_H

#include <cstdint>

class Packet {
 public:
  Packet(uint8_t* start, uint8_t len) : start_(start), len_(len) { validate(); }

  int getType() const {
    if (!valid_) {
      return -1;
    }
    return start_[3];
  };

  bool isValid() const { return valid_; }

  uint8_t* data() const {
    if (!valid_) {
      return nullptr;
    }
    return start_ + 4;
  }

  int dataLength() const {
    if (!valid_) {
      return -1;
    }
    // 3 byte preamble + 1 byte type + 2 byte CRC
    return len_ - 6;
  }

  void setShouldForward(bool shouldForward) {
    this->shouldForward_ = shouldForward;
  }

  bool shouldForward() { return this->shouldForward_; }

  void recalculateCrcIfValid();

  uint8_t* start() const { return start_; }

  uint8_t len() const { return len_; }

 private:
  void validate();
  uint8_t* start_;
  uint8_t len_;
  bool valid_ = false;
  bool shouldForward_ = true;
};

#endif  // PACKET_H