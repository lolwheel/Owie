#include "bms_relay.h"

#include <unity.h>

#include <deque>
#include <memory>

std::unique_ptr<BmsRelay> relay;
std::deque<int> mockBmsData;
std::vector<int> mockDataOut;

void setUp(void) {
  relay.reset(new BmsRelay(
      [&]() {
        int data = mockBmsData.back();
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

void testUnknownByteGetsForwardedImmediately(void) {
  addMockData({0x1});
  relay->loop();
  TEST_ASSERT_TRUE(mockBmsData.empty());
  TEST_ASSERT_EQUAL(1, mockDataOut.size());
  TEST_ASSERT_EQUAL(0x1, mockDataOut.back());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(testUnknownByteGetsForwardedImmediately);
  UNITY_END();

  return 0;
}