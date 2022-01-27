#include "bms_relay.h"

#include <unity.h>

#include <deque>
#include <memory>

#include "packet.h"

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
  relay->addPacketCallback([&](BmsRelay*, Packet* p) {
    const uint8_t* start = p->start();
    receivedPacket.assign(start, start + p->len());
  });
  relay->loop();
  std::vector<uint8_t> expected(
      {0xFF, 0x55, 0xAA, 0x6, 0x1, 0x2, 0x3, 0x4, 0x2, 0xE});
  TEST_ASSERT_EQUAL(expected.size(), receivedPacket.size());
  for (int i = 0; i < expected.size(); i++) {
    TEST_ASSERT_EQUAL(expected[i], receivedPacket[i]);
  }
}

void testBatteryPercentageParsing() {
  addMockData({0xFF, 0x55, 0xAA, 0x3, 0x2B, 0x02, 0x2C, 0xFF, 0x55, 0xAA});
  relay->loop();
  TEST_ASSERT_EQUAL(43, relay->getBatteryPercentage());
}

void testCurrentParsing() {
  addMockData({0xff, 0x55, 0xaa, 0x5, 0xff, 0xe8, 0x3, 0xea, 0xFF, 0x55, 0xAA});
  relay->loop();
  TEST_ASSERT_FLOAT_WITHIN(0.01, -1.2, relay->getCurrentInAmps());
}

void testCellVoltageParsing() {
  addMockData({0xff, 0x55, 0xaa, 0x02, 0x0f, 0x14, 0x0f, 0x14, 0x0f, 0x14, 0x0f,
               0x13, 0x0f, 0x14, 0x0f, 0x14, 0x0f, 0x14, 0x0f, 0x13, 0x0f, 0x14,
               0x0f, 0x13, 0x0f, 0x13, 0x0f, 0x13, 0x0f, 0x13, 0x0f, 0x14, 0x0f,
               0x14, 0x00, 0x2a, 0x04, 0x31, 0xFF, 0x55, 0xAA});
  relay->loop();
  uint16_t expected[15] = {3860, 3860, 3860, 3859, 3860, 3860, 3860, 3859,
                           3860, 3859, 3859, 3859, 3859, 3860, 3860};
  TEST_ASSERT_EQUAL_INT16_ARRAY(expected, relay->getCellMillivolts(), 15);
  TEST_ASSERT_EQUAL(57894, relay->getTotalVoltageMillivolts());
}

void testTemperatureParsing() {
  addMockData({0xff, 0x55, 0xaa, 0x04, 0x13, 0x14, 0x14, 0x14, 0x16, 0x02, 0xFF,
               0x55, 0xAA});
  relay->loop();
  int8_t expected[5] = {19, 20, 20, 20, 22};
  TEST_ASSERT_EQUAL_INT8_ARRAY(expected, relay->getTemperaturesCelsius(), 5);
  TEST_ASSERT_EQUAL(20, relay->getAverageTemperatureCelsius());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(testUnknownBytesGetsForwardedImmediately);
  RUN_TEST(testSerialGetsRecordedAndIntercepted);
  RUN_TEST(testPacketCallback);
  RUN_TEST(testBatteryPercentageParsing);
  RUN_TEST(testCurrentParsing);
  RUN_TEST(testCellVoltageParsing);
  UNITY_END();

  return 0;
}