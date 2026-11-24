// WO-2026-11-09-001: the real storm switch (extracted from setup() by the .sh) over every real RESET_REASON_* enumerator.
#include <cstdio>
#include <cstring>

#include "decision.h"

int main() {
  int failures = 0, rows = 0;
#define ROW(name) { \
    ++rows; \
    const bool want = std::strcmp(#name, "RESET_REASON_NONE") == 0 || std::strcmp(#name, "RESET_REASON_PANIC") == 0 || \
                      std::strcmp(#name, "RESET_REASON_WATCHDOG") == 0 || std::strcmp(#name, "RESET_REASON_UNKNOWN") == 0; \
    if (stormReasonQualifies(name) != want) { std::printf("FAIL: %s (%d) qualifies=%d, want %d\n", #name, (int)name, !want, want); failures++; } }
#include "reasons.h"
#undef ROW

  for (int reason = -1; reason < 256; reason++) {
    const bool want = reason == RESET_REASON_NONE || reason == RESET_REASON_PANIC || reason == RESET_REASON_WATCHDOG ||
                      reason == RESET_REASON_UNKNOWN;
    if (stormReasonQualifies(reason) != want) { std::printf("FAIL: integer reason %d\n", reason); failures++; }
  }

  if (failures == 0) std::printf("PASS storm_reason_qualifies_test (%d enumerators)\n", rows);
  return failures == 0 ? 0 : 1;
}
