#pragma once
#include <stdint.h>
#include <time.h>
// WO-2026-10-17-001: the unvalidated retained boot-storm and breadcrumb state. Member order pins the layout.
constexpr uint32_t kRetainedBootMagic = 0x52424F31UL; constexpr uint8_t kRetainedBootVersion = 1;
struct RetainedBoot {
  uint32_t magic; uint8_t version;
  uint8_t bootStormCount, bootStormLastResetReason, bootStormTripCount;
  time_t bootStormWindowStart;
  uint32_t bootStormLastResetReasonData, appBreadcrumbMs;
  uint8_t appBreadcrumb; bool bootInProgress, bootStormAlertPending;
};
// Wrong magic or version: every member returns to its initial value (all zero; BREADCRUMB_NONE is 0).
inline void validateRetainedBoot(RetainedBoot &r) {
  if (r.magic == kRetainedBootMagic && r.version == kRetainedBootVersion) return;
  r = RetainedBoot();
  r.magic = kRetainedBootMagic; r.version = kRetainedBootVersion;
}
