#ifndef PACKET_TRACKER_H
#define PACKET_TRACKER_H

#include <cmath>
#include <functional>
#include <vector>

#include "packet.h"
#include "welford.h"

class IndividualPacketStat {
 public:
  IndividualPacketStat() : id(-1), total_num(0), last_packet_millis(0) {}
  // Packet message id, -1 if not initialized
  int id;
  int32_t total_num;
  std::vector<uint8_t> last_seen_valid_packet;
  unsigned long last_packet_millis;
  int32_t mean_period_millis() const { return (int32_t)mean_and_dev_.mean(); }
  int32_t deviation_millis() const { return (int32_t)mean_and_dev_.sd(); };

 private:
  Welford<float> mean_and_dev_;
  friend class PacketTracker;
};

struct GlobalStats {
  GlobalStats()
      : total_known_packets_received(0),
        total_known_bytes_received(0),
        total_packet_checksum_mismatches(0),
        total_unknown_bytes_received(0) {}
  int32_t total_known_packets_received;
  int32_t total_known_bytes_received;
  int32_t total_packet_checksum_mismatches;
  int32_t total_unknown_bytes_received;
};

class PacketTracker {
 public:
  PacketTracker();

  void processPacket(const Packet& packet, const unsigned long millis);
  void unknownBytes(int num);
  const GlobalStats& getGlobalStats() const { return global_stats_; }
  const std::vector<IndividualPacketStat>& getIndividualPacketStats() const {
    return individual_packet_stats_;
  }

 private:
  GlobalStats global_stats_;
  std::vector<IndividualPacketStat> individual_packet_stats_;
};

#endif