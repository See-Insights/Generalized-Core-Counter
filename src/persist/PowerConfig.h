#pragma once

/**
 * @file PowerConfig.h
 * @brief Facade: persisted power configuration - solar, tier, low-battery,
 *        and the F2a thermal charge-inhibit overrides.
 *
 * WO-2026-09-23-001 (Step 5). Measured power values (state of charge,
 * battery state, enclosure temperature) are readings, not configuration, and
 * live in CurrentReadings.h.
 *
 * INCLUDES ARE THE POINT OF THIS HEADER. It must never include
 * StorageHelperRK.h, MyPersistentData.h, Particle.h, or a sibling facade.
 * Note it also does not include power/ChargeInhibitPolicy.h: the four
 * thermal accessors are plain floats, exactly as today, so no consumer is
 * forced to take on the policy type to read one threshold. The
 * resolveThermalThresholds() set-validity fallback stays behind these
 * accessors, unchanged and still private to MyPersistentData.cpp.
 */

#include <stdint.h>

namespace PowerConfig {

// *************** Power mode configuration ***************

bool get_solarPowerMode();
void set_solarPowerMode(bool value);

/// Legacy flag - kept for storage compatibility.
bool get_lowPowerMode();
void set_lowPowerMode(bool value);

bool get_lowBatteryMode();
void set_lowBatteryMode(bool value);

// *************** Battery-aware back-off ***************

/// BatteryTier enum value (0 = HEALTHY, 1 = CONSERVING, 2 = CRITICAL, 3 = SURVIVAL).
/// Exposed as uint8_t, as persisted, so this header needs no policy type.
uint8_t get_currentBatteryTier();
void set_currentBatteryTier(uint8_t value);

// *************** F2a thermal charge-inhibit overrides ***************
// Getters resolve the stored set as a whole and fall back atomically to the
// compiled default set (37/0/35/3) when it is invalid - behavior unchanged
// from sysStatusData::resolveThermalThresholds().

float get_thermalChargeArmHighC();
void set_thermalChargeArmHighC(float value);

float get_thermalChargeArmLowC();
void set_thermalChargeArmLowC(float value);

float get_thermalChargeReleaseHighC();
void set_thermalChargeReleaseHighC(float value);

float get_thermalChargeReleaseLowC();
void set_thermalChargeReleaseLowC(float value);

}  // namespace PowerConfig
