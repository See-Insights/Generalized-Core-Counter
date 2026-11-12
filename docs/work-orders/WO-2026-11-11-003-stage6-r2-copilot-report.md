# WO-2026-11-11-003 Stage 6 round 2 report (Amendment 1)

**Model/level:** claude-sonnet-5.5. The dispatch asked for low; I cannot observe the reasoning level I actually ran at.
**Interpreter:** `.sh` via zsh, `.py` via python3 (CI discovery, `tests/*.sh` and `tests/*.py`).
**Status:** changes uncommitted.

## Baseline
120/120 (sh via zsh, py via python3) on the head before any change.

## Failing before the fix
New `tests/debounce_awake_pir_test.sh`, run on the current head:
```
FAIL awake first detection setting1=0: led=60000 ms (want 300000), logged=60 s (want 300)
FAIL awake already occupied setting1=0: led=60000 ms (want 300000), logged=60 s (want 300)
```
The two sleep-path cases already passed (300000 ms / 300 s).

## Test pattern
Closest to `debounce_expiry_c1_test.sh` (extract real source, compile on the host against fakes, mutate a copy).
The four real blocks of `State_Modes.cpp` (awake first detection, awake already-occupied, sleep first detection,
sleep already-occupied) are extracted by start/end marker lines and run with `setting1 = 0`: LED ms and logged seconds
must be 300000 / 300 in all four. Also checked: `setting1 = 60000` gives 60000/60, and `1500` awake gives 1000 ms / 1 s
(old truncation kept). `DEBOUNCE_AWAKE_SRC` points the test at a mutant copy. No `src/` change was needed to run it.

## `src/` change
`git diff --numstat -- src`: `2 4 src/state/State_Modes.cpp` (+2/-4, net -2; within the +4 budget). No helper added.
Both awake branches now do `debounceSeconds = Config::occupancyDebounceMsForRuntime() / 1000UL;` when the seconds are 0.
Removed: the "60 sec default" `Log.warn` (3 lines) and both `debounceSeconds = 60` literals.
Nonzero `setting1` is unchanged, including the `setting1=... => debounce=... sec` log line and the LED window
(`seconds * 1000`, so sub-second remainders still truncate as before).

## Mutation (backup/restore with `cp`; mutants were copies fed through `DEBOUNCE_AWAKE_SRC`)
| Mutation | Result |
|---|---|
| `debounceSeconds = 60` in the first awake branch | FAILED (led=60000, logged=60) |
| `debounceSeconds = 60` in the already-occupied branch | FAILED (led=60000, logged=60) |
| `Config.h` constant back at 60000 (`debounce_default_test.sh`) | FAILED (`default debounce is 60000`); restored with `cp`, diff confirms `src/` is only `State_Modes.cpp` |

## Build (fresh `BUILD_PATH_BASE`, Device OS 6.6.0, boron, `git archive HEAD` + modified `State_Modes.cpp`)
text 153028 / data 1114 / bss 2240, against round 1 153108/1114/2240: delta -80/0/0.

## Verification
Full suite: **121/121 (sh via zsh, py via python3)**. The first run was 120/121: `tests/sleep_wake_structural_test.py`
pins `State_Modes.cpp` against its base and rejected the new line. I updated its allow-list and base transform
for the two awake branches (test-only, `tests/` is in scope); it passes again.
I claim nothing about hardware.

## Docs (the Low)
Added the omitted hits to the "unrelated" classification in `WO-2026-11-11-003-stage6-copilot-report.md`
(`debounce_default_test.sh:3`, `debounce_expiry_c1_test.sh:35,52`, `firmware_update_dwell_test.sh:435`,
`pmic_fault_monitor_test.cpp:311`, `sleep_execution_equivalence_test.sh:166,168`, the two cloud-gate transcript hits,
the 200 execution-transcript hits). No other unclassified hit remained in `git grep -n 60000 -- tests`.

## Deviations
- The build's source copy was a `git archive` extract in the system temp dir (not under `build-tmp/`, to avoid the
  source-scanning structural test). A `rm -rf` of that temp dir was refused by the environment, so one directory
  `tmp.tYIXXIFKP4` under the user temp folder remains for you to delete.
- Mutant copies in `build-tmp/wo-2026-11-11-003-stage6-r2/` were removed; the scratch dir holds the build log and objs.

## Known gaps
- The new test's block extraction depends on comment/log marker lines in `State_Modes.cpp`; a rewording fails it loudly (EXTRACT_FAILED), not silently.
- `hasNonDefaultConfig()` still has no caller (from round 1). Anything else in the verdict beyond the Medium and the Low is not addressed.
