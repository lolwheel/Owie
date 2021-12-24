#include "recovery.h"

#include "EEPROM_Rotate.h"
#include "dprint.h"
#include "task_queue.h"

namespace {
EEPROM_Rotate getEeprom() {
  EEPROM_Rotate e;
  e.size(4);
  e.offset(SPI_FLASH_SEC_SIZE - 3);
  e.begin(SPI_FLASH_SEC_SIZE);
  return e;
}

void resetQuickPowerCycleCount() {
  int quickPowerCycleCount = 0;
  auto e = getEeprom();
  e.put(0, quickPowerCycleCount);
  e.commit();
  DPRINTF("Wrote QPC = %d\n", quickPowerCycleCount);
}
}  // namespace

bool isInRecoveryMode() {
  bool recovery = false;
  int quickPowerCycleCount;
  auto eeprom = getEeprom();
  eeprom.get(0, quickPowerCycleCount);
  DPRINTF("Read QPC = %d\n", quickPowerCycleCount);
  if (quickPowerCycleCount < 0 || quickPowerCycleCount > 3) {
    quickPowerCycleCount = 0;
  }
  if (quickPowerCycleCount > 1) {
    recovery = true;
    quickPowerCycleCount = 0;
  } else {
    quickPowerCycleCount++;
    // Reset in 10 seconds;
    TaskQueue.postOneShotTask(resetQuickPowerCycleCount, 10000L);
  }
  eeprom.put(0, quickPowerCycleCount);
  eeprom.commit();
  DPRINTF("Wrote QPC = %d\n", quickPowerCycleCount);
  return recovery;
}