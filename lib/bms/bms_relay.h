#ifndef BMS_RELAY_H
#define BMS_RELAY_H

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

  int dataLength() const {
    if (!valid_) {
      return -1;
    }
    // 3 byte preamble + 1 byte type + 2 byte CRC
    return len_ - 6;
  }

  void doneUpdatingData();

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
  /**
   * @brief Function polled for new byte from BMS. Expected to return negative
   * value when there's no data available on the wire.
   */
  typedef std::function<int()> Source;
  /**
   * @brief Function called to send data to the MB.
   */
  typedef std::function<size_t(uint8_t)> Sink;
  BmsRelay(const Source& source, const Sink& sink)
      : source_(source), sink_(sink) {
    sourceBuffer_.reserve(128);
  }

  void loop();

  void setByteReceivedCallback(const std::function<void()>& callback) {
    byteReceivedCallback_ = callback;
  };

  /**
   * @brief If set to non-zero value, spoofs captured BMS serial
   * with the number provided here. The serial number can be found
   * on the sticker on the bottom side of BMS. The lower number on
   * the sticker without the 4 leading (most significant) digits
   * goes here.
   */
  void setBMSSerialOverride(uint32_t serial) { serial_override_ = serial; }

  /**
   * @brief Get the captured BMS serial number.
   *
   * @return non-zero serial, if it's already been captured.
   */
  uint32_t getCapturedBMSSerial() { return captured_serial_; }

 private:
  void processNextByte();
  bool shouldForward(Packet& p);

  std::function<void()> byteReceivedCallback_;
  std::vector<uint8_t> sourceBuffer_;
  uint32_t serial_override_ = 0;
  uint32_t captured_serial_ = 0;
  const Source source_;
  const Sink sink_;
};

#endif  // BMS_RELAY_H