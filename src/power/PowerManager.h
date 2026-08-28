#pragma once

#include "Particle.h"

/**
 * @file PowerManager.h
 * @brief Platform-neutral power telemetry, classification, and remediation interfaces.
 */

/**
 * @brief Indicates whether a power-related reading is available and trustworthy.
 */
enum class PowerAvailability : uint8_t {
  Valid,
  Unknown,
  NotAvailable,
  Fallback,
};

/**
 * @brief Normalized battery charge context across supported platforms.
 */
enum class PowerBatteryContext : uint8_t {
  Unknown = 0,
  NotCharging = 1,
  Charging = 2,
  Charged = 3,
  Discharging = 4,
  Fault = 5,
  Disconnected = 6,
  NotApplicable = 7,
};

/**
 * @brief Input-power profile to apply for PMIC-aware platforms.
 */
enum class PowerInputProfile : uint8_t {
  UsbBench,
  Solar35W,
  Auto,
  NotApplicable,
};

/**
 * @brief Why a given input-power profile was selected.
 */
enum class PowerProfileSelectionReason : uint8_t {
  Unknown,
  UsbPowerSource,
  VinPowerSource,
  BatteryKeepLast,
  BatteryFallback,
  UnknownSourceKeepLast,
  UnknownSourceFallback,
  UnsupportedPlatform,
};

/**
 * @brief Hardware power-management capabilities detected at runtime.
 */
struct PowerCapabilities {
  bool hasFuelGauge = false;
  bool hasPmic = false;
  bool hasSoc = false;
  bool hasBatteryVoltage = false;
  bool hasBatteryContext = false;
  bool hasPowerSource = false;
  bool hasChargingControl = false;
  bool hasPmicPowerConfiguration = false;
  bool hasPmicRemediation = false;
};

/**
 * @brief Snapshot of raw power telemetry collected for the current wake cycle.
 */
struct PowerReading {
  float soc = NAN;
  PowerAvailability socStatus = PowerAvailability::Unknown;

  float batteryVoltage = NAN;
  PowerAvailability batteryVoltageStatus = PowerAvailability::Unknown;

  PowerBatteryContext batteryContext = PowerBatteryContext::Unknown;
  PowerAvailability batteryContextStatus = PowerAvailability::Unknown;

  int powerSource = -1;
  PowerAvailability powerSourceStatus = PowerAvailability::Unknown;

  bool sampledPreRadio = false;
  bool quickStartUsed = false;
  bool fallbackUsed = false;
  uint8_t stabilizationAttempts = 0;

  /**
   * @brief True only on cycles where the Boron USB source override fired
   * (i.e. actually corrected the raw power source this cycle). Distinct
   * from fallbackUsed, which tracks unrelated fallback behavior.
   */
  bool overrideActive = false;
};

/**
 * @brief Charging-control action requested by the latest policy evaluation.
 */
struct PowerAction {
  bool chargingControlAvailable = false;
  bool shouldEnableCharging = true;
  bool chargingActionNeeded = false;
};

/**
 * @brief Consolidated report returned by the power manager.
 */
struct PowerReport {
  bool valid = false;
  PowerCapabilities capabilities;
  PowerReading reading;
  PowerAction action;
  PowerInputProfile activeInputProfile = PowerInputProfile::NotApplicable;
  PowerProfileSelectionReason inputProfileReason =
      PowerProfileSelectionReason::Unknown;
  PowerAvailability inputProfileStatus = PowerAvailability::Unknown;
  int powerConfigurationResult = 0;
};

/**
 * @brief Wake-context inputs used when sampling and classifying power state.
 */
struct PowerInputs {
  float enclosureTempC = NAN;
  bool wokeFromLowPowerSleep = false;
  bool radioIsOn = false;
};

/**
 * @brief Singleton that samples platform power telemetry and produces policy guidance.
 */
class PowerManager {
public:
  /**
   * @brief Returns the singleton power manager instance.
   *
   * @return Reference to the shared PowerManager
   */
  static PowerManager &instance();

  /**
   * @brief Detects power capabilities and prepares the manager for use.
   *
   * @return true when setup completed successfully
   */
  bool setup();

  /**
   * @brief Re-evaluates the active input profile and latest power report.
   *
   * @return true when a fresh report is available
   */
  bool refreshInputProfile();

  /**
   * @brief Returns the most recent consolidated power report.
   *
   * @return Reference to the latest PowerReport
   */
  const PowerReport &latestReport() const;

  /**
   * @brief Returns the detected hardware power capabilities.
   *
   * @return Reference to the capabilities snapshot
   */
  const PowerCapabilities &capabilities() const;

  /**
   * @brief Returns whether the current platform exposes a PMIC.
   *
   * @return true when PMIC support is available
   */
  bool hasPmic() const;

  /**
   * @brief Returns whether the current platform exposes a fuel gauge.
   *
   * @return true when fuel-gauge telemetry is available
   */
  bool hasFuelGauge() const;

  /**
   * @brief Returns the currently accepted battery state of charge.
   *
   * This is a live ledger pass-through. The cached PowerReport SOC and battery
   * context remain unpopulated in Phase 1.
   *
   * @return Accepted state of charge percentage
   */
  float soc() const;

  /**
   * @brief Returns the currently accepted raw battery-state value.
   *
   * @return Raw battery-state value from the persistent status ledger
   */
  uint8_t batteryState() const;

  /**
   * @brief Converts availability enum values to stable log labels.
   *
   * @param availability Availability enum to stringify
   * @return Stable C string label
   */
  static const char *availabilityLabel(PowerAvailability availability);

  /**
   * @brief Converts battery-context enum values to stable log labels.
   *
   * @param context Battery context enum to stringify
   * @return Stable C string label
   */
  static const char *batteryContextLabel(PowerBatteryContext context);

  /**
   * @brief Converts input-profile enum values to stable log labels.
   *
   * @param profile Input profile enum to stringify
   * @return Stable C string label
   */
  static const char *inputProfileLabel(PowerInputProfile profile);

  /**
   * @brief Converts platform power-source identifiers to a stable log label.
   *
   * @param powerSource Raw platform power-source identifier
   * @return Stable C string label
   */
  static const char *powerSourceLabel(int powerSource);

  /**
   * @brief Converts profile-selection reasons to stable log labels.
   *
   * @param reason Profile selection reason enum to stringify
   * @return Stable C string label
   */
  static const char *profileSelectionReasonLabel(
      PowerProfileSelectionReason reason);

  /**
   * @brief Converts power input profile to compact log label.
   *
   * @param profile Profile enum to stringify
   * @return Stable C string label (e.g. "USB", "SOLAR")
   */
  static const char *compactProfileLabel(PowerInputProfile profile);

private:
  PowerManager();

  PowerReport report_;
  bool setupComplete_;
};