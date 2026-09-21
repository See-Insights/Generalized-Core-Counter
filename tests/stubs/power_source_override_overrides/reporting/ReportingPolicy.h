#pragma once

#include <cstdint>
#include <ctime>

// See MyPersistentData.h in this override directory for why the real,
// pure, dependency-free cloud/BatteryBackoffPolicy.h is included directly
// rather than reimplemented (WO-2026-09-21 Step 4 bench telemetry adds
// batteryTier/batteryTierName(), which need the real BatteryTier enum).
#include "cloud/BatteryBackoffPolicy.h"

// Minimal host-side stand-in for reporting/ReportingPolicy.h. The real
// resolver pulls in battery-backoff and connectivity-window policy modules
// that are unrelated to the power-source telemetry wiring under test here;
// only the fields/functions src/cloud/DeviceStatusPublisher.cpp actually
// reads are reproduced, with fixed, test-controlled values.

enum ReportingAdjustmentReason : uint8_t {
  REPORTING_ADJUSTMENT_NONE = 0,
  REPORTING_ADJUSTMENT_LOW_BATTERY = 1,
};

struct ReportingPolicy {
  uint32_t configuredIntervalSec = 3600;
  uint32_t effectiveIntervalSec = 3600;
  time_t nextReportEpoch = 0;
  bool windowOpen = true;
  ReportingAdjustmentReason adjustmentReason = REPORTING_ADJUSTMENT_NONE;
  BatteryTier batteryTier = TIER_HEALTHY;
};

namespace ReportingPolicyResolver {

inline ReportingPolicy resolveRuntime(float /*currentSoC*/, time_t /*nowEpoch*/) {
  return ReportingPolicy();
}

inline const char *adjustmentReasonName(ReportingAdjustmentReason /*reason*/) {
  return "none";
}

inline const char *batteryTierName(BatteryTier tier) {
  switch (tier) {
    case TIER_CONSERVING:
      return "CONSERVING";
    case TIER_CRITICAL:
      return "CRITICAL";
    case TIER_SURVIVAL:
      return "SURVIVAL";
    case TIER_HEALTHY:
    default:
      return "HEALTHY";
  }
}

} // namespace ReportingPolicyResolver
