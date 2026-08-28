#pragma once

#include <cstdint>

// Minimal host-side stand-in for the ../MyPersistentData.h that
// src/reporting/RuntimeReportingPolicy.cpp includes. Only the one accessor
// the adapter actually calls (sysStatus.get_currentBatteryTier()) is
// provided.

struct TestSystemStatus {
  uint8_t currentBatteryTier = 0;

  uint8_t get_currentBatteryTier() const { return currentBatteryTier; }
};

inline TestSystemStatus testSysStatus;

#define sysStatus testSysStatus
