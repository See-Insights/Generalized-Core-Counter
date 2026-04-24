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

#ifndef ALERT44_DIAG_ENABLED
/**
 * @brief Enable temporary ledger-sync helper diagnostics for Alert 44 soak validation.
 */
#define ALERT44_DIAG_ENABLED 1
#endif

// Optional convenience: enable DEBUG_SERIAL in DEV builds unless overridden.
#if DEV_BUILD && !defined(DEBUG_SERIAL)
#define DEBUG_SERIAL
#endif
