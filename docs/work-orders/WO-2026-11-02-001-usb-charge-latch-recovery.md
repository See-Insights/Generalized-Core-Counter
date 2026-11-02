# WO-2026-11-02-001 — USB charge-latch recovery (recovery item 22, USB scope)

**Status:** APPROVED for Stage 6 by the architect; Chip authorizes the dispatch.
**Governing sections:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 5 (architecture approval gate), Stage 6 (implementation), Stage 7 (independent verification); §5 model routing (power logic is safety-relevant).
**Inputs:** `WO-2026-11-01-002-pmic-charger-reapply-step0.md` and its report (#168, 958fd35); recovery item 22 (LAB-01 USB account).

## Goal (plain)
If a USB-powered unit has good power but its charger sits at "Not Charging" for two hours on a battery that isn't full, toggle charging once (the existing level-1 cycle) and raise alert 21. Log the charger registers when it fires, so the next case shows us the cause.

LAB-01, 1 Nov: "Not Charging" at 3.71–3.75 V from 12:00 to 15:00Z on USB, no charge-fault bit, so the existing fault-driven toggle (`PmicFaultMonitor.cpp`, the `kPmicChargeFaultMask` path) never ran. It charged only after the 15:30Z failsafe reset.

## Rulings on the Step 0's five questions
1. **Scope:** USB-only first; N = 2 h. **Exclusion uses voltage only: skip at ≥ 4.05 V.** Drop the 80% SoC condition: item 23 showed the gauge can be stale, and a stale-high reading would block the recovery.
2. **Weak supply:** `powerGood` false → no action. If the cycle doesn't restore charging: at most one attempt per 24 h, alert 21 stays raised, no escalation to resets. No loop.
3. **Seam:** yes, expose the smallest "request level-1 cycle" seam from `PmicFaultMonitor`; reuse its existing machinery and cooldown. No new state machine.
4. **Gauge quick-start: out of this WO.** Not safe while charging (Step 0), MAX17043 datasheet not in the repo, and `SocTrust` already keeps the tiers correct (item 23). Revisit under item 23.
5. **Solar:** out until Trail02 register evidence exists.
6. **Added:** when the check fires, log REG01, REG08 and REG09 once (one `Log.warn` line). This is the pre-reset snapshot we lacked.

## Conditions (all must hold for N = 2 h continuously)
USB input present and `powerGood`; charge status "Not Charging"; no charge-fault or BAT_FAULT bit; battery voltage < 4.05 V; not thermally regulated; `safeToCharge`. Any condition breaking resets the timer (millis-based).

## Budget
- `src/`: **+20 lines hard** (net). Without the quick-start, expect under the Step 0's +18. Record budget vs actual at close.
- Tests: host tests for the condition (each clause breaks it; 2 h boundary; 24 h attempt limit; ≥ 4.05 V exclusion). Record total test lines, fixture lines and suite count against `0c565fa` (ruling 9).

## Agents
- **Stage 6 — Copilot:** standard model, medium reasoning (single-file, small).
- **Stage 7 — Codex:** top model (`gpt-5.6-sol`), highest reasoning, stdin closed, new output file per round (power logic, §5 escalation). Mandatory linkage, local toolchain build, `nm` and tests per Stage 7.

## Release vehicle
Not decided. Candidate: v49 alongside the binding-delete guard, after the AB1805 spike. Chip decides.

## Out of scope
Gauge quick-start; solar units; changes to the fault-driven toggle path; failsafe changes.

## Amendment 1 (2026-11-02, after Stage 7 round 1 NOT VERIFIED)
**Goal restated:** a USB unit that has good power but is stuck at "Not Charging" for 2 h gets one charging toggle per 24 h, an alert, and one register log line. Nothing more.

1. **Raise alert 22, not 21.** 22 is unused, and it already sits in the "charging resumed, codes 20–23" clear (`alert-codes.md` row 22; P:369–371), so it clears when charging resumes with no new clear code. Add its severity entry (3, matching 20 and 21). **Restore the alert-21 clear to the original `regs.chargeStatus != 2` exactly.** Stage 7 must confirm the 20–23 clear fires on Pre-charge, Fast and Done.
2. **USB gating:** use the existing corrected-source test (the `usbBackedFaultContext` expression: `activeInputProfile` / `isUsbBackedSource(...)`), hoisted so both paths share it, AND VBUS status. Raw VBUS alone is not enough (VIN can read as USB).
3. **Request the level-1 cycle through the existing path and honour its 1 h cooldown;** don't write the remediation fields directly.
4. **Drop** the `vcell = 99.0f` default parameter (update the test calls) and the `vcell > 2.5f` bound.
5. **Normal style:** no comma statements, no one-line struct.
6. **Docs in the same PR:** `docs/reference/alert-codes.md` row 22 (meaning, raise site, severity, clear) and row 21 unchanged apart from line references.

**Budget raised: `src/` +30 hard (was +20).** Reason: the reformat and the severity entry. If round 2 can't fit +30, stop and report rather than trimming style.
