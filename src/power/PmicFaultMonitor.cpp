#include "Particle.h"
#include "PmicFaultMonitor.h"

#include "PowerManager.h"
#include "PowerDiagnostics.h"
#include "MyPersistentData.h"
#include "observability/WakeCycleStats.h"
#include "sensors/BatteryAuthorityPolicy.h"

// Defined in sensors/SensorManager.cpp - shared with the pre-charge
// stabilization retry path there, so it stays defined in one place.
extern void boundedBatterySettleDelay(unsigned long delayMs);
extern unsigned long currentWakeAwakeMs();

namespace {

constexpr uint8_t kPmicChargeFaultMask = 0x30; // REG09 bits 5:4
constexpr uint8_t kPmicBatFaultMask = 0x08;    // REG09 bit 3
constexpr uint8_t kPmicNtcFaultMask = 0x07;    // REG09 bits 2:0
constexpr unsigned long kChargeSummaryIntervalMs = 15UL * 60UL * 1000UL;

uint8_t pmicChargeFaultCode(uint8_t faultReg) { return (faultReg & kPmicChargeFaultMask) >> 4; }
bool isUsbBackedSource(int source) { return source == 2 || source == 3 || source == 4; }

// "Off/Pre/Fast/Done/Fault" compact charge-state label shared by the
// ChargeDiag line and the periodic charge summary.
const char *chargeLabel(uint8_t status, bool fault) {
  if (fault) return "FAULT";
  static const char *kLabels[] = {"OFF", "PRE", "FAST", "DONE"};
  return kLabels[status & 0x03];
}

const char *vbusLabel(uint8_t vbusStatus) {
  switch (vbusStatus) {
    case 1: return "USB";
    case 2: return "ADAPT";
    case 3: return "OTG";
    default: return "NONE";
  }
}

// Charging trend telemetry baseline (diagnostic-only, no behavior changes).
// Retained so it survives sleep, matching prior field-tested behavior.
retained float lastTrendSoc = -999.0f;
retained float lastTrendVcell = -999.0f;
retained time_t lastTrendEpoch = 0;

// Advances one phase of a non-blocking disable->wait->enable charge cycle,
// shared by remediation levels 1 (soft reset) and 2 (aggressive power cycle
// w/ watchdog). Returns true once charging has been re-enabled.
//
// `safeToCharge` gates the ONLY step that can turn charging back on
// (pmic.enableCharging(), below) - not the disable step, since disabling is
// always safe regardless of thermal state. This is intentionally the single
// choke point for that call: every caller advances the same phase/
// phaseStartMs state through this function, so gating here (rather than at
// each call site) means no caller can enable charging without being told it
// is safe, even as new call sites are added later.
//
// If phase 1 (waiting to re-enable) is reached while safeToCharge is false,
// this FREEZES the sequence: phase/phaseStartMs are left untouched and
// false is returned without calling enableCharging(). The sequence keeps its
// owner (the caller's remediationInProgress/ActiveLevel/Phase state is not
// cleared) and simply resumes - including completing the elapsed-wait check
// and the re-enable - the next time this is called with safeToCharge true.
bool advanceChargeCyclePhase(PMIC &pmic, uint8_t &phase, unsigned long &phaseStartMs,
                              unsigned long waitMs, bool setWatchdogOnResume,
                              bool safeToCharge) {
  const unsigned long now = millis();
  if (phase == 0) {
    pmic.disableCharging();
    phaseStartMs = now;
    phase = 1;
    return false;
  }
  if (!safeToCharge) return false; // Frozen: never re-enable while unsafe.
  if (now - phaseStartMs < waitMs) return false;
  if (setWatchdogOnResume) pmic.setWatchdog(0b01); // 40 seconds
  pmic.enableCharging();
  return true;
}

void readRegisters(PMIC &pmic, PmicFaultMonitor::Registers &regs) {
  // WO-2026-08-25-001 Amendment C, Decision C3 point 4: the BQ24195 FAULT
  // register (REG09) can report a fault that reflects state from before this
  // read - Device OS's own driver reads it twice and ORs the two readings so
  // a fault bit that is only briefly latched is not silently missed by a
  // single read. This module previously read it once; mirror Device OS's
  // pattern here.
  const uint8_t faultRead1 = pmic.getFault();
  const uint8_t faultRead2 = pmic.getFault();
  regs.faultReg = faultRead1 | faultRead2;
  regs.systemStatus = pmic.getSystemStatus();
  regs.vbusStatus = (regs.systemStatus >> 6) & 0x03;
  regs.chargeStatus = (regs.systemStatus >> 4) & 0x03;
  regs.powerGood = (regs.systemStatus & 0x04) != 0;
  regs.thermalRegulation = (regs.systemStatus & 0x02) != 0;
  regs.inVsysMin = (regs.systemStatus & 0x01) != 0;
}

} // namespace

namespace PmicFaultMonitor {

Registers pollAndRemediate(PMIC &pmic, bool safeToCharge, uint8_t battState, int powerSource,
                            bool chargeDisableConfigVerified) {
  static unsigned long lastRemediationAttempt = 0;
  static uint8_t remediationLevel = 0;       // 0=none, 1=soft reset, 2=power cycle
  static uint8_t consecutiveFaults = 0;
  static bool remediationInProgress = false;
  static uint8_t remediationActiveLevel = 0; // 0=none, 1=soft, 2=aggressive
  static uint8_t remediationPhase = 0;       // 0=start, 1=wait
  static unsigned long remediationPhaseStartMs = 0;
  static bool immediateResetConsumed = false;
  static uint8_t immediateResetFaultReg = 0;
  constexpr unsigned long kRemediationCooldownMs = 3600000UL; // 1 hour

  Registers regs;
  readRegisters(pmic, regs);

  const PowerReport &powerReport = PowerManager::instance().latestReport();

  // Immediate one-shot reset attempt: only when safe, USB-backed (a solar dip
  // is not fixed by toggling charging), not already tried for this exact
  // fault, not thermally regulated, and not already in a severe battery state.
  bool persistentAfterImmediateReset = false;
  if (regs.faultReg & kPmicChargeFaultMask) {
    const bool usbInputPresent = (regs.vbusStatus == 1 || regs.vbusStatus == 2 || regs.vbusStatus == 3);
    const bool usbBackedFaultContext =
        powerReport.activeInputProfile == PowerInputProfile::UsbBench ||
        isUsbBackedSource(powerReport.reading.powerSource) || isUsbBackedSource(powerSource) ||
        usbInputPresent;
    const bool severeBatteryState = (battState == 5 || battState == 6);
    const uint8_t initialChargeFault = pmicChargeFaultCode(regs.faultReg);
    const bool alreadyAttempted = immediateResetConsumed && immediateResetFaultReg == regs.faultReg;
    const bool canAttemptImmediateReset =
        !alreadyAttempted && safeToCharge && regs.powerGood && usbInputPresent &&
        usbBackedFaultContext && !regs.thermalRegulation && !severeBatteryState &&
        initialChargeFault != 0x02;

    if (canAttemptImmediateReset) {
      Log.warn("PMIC: charge fault active; attempting charging reset");
      pmic.disableCharging();
      boundedBatterySettleDelay(500UL);
      pmic.enableCharging();
      boundedBatterySettleDelay(500UL);

      readRegisters(pmic, regs);
      immediateResetConsumed = true;
      immediateResetFaultReg =
          (regs.faultReg & kPmicChargeFaultMask) ? regs.faultReg : (uint8_t)(initialChargeFault << 4);
      lastRemediationAttempt = millis();

      Log.info("PMIC: fault reset result faultReg=0x%02x charge=%s", regs.faultReg,
                chargeLabel(regs.chargeStatus, false));

      if (!(regs.faultReg & kPmicChargeFaultMask)) {
        Log.info("PMIC: charge fault cleared");
        consecutiveFaults = 0;
        remediationLevel = 0;
        remediationInProgress = false;
        remediationActiveLevel = 0;
        remediationPhase = 0;
        immediateResetConsumed = false;
        immediateResetFaultReg = 0;

        const int8_t currentAlert = current.get_alertCode();
        if (currentAlert == 21 || currentAlert == 23) {
          current.set_alertCode(0);
          current.set_lastAlertTime(0);
        }
      } else {
        persistentAfterImmediateReset = true;
      }
    }
  }

  // NTC_FAULT (bits 2:0): the PMIC's own TS/thermal-status classification of
  // the *charge-path* thermistor network, independent of CHRG_FAULT (bits
  // 5:4) and BAT_FAULT (bit 3) above. WO-2026-08-25-001 Amendment B, Blocker
  // 4: this pack has no NTC, but the carrier/TS network can still produce
  // this fault class, and it was previously neither classified nor alerted -
  // the monitor only ever checked CHRG_FAULT/BAT_FAULT. It is a thermal-status
  // fault by definition, so it is one of the two classes for which "recovers
  // naturally, don't fight it" is a defensible remediation-suppression
  // rationale (see thermallyCorrelatedFault below).
  const uint8_t ntcFaultCode = regs.faultReg & kPmicNtcFaultMask;
  if (ntcFaultCode != 0) {
    Log.warn("PMIC: NTC fault active (code=0x%02x) - charge-path thermistor/TS network fault", ntcFaultCode);
    current.raiseAlert(20); // Reuse the thermal alert code - this is a thermal-status class fault.
  }

  // CHRG_FAULT (bits 5:4). BAT_FAULT is separate, handled below, and does not
  // enter this remediation path (a hardware condition, not fixable by toggle).
  if (regs.faultReg & kPmicChargeFaultMask) {
    const uint8_t chargeFault = pmicChargeFaultCode(regs.faultReg);
    consecutiveFaults++;

    switch (chargeFault) {
      case 0x01: // Input fault: usually a transient solar dip - log only.
        Log.info("PMIC: Input fault - VBUS out of range (likely solar variation)");
        if (persistentAfterImmediateReset) {
          Log.error("PMIC: charge fault persists after reset - alert 21");
          current.raiseAlert(21);
        }
        break;
      case 0x02: // Thermal shutdown.
        Log.error("PMIC: Thermal shutdown - charging stopped due to temperature");
        current.raiseAlert(20);
        break;
      case 0x03: // Charge safety timer expired - common stuck-charging indicator.
        Log.error(persistentAfterImmediateReset ? "PMIC: charge fault persists after reset - alert 21"
                                                 : "PMIC: Charge safety timer expired - charging timeout");
        current.raiseAlert(21);
        break;
      default:
        if (persistentAfterImmediateReset) {
          Log.error("PMIC: charge fault persists after reset - alert 21");
          current.raiseAlert(21);
        } else {
          Log.warn("PMIC: Charge fault detected (code=0x%02x)", chargeFault);
          current.raiseAlert(23);
        }
        break;
    }

    // Per-fault-class remediation suppression (WO-2026-08-25-001 Amendment B,
    // Blocker 4; corrected under Amendment C, Decision C3 point 1). "It
    // recovers naturally" is true ONLY for a fault directly explained by the
    // same enclosure-temperature condition ChargeInhibit is managing -
    // thermal shutdown (CHRG_FAULT==0x02). It is FALSE for an input fault
    // (independent cause: e.g. solar variation, cabling) and for
    // safety-timer expiry (needs an explicit charge-cycle/timer
    // intervention) and the general/unclassified case - blanket-suppressing
    // those for the entire hot interval would silently drop the only
    // attempted recovery for those fault classes.
    //
    // Classified strictly per the CHRG_FAULT code being handled in THIS
    // switch - NOT by whether NTC_FAULT (bits 2:0, a physically separate
    // register field, already alerted independently above) happens to also
    // be nonzero on the same register read. NTC_FAULT and CHRG_FAULT can be
    // simultaneously nonzero without being the same event (e.g. a TS-network
    // fault occurring at the same poll as an unrelated VBUS input dip); OR-ing
    // ntcFaultCode into this flag previously suppressed remediation for that
    // independent, non-thermal CHRG_FAULT too.
    const bool thermallyCorrelatedFault = (chargeFault == 0x02);

    // Even for a non-thermal-correlated fault, if ChargeInhibit's DCT config
    // disable was READ BACK as set this cycle (System.getPowerConfiguration()
    // confirmed it, not merely the software safeToCharge intent), a raw PMIC
    // toggle here would likely fight a disable that is already in effect, or
    // about to be applied by Device OS's asynchronous reconciliation, at the
    // hardware level. If it was NOT read back as set, proceed anyway - this
    // fault may be exactly why the disable never took, and this is the one
    // path that could recover it.
    const bool configDisableBlocksToggle =
        !safeToCharge && !thermallyCorrelatedFault && chargeDisableConfigVerified;

    if ((!safeToCharge && thermallyCorrelatedFault) || configDisableBlocksToggle) {
      if (thermallyCorrelatedFault) {
        Log.info("PMIC: Fault detected but charging disabled due to temperature (%.1fC) - fault class is "
                 "thermally correlated, skipping remediation (recovers naturally)",
                 (double)current.get_internalTempC());
      } else {
        Log.info("PMIC: Fault detected, thermal disable config is read-back verified - deferring "
                 "remediation for this non-thermal fault class until charging is re-enabled");
      }
      // Freeze remediation state rather than abandoning it (ALSO FIX): if a
      // level-1/2 sequence already disabled charging in phase 0
      // (remediationPhase==1, i.e. pmic.disableCharging() already ran and it
      // is waiting to resume/re-enable) and this branch zeroed
      // remediationInProgress/ActiveLevel/Phase here, charging would be left
      // disabled at the PMIC with no software owner left to re-enable it.
      // Leaving the state untouched lets it resume - and finish, including
      // the re-enable step - the moment it is safe/no-longer-hardware-blocked
      // again, instead of stranding it disabled.
    } else {
      const unsigned long now = millis();
      if (remediationInProgress) {
        const unsigned long waitMs = (remediationActiveLevel == 2) ? 1000UL : 500UL;
        if (advanceChargeCyclePhase(pmic, remediationPhase, remediationPhaseStartMs, waitMs,
                                    remediationActiveLevel == 2, safeToCharge)) {
          Log.info(remediationActiveLevel == 2 ? "PMIC: Charging re-enabled with watchdog supervision"
                                                : "PMIC: Charging re-enabled after soft reset");
          if (remediationActiveLevel == 2) remediationLevel = 0; // Reset level after power cycle attempt.
          remediationInProgress = false;
          remediationActiveLevel = 0;
          remediationPhase = 0;
          lastRemediationAttempt = now;
        }
      } else if (now - lastRemediationAttempt > kRemediationCooldownMs) {
        if (consecutiveFaults >= 3 && remediationLevel < 2) {
          remediationLevel = 2; // Escalate to power cycle reset.
        } else if (consecutiveFaults >= 2 && remediationLevel < 1) {
          remediationLevel = 1; // Escalate to disable/enable charging.
        }

        if (remediationLevel == 1) {
          Log.warn("PMIC: Attempting soft remediation - cycle charging (level 1)");
          remediationInProgress = true;
          remediationActiveLevel = 1;
          remediationPhase = 0;
        } else if (remediationLevel == 2) {
          Log.error("PMIC: Attempting aggressive remediation - power cycle reset (level 2)");
          remediationInProgress = true;
          remediationActiveLevel = 2;
          remediationPhase = 0;
        } else {
          Log.info("PMIC: Fault detected but remediation level 0 - monitoring only");
        }
      } else {
        Log.info("PMIC: Fault detected but in cooldown period (%lu min remaining)",
                  (kRemediationCooldownMs - (now - lastRemediationAttempt)) / 60000UL);
      }
    }
  } else {
    // No faults on this read. WO-2026-08-25-001 Amendment C, Decision C3
    // point 2: this previously zeroed remediationInProgress / ActiveLevel /
    // Phase unconditionally whenever CHRG_FAULT read back clear - including
    // the case where a level-1/2 cycle had already run phase 0
    // (pmic.disableCharging()) and was waiting in phase 1 to re-enable.
    // CHRG_FAULT can read clear WHILE charging is disabled (disabling
    // charging is not itself a fault condition), so that unconditional clear
    // could strand charging disabled at the PMIC with no software owner left
    // to finish the re-enable step. Finish any in-progress remediation cycle
    // FIRST; only clear counters once it has actually completed.
    //
    // NOTE: a clear CHRG_FAULT read here does NOT by itself mean it is safe
    // to re-enable charging - the thermal ChargeInhibit path can independently
    // hold safeToCharge==false (e.g. a hot pack) with no PMIC fault at all.
    // advanceChargeCyclePhase() is passed safeToCharge and freezes the
    // sequence (no enableCharging(), state untouched) rather than advancing
    // it while unsafe; see that function's contract above.
    if (remediationInProgress) {
      const unsigned long now = millis();
      const unsigned long waitMs = (remediationActiveLevel == 2) ? 1000UL : 500UL;
      if (advanceChargeCyclePhase(pmic, remediationPhase, remediationPhaseStartMs, waitMs,
                                  remediationActiveLevel == 2, safeToCharge)) {
        Log.info(remediationActiveLevel == 2 ? "PMIC: Charging re-enabled with watchdog supervision"
                                              : "PMIC: Charging re-enabled after soft reset");
        if (remediationActiveLevel == 2) remediationLevel = 0; // Reset level after power cycle attempt.
        remediationInProgress = false;
        remediationActiveLevel = 0;
        remediationPhase = 0;
        lastRemediationAttempt = now;
      }
      // Whether or not this phase completed, do not fall through to the
      // fault-free counter clear below yet - a completed cycle still needs
      // one more no-fault poll to confirm charging stayed healthy before we
      // treat it as recovered, and an incomplete cycle must not have its
      // state clobbered mid-sequence.
      immediateResetConsumed = false;
      immediateResetFaultReg = 0;
    } else if (consecutiveFaults > 0) {
      // No faults, and any remediation cycle already finished cleanly -
      // clear counters if charging is healthy.
      immediateResetConsumed = false;
      immediateResetFaultReg = 0;
      Log.info("PMIC: Charging healthy - clearing fault counters");
      consecutiveFaults = 0;
      remediationLevel = 0;
      remediationInProgress = false;
      remediationActiveLevel = 0;
      remediationPhase = 0;

      const int8_t currentAlert = current.get_alertCode();
      if (currentAlert >= 20 && currentAlert <= 23) {
        Log.info("PMIC: Clearing battery/charging alert %d - charging resumed", currentAlert);
        current.set_alertCode(0);
        current.set_lastAlertTime(0);
      }
    }

    // Narrow recovery path for alert 21 (charge timeout/stuck charging).
    const int8_t currentAlert = current.get_alertCode();
    if (currentAlert == 21 && regs.chargeStatus != 2) {
      Log.info("PMIC: clearing alert 21 after charge recovery");
      current.set_alertCode(0);
      current.set_lastAlertTime(0);
    }
  }

  // BAT_FAULT (bit 3): battery overvoltage protection - a hardware condition,
  // separate from CHRG_FAULT, not resolvable by toggling charging, so it does
  // not increment consecutiveFaults or enter remediation.
  if (regs.faultReg & kPmicBatFaultMask) {
    Log.error("PMIC: Battery overvoltage protection active (BAT_FAULT)");
    current.raiseAlert(23);
  }

  return regs;
}

void logChargeDiag(const Registers &regs, float vcell, float soc, int powerSource) {
  const bool fault = (regs.faultReg & (kPmicChargeFaultMask | kPmicBatFaultMask)) != 0;
  const PowerReport &powerReport = PowerManager::instance().latestReport();

  // src= is deliberately the RAW, uncorrected `powerSource` sample, not the
  // PowerSourceOverride-corrected value - kept independent so it remains the
  // ground-truth evidence for diagnosing this bug class in the field.
  //
  // Consumers must read PowerDiag's `source=` for the effective value; treating
  // this raw src= as effective produces phantom VIN/UNKNOWN "power events" (a
  // confirmed 2026-08-28 Dev-09 instance, where battery telemetry was unchanged
  // throughout - see WO-2026-08-28-003). Documented for analysts in
  // docs/FIELD_MEANINGS_REFERENCE.md, "Mixed Corrected/Raw Power Source Values".
  Log.info(
      "ChargeDiag: chg=%s(%u) fault=0x%02x vbus=%s pg=%d th=%s vsys=%d vcell=%.3f soc=%.1f src=%s prof=%s",
      chargeLabel(regs.chargeStatus, fault), regs.chargeStatus, regs.faultReg, vbusLabel(regs.vbusStatus),
      regs.powerGood ? 1 : 0, regs.thermalRegulation ? "REG" : "OK", regs.inVsysMin ? 1 : 0, (double)vcell,
      (double)soc, PowerManager::powerSourceLabel(powerSource),
      PowerManager::compactProfileLabel(powerReport.activeInputProfile));

#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
  PowerDiagnostics::recordChargeDiagEvent(regs.chargeStatus, regs.faultReg,
                                           (regs.chargeStatus == 1 || regs.chargeStatus == 2), vcell, soc,
                                           powerSource, powerReport.activeInputProfile);
#endif
}

void trackAndReport(const Registers &regs, int powerSource, float soc, float vcell,
                     float finalAcceptedSoc) {
  const PowerReport &powerReport = PowerManager::instance().latestReport();

  // Solar-profile-vs-USB-vbus mismatch: warning + long-run SOC/Vcell trend,
  // sampled at least every 30 minutes (diagnostic-only, no behavior change).
  if (powerReport.activeInputProfile == PowerInputProfile::Solar35W && regs.vbusStatus == 1) {
    Log.warn("Power mismatch: prof=SOLAR vbus=USB fault=0x%02X src=%d", regs.faultReg, powerSource);
    const bool trendBaselineValid = (lastTrendSoc > -500.0f && lastTrendVcell > 0.0f && lastTrendEpoch > 0);
    if (trendBaselineValid && Time.isValid() && Time.now() > lastTrendEpoch) {
      const float elapsedHours = (float)(Time.now() - lastTrendEpoch) / 3600.0f;
      if (elapsedHours >= 0.5f) {
        Log.info("PowerTrend: soc=%.1f dsoc=%+.1f vcell=%.3f dv=%+.3f hrs=%.1f chg=%u", (double)soc,
                  (double)(soc - lastTrendSoc), (double)vcell, (double)(vcell - lastTrendVcell),
                  (double)elapsedHours, (unsigned)regs.chargeStatus);
        lastTrendSoc = soc;
        lastTrendVcell = vcell;
        lastTrendEpoch = Time.now();
      }
    } else if (Time.isValid()) {
      lastTrendSoc = soc;
      lastTrendVcell = vcell;
      lastTrendEpoch = Time.now();
    }
  }

  // Periodic (15 min, or source/profile-change-triggered) charge summary.
  {
    static bool baselineValid = false;
    static unsigned long baselineMs = 0;
    static float baselineSoc = 0.0f;
    static float baselineVcell = 0.0f;
    static int baselineSource = -1;
    static PowerInputProfile baselineProfile = PowerInputProfile::NotApplicable;

    const unsigned long nowMs = millis();
    const bool sourceOrProfileChanged =
        baselineValid && (baselineSource != powerSource || baselineProfile != powerReport.activeInputProfile);

    if (!baselineValid || sourceOrProfileChanged) {
      baselineValid = true;
      baselineMs = nowMs;
      baselineSoc = soc;
      baselineVcell = vcell;
      baselineSource = powerSource;
      baselineProfile = powerReport.activeInputProfile;
      if (sysStatus.get_verboseMode()) Log.info("Charge: base soc=%.2f v=%.3f", (double)soc, (double)vcell);
    } else if ((nowMs - baselineMs) >= kChargeSummaryIntervalMs) {
      const Observability::WakeCycleStats &stats = Observability::cycleStats();
      Log.info("Charge: soc=%.1f d15=%+.2f v=%.3f dv=%+.3f chg=%s src=%s prof=%s a=%lus c=%lus t=%lus",
                (double)soc, (double)(soc - baselineSoc), (double)vcell, (double)(vcell - baselineVcell),
                chargeLabel(regs.chargeStatus, regs.faultReg != 0), PowerManager::powerSourceLabel(powerSource),
                PowerManager::compactProfileLabel(powerReport.activeInputProfile),
                (unsigned long)(currentWakeAwakeMs() / 1000UL), (unsigned long)(stats.connect_duration_ms / 1000UL),
                (unsigned long)(stats.teardown_duration_ms / 1000UL));
      baselineMs = nowMs;
      baselineSoc = soc;
      baselineVcell = vcell;
      baselineSource = powerSource;
      baselineProfile = powerReport.activeInputProfile;
    }
  }

  // Stuck-fast-charging: >6h Fast Charging with no material SOC/Vcell gain
  // -> alert 21, unless plausibly explained by load (window just resets).
  {
    static uint8_t lastChargeStatus = 0xFF;
    static unsigned long chargeStateStartTime = 0;
    static float fastChargeStartSoc = -1.0f;
    static float fastChargeStartVcell = -1.0f;

    if (regs.chargeStatus == 2) { // Fast Charging
      if (lastChargeStatus != 2) {
        chargeStateStartTime = millis();
        fastChargeStartSoc = finalAcceptedSoc;
        fastChargeStartVcell = vcell;
      } else if (chargeStateStartTime != 0) {
        const float socGain = (fastChargeStartSoc == fastChargeStartSoc) ? (finalAcceptedSoc - fastChargeStartSoc) : 0.0f;
        const float vcellGain = (fastChargeStartVcell > 0.0f) ? (vcell - fastChargeStartVcell) : 0.0f;
        const bool meaningfulChargeProgress = BatteryAuthorityPolicy::stuckChargingHasMeaningfulProgress(
            fastChargeStartSoc, finalAcceptedSoc, fastChargeStartVcell, vcell);
        if (meaningfulChargeProgress) {
          chargeStateStartTime = millis();
          fastChargeStartSoc = finalAcceptedSoc;
          fastChargeStartVcell = vcell;
        } else if (millis() - chargeStateStartTime > 6UL * 3600000UL) { // 6 hours
          const unsigned long awakeMs = currentWakeAwakeMs();
          const Observability::WakeCycleStats &stats = Observability::cycleStats();
          const bool ambiguousHighLoad = regs.inVsysMin || regs.thermalRegulation || awakeMs > 300000UL ||
                                          stats.connect_duration_ms > 120000UL || stats.teardown_duration_ms > 10000UL;
          if (ambiguousHighLoad) {
            Log.warn("PMIC: Fast Charging 6+ hours with limited gain under load socGain=%.2f vcellGain=%.3f awake=%lu conn=%lu td=%lu vsysMin=%d thermal=%d",
                      (double)socGain, (double)vcellGain, awakeMs, stats.connect_duration_ms, stats.teardown_duration_ms,
                      regs.inVsysMin ? 1 : 0, regs.thermalRegulation ? 1 : 0);
            chargeStateStartTime = millis();
            fastChargeStartSoc = finalAcceptedSoc;
            fastChargeStartVcell = vcell;
          } else {
            Log.error("PMIC: Stuck in Fast Charging for 6+ hours with no material gain soc=%.1f socGain=%.2f vcell=%.3f vcellGain=%.3f",
                       (double)finalAcceptedSoc, (double)socGain, (double)vcell, (double)vcellGain);
            current.raiseAlert(21);
          }
        }
      }
    } else {
      chargeStateStartTime = 0;
      fastChargeStartSoc = -1.0f;
      fastChargeStartVcell = -1.0f;
    }

    lastChargeStatus = regs.chargeStatus;
  }
}

} // namespace PmicFaultMonitor
