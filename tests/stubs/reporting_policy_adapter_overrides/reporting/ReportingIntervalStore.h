#pragma once

#include <cstdint>

#include "Config.h"

// Minimal host-side stand-in for src/reporting/ReportingIntervalStore.h,
// scoped to the one accessor src/reporting/RuntimeReportingPolicy.cpp calls
// (reportingIntervalSec(), backed in production by
// Config::reportingIntervalSecForRuntime()). Delegates to this directory's
// existing Config.h stub so Config::testReportingIntervalSec remains the
// one place a test drives this value - no change needed to
// tests/reporting_policy_adapter_test.cpp's resetGlobals().

namespace ReportingIntervalStore {

inline uint16_t reportingIntervalSec() { return Config::reportingIntervalSecForRuntime(); }

} // namespace ReportingIntervalStore
