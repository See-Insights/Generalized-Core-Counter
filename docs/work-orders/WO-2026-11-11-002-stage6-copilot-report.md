# WO-2026-11-11-002 Stage 6 report (Copilot)

**Model/level:** claude-sonnet-5.5; dispatched at medium. I cannot observe the level actually run.
**Interpreter:** Python 3.14.6; zsh 5.9 (arm64 macOS). Run inside the git worktree on `873328f` (`main` e8b15ad plus WO files). Nothing committed.

## Baseline (before editing)
118/118 (68 `.sh` via zsh, 50 `.py` via python3), 0 failures.

## Step 0 (`src/cloud/ConfigApply.cpp`, this base)
Merge helpers: `getTopLevelMap` :10, `getNestedMap` :19, `getMergedIntValue` :29, `getMergedBoolValue` :43, `getMergedFloatValue` :57, `getMergedStringValue` :71. Device wins, then default, else the stored value is kept.

| Section path | Keys (`file:line`) | n |
|---|---|---|
| messaging (:184) | serial :197, verboseMode :205, verboseTimeoutMin :213 | 3 |
| sensor (:335) | type :350, setting1 :363, setting2 :379, setting3 :387, setting4 :395 | 5 |
| timing (:235) | timezone :249, reportingIntervalSec :264, openHour :282, closeHour :283, connectAttemptBudgetSec :304 | 5 |
| modes (:413) | sensorMode :430, connectionMode :445, reportingMode :463, samplingMode :479, cloudDisconnectBudgetSec :493, modemOffBudgetSec :506, enableHibernateSleep :518 | 7 |
| reporting.webhook (:536) | name :548, enabled :562 and :584 (same key, deduped), timeoutMs :570 | 3 |
| power.thermalChargeInhibit (:605) | armHighC :627, armLowC :628, releaseHighC :629, releaseLowC :630 | 4 |

**Count: 27 distinct. Matches the WO.**

Other reads of the defaults ledger: `has()`/`get()` in `ConfigApply.cpp` occur only inside the helpers (:10-80). `Cloud.cpp:635,680` only pass `defaults` to `applyConfigurationFromLedger`. `LedgerClient.cpp:9-14` (`ledgerHasConfigContent`) checks only section presence (`sensor`, `timing`, `messaging`, `modes`, `reporting`), not keys, and `LedgerClient.cpp:178` calls it. No key is read by any other route, so the test sees every key and no stop condition applies. Neither `src/Config.h` nor `ChargeInhibitPolicy.h` reads the ledger; they hold compiled defaults only. Note: the `power` section is not in `ledgerHasConfigContent`. That is outside this WO and left unchanged.

## Failing before
The new file did not exist, so I copied the stale root `default-settings-v3.23.json` to scratch and pointed the test at it (`DEFAULTS_COVERAGE_JSON`). Exit 1:
- missing: `modes.cloudDisconnectBudgetSec`, `modes.modemOffBudgetSec`, and the 4 `power.thermalChargeInhibit.*` keys.
- dead: `messaging.disconnectedMode`.
The parser found 27 keys, so the floor was not what failed.

## Test
`tests/default_settings_coverage_test.py`: 69 lines, standard library only. It resolves the repo from `__file__`, and exits 0 when run bare. The two optional `DEFAULTS_COVERAGE_SRC` / `DEFAULTS_COVERAGE_JSON` environment overrides exist only for the mutations. Mapping is by variable name: each `getTopLevelMap(defaults, ...)` or `getNestedMap(defaults, ...)` variable gets a section path, then each `getMerged*Value(<var>, …, "key")` resolves through it.

## Mutations (all exit 1)
| Mutation | Result |
|---|---|
| Delete `modes.enableHibernateSleep` from the JSON | FAIL, missing `modes.enableHibernateSleep` |
| Add `messaging.disconnectedMode` | FAIL, dead `messaging.disconnectedMode` |
| `"newKey"` read added to a scratch copy of `ConfigApply.cpp` | FAIL, missing `modes.newKey` |
| Regex broken to match nothing | FAIL, "only 0 distinct firmware keys (floor 25)" (plus 27 dead) |

Mutations used copies in `build-tmp/wo-2026-11-11-002-stage6/` (script `mutate.py`). No real file was changed or restored.

## Files
- Created `docs/config/default-settings-42131.json` (WO content verbatim).
- Deleted root `default-settings-v3.23.json` (not committed; shows as ` D`).
- Created the test and this report.
- `build-tmp/` is git-ignored; scratch files left in place.

## References to the old file name
**Live references to update: none.** The only non-historical hits are `docs/releases/v3.23-implementation-status.md` (and its generated `docs/html/` page), both historical release records. **Left unchanged:** those two, the `docs/work-orders/*transcript*`, `*-report.md`, `*-verdict.md` and dispatch/WO files (including `2026-10-04-pre-step6-known-issues-codex-dispatch.md`, `WO-2026-11-02-004-stage7-round2-verdict.md`, `WO-2026-11-03-003-checks-report.md`).

## Suite after
119/119 (68 `.sh` via zsh, 51 `.py` via python3), 0 failures. Test exits 0 bare, also when invoked by absolute path. `git diff --numstat -- src` is empty (0 lines).

## Deviations
None. Budget: `src/` 0 (actual 0); test 69 lines (≤ ~80).

## Open questions
- The `power` section is not counted by `ledgerHasConfigContent`: a defaults ledger with only `power` looks empty to the sync-timeout path. Pre-existing, out of scope.
- No claim made about hardware or the live console ledger; Chip must paste the new file into the console.
