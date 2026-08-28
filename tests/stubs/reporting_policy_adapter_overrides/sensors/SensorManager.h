#pragma once

#include <cstdint>

#include "power/BatteryHealth.h"

// Host-side stand-in for sensors/SensorManager.h, scoped to exactly the
// surface src/reporting/RuntimeReportingPolicy.cpp (the production adapter)
// calls: cachedBatteryVoltageState() and cachedSocTrust(). The
// VcellSampleState enum mirrors the real SensorManager::VcellSampleState
// (WO-2026-08-25-001 Amendment C, Decision C2 / AC-C6) exactly, so a test
// driving this stub is exercising the same three-way branch the adapter
// actually compiles against in production.
class SensorManager {
 public:
  enum class VcellSampleState : uint8_t { Known, Invalid, Unavailable };

  static SensorManager &instance() {
    static SensorManager inst;
    return inst;
  }

  VcellSampleState cachedBatteryVoltageState(float &vcell) const {
    vcell = testVcell;
    return testVcellState;
  }

  BatteryHealth::SocTrust cachedSocTrust() const { return testTrust; }

  VcellSampleState testVcellState = VcellSampleState::Unavailable;
  float testVcell = 0.0f;
  BatteryHealth::SocTrust testTrust = BatteryHealth::SocTrust::Trusted;
};
