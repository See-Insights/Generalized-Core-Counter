#pragma once

#include <cstdint>

// Minimal host-side stand-in for src/reporting/BatteryTierStore.h, scoped to
// the one accessor src/reporting/RuntimeReportingPolicy.cpp calls
// (currentBatteryTier(), backed in production by
// sysStatus.get_currentBatteryTier()). Supersedes the MyPersistentData.h
// stub previously in this directory (WO-2026-09-14-001) now that
// RuntimeReportingPolicy.cpp depends on this narrow seam instead of the
// whole persistence bag.

namespace BatteryTierStore {

inline uint8_t testCurrentBatteryTier = 0;

inline uint8_t currentBatteryTier() { return testCurrentBatteryTier; }

} // namespace BatteryTierStore
