# WO-2026-11-11-003 Stage 6 round 3: Copilot report (Amendment 2)

Model: claude-sonnet-5.5. Level: dispatched medium; I cannot observe the level actually run, so I make no claim beyond that.
Interpreter: sh tests via `zsh`, py tests via `python3` (CI's discovery, inside the git worktree). Nothing committed, pushed or flashed.

## Baseline
`121/121 (sh via zsh, py via python3)` on the dispatched HEAD, before any edit.

## Tests first (failing on the dispatched head)
- (a) `tests/debounce_awake_pir_test.sh` rewritten: faithful fake (`isValid` switch; helper returns stored `setting1` when nonzero and valid, else 300000),
  matrix `setting1` = 0, 1, 999, 1000, 60000, 300000 plus invalid config with `setting1` 60000, in all four PIR blocks, and the real
  `occupancyDebounceMs()`/`debounceRemainingMs()` from `State_Common.h`; LED window, logged seconds and remaining debounce must be the one ms value.
  Head: 12 FAILs: awake first/re-arm `setting1` 1 and 999 gave a 0 ms LED window; all four paths with invalid config gave 60000 for LED and expiry (not 300000).
- (b) New `tests/debounce_sleep_plan_cap_test.sh` (see the pattern below). Head: `setting1` 0 gave a 60 s cap (want 300); 1 and 999 gave 0 s; 1001 gave 1 s; invalid config gave 60 s.
- (c) New `tests/debounce_single_source_structural_test.py`. Head: it named `State_Modes.cpp:76,77,81,97,99,240,258`, `State_Sleep.cpp:611,612` and `State_Common.h:324`.

## Sleep-plan test pattern
The closer real-code pattern was the extract-and-run style of `debounce_expiry_c1_test.sh`: the occupied branch is extracted from `State_Sleep.cpp`
and the wrapper and predicate from `State_Common.h`, compiled against fakes, with `remaining == 0` and a 3600 s scheduled wake.
Caps asserted: 0 -> 300 s, 1000 -> 1, 60000 -> 60, 300000 -> 300; sub-second 1 and 999 -> 1 s (deterministic round up), 1001 -> 2; invalid config with 60000 -> 300; an earlier scheduled wake (120 s) is kept.
`tests/sleep_plan_equivalence_test.*` stays the transcript check (see Existing tests).

## Fix
Item 3 of the sleep plan: work in ms, `const uint32_t debounceMs = Config::occupancyDebounceMsForRuntime();`; the seconds are formed only where `wakeInSeconds` is assigned,
as `(debounceMs - 1) / 1000UL + 1`, i.e. round **up** (ceil; the helper is never 0). I did not use the `remaining` branch's `/1000 + 1`: that adds a 1 s margin after a known deadline,
and for 1000 ms and 60000 ms it would change the cap from the existing 1 s and 60 s to 2 s and 61 s. Ceil keeps both exactly and gives sub-second values a deterministic 1 s instead of 0.

`src/` lines (`git diff --numstat -- src` on this HEAD): `State_Modes.cpp` +9/-23, `State_Sleep.cpp` +3/-4, `State_Common.h` +1/-2; total +13/-29.
Against `main` (`git diff --numstat origin/main -- src`): `Config.h` +1/-1, `State_Common.h` +1/-2, `State_Modes.cpp` +9/-25, `State_Sleep.cpp` +3/-4;
total **+14/-32 (net -18)**, within the net 0 budget (round 2 was +3/-5).
The awake first-detection info log now prints the effective value: `OCCUPANCY detection: debounce=%lu ms (%lu sec)`.

## Behaviour change of item 4 (the wrapper)
`occupancyDebounceMs()` now returns `Config::occupancyDebounceMsForRuntime()`. A device with an **invalid config and a nonzero `setting1`** previously expired its
debounce (and capped the closing credit) at `setting1`; it now uses 300000 ms, as the awake and sleep paths already did. Valid config: unchanged. The case is in the matrix (invalid + 60000 -> 300000 everywhere).

## Mutations (mutant copies of `src/` in `$TMPDIR`, removed afterwards; nothing restored with git)
| Mutation | Caught by |
|---|---|
| hard-coded 60 in the first awake branch | `debounce_awake_pir_test.sh`, structural test |
| hard-coded `60 * 1000UL` in the re-arm branch | `debounce_awake_pir_test.sh` |
| `debounceMs / 1000UL * 1000UL` in the first awake branch | `debounce_awake_pir_test.sh`, structural test |
| same in the re-arm branch | `debounce_awake_pir_test.sh`, structural test |
| `State_Sleep` `: 60000` fallback back | `debounce_sleep_plan_cap_test.sh`, structural test |
| wrapper back to its own fallback | `debounce_awake_pir_test.sh`, structural test |
| constant back to 60000 | `debounce_default_test.sh` |
`sleep_plan_equivalence_test.sh` also ran its own mutations (including "debounce divisor"), all caught.

## Existing tests changed (test-only)
Fakes made faithful to `Config.cpp` (stored `setting1` when nonzero, else the default): `debounce_expiry_c1_test.sh`, `occupancy_session_restart_test.sh`,
`untrusted_piece_c2_test.sh`, `sleep_wake_equivalence_test.cpp`, `sleep_plan_equivalence_test.cpp` (the helper emits the one `get_sensorSetting1` call the real one makes).
`fixtures/sleep_plan_transcript.txt` regenerated: exactly 3 scenarios change (`set1=0` occupied -> 300 s; `set1=999` -> 1 s; closed `set1=0` -> 300 s); `norm_base` skips those for the base comparison.
`sleep_plan_structural_test.py`, `sleep_preflight_structural_test.py`, `sleep_wake_structural_test.py` map the base text to the new text.
The "wake=0 reason=debounce" log-line check became "wake=1".

## Docs (the Low)
`stage6-copilot-report.md`: `debounce_awake_pir_test.sh:77,78,80` classified as explicit-setting inputs (not the compiled default).
`stage6-r2-copilot-report.md`: the "Nonzero `setting1` is unchanged" statement corrected: for 1-999 ms the round 2 awake window was 0 ms; round 3 restores the exact ms value.

## Verification
Full suite: **123/123 (sh via zsh, py via python3)** (121 plus the two new tests). Intermediate run 117/123, fixed as listed above.
Device OS 6.6.0 build (boron, gcc-arm 10.2.1, `make` in `modules/boron/user-part`, fresh `BUILD_PATH_BASE`, `-frandom-seed`, source copy in `$TMPDIR`, removed afterwards):
text/data/bss **152964 / 1114 / 2240**; round 2 153028/1114/2240 (-64); `main` 153108/1114/2240 (-144). I claim nothing about hardware.

## Deviations and known gaps
- `State_Common.h:~400` (untrusted-piece credit: `(lastOccupancyEvent - occupancyStartMs + occupancyDebounceMs()) / 1000UL`) was not on the Step 0 list. It sums milliseconds and converts once to seconds, a duration, not a debounce conversion; left unchanged and allowed by name in the structural test.
- `build-tmp/wo-2026-11-11-003-stage6-r3/` exists and is empty (all scratch was in `$TMPDIR`).
- The structural test is textual (statement-level taint by assignment); an indirect derivation through a struct field would not be seen.
