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

  typedef std::function<unsigned long()> Millis;

  BmsRelay(const Source& source, const Sink& sink, const Millis& millis,
           int32_t batteryCapacityMah);

  /**
   * @brief Call from arduino loop.
   */
  void loop();

  void addPacketCallback(const PacketCallback& callback) {
    packetCallbacks_.push_back(callback);
  }

  void setCurrentCallback(const std::function<void(float)>& c) {
    currentCallback_ = c;
  }

  void setPowerOffCallback(const std::function<void(void)>& c) {
    powerOffCallback_ = c;
  }

  void setSocRewriterCallback(const std::function<int8_t(int8_t, bool*)>& c) {
    socRewriterCallback_ = c;
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
   * @brief Battery percentage as reported by the BMS.
   */
  int8_t getBmsReportedSOC() { return bmsSocPercent_; }

  /**
   * @brief Spoofed battery percentage sent to the controller.
   */
  int8_t getOverriddenSoc() {
    return getMahTillEmpty() * 100 / getBatteryCapacityOverrideMah();
  }

  int32_t getMahTillEmpty() { return milliamp_seconds_till_empty_ / 3600; }

  /**
   * @brief Cell voltages in millivolts.
   * @return pointer to a 15 element array.
   */
  uint16_t* const getCellMillivolts() { return cell_millivolts_; }

  uint16_t getTotalVoltageMillivolts() { return total_voltage_millivolts_; }

  int32_t getBatteryCapacityOverrideMah() { return battery_capacity_override_; }

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
  std::function<void(float)> currentCallback_;
  std::function<int8_t(int8_t, bool*)> socRewriterCallback_;
  std::function<void(void)> powerOffCallback_;

  std::vector<uint8_t> sourceBuffer_;
  uint32_t serial_override_ = 0;
  uint32_t captured_serial_ = 0;
  float current_ = std::numeric_limits<float>::min();
  int8_t bmsSocPercent_ = -1;
  uint16_t cell_millivolts_[15] = {0};
  uint16_t total_voltage_millivolts_ = 0;
  int8_t temperatures_celsius_[5] = {0};
  int8_t avg_temperature_celsius_ = 0;
  unsigned long last_current_message_millis_ = 0;
  float last_current_ = 0;
  int32_t milliamp_seconds_till_empty_ = 0;
  bool milliamp_seconds_till_empty_initialized_ = false;
  const Source source_;
  const Sink sink_;
  const Millis millis_;
  const int32_t battery_capacity_override_ = 0;

  void chargingStatusParser(Packet& p);
  void bmsSerialParser(Packet& p);
  void currentParser(Packet& p);
  void batteryPercentageParser(Packet& p);
  void cellVoltageParser(Packet& p);
  void temperatureParser(Packet& p);
  void powerOffParser(Packet& p);
};

#endif  // BMS_RELAY_H