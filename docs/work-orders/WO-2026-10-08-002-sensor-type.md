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

## Stage 6 and Stage 7 (2026-10-08)

- **Stage 6** (Copilot, `claude-sonnet-5.5`, medium):
  - **Code:** the two getter swaps, validation limited to 1, and comment fixes. **+1 net `src/` line.**
  - **Tests:** `sensor_type_source_test` and `sensor_type_validation_test` extract the real blocks from `src/` and fail loudly with `EXTRACT_FAILED`. Three mutations were caught. Suite 72/72 → 73/73 (sh via zsh, py via python3).
  - **ARM build:** 151004 / 1090 / 2196.
- **Pre-ruling (architect):** if `alert-codes.md` lists specific triggers for alert 41, add both new causes (1b's over-range interval and 1c's unsupported `sensor.type`). If the entry is generic, no change. Docs only, not a round.
- **Stage 7** (Codex, `gpt-5.6-sol`, medium): **VERIFIED WITH CONCERNS** (`WO-2026-10-08-002-stage7-verdict.md`). All 8 checks pass on the code, and four mutations were caught, including Codex's own A1-only mutation. Finding **F1 (low):** add a one-line v40 trigger note to alert 41 in `alert-codes.md`.
- **Disposition of F1 under the pre-ruling: no change.** The alert-41 entry (`alert-codes.md:44`) is generic ("Configuration apply failed at connect (C:566)") and lists no causes. Codex agrees it is generic.
  - **The tension, for Stage 8:** the file's rule (`alert-codes.md`, "Keeping this current") requires an update when a WO "changes how an alert is raised". Alert 41's raise site and condition ("apply failed") are unchanged; there are only two new ways for the apply to fail.
  - **Where the causes are recorded:** both are in CHANGELOG "Unreleased" for v40.
  - **Options:** the architect may still want the one-line note.

## Closing record (2026-10-08)

**Result: VERIFIED WITH CONCERNS** at Stage 7 round 1 (`WO-2026-10-08-002-stage7-verdict.md`). **Stage 8 approved** by the architect. It ships in **v40** together with WO 1b.

### Rounds and models

| Stage | Agent / model | Result |
|---|---|---|
| Step 0 | Claude Code, separate session, `claude-sonnet-5-5`, `--effort high` | PROCEED, fix A (never worked; no restore). Migration gate checked: default `sensor.type` 1, no device overrides |
| Stage 6 | Copilot, `claude-sonnet-5.5`, medium | +1; 72/72 → 73/73 |
| Stage 7 | Codex, `gpt-5.6-sol`, medium | VERIFIED WITH CONCERNS; one low finding (F1) |

Every model was confirmed with a one-line probe (§5).

### What changed

- **Sensor creation reads the ledger-written store.** `SensorManager.cpp:307` and `Generalized-Core-Counter.cpp:1127` call `SystemConfig::SensorSettings::get_sensorType()`. The `sysStatus.sensorType` field stays as an unused legacy field, kept for the layout. Its only production reader is the dead `sensorISR`, left for WO 2a.
- **`sensor.type` validated to 1** (`ConfigApply.cpp:351`). Other values are rejected; the field keeps its value, the apply reports failure, and alert 41 is raised at connect. Other fields still apply, as accepted in WO 1b.
- **A type change takes effect at the next reboot** (ruling 2).
- **F1 (low), the alert reference:** resolved at Stage 8. The architect added the line to alert 41 in `docs/reference/alert-codes.md`: "Common causes from v40: `reportingIntervalSec` above 65535; `sensor.type` other than 1." Docs only, not a round.

### Budget versus actual

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| WO-2026-10-08-002 | +20 (about +2 expected) | — | **+1** (8 added, 7 removed) | **73/73 (sh via zsh, py via python3)**; 2 new tests; 4 of 4 mutations caught |

ARM build: 151004 / 1090 / 2196 (−24 text against main `23ccd81`).

### Migration and release

- **Migration gate (2026-10-08):** `default-settings.sensor.type` is 1, and no `device-settings` instance sets `sensor`. **Repeat before the v40 OTA** (recovery plan, "Rollout and follow-ups").
- **Release:** v40, with WO 1b. CHANGELOG "Unreleased" carries both lines.
- **Bench:** none for this WO. With the fleet all at type 1, behaviour is unchanged. The v40 soak covers it.
