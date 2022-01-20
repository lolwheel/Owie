#include "bms_relay.h"

#include <unity.h>

#include <deque>
#include <memory>

std::unique_ptr<BmsRelay> relay;
std::deque<int> mockBmsData;
std::vector<uint8_t> mockDataOut;

void setUp(void) {
  relay.reset(new BmsRelay(
      [&]() {
        if (mockBmsData.empty()) {
          return -1;
        }
        int data = mockBmsData.front();
        mockBmsData.pop_front();
        return data;
      },
      [&](uint8_t b) {
        mockDataOut.push_back(b);
        return 1;
      }));
  mockBmsData.clear();
  mockDataOut.clear();
}

void addMockData(const std::vector<int>& data) {
  mockBmsData.insert(mockBmsData.end(), data.begin(), data.end());
}

void expectDataOut(const std::vector<uint8_t>& expected) {
  TEST_ASSERT_TRUE(expected == mockDataOut);
}

void testUnknownBytesGetsForwardedImmediately(void) {
  addMockData({0x1, 0x2, 0x3});
  relay->loop();
  TEST_ASSERT_TRUE(mockBmsData.empty());
  expectDataOut({0x1, 0x2, 0x3});
}

void testSerialGetsRecordedAndIntercepted(void) {
  addMockData(
      {0x1, 0x2, 0x3, 0xFF, 0x55, 0xAA, 0x6, 0x1, 0x2, 0x3, 0x4, 0x2, 0xE});
  relay->setBMSSerialOverride(0x8040201);
  relay->loop();
  TEST_ASSERT_TRUE(mockBmsData.empty());
  // Expect no data so far because we identify packets only when we see the
  // header of the next one.
  expectDataOut({0x1, 0x2, 0x3});
  // Feed the header of next packet
  addMockData({0xFF, 0x55, 0xAA});
  relay->loop();
  // Now the original packet should have been parsed and rewritten.
  TEST_ASSERT_EQUAL(0x1020304, relay->getCapturedBMSSerial());
  expectDataOut(
      {0x1, 0x2, 0x3, 0xFF, 0x55, 0xAA, 0x6, 0x8, 0x4, 0x2, 0x1, 0x2, 0x13});
}

void testPacketCallback() {
  addMockData({0x1, 0x2, 0x3, 0xFF, 0x55, 0xAA, 0x6, 0x1, 0x2, 0x3, 0x4, 0x2,
               0xE, 0xFF, 0x55, 0xAA});
  std::vector<uint8_t> receivedPacket;
  relay->setPacketReceivedCallback([&](const Packet& p) {
    const uint8_t* start = p.start();
    receivedPacket.assign(start, start + p.len());
  });
  relay->loop();
  std::vector<uint8_t> expected(
      {0xFF, 0x55, 0xAA, 0x6, 0x1, 0x2, 0x3, 0x4, 0x2, 0xE});
  TEST_ASSERT_EQUAL(expected.size(), receivedPacket.size());
  for (int i = 0; i < expected.size(); i++) {
    TEST_ASSERT_EQUAL(expected[i], receivedPacket[i]);
  }
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(testUnknownBytesGetsForwardedImmediately);
  RUN_TEST(testSerialGetsRecordedAndIntercepted);
  RUN_TEST(testPacketCallback);
  UNITY_END();

  return 0;
}