#pragma once

#include <cstdint>

// Narrow read seam between the reporting adapter (RuntimeReportingPolicy.cpp)
// and the persistence bag - see WO-2026-09-14-001. Declares only the one
// accessor the adapter needs (sysStatus.get_currentBatteryTier(), in
// production), so this header - and anything that includes only it - stays
// host-compilable without pulling in MyPersistentData.h or its
// StorageHelperRK.h dependency.
//
// This header must never include MyPersistentData.h or StorageHelperRK.h;
// the implementation (BatteryTierStore.cpp) does that instead.
// tests/battery_tier_store_seam_structural_test.py enforces both halves of
// this contract against the real shipped source.
//
// Non-relative include: unlike MyPersistentData.h and Config.h, this name is
// unique and does not collide with a Device OS header, so callers use a
// plain #include "reporting/BatteryTierStore.h" - no relative-path
// workaround needed, and a test's -I override directory can shadow it (a
// relative include cannot be shadowed this way, which is what broke
// tests/reporting_policy_adapter_test.sh from 2026-08-28 onward).

namespace BatteryTierStore {

uint8_t currentBatteryTier();

} // namespace BatteryTierStore
