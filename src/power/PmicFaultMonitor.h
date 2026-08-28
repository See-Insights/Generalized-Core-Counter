#pragma once

#include <stdint.h>

class PMIC;

// PMIC fault monitoring/remediation/telemetry for the BQ24195 (Boron/cellular
// only). SEPARATE concern from F2a's thermal ChargeInhibit: this reacts to
// charging FAULTS the PMIC itself reports (input/thermal-shutdown/
// safety-timer/battery-overvoltage) via escalating remediation, while
// ChargeInhibit proactively disables charging on enclosure temperature before
// a fault occurs. Both run every battery sample; remediation here always
// yields to `safeToCharge == false` (never fights the thermal inhibit).
//
// Alerts: 20=thermal shutdown, 21=charge timeout/stuck charging,
// 23=battery/general charge fault. Remediation: level 0 monitor-only,
// level 1 soft reset (2+ consecutive faults), level 2 power cycle + watchdog
// (3+ consecutive faults), 1h cooldown, auto-clears when healthy.
namespace PmicFaultMonitor {

// One PMIC register snapshot (REG08 status + REG09 fault).
struct Registers {
  uint8_t faultReg = 0;
  uint8_t systemStatus = 0;
  uint8_t vbusStatus = 0;
  uint8_t chargeStatus = 0;
  bool powerGood = false;
  bool thermalRegulation = false;
  bool inVsysMin = false;
};

// Reads REG08/REG09, attempts an immediate charge-fault reset when safe and
// USB-backed, then runs/advances the escalating non-blocking remediation
// state machine. Returns the resulting snapshot (post-reset, if attempted).
//
// `chargeDisableConfigVerified` is ChargeInhibit::ApplyResult::configReadbackVerified
// from this same cycle's ChargeInhibit::apply() call - whether
// System.getPowerConfiguration() read back the thermal charge disable as
// set, NOT whether the PMIC hardware has actually stopped charging yet
// (Device OS applies this asynchronously - see ChargeInhibit.h). Used as a
// same-cycle proxy, per fault class, for whether a raw PMIC toggle here
// would likely fight a disable that is already in effect or about to take
// effect (WO-2026-08-25-001 Amendment B Blocker 4; renamed under Amendment C
// Decision C3 point 3).
Registers pollAndRemediate(PMIC &pmic, bool safeToCharge, uint8_t battState, int powerSource,
                            bool chargeDisableConfigVerified);

// Compact per-cycle ChargeDiag log line (also carries the trace fields
// previously logged separately) + pdiag forensic record. Reuses the
// Registers snapshot from pollAndRemediate() rather than re-reading the PMIC.
void logChargeDiag(const Registers &regs, float vcell, float soc, int powerSource);

// Solar/USB mismatch trend telemetry, periodic (15 min, or source/profile
// change triggered) charge summary, and stuck-fast-charging detection (>6h
// Fast Charging with no material SOC/Vcell gain under load -> alert 21).
void trackAndReport(const Registers &regs, int powerSource, float soc, float vcell,
                     float finalAcceptedSoc);

} // namespace PmicFaultMonitor
