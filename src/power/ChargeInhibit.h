#pragma once

/**
 * @file ChargeInhibit.h
 * @brief F2a - thermal charge inhibit mechanism (WO-2026-08-25-001).
 *
 * MECHANISM CORRECTION (see WO "Mechanism correction" section): a bare
 * `PMIC::disableCharging()` call is undone by Device OS's own power-manager
 * thread on Device OS 6.4.1 - `PowerManager::handleCharging()`
 * (system_power_manager.cpp:306-313) re-enables charging unconditionally
 * whenever its own `HAL_POWER_CHARGE_STATE_DISABLE` config flag is not set.
 * This module instead uses `System.setPowerConfiguration()` with
 * `SystemPowerFeature::DISABLE_CHARGING`, which Device OS checks first and
 * will not silently re-enable behind.
 *
 * `System.setPowerConfiguration()` replaces the *entire* DCT power config on
 * every call (verified: hal/shared/power_hal.cpp -> dct_write_app_data at
 * DCT_POWER_CONFIG_OFFSET), so `apply()` always rebuilds the full
 * configuration - the active input profile's voltage/current limits plus the
 * charge-disable feature bit - rather than only touching the bit it cares
 * about. Re-asserting on every battery measurement (not just on transitions)
 * is what makes this inhibit self-clearing without a latch: the very next
 * measurement after the enclosure cools clears it automatically.
 *
 * Scope: THERMAL ONLY. The SourceUncertain inhibit (F2b, deferred) is a
 * different, latching, USB-only fail-safe with no natural clear condition and
 * must never apply to a solar-configured device - seeF2a/F2b distinction in
 * the work order. This module does not implement it.
 */

#include "power/ChargeInhibitPolicy.h"
#include "power/PowerManager.h"

namespace ChargeInhibit {

struct ApplyResult {
  bool inhibited = false;
  bool supported = false;
  int  systemResult = 0;

  // WO-2026-08-25-001 Amendment C, Decision C3 point 3: this is a DCT
  // CONFIG READBACK, NOT verified PMIC hardware state. It is true only when
  // System.getPowerConfiguration() reads back the DISABLE_CHARGING feature
  // bit as matching `inhibited` immediately after
  // System.setPowerConfiguration() returned success. Device OS applies this
  // config to the PMIC ASYNCHRONOUSLY - system_power_manager's
  // handleCharging() thread reconciles PMIC::isChargingEnabled() against the
  // config on its own schedule, which has not necessarily run yet by the
  // time this call returns. Renamed (was `verified`) so callers cannot
  // mistake a same-cycle config write for a same-cycle hardware guarantee;
  // callers needing the latter must read PMIC::isChargingEnabled() directly,
  // on a later cycle, after the async reload has had a chance to run.
  bool configReadbackVerified = false;
};

/**
 * @brief Re-asserts (or clears) the thermal charge inhibit for this cycle.
 *
 * Call on every battery measurement, unconditionally - not only on state
 * transitions - so the inhibit is bounded even across a reset (the first
 * post-reset measurement re-evaluates from the real temperature) and clears
 * itself the cycle after the enclosure cools, with no latch.
 *
 * @param inhibited Result of ChargeInhibitPolicy::evaluateThermal() this cycle
 * @param activeProfile Currently active input profile (read-only; F2a does
 *        not change profile selection, only whether charging is allowed
 *        within it)
 * @return Outcome of the configuration write/readback
 */
ApplyResult apply(bool inhibited, PowerInputProfile activeProfile);

} // namespace ChargeInhibit
