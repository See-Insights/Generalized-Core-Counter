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

## Verification (Stage 7, preview)

Release ELF before and after: only the removed symbols disappear; no other symbol changes size. Text size drops. Tests `N/N (sh via zsh, py via python3)`. Fresh `BUILD_PATH_BASE`.

## Bench

None before merge (no behavior change). Rides the next release soak.

## Closing record

Budget versus actual (net, gross, text bytes), what was kept for 2b and why.
