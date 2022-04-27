#include "settings.h"

#include <Esp.h>

#include "EEPROM_Rotate.h"
#include "dprint.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "spi_flash_geometry.h"
#include "task_queue.h"

namespace {
SettingsMsg __settings = SettingsMsg_init_default;

SettingsMsg DEFAULT_SETTINGS = SettingsMsg_init_default;

// 3 bytes needed by EEPROM_Rotate + 2 byte proto message size
const size_t MAX_SETTINGS_SIZE = SPI_FLASH_SEC_SIZE - 5;

EEPROM_Rotate& getEeprom() {
  static EEPROM_Rotate e;
  static bool initialized = false;
  if (!initialized) {
    e.size(4);
    e.offset(MAX_SETTINGS_SIZE);
    e.begin(SPI_FLASH_SEC_SIZE);
    initialized = true;
  }
  return e;
}
}  // namespace

SettingsMsg *Settings = &__settings;

void sanitizeSettings() {
  // check the wifi power Setting and write back a sane default if is out of bounds
  // the defined sane range is between 8dBm and 17dBm. Lower values may prevent the WLAN to show up under the batterybox
  // and a to high setting may generate signal noise.
  // defaulting to 9 brings up the WLAN with a decent range of ~1-2 meters on PINTS and ~1 meter on a XR with an acceptable signal strength.
  if (Settings->wifi_power < 8 || Settings->wifi_power > 17) {
    Settings->wifi_power = 9;
  }
}

void loadSettings() {
  auto& e = getEeprom();
  uint16_t len = *(uint16_t*)e.getConstDataPtr();
  if (len <= MAX_SETTINGS_SIZE) {
    auto istream =
        pb_istream_from_buffer(getEeprom().getConstDataPtr() + 2, len);
    if (pb_decode(&istream, &SettingsMsg_msg, Settings)) {
      DPRINTF("Read and decoded settings, size = %d bytes.", len);
      sanitizeSettings();
      return;
    }
  }
  DPRINTLN("Failed to decode settings, resetting.");
  nukeSettings(); // nukeSettings() calls saveSettings()
}

int32_t saveSettings() {
  auto& e = getEeprom();
  auto stream = pb_ostream_from_buffer(e.getDataPtr() + 2, MAX_SETTINGS_SIZE);
  if (!pb_encode(&stream, &SettingsMsg_msg, Settings)) {
    DPRINTLN("Failed to encode settings.");
    return -1;
  }
  *(int16_t*)e.getDataPtr() = stream.bytes_written;
  e.commit();
  DPRINTF("Serialized settings, size = %d bytes.", stream.bytes_written);
  return stream.bytes_written;
}

int32_t saveSettingsAndRestartSoon() {
  int32_t code = saveSettings();
  TaskQueue.postOneShotTask([]() { ESP.restart(); }, 2000L);
  return code;
}

void disableFlashPageRotation() { getEeprom().rotate(false); }

void nukeSettings() {
  *Settings = DEFAULT_SETTINGS;
  sanitizeSettings();
  saveSettings();
}