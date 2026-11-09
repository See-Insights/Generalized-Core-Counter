# WO-2026-11-09-001 Stage 6 round 2: report

**Model:** claude-sonnet-5.5. **Reasoning level:** dispatched medium; the level actually run could not be observed from inside the session.
**Interpreter line:** zsh 5.9 (arm64-apple-darwin26.0); Python 3.14.6; Device OS 6.6.0 toolchain copy.
**Baseline before editing:** 123 test files (`tests/*.sh` + `tests/*.py`) in the worktree; `src/` numstat vs `1d1d5af`: `1 7 src/Generalized-Core-Counter.cpp` (net -6). I counted files before editing and ran the full suite only after.
Changes are uncommitted: `tests/storm_reason_qualifies_test.sh`, `tests/reset_reason_guard_structural_test.py`. The `.cpp` was not touched.

## Changes
- **F1** `tests/storm_reason_qualifies_test.sh:26-52`. The extractor takes the span from `bool qualifiesForStormCount` to the first `if (previousBootFailedEarly && qualifiesForStormCount)`. Comments are blanked, then it fails unless the span has exactly one `switch` and nothing else but the declaration (`bool qualifiesForStormCount = false;`). That one switch is compiled as before. The real `setup()` span (src lines 338-350) satisfies this with no `src/` change.
- **F2** `tests/reset_reason_guard_structural_test.py` (`guarded_hits`). Backslash-continued lines are joined into one logical directive and reported at the starting line. The storm-span check still uses the original physical line numbers (a directive counts as in-span if any of its physical lines is).
- **F3** not fixed. The module docstring now lists the limits: macro aliases, enumerators in quoted directive arguments, only `system_defs.h` enumerators, and brace/span checks not string-aware.

## Failing-before / passing-after
Old = `HEAD` tests; new = edited tests. Mutations ran on copies in `build-tmp/wo-2026-11-09-001-stage6-r2/` (since removed). The `.sh` ran with `STORM_TEST_SRC_ROOT` on the mutated copy. The `.py` ran from a copied repo layout. Exit 0 = pass, 1 = fail.

| Mutation | old .sh | old .py | new .sh | new .py |
|---|---|---|---|---|
| (a) correct decoy `switch (reason)` before the live one, POWER_MANAGEMENT added to the live one | 0 (escapes) | 0 | **1** | 0 |
| (b1) `qualifiesForStormCount = true;` before the switch | 0 | 0 | **1** | 0 |
| (b2) a `for` loop before the switch | 0 | 0 | **1** | 0 |
| (b3) a second assignment after the switch | 0 | 0 | **1** | 0 |
| (c) a `src/` header with `#if defined( \` newline `RESET_REASON_PANIC)` | 0 | 0 (escapes) | 0 | **1** |
| (d1) PANIC case removed | 1 | 0 | 1 | 0 |
| (d2) UNKNOWN case removed | 1 | 0 | 1 | 0 |
| (d3) `#ifdef RESET_REASON_PANIC` guard back | 1 | 1 | 1 | 1 |
| (d4) PIN_RESET added | 1 | 0 | 1 | 0 |
| (d5) result inverted | 1 | 0 | 1 | 0 |

The unmutated tree passes both new tests (16 enumerators / 46 enumerators).

## Verification
- CI-style discovery in this worktree (`tests/*.sh` via zsh, `tests/*.py` via python3): **123/123 (sh via zsh, py via python3)**. My first run read 122/123 because my own scratch `src/` copies tripped `thermal_coupling_structural_test.py`. After I removed the scratch copies, it passes.
- No new helper files.
- Toolchain 6.6.0 copy and `DEVICE_OS_ROOT=~/.particle/toolchains/deviceOS/6.6.0`: both tests pass. With 2.1.0, 6.3.4 and 6.4.1 both tests fail with the version message.
- `git diff --numstat 1d1d5af -- src` is still `1 7` (net -6).

## Deviations
None. I did not read the transcript file or the Stage 6 round 1 report.

## Open questions
- The span check treats any preprocessor line inside the span as executable code, so it fails on one. This is stricter than the WO text; say if you want it relaxed.
- No hardware claims.
