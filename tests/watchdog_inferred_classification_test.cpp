// v50 S2 (WO-2026-11-05-004, amended by WO-2026-11-08-001): the reason-20 (PIN_RESET) boot is classified from the
// persisted evidence byte only. Runs the real HibernateWakeDiagnostics::isInferredWatchdog() (header-only) and the
// REAL boot classification block extracted from setup() (BOOT_BLOCK_H, see the .sh) against a fake persisted byte.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include "time/HibernateWakeDiagnostics.h"

using HibernateWakeDiagnostics::isInferredWatchdog;
using HibernateWakeDiagnostics::kPowerOffSentinel;

#define RESET_REASON_PIN_RESET 20
#define RESET_REASON_WATCHDOG 10
struct LogFake { template <typename... A> void info(const char *, A...) {} } Log;
static uint8_t persistedByte = 0;  // survives "boots" like the flash-backed record
static int flushes = 0;
static bool flushedWithByteClear = false;
namespace RecoveryState {
inline uint8_t get_powerOffRequested() { return persistedByte; }
inline void set_powerOffRequested(uint8_t v) { persistedByte = v; }
} // namespace RecoveryState
namespace SystemConfig {
inline void flushNow() { flushes++; flushedWithByteClear = (persistedByte == 0); }
} // namespace SystemConfig
static bool startupPinResetChecked, startupInferredWatchdog;

// Returns inferredWatchdog for one boot with the given OS reset reason.
static bool boot(int reason) {
#include BOOT_BLOCK_H
  return inferredWatchdog;
}

int main() {
  // Only the exact sentinel is evidence: 0, the legacy structuresVersion 2, 1 and other values are "no record".
  assert(kPowerOffSentinel == 0xA5);
  assert(!isInferredWatchdog(kPowerOffSentinel));
  for (int v : {0, 1, 2, 0x5A, 0xA4, 0xA6, 0xFF}) assert(isInferredWatchdog((uint8_t)v));

  // Two-boot consumption: a power-up after power-off consumes the evidence; the next reason-20 is a real watchdog.
  persistedByte = kPowerOffSentinel; flushes = 0;
  assert(!boot(RESET_REASON_PIN_RESET));           // boot 1: power-up wake
  assert(persistedByte == 0 && flushes == 1 && flushedWithByteClear);
  assert(startupPinResetChecked && !startupInferredWatchdog);
  assert(boot(RESET_REASON_PIN_RESET));            // boot 2: no evidence left -> inferred watchdog
  assert(startupInferredWatchdog && persistedByte == 0 && flushes == 2);

  // A non-PIN_RESET boot never classifies but still consumes (clears and flushes) the byte.
  startupPinResetChecked = false; persistedByte = kPowerOffSentinel; flushes = 0;
  assert(!boot(RESET_REASON_WATCHDOG) && !startupPinResetChecked && persistedByte == 0 && flushes == 1);
  puts("watchdog_inferred_classification_test: ok");
  return 0;
}
