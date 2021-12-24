#ifndef DPRINT_H
#define DPRINT_H

#ifdef DEBUG_ESP_PORT
#include "Arduino.h"

#define DPRINTF(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#define DPRINTLN(...) DEBUG_ESP_PORT.println(__VA_ARGS__)

#else
#define DPRINTF(...)
#define DPRINTLN(...)
#endif

#endif  // DPRINT_H