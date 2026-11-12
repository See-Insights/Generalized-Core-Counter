# WO-2026-11-11-002 Stage 6 round 2 report (Copilot), Amendment 1

**Model/level:** claude-sonnet-5.5; dispatched at medium. I cannot observe the level actually run.
**Interpreter:** Python 3, zsh (macOS), run in the git worktree. Nothing committed.

## Baseline (before editing)
119/119 (sh via zsh, py via python3), 0 failures. Discovery as in `.github/workflows/build.yml:45-52` (`tests/*.sh` via zsh, `tests/*.py` via python3).

## Failing before (new behaviour; the round-1 test would have passed all three)
Scratch copies in `build-tmp/wo-2026-11-11-002-stage6-r2/` (script `mutate.py`), pointed at with `DEFAULTS_COVERAGE_DIR` / `DEFAULTS_COVERAGE_SRC`; the bare run uses `docs/config/`.

## Test
`tests/default_settings_coverage_test.py`: **79 lines** (was 69), stdlib only. It globs `default-settings-*.json` in the config dir, fails if none, and per file checks: no missing keys, no dead keys, at least 25 leaf keys. It prints PASS/FAIL lists per file. The parser floor (25 firmware keys) is kept. Bare run exits 0.

## README evidence (`docs/config/README.md`, 15 lines)
Code agrees with the table; no stop condition.
- `src/cloud/ConfigApply.cpp:344-347` declare `setting1..4`; `:363,379,387,395` read them via `getMergedIntValue` and store with `set_sensorSetting1..4`.
- `sensor.type` accepts only 1: `ConfigApply.cpp:351` (`validateRange(sensorType, 1, 1, ...)`); `src/MyPersistentData.h:493` "1=PIR".
- `setting1` is the debounce in ms: `MyPersistentData.h:494`; `src/state/State_Common.h:319-324` (`occupancyDebounceMs()`); `src/state/State_Modes.cpp:76-78` (`setting1Raw / 1000` seconds, "setting1 is milliseconds" at :241); `src/state/State_Sleep.cpp:611-612`; `src/Config.cpp:97-100` (missing/zero is ConfigInvalid). Runtime fallback `DEFAULT_OCCUPANCY_DEBOUNCE_MS = 60000` at `src/Config.h:35`.
- `setting2..4` unused for type 1: the only consumer outside persistence and the status hash (`DeviceStatusPublisher.cpp:103-106`) is `src/sensors/SensorManager.cpp:336` (`setting2` polling interval), reached only for non-interrupt sensors; the PIR is interrupt-driven (`SensorManager.cpp:324-330` returns early). `setting3/4` have no readers at all.
- The 300000 production value is the value in `default-settings-42131.json:3`; it is not a firmware constant (only the 60000 fallback is).

## Mutations (all in scratch copies)
| Mutation | Exit | Result |
|---|---|---|
| R1: delete `modes.enableHibernateSleep` | 1 | missing `modes.enableHibernateSleep` |
| R1: add `messaging.disconnectedMode` | 1 | dead `messaging.disconnectedMode` |
| R1: `getMergedIntValue(... "newKey" ...)` in copy of `ConfigApply.cpp` | 1 | missing `modes.newKey` |
| R1: all `getMerged` renamed so the regex matches nothing | 1 | "only 0 distinct firmware keys (floor 25)" (plus dead keys) |
| R2: scratch config dir with no default file | 1 | "no default-settings-*.json files in ..." |
| R2: second file, key removed | 1 | 42131 PASS; 99999 missing `modes.enableHibernateSleep` |
| R2: second file, dead key added | 1 | 42131 PASS; 99999 dead `messaging.deadKey` |
| Control: second file identical | 0 | both PASS |

## Suite after
119/119 (sh via zsh, py via python3), 0 failures. Test exits 0 bare. `git diff --numstat -- src` is empty (0 lines). `docs/config/default-settings-42131.json` untouched.

## Deviations
- The round-1 env var `DEFAULTS_COVERAGE_JSON` (a single file) is replaced by `DEFAULTS_COVERAGE_DIR` (a directory); `DEFAULTS_COVERAGE_SRC` is kept.
- My first regex mutation replaced only the first `getMerged`, so it did not fail; I fixed the script to replace all and re-ran. The table shows the corrected result.

## Open questions
None. Changes are uncommitted: `tests/default_settings_coverage_test.py` modified; `docs/config/README.md` and this report new.
