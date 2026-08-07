#pragma once

#include "power/PowerManager.h"

namespace PowerPlatform {

struct PowerSourceSnapshot {
  int source = -1;
  PowerAvailability status = PowerAvailability::Unknown;
};

struct PowerConfigurationApplyResult {
  bool applied = true;
  int systemResult = SYSTEM_ERROR_NONE;
};

inline PowerCapabilities detectCapabilities() { return {}; }
inline PowerSourceSnapshot readPowerSource() { return {}; }
inline PowerConfigurationApplyResult applyInputProfile(PowerInputProfile) { return {}; }

} // namespace PowerPlatform