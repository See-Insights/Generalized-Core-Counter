#pragma once

#include <cstdint>

// Minimal host-side stand-in for ../Config.h, scoped to the one function
// src/reporting/RuntimeReportingPolicy.cpp calls.

namespace Config {

inline uint16_t testReportingIntervalSec = 3600;

inline uint16_t reportingIntervalSecForRuntime() { return testReportingIntervalSec; }

} // namespace Config
