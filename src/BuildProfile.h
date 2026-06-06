#pragma once

/**
 * @file BuildProfile.h
 * @brief Build-profile guardrails for serial/debug waits.
 *
 * @details
 * - FIELD builds must never block on USB serial.
 * - DEV builds may opt-in to blocking waits by defining
 *   ALLOW_BLOCKING_SERIAL_WAITS=1 at build time.
 * - Logging remains enabled in all profiles; these flags only affect
 *   whether blocking waits are compiled into the firmware.
 *
 * Usage examples (build flags):
 * - FIELD (non-blocking): -DDEV_BUILD=0
 * - DEV (non-blocking): -DDEV_BUILD=1
 * - DEV (blocking waits enabled): -DDEV_BUILD=1 -DALLOW_BLOCKING_SERIAL_WAITS=1
 * - Connectivity failsafe bench test: -DCONNECTIVITY_FAILSAFE_TEST_MODE=1
 * - PMIC forensics override: -DENABLE_PMIC_FORENSICS=0
 * - Ledger trace logging: -DENABLE_LEDGER_TRACE=1
 * - Connect trace logging: -DENABLE_CONNECT_TRACE=1
 * - Connect decision trace logging: -DENABLE_CONNECT_DECISION_TRACE=1
 * - Performance trace logging: -DENABLE_PERF_TRACE=1
 * - Sleep-gate trace logging: -DENABLE_GATE_TRACE=1
 * - Sleep routine trace logging: -DENABLE_SLEEP_TRACE=1
 * - Routine PMIC trace logging: -DENABLE_PMIC_TRACE=1
 * - Routine config trace logging: -DENABLE_CONFIG_TRACE=1
 *
 * Trace flag notes:
 * - ENABLE_LEDGER_TRACE=1 enables detailed ledger request lifecycle logs.
 * - ENABLE_CONNECT_TRACE=1 enables connection acquire diagnostics.
 * - ENABLE_CONNECT_DECISION_TRACE=1 enables connection decision rationale logs.
 * - ENABLE_PERF_TRACE=1 enables successful report performance timing logs.
 * - ENABLE_GATE_TRACE=1 enables sleep gate polling/detail logs.
 * - ENABLE_SLEEP_TRACE=1 enables routine sleep/watchdog/pre-sleep battery logs.
 * - ENABLE_PMIC_TRACE=1 enables routine PMIC state logs.
 * - ENABLE_CONFIG_TRACE=1 enables routine config apply/validation diagnostics.
 * - Defaults remain 0 for field-safe release logging.
 *
 * Current repo defaults are set for field-safe behavior.
 * Switch to developer behavior by setting DEV_BUILD=1 (and optionally
 * ALLOW_BLOCKING_SERIAL_WAITS=1) at build time.
 */
#ifndef DEV_BUILD
/**
 * @brief Set to 1 for developer builds.
 */
#define DEV_BUILD 0
#endif

#ifndef FIELD_BUILD
/**
 * @brief Derived flag for field builds (true when DEV_BUILD=0).
 */
#define FIELD_BUILD (!DEV_BUILD)
#endif

#ifndef ALLOW_BLOCKING_SERIAL_WAITS
/**
 * @brief Allow explicit blocking waits for USB serial attachment.
 */
#define ALLOW_BLOCKING_SERIAL_WAITS 0
#endif

#ifndef CONNECTIVITY_FAILSAFE_TEST_MODE
/**
 * @brief Bench-only override for connectivity failsafe timing.
 *
 * Set to 1 only for explicit test builds that need short stale/cooldown
 * windows. Production builds must leave this at 0.
 *
 * This is compile-time only. It is not sourced from cloud/ledger config,
 * and should not be toggled through Config.h. Enable it only with an
 * explicit build flag or a deliberate local edit in this build-profile layer.
 */
#define CONNECTIVITY_FAILSAFE_TEST_MODE 0
#endif

#if (CONNECTIVITY_FAILSAFE_TEST_MODE != 0) && (CONNECTIVITY_FAILSAFE_TEST_MODE != 1)
#error "CONNECTIVITY_FAILSAFE_TEST_MODE must be 0 or 1"
#endif

#ifndef ENABLE_PMIC_FORENSICS
/**
 * @brief Enable retained PMIC contradiction forensic instrumentation.
 *
 * Release v13 defaults this to 1 so contradiction counters, logs, and
 * startup/device-status telemetry are included in production soak builds.
 * Set to 0 only for an explicit comparison build that needs the PMIC
 * forensic path compiled out.
 */
#define ENABLE_PMIC_FORENSICS 1
#endif

#if (ENABLE_PMIC_FORENSICS != 0) && (ENABLE_PMIC_FORENSICS != 1)
#error "ENABLE_PMIC_FORENSICS must be 0 or 1"
#endif

#ifndef ENABLE_LEDGER_TRACE
#define ENABLE_LEDGER_TRACE 0
#endif

#if (ENABLE_LEDGER_TRACE != 0) && (ENABLE_LEDGER_TRACE != 1)
#error "ENABLE_LEDGER_TRACE must be 0 or 1"
#endif

#ifndef ENABLE_CONNECT_TRACE
#define ENABLE_CONNECT_TRACE 0
#endif

#if (ENABLE_CONNECT_TRACE != 0) && (ENABLE_CONNECT_TRACE != 1)
#error "ENABLE_CONNECT_TRACE must be 0 or 1"
#endif

#ifndef ENABLE_CONNECT_DECISION_TRACE
#define ENABLE_CONNECT_DECISION_TRACE 0
#endif

#if (ENABLE_CONNECT_DECISION_TRACE != 0) && (ENABLE_CONNECT_DECISION_TRACE != 1)
#error "ENABLE_CONNECT_DECISION_TRACE must be 0 or 1"
#endif

#ifndef ENABLE_PERF_TRACE
#define ENABLE_PERF_TRACE 0
#endif

#if (ENABLE_PERF_TRACE != 0) && (ENABLE_PERF_TRACE != 1)
#error "ENABLE_PERF_TRACE must be 0 or 1"
#endif

#ifndef ENABLE_GATE_TRACE
#define ENABLE_GATE_TRACE 0
#endif

#if (ENABLE_GATE_TRACE != 0) && (ENABLE_GATE_TRACE != 1)
#error "ENABLE_GATE_TRACE must be 0 or 1"
#endif

#ifndef ENABLE_SLEEP_TRACE
#define ENABLE_SLEEP_TRACE 0
#endif

#if (ENABLE_SLEEP_TRACE != 0) && (ENABLE_SLEEP_TRACE != 1)
#error "ENABLE_SLEEP_TRACE must be 0 or 1"
#endif

#ifndef ENABLE_PMIC_TRACE
#define ENABLE_PMIC_TRACE 0
#endif

#if (ENABLE_PMIC_TRACE != 0) && (ENABLE_PMIC_TRACE != 1)
#error "ENABLE_PMIC_TRACE must be 0 or 1"
#endif

#ifndef ENABLE_CONFIG_TRACE
#define ENABLE_CONFIG_TRACE 0
#endif

#if (ENABLE_CONFIG_TRACE != 0) && (ENABLE_CONFIG_TRACE != 1)
#error "ENABLE_CONFIG_TRACE must be 0 or 1"
#endif

// Optional convenience: enable DEBUG_SERIAL in DEV builds unless overridden.
#if DEV_BUILD && !defined(DEBUG_SERIAL)
#define DEBUG_SERIAL
#endif
