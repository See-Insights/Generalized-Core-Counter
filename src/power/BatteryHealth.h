#pragma once

/**
 * @file BatteryHealth.h
 * @brief F1 - SOC / battery health trust signal (WO-2026-08-25-001).
 *
 * PURE BY DESIGN. This module emits a trust signal for the fuel-gauge SOC
 * reading; it never corrects the gauge, never latches, never retries, never
 * calls quickStart(), and never raises an alert. The prior WO
 * (WO-2026-08-24-002 / the STALE_SOC machinery it produced) tried to correct
 * the gauge in place and failed four consecutive Stage 7 reviews doing it.
 *
 * The fix is architectural, not mechanical: stop depending on SOC when it is
 * untrustworthy (see PowerTier::evaluate(), which falls back to vcell-only
 * when trust != Trusted) instead of trying to repair SOC so it can keep being
 * depended on. That removes the latch, the retry, the cooldown, the pending
 * state and the alert plumbing in one move - all of which existed only to
 * make an in-place correction stick.
 *
 * Deviation from the work order (documented, see
 * /tmp/wo26/IMPLEMENTATION_REPORT.md): the WO states the OCV knots are
 * "specified in the WO" and "retained from WO-2026-08-24-002", but neither
 * document actually contains a numeric knot table - WO-2026-08-24-002 only
 * *names* an implementation symbol (`kOcvCurve`) that does not exist anywhere
 * in this repository (verified: no `OCV`/`resting` code exists prior to this
 * change). Rather than fabricate a curve and label it "validated", this
 * implementation uses a clearly-labeled standard single-cell LiPo/Li-ion
 * resting-OCV reference curve - the same category of curve the superseded WO
 * describes testing as one of its three candidate curves ("an independent
 * published LiPo table"). It is replaceable by updating kOcvKnots below if a
 * cell-specific validated curve becomes available.
 */

#include <stdint.h>

namespace BatteryHealth {

enum class SocTrust : uint8_t { Trusted, Suspect, Untrusted };

struct Reading {
  float    soc;                 // gauge SOC as reported
  float    vcell;               // measured cell voltage
  float    restingSocEstimate;  // from the (labeled-standard) OCV table
  float    residual;            // soc - restingSocEstimate
  SocTrust trust;
  bool     vcellUsable;
};

/// Piecewise-linear interpolation over a standard single-cell LiPo resting
/// OCV/SOC reference curve. Pure function of voltage only - no state.
/// Clamped to [0, 100] outside the table's voltage range.
float restingSocFromVcell(float vcell);

/// Pure. No state, no latch, no retry, no side effects. `chargingActive`
/// widens the negative-residual allowance (charge inflates terminal voltage
/// above true OCV, understating residual); `radioActive` widens the
/// positive-residual allowance (load sags terminal voltage below true OCV,
/// overstating residual).
Reading evaluate(float soc, float vcell, bool chargingActive, bool radioActive);

} // namespace BatteryHealth
