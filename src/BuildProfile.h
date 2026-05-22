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
 *
 * Current repo defaults are set for field-safe behavior.
 * Switch to developer behavior by setting DEV_BUILD=1 (and optionally
 * ALLOW_BLOCKING_SERIAL_WAITS=1) at build time.
 */
#ifndef DEV_BUILD
/**
 * @brief Set to 1 for developer builds.
 */
#define DEV_BUILD 1
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
#define ALLOW_BLOCKING_SERIAL_WAITS 1
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

// Optional convenience: enable DEBUG_SERIAL in DEV builds unless overridden.
#if DEV_BUILD && !defined(DEBUG_SERIAL)
#define DEBUG_SERIAL
#endif
