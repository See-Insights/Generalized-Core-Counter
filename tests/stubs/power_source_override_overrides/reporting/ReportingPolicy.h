#pragma once

#include <cstdint>
#include <ctime>

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
};

namespace ReportingPolicyResolver {

inline ReportingPolicy resolveRuntime(float /*currentSoC*/, time_t /*nowEpoch*/) {
  return ReportingPolicy();
}

inline const char *adjustmentReasonName(ReportingAdjustmentReason /*reason*/) {
  return "none";
}

} // namespace ReportingPolicyResolver
