#pragma once

#include <cmath>
#include <cstdint>

// Minimal host-side stand-in for Particle.h, scoped to what
// src/power/PowerPlatform.cpp and src/power/ChargeInhibit.cpp actually need
// (WO-2026-08-25-001 Decision C5 host test, AC-C12/AC-C13).
//
// This is the FIRST host test to compile PowerPlatform.cpp's
// HAL_PLATFORM_CELLULAR-gated branch (applyInputProfile()/
// baseConfigurationForProfile()) together with ChargeInhibit.cpp, so
// PLATFORM_ID/HAL_PLATFORM_CELLULAR are defined here to select the Boron
// path, and SystemPowerConfiguration/SystemPowerFeature/System.get|set
// PowerConfiguration() are modeled with enough fidelity to observe
// composition (the DISABLE_CHARGING bit specifically), not full Device OS
// fidelity - see IMPLEMENTATION_REPORT_R6.md for what this can and cannot
// prove (it cannot schedule or observe the real Device OS power-manager
// worker thread race; only the on-device AC-C14 check can).

constexpr int SYSTEM_ERROR_NONE = 0;

#define PLATFORM_BORON 88
#define PLATFORM_MSOM 32
#define PLATFORM_ID PLATFORM_BORON
#define HAL_PLATFORM_CELLULAR 1

struct TestLog {
  template <typename... Args>
  void info(const char *, Args...) {}

  template <typename... Args>
  void warn(const char *, Args...) {}
};
inline TestLog Log;

enum class SystemPowerFeature {
  DISABLE_CHARGING,
  USE_VIN_SETTINGS_WITH_USB_HOST,
};

// Models the fields applyInputProfile()/ChargeInhibit::apply() actually set,
// plus the single feature bit (DISABLE_CHARGING) this dispatch cares about.
// Chainable setters mirror the real Device OS SystemPowerConfiguration API
// surface used by production code.
class SystemPowerConfiguration {
 public:
  SystemPowerConfiguration &powerSourceMaxCurrent(int v) {
    powerSourceMaxCurrent_ = v;
    return *this;
  }
  SystemPowerConfiguration &powerSourceMinVoltage(int v) {
    powerSourceMinVoltage_ = v;
    return *this;
  }
  SystemPowerConfiguration &batteryChargeCurrent(int v) {
    batteryChargeCurrent_ = v;
    return *this;
  }
  SystemPowerConfiguration &batteryChargeVoltage(int v) {
    batteryChargeVoltage_ = v;
    return *this;
  }
  SystemPowerConfiguration &feature(SystemPowerFeature f) {
    if (f == SystemPowerFeature::DISABLE_CHARGING) disableCharging_ = true;
    return *this;
  }
  SystemPowerConfiguration &clearFeature(SystemPowerFeature f) {
    if (f == SystemPowerFeature::DISABLE_CHARGING) disableCharging_ = false;
    return *this;
  }
  bool isFeatureSet(SystemPowerFeature f) const {
    return f == SystemPowerFeature::DISABLE_CHARGING && disableCharging_;
  }

  int powerSourceMaxCurrent_ = 0;
  int powerSourceMinVoltage_ = 0;
  int batteryChargeCurrent_ = 0;
  int batteryChargeVoltage_ = 0;
  bool disableCharging_ = false;
};

// Test-controllable stand-in for Device OS's System object. Persists the
// last-written SystemPowerConfiguration so getPowerConfiguration() reflects
// setPowerConfiguration()'s effect - the read-modify-write composition under
// test (AC-C12/AC-C13) depends on this round trip.
class TestSystemClass {
 public:
  int setPowerConfiguration(const SystemPowerConfiguration &conf) {
    lastConfig_ = conf;
    setPowerConfigurationCallCount++;
    return SYSTEM_ERROR_NONE;
  }
  SystemPowerConfiguration getPowerConfiguration() const { return lastConfig_; }
  int powerSource() const { return 0; }

  int setPowerConfigurationCallCount = 0;

 private:
  SystemPowerConfiguration lastConfig_;
};
inline TestSystemClass System;

// Test-controllable stand-in for the BQ24195 PMIC driver surface.
// PowerPlatform.cpp's logAppliedProfileConfig() only calls
// isChargingEnabled() for diagnostics logging.
class PMIC {
 public:
  explicit PMIC(bool = false) {}
  bool isChargingEnabled() const { return true; }
};
