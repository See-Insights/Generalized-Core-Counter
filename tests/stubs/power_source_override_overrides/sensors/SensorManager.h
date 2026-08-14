#pragma once

// Minimal host-side stand-in for SensorManager. The real class integrates
// with on-device sensor hardware and is out of scope for the
// DeviceStatusPublisher telemetry-wiring test; only the two accessors that
// src/cloud/DeviceStatusPublisher.cpp actually calls are provided.
class SensorManager {
 public:
  static SensorManager &instance() {
    static SensorManager inst;
    return inst;
  }

  bool cachedBatteryVoltage(float &vcell) const {
    vcell = cachedBatteryVoltageValue;
    return true;
  }

  const char *cachedChargeStateLabel() const { return cachedChargeStateLabelValue; }

  float cachedBatteryVoltageValue = 3.7f;
  const char *cachedChargeStateLabelValue = "UNKNOWN";
};
