#pragma once

#include <ctime>

// Minimal host-side stand-in for state/State_Common.h, scoped to the one
// function src/reporting/RuntimeReportingPolicy.cpp calls.

inline bool testWindowOpen = true;

inline bool isWithinOpenHoursAt(time_t) { return testWindowOpen; }
