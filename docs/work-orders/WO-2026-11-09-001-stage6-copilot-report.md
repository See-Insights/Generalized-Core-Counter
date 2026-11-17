# WO-2026-11-09-001 Stage 6 report (Copilot): storm detector counts PANIC and UNKNOWN

**Model:** claude-sonnet-5.5 (self-reported). **Reasoning level:** dispatched medium; the level actually run **could not be observed** from inside the session.
**Interpreter:** Python 3.14.6; zsh 5.9 (arm64-apple-darwin26.0); GNU Arm Embedded 10.2.1; Device OS 6.6.0 Boron build, buildscripts 1.17.2 toolchain. Nothing committed, pushed or flashed. **No hardware claim.**

## Result
Implemented per WO change 1 (inline-switch shape). **Tests: 123/123 (sh via zsh, py via python3)**, measured baseline before editing **121/121**. `src/` net **-6** (`git diff --numstat 1d1d5af -- src`: `+1 -7`, one file). Build 151092 / 1114 / 2256 (head 151084 / 1114 / 2256; +8 text).

## Step 0 (read-only, on 1d1d5af)
1. Switch `src/Generalized-Core-Counter.cpp:338-352`; uses `:357-393` (warn at 357-361, count increment 363-367, window reset 376-382, trip 385+). `RESET_REASON_NONE=0, UNKNOWN=10, WATCHDOG=60, PANIC=130` are enumerators of `System_Reset_Reason` in `services/inc/system_defs.h` in both 6.4.1 (lines 51-68) and 6.6.0 (52-69); all 16 enumerators identical between the two. `USER_APPLICATION` exists in neither.
2. Host tests find headers via `DEVICE_OS_ROOT` (default `~/.particle/toolchains/deviceOS/6.4.1`). `system_defs.h` compiles standalone with `-Wall -Wextra -Werror -pedantic` using only `-I` for `services/inc hal/inc hal/shared hal/src/gcc wiring/inc dynalib/inc` (the CI sparse-checkout set); no stubs and no `src/` change needed. Verified for 6.4.1 and 6.6.0.
3. Scan hits on enumerator names in `src/`: only `Generalized-Core-Counter.cpp:341` (`#ifdef RESET_REASON_PANIC`) and `:345` (`#ifdef RESET_REASON_UNKNOWN`), both the storm switch. **Other hits, not fixed (not enumerators; guard on the undeclared `RESET_REASON_USER_APPLICATION`, always false):** `:348` (storm switch, removed here), `:600` (`clearOomAlertOnBoot` switch) and `:621` (reset-count switch). Lines 600/621 are dead code, harmless today; they are for the architect.
4. Other consumers: `qualifiesForStormCount` is used only at the warn, count, window reset and trip lines in `setup()`. `bootStormLastResetReason` is stored from `reason` unconditionally (`:330`), independent of qualification; alert/forensics read the stored value and `bootStormAlertPending`. No other consumer interacts; the only behaviour change is that reasons 10 and 130 now count (wanted, rulings 2-3).
5. Net `src/`: -6 (within +8).

## Failing before
- `python3 tests/reset_reason_guard_structural_test.py` on the head: FAIL, `Generalized-Core-Counter.cpp:341` and `:345`.
- `tests/storm_reason_qualifies_test.sh` run with `STORM_TEST_SRC_ROOT` pointing at the head's file (`git show 1d1d5af:src/Generalized-Core-Counter.cpp`), under both 6.4.1 and 6.6.0: FAIL `RESET_REASON_UNKNOWN (10) qualifies=0`, `RESET_REASON_PANIC (130) qualifies=0` (and the integer sweep at 10 and 130). This is the defect reproduced.

## Change
`src/Generalized-Core-Counter.cpp`: the switch now has direct `NONE`, `PANIC`, `WATCHDOG`, `UNKNOWN` cases; the three `#ifdef`s and `USER_APPLICATION` are gone; one comment line added. Nothing else in the storm logic changed.

## Tests (new, all under `tests/`)
- `storm_reason_qualifies_test.sh` + `.cpp` (2a): extracts the real switch text from `setup()` (preprocessor lines kept), compiles it against the real `system_defs.h`, checks every `RESET_REASON_*` enumerator generated from the header (16 in both versions, so no absent-enumerator handling was needed; a version lacking one would just have fewer rows) plus an integer sweep -1..255. Only NONE/PANIC/WATCHDOG/UNKNOWN qualify. Passes with `DEVICE_OS_ROOT` 6.4.1 and 6.6.0 and with it unset (local default).
- `reset_reason_guard_structural_test.py` (2b): parses all enumerators from the real `system_defs.h` (45 in 6.4.1, 46 in 6.6.0) and fails on any `#ifdef/#ifndef/#if/#elif` in `src/` naming one. Inside the storm switch it also rejects any `RESET_REASON_*` guard (so the undeclared `USER_APPLICATION` is caught there).
- `retained_boot_structural_test.py` and `retained_boot_test.*`: unchanged, still pass.

## Mutations (each applied to the main file after cp backup; restored with cp, `cmp` identical)
| Mutation | Caught by |
|---|---|
| PANIC case removed | storm_reason_qualifies; retained_boot_structural |
| UNKNOWN case removed | storm_reason_qualifies; retained_boot_structural |
| `#ifdef RESET_REASON_PANIC` back around the case | storm_reason_qualifies; reset_reason_guard |
| `USER_APPLICATION` re-added behind `#ifdef` | reset_reason_guard (compile test cannot: the case is compiled out) |
| `PIN_RESET` added | storm_reason_qualifies |
| result inverted | storm_reason_qualifies |

## Build and nm
Device OS 6.6.0, fresh `BUILD_PATH_BASE` as a make argument, `EXTRA_CFLAGS=-frandom-seed=$$@`, scratch export of `src lib project.properties`: rc 0, **151092 / 1114 / 2256**. The decision is inlined in `setup` (`nm`: `000b5efc T setup`); `objdump` shows the compares against 60, 0, 10 (`b604e`, `b6066`, `b606c`) and 130 (`b628a`), the last two absent on the head. Scratch build outputs were removed from `build-tmp/` afterwards; only `build-tmp/wo-2026-11-09-001-stage6/build.log` remains.

## Verification (workflow §10)
CI-style discovery inside this worktree with `DEVICE_OS_ROOT` = 6.4.1: **123/123 (sh via zsh, py via python3)**. New tests also pass bare (no env). `thermal_coupling_structural_test.py` passes on a clean export of tracked files (scratch copies removed from `build-tmp/` first).

## Deviations
- **Preferred pure-function shape not used.** I first built it in `persist/RetainedBoot.h` (needs `#include "system_defs.h"`), but it broke `tests/facade_header_isolation_test.py` (the facade must compile standalone on the host with no Device OS headers; 122/123). Reverted fully and used the WO's fallback (inline switch, test extracts the real text). `retained_boot_test.*` and `retained_boot_structural_test.py` edits made during that attempt were also reverted.
- The structural scan's extra non-enumerator rule is limited to the storm switch (see Step 0 item 3) so that the dead `USER_APPLICATION` guards at `:600/:621` do not have to be fixed.

## Open questions
1. Should the dead `RESET_REASON_USER_APPLICATION` guards at `:600` and `:621` be removed (separate WO)? They are not enumerators, so the compiled-out behaviour there is as intended today.
2. The `retained_boot_test.cpp` mirror still hard-codes `0/10/60/130`; the real switch is covered by the new compile test, the mirror by the structural pin.
