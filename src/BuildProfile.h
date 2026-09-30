#pragma once

/**
 * @file BuildProfile.h
 * @brief Single owner of compile-time build profile, logging policy and the
 *        compiled build-flags witness.
 *
 * @details
 * - FIELD builds must never block on USB serial.
 * - DEV builds may opt-in to blocking waits by defining
 *   ALLOW_BLOCKING_SERIAL_WAITS=1 at build time.
 * - Logging remains enabled in all profiles; these flags only affect
 *   whether blocking waits are compiled into the firmware.
 *
 * Profile selector (the one switch that picks a whole build shape):
 * - Release (the default): no build flag needed.
 * - Bench: -DBUILD_PROFILE_BENCH=1 turns on the bench diagnostics
 *   (ENABLE_DIAGNOSTICS_PUBLISH_MODE). It deliberately does NOT turn on
 *   ENABLE_RTC_SKEW_TEST, which stays an explicit, separately-requested flag.
 * - Any individual flag below can still be overridden with its own -D.
 *
 * Usage examples (build flags):
 * - FIELD (non-blocking): -DDEV_BUILD=0
 * - DEV (non-blocking): -DDEV_BUILD=1
 * - DEV (blocking waits enabled): -DDEV_BUILD=1 -DALLOW_BLOCKING_SERIAL_WAITS=1
 * - Bench profile: -DBUILD_PROFILE_BENCH=1
 * - Serial log level (0=none,1=error,2=warn,3=info+filters,4=all): -DSERIAL_LOG_LEVEL=4
 * - Connectivity failsafe bench test: -DCONNECTIVITY_FAILSAFE_TEST_MODE=1
 * - PMIC forensics override: -DENABLE_PMIC_FORENSICS=0
 * - Ledger trace logging: -DENABLE_LEDGER_TRACE=1
 * - Connect trace logging: -DENABLE_CONNECT_TRACE=1
 * - Connect decision trace logging: -DENABLE_CONNECT_DECISION_TRACE=1
 * - Performance trace logging: -DENABLE_PERF_TRACE=1
 * - Sleep-gate trace logging: -DENABLE_GATE_TRACE=1
 * - Sleep routine trace logging: -DENABLE_SLEEP_TRACE=1
 * - Routine config trace logging: -DENABLE_CONFIG_TRACE=1
 * - Diagnostics publish mode: -DENABLE_DIAGNOSTICS_PUBLISH_MODE=1
 * - RTC skew bench hook (Boron only): -DENABLE_RTC_SKEW_TEST=1
 *
 * WO-2026-08-24-001: the Boron USB power-source override in
 * PowerManager::refreshInputProfile() previously had its own build flag
 * (ENABLE_BORON_USB_SOURCE_OVERRIDE) which was found compiled OFF in a
 * shipped product build despite the source default being 1 and the version
 * string being identical to a build where it was ON -- an undetectable
 * silent-drift failure mode. The flag has been removed; the override now
 * compiles unconditionally on Boron (still gated only by the real
 * PLATFORM_ID == PLATFORM_BORON platform check in PowerManager.cpp). Its own
 * `usbEnumerated` predicate already self-limits cost on non-USB devices.
 *
 * Trace flag notes:
 * - ENABLE_LEDGER_TRACE=1 enables detailed ledger request lifecycle logs.
 * - ENABLE_CONNECT_TRACE=1 enables connection acquire diagnostics.
 * - ENABLE_CONNECT_DECISION_TRACE=1 enables connection decision rationale logs.
 * - ENABLE_PERF_TRACE=1 enables successful report performance timing logs.
 * - ENABLE_GATE_TRACE=1 enables sleep gate polling/detail logs.
 * - ENABLE_SLEEP_TRACE=1 enables routine sleep/watchdog/pre-sleep battery logs.
 * - ENABLE_CONFIG_TRACE=1 enables routine config apply/validation diagnostics.
 * - Defaults remain 0 for field-safe release logging.
 *
 * Current repo defaults are set for field-safe behavior.
 * Switch to developer behavior by setting DEV_BUILD=1 (and optionally
 * ALLOW_BLOCKING_SERIAL_WAITS=1) at build time.
 */
#ifndef BUILD_PROFILE_BENCH
/**
 * @brief Build profile selector: 0 = release (default), 1 = bench.
 *
 * Bench turns on the bench diagnostics (ENABLE_DIAGNOSTICS_PUBLISH_MODE). It
 * must NOT turn on ENABLE_RTC_SKEW_TEST, which writes a deliberately wrong
 * RTC value and stays an explicit, separately-requested flag.
 */
#define BUILD_PROFILE_BENCH 0
#endif

#if (BUILD_PROFILE_BENCH != 0) && (BUILD_PROFILE_BENCH != 1)
#error "BUILD_PROFILE_BENCH must be 0 or 1"
#endif

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
 *
 * Disabled in v22-Diag-Soak: Pi forwarder now captures serial continuously,
 * so the bench-only forced wait serves no purpose and wastes battery on reset.
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
 * Release v14 defaults this to 1 so contradiction counters, logs, and
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

#ifndef ENABLE_CONFIG_TRACE
#define ENABLE_CONFIG_TRACE 0
#endif

#if (ENABLE_CONFIG_TRACE != 0) && (ENABLE_CONFIG_TRACE != 1)
#error "ENABLE_CONFIG_TRACE must be 0 or 1"
#endif

#ifndef ENABLE_RTC_SKEW_TEST
/**
 * @brief TEMPORARY DIAGNOSTIC: Write a deliberately wrong RTC value once on boot.
 *
 * Set to 1 to enable, then reflash. Runs automatically during setup(),
 * immediately after ab1805.setup() and before the hibernate-wake
 * classification block: it reads the current AB1805 RTC value, writes back
 * a compile-time-skewed value (see RtcSkewTest::kSkewSeconds in
 * src/time/RtcSkewTest.h), and logs both readings. Deliberately NOT tied to
 * BUILD_PROFILE_BENCH: a bench build must not silently corrupt the RTC.
 * This reproduces the
 * Dev-11 field defect (RTC holding a plausible-but-wrong time, so
 * Time.isValid() is true but the value is bad) on demand, which a simple
 * LiPo/USB power-cycle cannot: that only clears the RTC to *unset*, a
 * different state that cannot reach hibernate at all (see
 * WO-2026-08-31-003).
 *
 * One-shot guard (backed by retained RAM - src/time/RtcSkewTest.h) fires
 * ONCE ACROSS RESETS, not once per boot: it does not re-arm on a
 * subsequent wake/reset while power is maintained, only on a fresh flash
 * or an explicit reset of the latch (Amendment A.2). Boron-only (the
 * AB1805 is Boron hardware). After the bench test completes, set back to 0
 * and reflash to remove the test from the binary - this must never ship in
 * a field build.
 */
#define ENABLE_RTC_SKEW_TEST 0
#endif

#if (ENABLE_RTC_SKEW_TEST != 0) && (ENABLE_RTC_SKEW_TEST != 1)
#error "ENABLE_RTC_SKEW_TEST must be 0 or 1"
#endif

#ifndef ENABLE_DIAGNOSTICS_PUBLISH_MODE
/**
 * @brief Batch PowerDiag/ChargeDiag lines and publish on next connect.
 *
 * When enabled, each PowerDiag/ChargeDiag/stale-SOC-resync diagnostic emission
 * within a connect/sleep cycle is also captured into a small in-RAM batch. Once
 * per cycle, at each true cycle-ending point, the batch is serialized to one
 * compact "pdiag" JSON event and queued via PublishQueuePosix, which persists
 * it to flash and drains it to Particle Cloud on the next connection - even
 * across a fully-disconnected soak. End-to-end verified in v22-Diag-Soak field
 * soak, including nightly heap-guard flush and JSON truncation fixes, and
 * survived real production failure (660s connect timeout). Promoted from
 * bench-only to field soak in v22-Diag-Soak. Not sourced from cloud/ledger.
 *
 * Defaults to the build profile: off in release, on in bench.
 */
#define ENABLE_DIAGNOSTICS_PUBLISH_MODE BUILD_PROFILE_BENCH
#endif

#if (ENABLE_DIAGNOSTICS_PUBLISH_MODE != 0) && (ENABLE_DIAGNOSTICS_PUBLISH_MODE != 1)
#error "ENABLE_DIAGNOSTICS_PUBLISH_MODE must be 0 or 1"
#endif

// ===== Serial logging policy =====
// Level and per-category filters are build profile, not application logic, so
// they live here; cloud/Particle_Functions.cpp instantiates the one
// SerialLogHandler from them.
#ifndef SERIAL_LOG_LEVEL
/** @brief Serial log level: 0=none, 1=error, 2=warn, 3=info+filters, 4=all. */
#define SERIAL_LOG_LEVEL 3
#endif

#if (SERIAL_LOG_LEVEL < 0) || (SERIAL_LOG_LEVEL > 4)
#error "SERIAL_LOG_LEVEL must be 0 (none), 1 (error), 2 (warn), 3 (info+filters) or 4 (all)"
#endif

/**
 * @brief Per-category log filters applied at SERIAL_LOG_LEVEL 3: holds noisy
 *        Device OS categories at WARN. Expanded inside the SerialLogHandler
 *        initializer list, so LOG_LEVEL_* resolves at the use site.
 */
#define SERIAL_LOG_CATEGORY_FILTERS                                            \
    {"mux", LOG_LEVEL_WARN}, {"system.nm", LOG_LEVEL_WARN},                    \
    {"system", LOG_LEVEL_WARN}, {"comm.dtls", LOG_LEVEL_WARN},                 \
    {"comm.protocol", LOG_LEVEL_WARN},                                         \
    {"comm.protocol.handshake", LOG_LEVEL_WARN},                               \
    {"net.pppncp", LOG_LEVEL_WARN}, {"app.ab1805", LOG_LEVEL_WARN}

// ===== Compiled build-flags witness (WO-2026-08-24-001) =====
// Compact bitmask of the build flags ACTUALLY compiled in, derived from the
// same conditions that gate each feature so it cannot silently drift from the
// shipped binary the way ENABLE_BORON_USB_SOURCE_OVERRIDE did. Defined once
// here, expanded at both emission sites (the Boot: serial line's "flags=" and
// the ledger firmware object's "flags").
//
// Bit positions are an external contract: fleet tooling reads this word, so a
// retired flag leaves its bit RESERVED rather than renumbering the rest.
// Reserved: 0x0400 (retired ENABLE_PMIC_TRACE), 0x1000 (retired
// ENABLE_PMIC_CHARGE_CYCLE_TEST); both were always 0 in a release build.
//
// Bit 0x4000 mirrors the SAME condition that gates the USB source override in
// PowerManager::refreshInputProfile(); set => override compiled in. A CLEAR
// bit is EXPECTED and CORRECT on non-Boron platforms; it is a defect signal
// ONLY on a Boron build. It is resolved where this header is processed, so an
// emitting translation unit must reach Particle.h (which defines
// PLATFORM_BORON) first; both do, via Config.h -> power/ConnectivityPolicy.h.
#if defined(PLATFORM_ID) && defined(PLATFORM_BORON) && (PLATFORM_ID == PLATFORM_BORON)
#define BUILD_FLAGS_BORON_USB_SOURCE_OVERRIDE_BIT 0x4000
#else
#define BUILD_FLAGS_BORON_USB_SOURCE_OVERRIDE_BIT 0
#endif

#define COMPILED_BUILD_FLAGS                                                   \
    ((DEV_BUILD ? 0x0001 : 0) |                                                \
     (ALLOW_BLOCKING_SERIAL_WAITS ? 0x0002 : 0) |                              \
     (CONNECTIVITY_FAILSAFE_TEST_MODE ? 0x0004 : 0) |                          \
     (ENABLE_PMIC_FORENSICS ? 0x0008 : 0) |                                    \
     (ENABLE_LEDGER_TRACE ? 0x0010 : 0) |                                      \
     (ENABLE_CONNECT_TRACE ? 0x0020 : 0) |                                     \
     (ENABLE_CONNECT_DECISION_TRACE ? 0x0040 : 0) |                            \
     (ENABLE_PERF_TRACE ? 0x0080 : 0) |                                        \
     (ENABLE_GATE_TRACE ? 0x0100 : 0) |                                        \
     (ENABLE_SLEEP_TRACE ? 0x0200 : 0) |                                       \
     (ENABLE_CONFIG_TRACE ? 0x0800 : 0) |                                      \
     (ENABLE_DIAGNOSTICS_PUBLISH_MODE ? 0x2000 : 0) |                          \
     BUILD_FLAGS_BORON_USB_SOURCE_OVERRIDE_BIT)
