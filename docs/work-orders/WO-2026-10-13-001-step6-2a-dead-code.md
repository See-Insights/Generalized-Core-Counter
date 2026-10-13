# WO-2026-10-13-001: Step 6 WO 2a — remove dead code

**Author:** architect, 2026-10-13 · **Status:** Step 0 (read-only)
**Branch:** `wo/2026-10-13-001-step6-2a-dead-code`, from current main
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 2 and Stages 5–8; §12.1 (history first); §13 (b), (c), (g), (j)

## Why now

Chip, 2026-10-13: structure before more field fixes. The Grok architecture review (`docs/architecture-review-2026-09-03.md`) asked to shrink the four cores; since then `State_Sleep.cpp` grew (1,715 → 1,812) and `MyPersistentData` grew (~1,842 → 2,064), while `SensorManager.cpp` is unchanged (1,418). 2a is the one Step 6 WO that only removes code, and it clears the way for 2b (sensor shape).

## Plain goal

Delete code that nothing in a release build calls, with **no change in device behavior**.

## Candidates (from the Codex Step 6 ownership report, `docs/work-orders/2026-10-06-step6-ownership-codex-report.md` §3, plus later findings)

1. The PMIC charge-cycle test, `SensorManager.cpp` around `:1275–1418` (its build flag is already retired, `BuildProfile.h:283`).
2. Uncalled `SensorManager` methods: `setup`, `getSensorData`, `getSignalStrength`.
3. Other unused interfaces and state from the Codex table: Cloud's two backoff wrappers, PowerManager capability/availability/context wrappers, `PowerPlatform::hasFuelGauge`, the PowerTier label, the factory name helper, unused `PowerInputs`, the write-only setup flag, the PIR ISR count.
4. `sensorISR` (`Generalized-Core-Counter.cpp:116`, `:2709`): defined, never attached (WO-2026-10-07-001 evidence).
5. The two stubs the Grok review named: `src/state/StateHandlers.cpp` (a 9-line note) and the empty `src/sensors/SensorData.h`.
6. `BatteryAuthority::clearLowBatteryMode()` if any remnant remains (WO-002 deleted it; confirm).

**Not in scope:** any persisted field (layout must not change; `samplingMode` stays, 2b may use it); any move or rename (that is 2b and later); anything a test-mode or bench build uses unless that build flag is itself retired.

## Budget

Net `src/` lines **≤ 0**, target **−180 to −280**. Gross additions to `src/` **0** (deletions only; at most a forward-declaration or include removal). Test changes counted separately. ARM text size must not grow.

## Agents and models

- **Step 0:** Claude Code, own session, Sonnet tier, **medium** reasoning, after a probe. *Why:* mechanical caller search and symbol checks; no design.
- **Stage 6:** Copilot, Sonnet tier, **medium**.
- **Stage 7:** Codex `gpt-5.6-sol`, **medium**.

## Step 0 questions (read-only; report ≤ 150 lines, committed and pushed on the WO branch)

1. For each candidate: re-check at current main that nothing calls it — source search **and** the release ELF (`nm`), including test-mode/bench builds still in `BuildProfile.h`. Mark each *dead*, *used (where)*, or *moved*. Give the exact line range and line count.
2. History first (§12.1): for `sensorISR` and the PIR ISR count only, say whether they were ever the seed of the interrupt reading mode the generalized design needs (Chip, 2026-10-07). If so, list them as *keep for 2b* rather than delete.
3. Which host tests reference any candidate, and what they would need.
4. Total net lines, split into two commits: (a) the charge-cycle test alone, (b) everything else — per the Codex report's advice to isolate the large deletion.
5. **Stop and report** if any candidate is used by a release build, if removal needs any `src/` addition beyond deleting a declaration, or if the total is under −100 (then 2a may not be worth a separate WO; say so).

## Step 0 (done) and the architect's rulings (2026-10-13)

`WO-2026-10-13-001-step0-report.md`: **PROCEED**, about −450 net `src/` lines, with 0 additions. Step 0 ran as a separate session on `claude-sonnet-5-5` at `--effort medium`. Every candidate is dead in the release build and in the live test and bench builds. `sensorISR` and `_isrCount` are deleted, not kept for 2b. `clearLowBatteryMode` was already gone.

Rulings:
1. **Include `setupComplete_` (3k).** A modified line that only removes code is allowed: `PowerManager.cpp:90`, `: report_(), setupComplete_(false)` → `: report_()`.
2. **Delete `src/cloud/BatteryBackoff.cpp` entirely,** including its history comments. The history stays in git and WO-2026-09-21.
3. **Delete the doc example at `SensorManager.h:43–49`.** It names the deleted `setup()` and `getSensorData()`.
4. **~~Add `PowerManager::profileSelectionReasonLabel`~~ — WITHDRAWN (architect, 2026-10-13).** It is live: called unqualified at `PowerManager.cpp:206`, and inlined, so it has no ELF symbol. Copilot was right to keep it. Original ruling: add `PowerManager::profileSelectionReasonLabel` (`PowerManager.cpp:264–268`, `PowerManager.h:266–274`). It has no caller and no ELF symbol.
5. **Fix `ARCHITECTURE_OVERVIEW.md:11` and `:88`,** which name the two deleted stubs.
6. **List the 2b leftovers in the closing record:**
   - `PowerDiagnostics::availabilityLabel` and `batteryContextLabel`;
   - `PowerCapabilities::hasFuelGauge`;
   - `ISensor::setup()`;
   - `sensorDetect`, which is never written after 2a;
   - the legacy `SystemConfig::get_sensorType()`.
7. **Stage 7:** `PIRSensor::pirISR()` may shrink, by losing `_isrCount++`, and so may `PowerManager::PowerManager()`, by losing `setupComplete_`. No other symbol may change size.
8. **Two commits:** (a) the charge-cycle test alone, then (b) everything else.

**Stage 7 pre-ruling (architect, 2026-10-13):** the `SensorManager::batteryState` lambda's +8 bytes, the `SystemClass::powerSource()` `.isra` clone that is now inlined, and the `CSWTCH.*` renumbering are **accepted as observations**, provided Codex confirms each is compiler fallout from a deletion, with no logic change.

## Verification (Stage 7, preview)

Release ELF before and after: only the removed symbols disappear; no other symbol changes size. Text size drops. Tests `N/N (sh via zsh, py via python3)`. Fresh `BUILD_PATH_BASE`.

## Bench

None before merge (no behavior change). Rides the next release soak.

## Closing record (2026-10-13)

**Result:** **VERIFIED WITH CONCERNS** at Stage 7 round 1 (`WO-2026-10-13-001-stage7-verdict.md`), with no FAIL. **Stage 8 approved** by the architect. **Release: v43.** Don't merge until product v42 is uploaded from the `ff1715c` build.

| Stage | Agent / model | Result |
|---|---|---|
| Step 0 | Claude Code, separate session, `claude-sonnet-5-5`, `--effort medium` | PROCEED, est. −450 |
| Stage 6 | Copilot, `claude-sonnet-5.5`, medium | −460 net (+1/−461); kept ruling 4's function as live; 76/76 |
| Stage 7 | Codex, `gpt-5.6-sol`, medium | VERIFIED WITH CONCERNS; every extra symbol change traced to a deletion |

Every model was probed first (§5). One round; no controller edits.

| Item | Budget | Actual | Gross `src/` additions | ARM text |
|---|---|---|---|---|
| Commit (a): PMIC charge-cycle test | ≤ 0 | **−158** | 0 | 151460 → 151444 |
| Commit (b): everything else | ≤ 0 | **−302** | 1 (ruling 1's modified line) | → 151372 |
| **WO total** | ≤ 0, target −180 to −280 | **−460** | **1** | **−88 bytes**; bss 2220 → 2212; data unchanged |

- **Tests:** **76/76 (sh via zsh, py via python3)** before and after. No test changed.
- **Deleted:**
  - the charge-cycle test;
  - `SensorManager` `setup`, `getSensorData` and `getSignalStrength`, and the doc example;
  - all of `BatteryBackoff.cpp`, with the two Cloud wrapper declarations;
  - the `PowerManager` wrappers (`capabilities`, `hasPmic`, `hasFuelGauge`, `availabilityLabel`, `batteryContextLabel`), `PowerInputs` and `setupComplete_`;
  - `PowerPlatform::hasFuelGauge`, `PowerTier::label` and `SensorFactory::getSensorTypeName`;
  - `PIRSensor::_isrCount` and `sensorISR`;
  - `StateHandlers.cpp` and `SensorData.h`, plus their two `ARCHITECTURE_OVERVIEW.md` entries.
- **Ruling 4 withdrawn:** `PowerManager::profileSelectionReasonLabel` is **kept as live**. It is called unqualified at `PowerManager.cpp:206` and inlined, so it has no ELF symbol. Step 0 searched only the qualified name. Copilot kept it, and Codex confirmed. §13 (c) now requires bare-name caller searches.
- **Accepted symbol changes:**
  - ruling 7: `pirISR()` 0x18→0x0c and `PowerManager()` 0x60→0x5c;
  - the pre-ruling, each confirmed as compiler fallout: the `batteryState` lambda +8 with the `powerSource()` `.isra` clone inlined, and `CSWTCH.*` renumbering;
  - accepted under ruling 1 at Stage 8: `PowerManager::setup()` 0x32→0x2e and the `PowerManager` instance 0x38→0x34.
- **Kept for 2b, and why.** Each is dead or orphaned after 2a, but removing it touches a live branch, an interface, or a persisted-field accessor:
  - `PowerDiagnostics::availabilityLabel` and `batteryContextLabel`, whose only callers were the deleted wrappers;
  - `PowerCapabilities::hasFuelGauge`, set at `PowerPlatform.cpp` and never read;
  - `ISensor::setup()`, still in `PIRSensor`'s vtable;
  - `sensorDetect`, never written after `sensorISR`'s removal but still read in Sleep and Idle;
  - the legacy `SystemConfig::get_sensorType()`, which has no reader but whose persisted field stays.
- **Bench:** none, since there is no behavior change. It rides the v43 soak.
