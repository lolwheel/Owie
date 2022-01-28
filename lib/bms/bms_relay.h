#ifndef BMS_RELAY_H
#define BMS_RELAY_H

#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

class Packet;

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
  typedef std::function<void(uint8_t)> Sink;

  /**
   * @brief Packet callback.
   */
  typedef std::function<void(BmsRelay*, Packet*)> PacketCallback;

  BmsRelay(const Source& source, const Sink& sink);

  /**
   * @brief Call from arduino loop.
   */
  void loop();

  void addPacketCallback(const PacketCallback& callback) {
    packetCallbacks_.push_back(callback);
  }

  void setUnknownDataCallback(const Sink& c) { unknownDataCallback_ = c; }

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

  /**
   * @brief Current In Amps.
   */
  float getCurrentInAmps() { return current_; }

  /**
   * @brief Battery percentage.
   */
  int8_t getBatteryPercentage() { return battery_percentage_; }

  /**
   * @brief Cell voltages in millivolts.
   * @return pointer to a 15 element array.
   */
  uint16_t* const getCellMillivolts() { return cell_millivolts_; }

  uint16_t getTotalVoltageMillivolts() { return total_voltage_millivolts_; }

  /**
   * @brief Cell voltages in millivolts.
   * @return pointer to a 5 element array.
   */
  int8_t* const getTemperaturesCelsius() { return temperatures_celsius_; }

  uint8_t getAverageTemperatureCelsius() { return avg_temperature_celsius_; }

 private:
  void processNextByte();
  bool shouldForward(Packet& p);
  void purgeUnknownData();

  std::vector<PacketCallback> packetCallbacks_;
  Sink unknownDataCallback_;
  std::vector<uint8_t> sourceBuffer_;
  uint32_t serial_override_ = 0;
  uint32_t captured_serial_ = 0;
  float current_ = std::numeric_limits<float>::min();
  int8_t battery_percentage_ = -1;
  uint16_t cell_millivolts_[15] = {0};
  uint16_t total_voltage_millivolts_ = 0;
  int8_t temperatures_celsius_[5] = {0};
  int8_t avg_temperature_celsius_ = 0;
  const Source source_;
  const Sink sink_;

  void bmsSerialParser(Packet& p);
  void currentParser(Packet& p);
  void batteryPercentageParser(Packet& p);
  void cellVoltageParser(Packet& p);
  void temperatureParser(Packet& p);
  void powerOffParser(Packet& p);
};

#endif  // BMS_RELAY_H