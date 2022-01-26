#include "bms_relay.h"
#include "packet.h"

void bmsSerialParser(BmsRelay* relay, Packet* p) {
  if (p->getType() != 6) {
    return;
  }
  // 0x6 message has the BMS serial number encoded inside of it.
  // It is the last seven digits from the sticker on the back of the BMS.
  if (p->dataLength() != 4) {
    return;
  }
  if (relay->captured_serial_ == 0) {
    for (int i = 0; i < 4; i++) {
      relay->captured_serial_ |= p->data()[i] << (8 * (3 - i));
    }
  }
  if (relay->serial_override_ == 0) {
    return;
  }
  uint32_t serial_override_copy = relay->serial_override_;
  for (int i = 3; i >= 0; i--) {
    p->data()[i] = serial_override_copy & 0xFF;
    serial_override_copy >>= 8;
  }
}
