UNDER REVIEW: wo/2026-11-09-001-v50-storm-detector-reasons @ 8c82f1336b6996715bbbf394adf60d6ded2d548d

NOT VERIFIED

The firmware fix is correct, builds correctly, and has the intended behavior. The verdict fails on test fidelity: mutations demonstrated that the compile-level extractor can silently test the wrong switch, and the generalized guard scan has uncaught lexical forms.

| # | Verdict | Evidence |
|---|---|---|
| 1. `src/` change | PASS | `src/Generalized-Core-Counter.cpp:338-349` contains direct `NONE`, `PANIC`, `WATCHDOG`, `UNKNOWN` cases, without guards or `USER_APPLICATION`. No other `setup()` change. `src/` is +1/−7, net −6. |
| 2. Behavior/interactions | PASS | `Generalized-Core-Counter.cpp:330-394,540-550,1080-1084`. Reasons 10/130 count only with `previousBootFailedEarly`. Warn, increment, window reset, trip, holdoff and alert behavior are otherwise unchanged. |
| 3. Compile-level test | FAIL | `tests/storm_reason_qualifies_test.sh:26-40` can extract the first correct-looking switch while a later live switch has wrong behavior. |
| 4. Guard scan | FAIL | `tests/reset_reason_guard_structural_test.py:24,51-63` misses backslash-continued guards and macro aliases that expand to enumerators. |
| 5. Build/suite/mutations | FAIL | Build and 123/123 suite pass, but three relevant tests passed an intentionally wrong live-switch mutation. |
| 6. Other interactions | PASS | 5096 still counts reason 60. I21 touches a later `setup()` hunk; `git merge-tree` auto-merged the two source changes without conflict. No new holdoff or boot interaction found. |

Findings

1. Medium — `tests/storm_reason_qualifies_test.sh:26-40`

   Path that breaks: insert a correct, dead `switch (reason)` immediately after `qualifiesForStormCount`, followed by the live switch with `RESET_REASON_POWER_MANAGEMENT` added. The extractor selects the dead switch. `storm_reason_qualifies_test`, the guard scan, and `retained_boot_structural_test` all pass, while real firmware counts healthy hibernate reason 30.

   Smallest fix: extract and compile the complete decision span—from the real `bool qualifiesForStormCount` declaration to its first consumer—instead of only the first switch. Alternatively, require exactly one switch in that span and reject executable code before or after it.

2. Medium — `tests/reset_reason_guard_structural_test.py:24,51-63`

   Path that breaks: a valid directive split as `#if defined( \` followed by `RESET_REASON_PANIC)` in a `src/` header passes the scan.

   Smallest fix: coalesce backslash-continued physical lines into logical preprocessor directives while retaining the starting line number.

3. Low — `tests/reset_reason_guard_structural_test.py:51-63`

   Path that breaks: `#define ALIAS RESET_REASON_PANIC` followed by `#if ALIAS` passes. Conversely, an enumerator name inside a quoted directive argument produces a false positive.

   Smallest fix: reject object-like `#define` replacements containing parsed enumerators, and blank string/character literals before token matching.

Verification details

- Both Device OS 6.4.1 and 6.6.0 declare the same 16 reset reasons, including `NONE=0`, `UNKNOWN=10`, `WATCHDOG=60`, and `PANIC=130`. `project.properties` ships 6.6.0.
- CI checks out exact Device OS commit `74391b2c48d1a3a2d66f1583a28e50c613d2bbaf`, which is the local `v6.6.0` toolchain commit. Both tests reject a 6.4.1 header tree.
- Current compile test: PASS, 16 enumerators. Original `1d1d5af` through `STORM_TEST_SRC_ROOT`: FAIL at reasons 10 and 130.
- Guard scan: PASS on current source, 46 `system_defs.h` enumerators. No additional existing guarded enumerator was found. The two remaining `RESET_REASON_USER_APPLICATION` guards are not enumerators and are closed/out of scope.
- Direct `#ifdef`, `#ifndef`, `#if defined(X)`, `defined X`, and `#elif defined` forms are caught, including inside any scanned `src/*.h` regardless of conditional inclusion. Enumerators declared only in other HAL/system headers are not covered.
- Compile extraction preserves preprocessor lines, but brace counting is not comment/string aware.

Behavior audit

- Reasons 30 (S2 hibernate), 20 or fallback 140 (S3 recovery), 70 (OTA), 50 (brownout), 40 (power-down), and USER 140/data 2 or 6 remain nonqualifying.
- A normal cold boot cannot be counted solely because it reports 10 or 130: retained `bootInProgress` must also show that the previous boot failed before `setup()` completed.
- `bootStormLastResetReason` and data remain written unconditionally. Qualification only affects the warning, counter increment, trusted-window restart value, trip, holdoff and pending alert.
- Bench 5096 uses WATCHDOG reason 60, which counted before and still counts.

Build and tests

- HEAD: text/data/bss **151092 / 1114 / 2256**
- Integration base `1d1d5af`: **151084 / 1114 / 2256**
- Delta: **+8 / 0 / 0**
- `nm`: `setup` at `0x000b5efc`, size `0xe34`.
- `objdump`: qualifying path compares 60, 0, 10 and 130; base compares only 0 and 60.
- CI-style discovery in the real worktree: **123/123**, with no initial failure from either known load-sensitive test.
- Thermal structural test passed on a clean tracked-file export.
- Tracked worktree and index remain clean; final HEAD rechecked exactly.

Mutation summary: POWER_MANAGEMENT addition caught; helper move failed loudly; direct `#if defined(PANIC)` caught by both tests; wrong live switch behind a correct decoy escaped; split guard escaped; macro alias escaped; removing only the compile test’s version guard was caught by the structural test’s independent version guard.

Model: `gpt-5.6-sol`; reasoning: `xhigh`.

Interpreter/toolchain: `/bin/zsh` 5.9; `/opt/homebrew/bin/python3` 3.14.6; Apple clang 14.0.3; GNU Arm Embedded 10.2.1; Device OS 6.6.0/buildscripts 1.17.2.

Round 1 of at most 2 — return to the architect for the test fixes. No hardware or flashing was run or authorized.