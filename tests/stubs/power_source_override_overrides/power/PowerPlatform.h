#pragma once

#include "power/PowerManager.h"

// Mutable host-side stand-in for PowerPlatform, letting tests control
// detectCapabilities()/readPowerSource()/applyInputProfile() return values
// per-scenario (unlike the plain tests/stubs/power/PowerPlatform.h, which
// always reports hasPmicPowerConfiguration=false and therefore never
// reaches PowerManager::refreshInputProfile()'s override block).
namespace PowerPlatform {

struct PowerSourceSnapshot {
  int source = -1;
  PowerAvailability status = PowerAvailability::Unknown;
};

struct PowerConfigurationApplyResult {
  bool applied = true;
  int systemResult = SYSTEM_ERROR_NONE;
};

struct TestPowerPlatformState {
  PowerCapabilities capabilities;
  PowerSourceSnapshot snapshot;
  PowerConfigurationApplyResult applyResult;
};

extern TestPowerPlatformState testPowerPlatformState;

inline PowerCapabilities detectCapabilities() {
  return testPowerPlatformState.capabilities;
}

inline PowerSourceSnapshot readPowerSource() {
  return testPowerPlatformState.snapshot;
}

inline PowerConfigurationApplyResult applyInputProfile(PowerInputProfile) {
  return testPowerPlatformState.applyResult;
}

} // namespace PowerPlatform
