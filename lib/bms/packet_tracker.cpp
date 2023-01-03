#include "packet_tracker.h"

#include <cstring>

#include "defer.h"

PacketTracker::PacketTracker()
    : individual_packet_stats_(sizeof(PACKET_LENGTHS_BY_TYPE)) {}

void PacketTracker::processPacket(const Packet& packet,
                                  const unsigned long now_millis) {
  if (!packet.isValid()) {
    global_stats_.total_packet_checksum_mismatches++;
  }
  // Though this should never happen, let's sanity check the packet
  // type so we don't accidentally OOM.
  int type = packet.getType();
  if (type < 0 || type > 255) {
    return;
  }
  if (type >= (int)individual_packet_stats_.size()) {
    individual_packet_stats_.resize(type + 1);
  }
  global_stats_.total_known_bytes_received += packet.len();
  global_stats_.total_known_packets_received++;

  IndividualPacketStat* stat = &individual_packet_stats_[type];
  defer { stat->last_packet_millis = now_millis; };

  stat->last_seen_valid_packet.resize(packet.len());
  memcpy(&stat->last_seen_valid_packet[0], packet.start(), packet.len());

  if (stat->total_num++ == 0) {
    stat->id = type;
    return;
  }
  // Shouldn't happen but let's guard against.
  if (now_millis < stat->last_packet_millis) {
    return;
  }
  stat->mean_and_dev_.add_value(float(now_millis - stat->last_packet_millis));
}

void PacketTracker::unknownBytes(int num) {
  global_stats_.total_unknown_bytes_received += num;
}