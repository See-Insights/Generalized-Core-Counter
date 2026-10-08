# WO-2026-10-08-002: `sensor.type` from the ledgers reaches sensor creation (Step 6 WO 1c)

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §3, §12.1–§12.4.
**Base:** main after #77 (WO 1b), `23ccd81`. It was merged into `wo/2026-10-08-002-sensor-type` at `2404ad9`. Step 0 ran at `8f82e1b`; 1b doesn't touch the lines this WO changes.
**Recorded by:** Claude Code, from the architect's WO and rulings (2026-10-08). The WO ID was assigned by Claude Code.

## Plain goal

The `sensor.type` set in the ledgers is the value sensor creation uses, so a configured sensor type is never silently ignored.

## Step 0 (done)

Records: `docs/work-orders/WO-2026-10-08-002-step0-report.md` and its dispatch. A separate read-only session ran on `claude-sonnet-5-5` at `--effort high`. Result: **PROCEED, fix A.**

- **The bug:** the ledger writes `SensorSettings::sensorType` (the `/usr/sensor.dat` store, `ConfigApply.cpp:353`), but creation reads `SystemConfig::get_sensorType()`, which is `sysStatus.sensorType` (`SensorManager.cpp:307`, `Generalized-Core-Counter.cpp:1127`). Only its default of 1 ever writes that field.
- **History:** it never worked. The second field arrived in `217a4ae` (V3.23), and creation always read `sysStatus`. There is nothing to restore.
- **Migration gate:** the default `sensor.type` is 1 and no device overrides it (checked 2026-10-08).

## Architect's rulings (2026-10-08)

**Approved: fix A.** Sensor creation reads `SystemConfig::SensorSettings::get_sensorType()` at `SensorManager.cpp:307` and `Generalized-Core-Counter.cpp:1127`, replacing `SystemConfig::get_sensorType()`.

1. **Narrow `sensor.type` validation to 1** (`ConfigApply.cpp:351`, `validateRange(sensorType, 1, 1, ...)`, 0 lines). The recovery plan notes that WO 2b's sensor list should supply the allowed types.
2. **A type change takes effect at the next reboot.** No re-create after config apply. The v40 CHANGELOG line says so.
3. **Leave `sensorISR` (`Generalized-Core-Counter.cpp` around `:2711`) for WO 2a.**
4. **Silent sensor-start failure** is logged under WO 2b's items in the recovery plan.

Also, **repeat the 1c migration check** is now a v40 release step (recovery plan, "Rollout and follow-ups").

## Size budget

At most **+20** net `src/` lines; **about +2 expected** (two getter swaps, one validation limit, and comments). Going over means stop and report.

## Agents and models

- **Implementation:** Copilot, Sonnet tier, medium.
- **Verification:** Codex, `gpt-5.6-sol`, **medium** (two getter swaps and one validation limit).
- Every model is confirmed with a one-line probe (§5). The two-round rule applies.

## Release

**v40**, together with WO 1b. The CHANGELOG "Unreleased" line is written.

## Closing record (to be completed)
