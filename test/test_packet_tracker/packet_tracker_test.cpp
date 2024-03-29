#include "packet_tracker.h"

#include <unity.h>

#include <memory>
#include <string>

#include "packet.h"

std::vector<std::string> tasksExecuted;
std::unique_ptr<PacketTracker> tracker;

void setUp(void) { tracker.reset(new PacketTracker()); }

void testGlobalTracking() {
  uint8_t data[] = {0xFF, 0x55, 0xAA, 0x6, 0x1, 0x2, 0x3, 0x4, 0x2, 0xE};
  Packet p(data, sizeof(data));
  tracker->processPacket(p, 0);
  // Global stats check
  TEST_ASSERT_EQUAL(1, tracker->getGlobalStats().total_known_packets_received);
  TEST_ASSERT_EQUAL(0,
                    tracker->getGlobalStats().total_packet_checksum_mismatches);
  TEST_ASSERT_EQUAL(10, tracker->getGlobalStats().total_known_bytes_received);
  TEST_ASSERT_EQUAL(0, tracker->getGlobalStats().total_unknown_bytes_received);

  tracker->processPacket(p, 1000);
  TEST_ASSERT_EQUAL(2, tracker->getGlobalStats().total_known_packets_received);
  TEST_ASSERT_EQUAL(0,
                    tracker->getGlobalStats().total_packet_checksum_mismatches);
  TEST_ASSERT_EQUAL(20, tracker->getGlobalStats().total_known_bytes_received);
  TEST_ASSERT_EQUAL(0, tracker->getGlobalStats().total_unknown_bytes_received);
}

void testindividualStatsCalculation() {
  uint8_t data[] = {0xFF, 0x55, 0xAA, 0x6, 0x1, 0x2, 0x3, 0x4, 0x2, 0xE};
  Packet p(data, sizeof(data));
  tracker->processPacket(p, 0);
  TEST_ASSERT_EQUAL(1, tracker->getIndividualPacketStats()[6].total_num);
  TEST_ASSERT_EQUAL(6, tracker->getIndividualPacketStats()[6].id);
  TEST_ASSERT_EQUAL(
      0, tracker->getIndividualPacketStats()[6].mean_period_millis());
  TEST_ASSERT_EQUAL(0,
                    tracker->getIndividualPacketStats()[6].deviation_millis());
  TEST_ASSERT_EQUAL(
      sizeof(data),
      tracker->getIndividualPacketStats()[6].last_seen_valid_packet.size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(
      data, &tracker->getIndividualPacketStats()[6].last_seen_valid_packet[0],
      sizeof(data));

  // Advance the time by 1000 millis and send the same packet
  tracker->processPacket(p, 1000);

  TEST_ASSERT_EQUAL(2, tracker->getIndividualPacketStats()[6].total_num);
  TEST_ASSERT_EQUAL(6, tracker->getIndividualPacketStats()[6].id);
  TEST_ASSERT_EQUAL(
      1000, tracker->getIndividualPacketStats()[6].mean_period_millis());

  // Advance time by 1006 millis and send the packet again
  tracker->processPacket(p, 2006);

  TEST_ASSERT_EQUAL(3, tracker->getIndividualPacketStats()[6].total_num);
  TEST_ASSERT_EQUAL(6, tracker->getIndividualPacketStats()[6].id);
  TEST_ASSERT_EQUAL(
      1003, tracker->getIndividualPacketStats()[6].mean_period_millis());
  TEST_ASSERT_EQUAL(4,
                    tracker->getIndividualPacketStats()[6].deviation_millis());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(testGlobalTracking);
  RUN_TEST(testindividualStatsCalculation);
  UNITY_END();

  return 0;
}