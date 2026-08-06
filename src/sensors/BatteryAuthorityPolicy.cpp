#include "sensors/BatteryAuthorityPolicy.h"

namespace BatteryAuthorityPolicy {

namespace {

constexpr uint8_t PMIC_CHRG_FAULT_MASK = 0x30;
constexpr uint8_t PMIC_BAT_FAULT_MASK = 0x08;

bool batterySocIsValid(float soc) {
  return (soc == soc) && soc >= 0.0f && soc <= 100.0f;
}

bool batteryVoltageLooksUsable(float vcell) {
  return (vcell == vcell) && vcell > 2.5f && vcell < 5.0f;
}

} // namespace

float batterySampleDelta(float authoritativeSoc, float soc) {
  return soc - authoritativeSoc;
}

bool batterySampleHasUnrealisticDelta(float authoritativeSoc, float soc) {
  return batterySampleDelta(authoritativeSoc, soc) <= -POST_CONNECT_DELTA_THRESHOLD;
}

bool batterySampleHasUncorroboratedIncrease(float authoritativeSoc, float soc, float vcell) {
  const bool largeIncrease =
      batterySampleDelta(authoritativeSoc, soc) >= POST_CONNECT_DELTA_THRESHOLD;
  const bool vcellCorroboratesIncrease = vcell >= 4.00f;
  return largeIncrease && !vcellCorroboratesIncrease;
}

bool shouldEvaluatePostConnectDelta(bool shouldStabilize,
                                    bool authoritativeSampleActive,
                                    bool cloudConnected,
                                    bool radioPoweredOn) {
  return !shouldStabilize &&
      authoritativeSampleActive &&
      (cloudConnected || radioPoweredOn);
}

bool staleSocConditionsMet(const StaleSocSample &sample) {
  const bool externalPowerPresent =
      sample.vbusStatus == 1 || sample.vbusStatus == 2 || sample.vbusStatus == 3;
  const bool pmicStateChargeDone =
      sample.chargeStatus == static_cast<uint8_t>(ChargeStatus::Done);
  const bool pmicStateNotChargingWithPower =
      sample.chargeStatus == static_cast<uint8_t>(ChargeStatus::NotCharging) &&
      sample.powerGood;
  const bool pmicNoFault =
      !(sample.faultReg & (PMIC_CHRG_FAULT_MASK | PMIC_BAT_FAULT_MASK));
  const bool vcellHighConfidence = sample.vcell >= 4.10f;
  const bool vcellLowConfidence = sample.vcell >= 4.05f && sample.vcell < 4.10f;
  const bool legacyChargingContextStaleSoc =
      (vcellHighConfidence || vcellLowConfidence) &&
      sample.soc < 30.0f &&
      externalPowerPresent &&
      (pmicStateChargeDone || pmicStateNotChargingWithPower);
  const bool vcellRestingHighWithLowSoc =
      sample.vcell >= 4.00f && sample.soc <= 25.0f;

  return batterySocIsValid(sample.soc) &&
      batteryVoltageLooksUsable(sample.vcell) &&
      (legacyChargingContextStaleSoc || vcellRestingHighWithLowSoc) &&
      pmicNoFault;
}

bool quietForFuelGaugeResync(bool radioPoweredOn, uint8_t chargeStatus) {
  return !radioPoweredOn &&
      chargeStatus != static_cast<uint8_t>(ChargeStatus::Pre) &&
      chargeStatus != static_cast<uint8_t>(ChargeStatus::Fast);
}

bool shouldResyncFuelGauge(bool staleSocConditionsMet,
                           uint8_t consecutiveCount,
                           uint8_t wakeCyclesSinceResync,
                           bool radioPoweredOn,
                           uint8_t chargeStatus) {
  return staleSocConditionsMet &&
      consecutiveCount >= STALE_SOC_TRIGGER_CONSECUTIVE_COUNT &&
      wakeCyclesSinceResync >= STALE_SOC_RESYNC_COOLDOWN_WAKE_CYCLES &&
      quietForFuelGaugeResync(radioPoweredOn, chargeStatus);
}

SocCommitResolution resolveSocCommit(const StaleSocSample &sample,
                                     bool rejectAuthoritativeOverwrite,
                                     uint8_t consecutiveCount,
                                     uint8_t wakeCyclesSinceResync,
                                     bool radioPoweredOn,
                                     ResyncActions &actions) {
  SocCommitResolution result = {
      false,
      staleSocConditionsMet(sample),
      false,
      false,
      sample.soc,
      sample.vcell,
  };

  if (!result.initialSampleStale) {
    result.shouldCommit = !rejectAuthoritativeOverwrite;
    if (result.shouldCommit) {
      actions.commitSoc(result.soc);
    }
    return result;
  }

  if (!shouldResyncFuelGauge(result.initialSampleStale,
                             consecutiveCount,
                             wakeCyclesSinceResync,
                             radioPoweredOn,
                             sample.chargeStatus)) {
    return result;
  }

  actions.quickStart();
  result.resyncAttempted = true;
  actions.settle();
  actions.readSample(result.soc, result.vcell);

  StaleSocSample settledSample = sample;
  settledSample.soc = result.soc;
  settledSample.vcell = result.vcell;
  result.settledSampleStale = staleSocConditionsMet(settledSample);
  result.shouldCommit = !rejectAuthoritativeOverwrite && !result.settledSampleStale;
  if (result.shouldCommit) {
    actions.commitSoc(result.soc);
  }
  return result;
}

bool stuckChargingHasMeaningfulProgress(float baselineSoc,
                                        float acceptedSoc,
                                        float baselineVcell,
                                        float vcell) {
  const float socGain = batterySocIsValid(baselineSoc) ? acceptedSoc - baselineSoc : 0.0f;
  const float vcellGain = batteryVoltageLooksUsable(baselineVcell) ? vcell - baselineVcell : 0.0f;
  return socGain >= 0.5f || vcellGain >= 0.015f;
}

uint8_t noteWakeCycle(uint8_t wakeCyclesSinceResync) {
  return wakeCyclesSinceResync < 0xFF ? wakeCyclesSinceResync + 1 : wakeCyclesSinceResync;
}

} // namespace BatteryAuthorityPolicy