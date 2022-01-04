#ifndef BMS_RELAY_H
#define BMS_RELAY_H

#include <Stream.h>

#include <functional>
#include <vector>

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

  int data_lenght() const {
    if (!valid_) {
      return -1;
    }
    // 3 byte preamble + 1 byte type + 2 byte CRC
    return len_ - 6;
  }

  uint8_t* start() const { return start_; }

  uint8_t len() const { return len_; }

 private:
  void validate();
  uint8_t* start_;
  uint8_t len_;
  bool valid_ = false;
};

class BmsRelay {
 public:
  BmsRelay(Stream* source, Stream* sink) : source_(source), sink_(sink) {
    sourceBuffer_.reserve(128);
  }

  void loop();
  void setByteReceivedCallback(const std::function<void()>& callback) {
    byteReceivedCallback_ = callback;
  };

 private:
  void processNextByte();
  bool shouldForward(const Packet& p);

  std::function<void()> byteReceivedCallback_;
  std::vector<uint8_t> sourceBuffer_;
  Stream* const source_;
  Stream* const sink_;
};

#endif  // BMS_RELAY_H