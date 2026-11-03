# (WITHDRAWN) WO-2026-11-02-002: an untrusted SoC must not push a healthy unit into CRITICAL (recovery item 23 sub-finding; v49 must-fix, gates wave 1)

**Author:** Claude Code, 2 Nov 2026, DRAFT for the architect's review · **Status:** WITHDRAWN (architect, 2 Nov: the 4x interval at about 48% is by design; PowerTier.cpp:27-29 already falls back to the voltage estimate). Kept as the trace; not dispatched.
**Branch:** `wo/2026-11-02-002-untrusted-soc-tier`, from `main`
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-7; §5 (battery/power logic is safety-relevant: Copilot top model, Codex top model); §13 (c), (j)
**Budget:** `src/` at most **+15 net**, hard. Stop and report rather than trim style.

## Plain goal
ChurchSt-2 (85.8%, hook `vc` 4.02-4.05, charging) was put on CONSERVING (2x) and then CRITICAL (4x) reporting on 2 Nov. An untrusted SoC makes the tier follow the cell voltage, and the voltage sample used was taken with the radio on, 0.14-0.15 V below rest. Intended behavior (architect): **an untrusted SoC gives a voltage-based tier, never worse than the voltage supports; charging at 3.8 V or more is never CRITICAL.**

## The trace (OBS, main `a7b69ce`; read, not run)
1. `State_Connect.cpp:521`: `measure.batteryState()` right after connect, radio on; the sample replaces the cached `vcell` (`SensorManager.cpp:242-251`) and sets `socTrust` using `radioActive` (`:811-813`). The status ledger is written next (`:523`).
2. The tier is re-evaluated live each time from the cache: `ReportingPolicyResolver::resolveRuntime` (`RuntimeReportingPolicy.cpp:38-47`) -> `BatteryAuthority::evaluateCurrent` (`BatteryAuthorityCommand.cpp:45-50`) -> `BatteryAuthority::evaluate` (`BatteryAuthority.cpp:19-39`), previous tier from `currentTier()`.
3. Untrusted: `BatteryTierGuard::socForTier` returns `restingSocFromVcell(vcell)` (`BatteryTierGuard.cpp:5-10`); knots 3.90 V = 50%, 3.88 V = 48% (`BatteryHealth.cpp:19-32,52-74`). Residual 85.8 - 50 = +35.8 against 28 (20 + radio 8): Untrusted (`:36-44,94-102`).
4. `calculateTier(50.0, CONSERVING)` = CONSERVING (the 50-55 hold); `calculateTier(48.0, CONSERVING)` = CRITICAL (`BatteryBackoffPolicy.h:19-41`). The floor `applyVcellFloor` acts only at 3.5 V (`BatteryTierGuard.cpp:12-18`).
5. The charge state never reaches the tier: `BatteryAuthority::evaluate(soc, vcellState, vcell, trust, previousTier)` has no charge input. Five call sites: `Generalized-Core-Counter.cpp:1098`, `State_Modes.cpp:227`, `State_Report.cpp:189`, `State_Sleep.cpp:1040`, `BatteryAuthorityCommand.cpp:49`.
6. Fleet, 2 Nov 12:00Z (`device-status` ledgers): only ChurchSt-2 is Untrusted and below HEALTHY; the 12 others are Trusted and HEALTHY.

## Changes (proposed; the architect picks A, A+B, or other)
**A. Charging floor (required).** Add a `charging` input to the tier pipeline and, when the SoC is untrusted and the cell is charging at `vcell >= 3.8 V`, floor the voltage-derived SoC at the CONSERVING band (55%), so the tier is never CRITICAL (at worst CONSERVING). Pieces: `BatteryTierGuard::socForTier(gaugeSoc, vcell, trust, charging)` (+3), `BatteryAuthority::evaluate` gains the argument and passes it (+1), a cached charging flag in `SensorManager` set where the trust is cached (`SensorManager.cpp:815`; +3 with an accessor), and the five call sites (+0 to +1 each). No default parameter (round-1 lesson, WO-2026-11-02-001): every caller passes it. Estimate **+10 to +12**.

**B. Radio-on samples cannot lower the tier (recommended with A; the 11:00Z case was not charging).** A loaded voltage is not a resting voltage. Keep the last radio-off `vcell` and trust for policy: in `batteryState()`, when the radio is on and a pre-radio sample exists, do not overwrite `_cachedBatteryVcell` / `_cachedSocTrust` (the existing pre-radio "authoritative" sample is the precedent, `SensorManager.cpp:795-801`). Exception: a radio-on `vcell` at or below the 3.5 V critical floor still overwrites (a real brownout must not be hidden: `PowerTier.h`). Estimate **+4 to +5**. Total A+B **about +15**: at the limit; if it does not fit, ship A and report.

**Out of scope:** the knot table, the 20-point and +8 radio thresholds (need data: the 378-sample analysis had no radio samples), the gauge re-estimate (item 23), the post-connect ledger write.

## Tests
- Host: `socForTier` untrusted + charging + 3.88 V gives CONSERVING, not CRITICAL; untrusted + not charging + 3.88 V unchanged (CRITICAL band); charging + 3.79 V not floored; charging floor does not apply when Trusted; `evaluate` pass-through; the five callers pass the real flag (structural test, like `pmic_latch_call_site_structural_test.py`).
- B: a radio-on sample with a lower `vcell` leaves the cached `vcell` and trust unchanged; a radio-on sample at 3.5 V or below overwrites; a radio-off sample overwrites.
- Mutations: drop the 3.8 V bound, drop the charging term, lower the floor to 50, drop the exception at 3.5 V, restore the overwrite.

## Evidence to re-check after the fix (v49 soak)
ChurchSt-2 ledger `tier` and `effectiveIntervalSec` after each connect; hourly reports continue; no unit shows Untrusted with a tier below HEALTHY at a hook `vc` of 4.0 V or more.

## Agents
Stage 6 Copilot top model, high (power logic). Stage 7 Codex `gpt-5.6-sol`, xhigh, new output file per round, stdin closed. Controller runs the suite and the local ARM build with `nm`.
