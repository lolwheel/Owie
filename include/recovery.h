#ifndef RECOVERY_H
#define RECOVERY_H

/**
 * @brief If the device has been turned on and off 3 times
 * with less than 10 seconds between the cycles, returns true.
 */
bool isInRecoveryMode();

#endif /* RECOVERY_H */