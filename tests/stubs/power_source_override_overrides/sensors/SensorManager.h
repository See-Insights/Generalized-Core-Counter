#pragma once

// WO-2026-09-21 Step 4 (bench telemetry): the real header declares
// BatteryHealth::SocTrust (power/BatteryHealth.h). That header is pure,
// dependency-free enum/inline-math like cloud/BatteryBackoffPolicy.h below,
// so it's included directly rather than reimplemented.
#include "power/BatteryHealth.h"

// Minimal host-side stand-in for SensorManager. The real class integrates
// with on-device sensor hardware and is out of scope for the
// DeviceStatusPublisher telemetry-wiring test; only the accessors that
// src/cloud/DeviceStatusPublisher.cpp actually calls are provided.
class SensorManager {
 public:
  enum class VcellSampleState : uint8_t { Known, Invalid, Unavailable };

  static SensorManager &instance() {
    static SensorManager inst;
    return inst;
  }

  bool cachedBatteryVoltage(float &vcell) const {
    vcell = cachedBatteryVoltageValue;
    return true;
  }

  VcellSampleState cachedBatteryVoltageState(float &vcell) const {
    vcell = cachedBatteryVoltageValue;
    return cachedBatteryVoltageStateValue;
  }

  const char *cachedChargeStateLabel() const { return cachedChargeStateLabelValue; }

  BatteryHealth::SocTrust cachedSocTrust() const { return cachedSocTrustValue; }

  float cachedBatteryVoltageValue = 3.7f;
  VcellSampleState cachedBatteryVoltageStateValue = VcellSampleState::Known;
  const char *cachedChargeStateLabelValue = "UNKNOWN";
  BatteryHealth::SocTrust cachedSocTrustValue = BatteryHealth::SocTrust::Trusted;
};
