#pragma once
#include <stdint.h>

constexpr unsigned long MODEM_UNSTABLE_SLOW_TEARDOWN_MS = 10000UL;

enum ModemUnstableReason : uint8_t {
  MODEM_UNSTABLE_REASON_NONE = 0,
  MODEM_UNSTABLE_REASON_SLOW_TEARDOWN = 1,
  MODEM_UNSTABLE_REASON_CONNECT_TIMEOUT = 2,
};

void markModemUnstable(ModemUnstableReason reason, unsigned long elapsedMs);
void maybeClearModemUnstable(unsigned long teardownElapsedMs);
void noteConnectTimeout();
void noteConnectSuccess();
void clearAtDailyClose();
