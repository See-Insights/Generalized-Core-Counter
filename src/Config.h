/**
 * @file Config.h
 * @brief Runtime configuration defaults and validation entry points.
 *
 * @details
 * Build-profile flags live in BuildProfile.h; connectivity budgets live in
 * power/ConnectivityPolicy.h (which includes BuildProfile.h itself). The
 * connection defaults below are aliases of the ConnectivityPolicy values, not
 * second copies of them.
 *
 * Safety note:
 * - CONNECTIVITY_FAILSAFE_TEST_MODE is a compile-time build-profile flag.
 * - It must not be sourced from cloud config and should not be toggled here.
 */

#ifndef GENERALIZED_CORE_COUNTER_CONFIG_H
#define GENERALIZED_CORE_COUNTER_CONFIG_H

#include <stdint.h>
#include "power/ConnectivityPolicy.h"

namespace Config {

enum Source : uint8_t {
	CONFIG_SOURCE_DEFAULT = 0,
	CONFIG_SOURCE_STORAGE = 1,
	CONFIG_SOURCE_LEDGER = 2,
};

constexpr const char *DEFAULT_TIMEZONE = "UTC0";
constexpr uint8_t DEFAULT_OPEN_HOUR = 6;
constexpr uint8_t DEFAULT_CLOSE_HOUR = 22;
constexpr uint16_t DEFAULT_REPORT_INTERVAL_SEC = 3600;
constexpr uint32_t DEFAULT_OCCUPANCY_DEBOUNCE_MS = 60000UL;
constexpr uint16_t DEFAULT_CONNECT_ATTEMPT_BUDGET_SEC =
	(uint16_t)(ConnectivityPolicy::CONNECT_BUDGET_DEFAULT_MS / 1000UL);
constexpr uint16_t DEFAULT_CLOUD_DISCONNECT_BUDGET_SEC = ConnectivityPolicy::DISCONNECT_CLOUD_DEFAULT_SEC;
constexpr uint16_t DEFAULT_MODEM_OFF_BUDGET_SEC = ConnectivityPolicy::DISCONNECT_MODEM_DEFAULT_SEC;

// WO-2026-09-24-004: the three open/close hour rules, written once.
//   1. Always-open is exactly openHour = 0, closeHour = 24.
//   2. closeHour > openHour (no overnight windows).
//   3. 0 <= openHour <= 12.
// Returns the rule broken, for logging, or nullptr when the pair is valid.
inline const char *hoursRuleFailure(int openHour, int closeHour) {
	if (openHour < 0 || openHour > 12) {
		return "rule 3: openHour must be 0-12";
	}
	if (closeHour > 24) {
		return "combined hour bounds: closeHour must be at most 24";
	}
	if (closeHour <= openHour) {
		return "rule 2: closeHour must be greater than openHour";
	}
	return nullptr;
}

inline bool hoursFollowRules(int openHour, int closeHour) {
	return hoursRuleFailure(openHour, closeHour) == nullptr;
}

const char *sourceToString(Source source);
Source getSource();
void setSource(Source source, const char *reason = nullptr, bool persist = true);

bool isValid(bool logFailures = true);
bool validateConfigFields(bool logFailures = true, const char **failureReason = nullptr);
bool isValid(bool logFailures, const char **failureReason);
uint16_t reportingIntervalSecForRuntime();
uint32_t occupancyDebounceMsForRuntime();

void markLedgerConfigurationValid();
void markStorageConfigurationLoaded();
void markFactoryDefaultsActive();

void logDiagnostics(const char *tag = "ConfigDiag");

} // namespace Config

#endif /* GENERALIZED_CORE_COUNTER_CONFIG_H */
