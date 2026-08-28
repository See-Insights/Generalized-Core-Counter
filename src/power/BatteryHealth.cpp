#include "power/BatteryHealth.h"

#include <cmath>

namespace BatteryHealth {

namespace {

struct OcvKnot {
  float vcell;
  float soc;
};

// Standard single-cell LiPo/Li-ion resting-OCV reference curve (see
// BatteryHealth.h deviation note - this is a labeled standard curve, not a
// device-specific validated curve, because no numeric knot table exists in
// either this WO or its superseded predecessor). Monotonically increasing in
// both columns; interpolated piecewise-linearly between knots.
constexpr OcvKnot kOcvKnots[] = {
    {3.00f, 0.0f},
    {3.30f, 5.0f},
    {3.50f, 10.0f},
    {3.60f, 20.0f},
    {3.70f, 30.0f},
    {3.80f, 40.0f},
    {3.90f, 50.0f},
    {3.95f, 60.0f},
    {4.00f, 70.0f},
    {4.05f, 80.0f},
    {4.10f, 90.0f},
    {4.20f, 100.0f},
};
constexpr size_t kOcvKnotCount = sizeof(kOcvKnots) / sizeof(kOcvKnots[0]);

// The single confirmed residual threshold from the 378-sample analysis
// (median -0.9, p95 +12.9). Untrusted at |residual| >= this value in the
// symmetric (non-charging, non-load) case.
constexpr float kUntrustedResidual = 20.0f;
// Intermediate band: still plausibly ordinary noise (p95 ~12.9) but no longer
// confidently clean. Suspect, not yet Untrusted.
constexpr float kSuspectResidual = 12.0f;
// Extra allowance applied on the appropriate side when charge/load bias is
// expected to widen genuine residuals (see BatteryHealth.h rationale).
constexpr float kBiasAllowance = 8.0f;

bool vcellUsableFor(float vcell) {
  return (vcell == vcell) && vcell > 2.5f && vcell < 5.0f;
}

} // namespace

float restingSocFromVcell(float vcell) {
  if (!(vcell == vcell)) { // NaN guard
    return 0.0f;
  }

  if (vcell <= kOcvKnots[0].vcell) {
    return kOcvKnots[0].soc;
  }
  if (vcell >= kOcvKnots[kOcvKnotCount - 1].vcell) {
    return kOcvKnots[kOcvKnotCount - 1].soc;
  }

  for (size_t i = 0; i + 1 < kOcvKnotCount; i++) {
    const OcvKnot &lo = kOcvKnots[i];
    const OcvKnot &hi = kOcvKnots[i + 1];
    if (vcell >= lo.vcell && vcell <= hi.vcell) {
      const float span = hi.vcell - lo.vcell;
      if (span <= 0.0f) {
        return lo.soc;
      }
      const float frac = (vcell - lo.vcell) / span;
      return lo.soc + frac * (hi.soc - lo.soc);
    }
  }

  // Unreachable given the clamps above; keep a safe fallback.
  return 0.0f;
}

Reading evaluate(float soc, float vcell, bool chargingActive, bool radioActive) {
  Reading reading{};
  reading.soc = soc;
  reading.vcell = vcell;
  reading.vcellUsable = vcellUsableFor(vcell);
  reading.restingSocEstimate = restingSocFromVcell(vcell);
  reading.residual = soc - reading.restingSocEstimate;

  if (!reading.vcellUsable) {
    reading.trust = SocTrust::Untrusted;
    return reading;
  }

  const float negativeUntrusted = kUntrustedResidual + (chargingActive ? kBiasAllowance : 0.0f);
  const float positiveUntrusted = kUntrustedResidual + (radioActive ? kBiasAllowance : 0.0f);
  const float negativeSuspect = kSuspectResidual + (chargingActive ? kBiasAllowance : 0.0f);
  const float positiveSuspect = kSuspectResidual + (radioActive ? kBiasAllowance : 0.0f);

  if (reading.residual <= -negativeUntrusted || reading.residual >= positiveUntrusted) {
    reading.trust = SocTrust::Untrusted;
  } else if (reading.residual <= -negativeSuspect || reading.residual >= positiveSuspect) {
    reading.trust = SocTrust::Suspect;
  } else {
    reading.trust = SocTrust::Trusted;
  }

  return reading;
}

} // namespace BatteryHealth
