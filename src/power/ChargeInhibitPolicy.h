#pragma once

/**
 * @file ChargeInhibitPolicy.h
 * @brief F2a - thermal charge-inhibit hysteresis policy (WO-2026-08-25-001).
 *
 * POLICY, NOT MECHANISM (see ConnectivityPolicy.h for the same convention in
 * this codebase). This header has no Particle/Device-OS dependency so it is
 * host-testable; ChargeInhibit.h/.cpp is the mechanism that actually calls
 * System.setPowerConfiguration().
 *
 * Thresholds are the field-proven arming values (0 C / 37 C) from the prior-art
 * `isItSafeToCharge()` implementation, unchanged. Hysteresis is added only on
 * the release side (3 C / 35 C) to stop chatter at the boundary - the prior
 * art had none. Both bands are ledger-configurable and per-device overridable
 * (see MyPersistentData get_/set_thermalChargeInhibit* and
 * Cloud::applyPowerConfig()); these constants are only the compiled-in
 * defaults.
 */

namespace ChargeInhibitPolicy {

struct ThermalThresholds {
  float armHighC = 37.0f;      // arm (start inhibiting) at/above this temperature
  float armLowC = 0.0f;        // arm (start inhibiting) at/below this temperature
  float releaseHighC = 35.0f;  // release (resume charging) once back at/below this
  float releaseLowC = 3.0f;    // release (resume charging) once back at/above this
};

/// The cell's charge-maximum ceiling (WO-2026-08-25-001 Amendment B, AC-B6),
/// per docs/datasheets/LP803860-2000mAh-3.7V-BD310021-20231106.pdf. No
/// ledger-supplied armHighC may exceed this - a malformed ledger must not be
/// able to configure charging up to (or past) the cell's own charge-maximum
/// temperature.
constexpr float kCellChargeMaxC = 45.0f;

/**
 * @brief Validates a candidate ThermalThresholds set. Pure, no I/O.
 *
 * Enforces:
 *  - armHighC > releaseHighC (a non-inverted high-side hysteresis band -
 *    otherwise the release condition could never be reached, or would fire
 *    before arming, chattering/deadlocking the inhibit);
 *  - armLowC < releaseLowC (same, low side);
 *  - armHighC <= kCellChargeMaxC (never permit arming at/above the cell's own
 *    charge-maximum temperature - the ceiling exists independently of
 *    whatever a malformed ledger configures).
 *
 * A caller that fails this check must reject the whole candidate set (keep
 * the existing, previously-valid thresholds) rather than partially applying
 * it - see Cloud::applyPowerConfig().
 *
 * @param thresholds Candidate thresholds to validate.
 * @return true if the set is well-formed and safe to adopt.
 */
inline bool isValidThermalThresholds(const ThermalThresholds &thresholds) {
  if (!(thresholds.armHighC > thresholds.releaseHighC)) {
    return false;
  }
  if (!(thresholds.armLowC < thresholds.releaseLowC)) {
    return false;
  }
  if (thresholds.armHighC > kCellChargeMaxC) {
    return false;
  }
  return true;
}

/**
 * @brief Resolves a candidate set of thresholds - typically read back from
 * persistent storage - to a safe set for immediate use.
 *
 * This is the SAME validator AC-B6 already applies to ledger candidates
 * (see Cloud::applyPowerConfig()), reused here so the stored/migrated read
 * path cannot disagree with the ledger-candidate path about what counts as
 * a valid set. It exists because a zero-padded/migrated persistent-storage
 * record (all four fields 0.0f, e.g. after StorageHelperRK::validate() zero-
 * fills an appended structure extension for an already-provisioned device)
 * is invalid AS A SET - armHighC(0) > releaseHighC(0) is false - even though
 * 0.0f is a perfectly plausible value for any single field considered alone
 * (armLowC's own compiled default IS 0.0f). Validating the whole set, and
 * falling back to the whole default set atomically when it fails, means a
 * caller never ends up mixing some stored fields with some defaults.
 *
 * @param candidate Stored/migrated thresholds to validate.
 * @param defaults Complete default set to substitute, as a whole, if
 * candidate is invalid. Defaults to the compiled-in ThermalThresholds().
 * @return candidate unchanged if valid as a whole; otherwise defaults, whole.
 */
inline ThermalThresholds resolveStoredThermalThresholds(
    const ThermalThresholds &candidate,
    const ThermalThresholds &defaults = ThermalThresholds()) {
  if (isValidThermalThresholds(candidate)) {
    return candidate;
  }
  return defaults;
}

/**
 * @brief Pure hysteresis evaluation. No state, no I/O.
 *
 * @param currentlyInhibited Whether charging is inhibited going into this call
 * @param tempC Latest enclosure-temperature measurement
 * @param thresholds Arm/release band to evaluate against
 * @return true when charging should be inhibited after this measurement
 */
inline bool evaluateThermal(bool currentlyInhibited, float tempC,
                            const ThermalThresholds &thresholds = ThermalThresholds()) {
  if (currentlyInhibited) {
    // Stay inhibited until back inside the (tighter) release band.
    const bool releasedByHeat = tempC <= thresholds.releaseHighC;
    const bool releasedByCold = tempC >= thresholds.releaseLowC;
    return !(releasedByHeat && releasedByCold);
  }

  // Not currently inhibited: arm on the (wider) arm band.
  return (tempC > thresholds.armHighC) || (tempC < thresholds.armLowC);
}

/**
 * @brief Result of a validity-gated thermal-inhibit decision.
 * @see evaluateThermalWithValidity
 */
struct ThermalInhibitDecision {
  bool inhibited;              // Inhibit state after this evaluation.
  bool heldWithoutFreshTemp;   // True when an already-armed inhibit is held
                               // without a this-boot measurement to justify
                               // either releasing or re-confirming it.
};

/**
 * @brief Validity-gated wrapper around evaluateThermal() (WO-2026-08-25-001
 *        Amendment B, Decision B2 / AC-B5). Pure, no state, no I/O.
 *
 * `evaluateThermal()` alone has no notion of whether `tempC` is a genuine
 * this-boot measurement or a stale/persisted value from a prior boot or
 * enclosure state - the exact gap Codex Stage 7 found reachable: an
 * unmeasured (but persisted-hot) temperature could ARM the inhibit on every
 * boot, and a closed-hours hibernate cycle that resets the TMP36 8-sample
 * counter before it completes could prevent that inhibit from ever
 * releasing, even on a physically cool device.
 *
 * Rules (see the work order Amendment B for the full rationale):
 *  - `temperatureMeasuredThisBoot == true`: ordinary evaluateThermal(); free
 *    to arm OR release.
 *  - `temperatureMeasuredThisBoot == false` and not currently inhibited: the
 *    arm decision from `tempC` is suppressed outright - stays not inhibited.
 *    This is the load-bearing rule.
 *  - `temperatureMeasuredThisBoot == false` and currently inhibited: the
 *    inhibit is held (release also requires a fresh reading), but
 *    `heldWithoutFreshTemp` is set so the caller can surface this in
 *    telemetry rather than let it persist unobserved.
 *
 * @param currentlyInhibited Whether charging is inhibited going into this call.
 * @param temperatureMeasuredThisBoot Whether THIS call produced a genuine
 *        (non-fallback) reading (WO-2026-08-25-001 Amendment C, Decision C1:
 *        a per-call local in
 *        SensorManager::measureTemperatureAndApplyChargeDecision(), not a
 *        sticky boot-scoped flag - a genuine reading followed by later
 *        sensor failures must not leave that earlier reading treated as
 *        still current).
 * @param tempC This call's enclosure-temperature measurement (may be
 *        stale/persisted when temperatureMeasuredThisBoot is false).
 * @param thresholds Arm/release band to evaluate against.
 * @return The resulting inhibit state and whether it is being held without a
 *         fresh measurement.
 */
inline ThermalInhibitDecision evaluateThermalWithValidity(
    bool currentlyInhibited, bool temperatureMeasuredThisBoot, float tempC,
    const ThermalThresholds &thresholds = ThermalThresholds()) {
  if (temperatureMeasuredThisBoot) {
    return {evaluateThermal(currentlyInhibited, tempC, thresholds), false};
  }
  if (currentlyInhibited) {
    return {true, true};
  }
  return {false, false};
}

} // namespace ChargeInhibitPolicy
