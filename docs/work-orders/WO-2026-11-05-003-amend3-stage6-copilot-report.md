# WO-2026-11-05-003 Amendment 3 ruling 1 - RTC write read-back, Stage 6 report

AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: low (the dispatch asked for medium; the harness ran this session at low) · python3 3.14.6 (.py), zsh (.sh). Nothing committed, pushed, merged or flashed. Bench not run.

## Diff summary
- `src/time/ExternalRtc.h`: new `kWriteVerifyToleranceSec = 2`; `set()` returns false if `hal_rtc_set_time` fails, then reads the chip back with `get()` and returns false if the read fails or `|readback - written| > 2`. `setFromSystem()` and its `Time.isValid()` precondition are unchanged; `Clock` and the skew hook keep their bool contract.
- `tests/stubs/external_rtc/Particle.h`: `noStick` (write succeeds, chip unchanged), `writeSkew` (chip stores written + skew), `readbackResult` (get error after a set).
- `tests/external_rtc_test.cpp`: equal passes; +2 and -2 pass; +3 and -3 fail; no-stick fails (also via `setFromSystem()`); read-back error fails; write error fails; precondition test unchanged.
- `tests/external_rtc_structural_test.py`: structural check that `get(back)` follows `hal_rtc_set_time` inside `set()`; 5 new mutations.

## Net `src/` lines
This change +3 (4 added, 1 removed). S1 total `git diff --numstat 06aacd9 -- src`: **+19** (was +16), within +20.

## Tests
Host suite: **116/116** (`*_test.sh` via zsh, `*_test.py` via python3, run in the checkout). `thermal_coupling_structural_test.py` also passes on a clean export of tracked files.
Mutations, all caught: no read-back; threshold removed (any difference passes); read-back error ignored; wrong sign (`back - t <= tol`, no abs); read-back placed before the set (structural). Existing mutations still caught.

## Build and nm
Device OS 6.6.0, Boron, gcc-arm 10.2.1, fresh `BUILD_PATH_BASE=build-tmp/wo-2026-11-05-003-amend3/bpb` (passed as a make argument): compiled; text/data/bss **152692 / 1114 / 2240** (Stage 6: 152604, so +88 text).
`nm`: `hal_rtc_get_time` and `hal_rtc_set_time` present. `set()` is inlined, so objdump of its caller `checkClockResync()` shows `bl hal_rtc_set_time` followed by `bl ExternalRtc::get(long long&)` (which calls `hal_rtc_get_time`), i.e. both reachable from `ExternalRtc::set`.

## Caller impact
No stop condition hit. `set()` has two callers: the clock resync write-back and the one-shot bench skew hook; one extra I2C read there is neither time-critical nor on a hot path.

## Deviations
- Reasoning level differs from the dispatch (above).
- My first build attempt passed `BUILD_PATH_BASE` as an environment variable, which the Device OS makefile overrides; it failed at link and wrote under the repo's git-ignored `target/`. I removed only `target/6.6.0/boron/wt-s1.*` that this build produced; the successful build's output is there now. Fixed by passing it as a make argument.
- The hardware behaviour (WRTC no-op) is not exercised here; bench step 2 remains.
