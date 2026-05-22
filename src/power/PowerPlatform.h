#pragma once

#include "power/PowerManager.h"

namespace PowerPlatform {

/**
 * @brief Snapshot of the platform's current power-source reading.
 */
struct PowerSourceSnapshot {
	int source = -1;
	PowerAvailability status = PowerAvailability::NotAvailable;
};

/**
 * @brief Result of applying a PMIC or input-power profile on the active platform.
 */
struct PowerConfigurationApplyResult {
	bool supported = false;
	bool applied = false;
	int systemResult = 0;
};

/**
 * @brief Detects power-management capabilities supported by the active platform.
 *
 * @return Capability snapshot for the current device
 */
PowerCapabilities detectCapabilities();

/**
 * @brief Returns whether the active platform has a PMIC.
 *
 * @return true when PMIC APIs are available
 */
bool hasPmic();

/**
 * @brief Returns whether the active platform has a fuel gauge.
 *
 * @return true when fuel-gauge APIs are available
 */
bool hasFuelGauge();

/**
 * @brief Records the last observed platform power source for later policy decisions.
 *
 * @param source Raw platform-specific power-source identifier
 */
void noteObservedPowerSource(int source);

/**
 * @brief Reads the current platform power source.
 *
 * @return Snapshot containing the source identifier and availability
 */
PowerSourceSnapshot readPowerSource();

/**
 * @brief Applies the requested input-power profile when the platform supports it.
 *
 * @param profile Input profile to apply
 * @return Result describing support, whether the change was applied, and system status
 */
PowerConfigurationApplyResult applyInputProfile(PowerInputProfile profile);

} // namespace PowerPlatform