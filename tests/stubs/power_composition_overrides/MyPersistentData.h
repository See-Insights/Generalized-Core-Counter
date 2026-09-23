#pragma once

#include <cstdint>

// Minimal host-side stand-in for MyPersistentData.h, scoped to what
// power_composition_test.cpp itself needs.
//
// WO-2026-09-23-001 (Step 5): this stub used to REPLICATE the real
// `current`/`sysStatus` macros, to keep PowerPlatform.cpp's
// applyDisableChargingBit() naming hazard (WO-2026-08-25-001 Decision C5)
// live on host. That hazard no longer exists - PowerPlatform.cpp does not
// include the persistence header at all now, so nothing #defines `current`
// in its translation unit - and the macros are gone with it.

struct TestCurrentStatus {
  float socValue = 0.0f;
};

struct TestSystemStatus {
  uint8_t currentBatteryTier = 0;
};

extern TestCurrentStatus testCurrent;
extern TestSystemStatus testSysStatus;

