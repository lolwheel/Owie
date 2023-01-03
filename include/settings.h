#ifndef SETTINGS_H
#define SETTINGS_H

#include "settings.pb.h"

extern SettingsMsg *Settings;

void loadSettings();
/**
 * @return Bytes written or -1 if writing failed.
 */
int32_t saveSettings();

int32_t saveSettingsAndRestartSoon();
/**
 * @brief Call befor OTA.
 */
void disableFlashPageRotation();

void nukeSettings();

#endif  // SETTINGS_H