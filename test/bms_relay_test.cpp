#include "bms_relay.h"

#include <unity.h>

#include <deque>
#include <memory>

std::unique_ptr<BmsRelay> relay;
std::deque<int> mockBmsData;
std::vector<int> mockDataOut;

void setUp(void) {
  relay.reset(new BmsRelay([&](void) { return mockBmsData.pop_front(); },
                           [&](int8_t b) { mockDataOut.push_back(b); }));
  mockBmsData.clear();
  mockDataOut.clear();
}

void testNothing(void) { TEST_ASSERT_EQUAL(42, 42); }

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(testNothing);
  UNITY_END();

  return 0;
}