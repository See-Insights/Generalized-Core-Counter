UNDER REVIEW: wo/2026-11-09-001-v50-storm-detector-reasons @ ee297737560884c62a2d2998cc7af786fd873088

NOT VERIFIED

The firmware change remains correct, but both Round 2 test fixes can still be defeated.

| # | Verdict | Evidence |
|---|---|---|
| 1. `src/` change | PASS | [Generalized-Core-Counter.cpp:338](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-storm-r1/src/Generalized-Core-Counter.cpp:338) has direct `NONE`, `PANIC`, `WATCHDOG`, `UNKNOWN` cases, no guard or `USER_APPLICATION`. `src/` is byte-identical to `8c82f13`; net −6 versus base. |
| 2. Behaviour/interactions | PASS | Byte-identical to the Round 1-approved source. Reasons 10/130 count only with `previousBootFailedEarly`; all other consumers and storm behavior remain unchanged. |
| 3. Compile-level test | FAIL | [storm_reason_qualifies_test.sh:31](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-storm-r1/tests/storm_reason_qualifies_test.sh:31) searches the entire file and can select an earlier helper’s copied decision instead of `setup()`’s live switch. |
| 4. Guard scan | FAIL | [reset_reason_guard_structural_test.py:68](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-storm-r1/tests/reset_reason_guard_structural_test.py:68) inserts whitespace while coalescing, missing valid mid-token line splices. |
| 5. Build/suite/mutations | FAIL | Fresh build passes at expected size, but two relevant mutations escape the test net. |
| 6. Other interactions | PASS | `src/` is byte-identical to the Round 1-reviewed commit; therefore the approved holdoff, 5096, boot and I21-overlap conclusions are unchanged. |

## Findings

1. **Medium — [tests/storm_reason_qualifies_test.sh:31](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-storm-r1/tests/storm_reason_qualifies_test.sh:31)**

   Path that breaks: add a helper before `setup()` containing a correct `bool qualifiesForStormCount` declaration, correct switch, and the exact first-consumer spelling. Add `RESET_REASON_POWER_MANAGEMENT` to the actual `setup()` switch. The compile test, guard scan and retained structural test all pass, while real firmware counts healthy reason 30.

   Smallest fix: extract and bound the `void setup()` body first, then locate exactly one declaration, switch and first consumer inside that body.

2. **Medium — [tests/reset_reason_guard_structural_test.py:68](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-storm-r1/tests/reset_reason_guard_structural_test.py:68)**

   Paths that break:

   ```cpp
   #if defined(RESET_REASON_PAN\
   IC)
   ```

   and:

   ```cpp
   #i\
   f defined(RESET_REASON_PANIC)
   ```

   Both preprocess successfully, but the scan passes them because line 72 inserts a space where C/C++ translation removes the backslash-newline without whitespace.

   Smallest fix: concatenate continued physical lines without adding a space—ideally perform translation-phase line splicing before comment blanking—while retaining the first physical line number. Add both mutations.

## Verification

- Current tests pass against Device OS 6.6.0: 16 reset enumerators and 46 total `system_defs.h` enumerators.
- The integration base through `STORM_TEST_SRC_ROOT` fails specifically for `UNKNOWN` 10 and `PANIC` 130.
- `project.properties` specifies 6.6.0. CI checks out exact Device OS commit `74391b2c48d1a3a2d66f1583a28e50c613d2bbaf`, the local `v6.6.0` commit.
- Normal, multi-line, CRLF, trailing-space and block-comment-adjacent continuation probes behaved as expected. A directive wholly inside a block comment was ignored.
- No additional guarded enumerator exists in current `src/`.
- F3 was not re-raised: macro aliases, quoted directive arguments, enumerators outside `system_defs.h`, and general non-parser limitations remain recorded limits.

The F1 span rule catches a second live switch, executable code before the first consumer, a simple helper move, and a string containing `switch`. A harmless `#if 1` inside the switch passes because it is preserved and compiled; therefore “any preprocessor line in the span fails” is not literally true. The real span contains no directive, so there is no current false positive.

## Behaviour

Reasons 30, 20, 40, 50, 70 and 140 remain nonqualifying. That covers S2 hibernate, S3 power-off/fallback, power-down, brownout, OTA, and USER resets with data 2 or 6. A healthy completed boot reporting 10 or 130 does not count unless retained `bootInProgress` shows the previous boot failed early. Bench 5096 still uses reason 60, which counted before and still counts.

## Build and suite

- Fresh Device OS 6.6.0 Boron build: **151092 text / 1114 data / 2256 bss**
- Integration base/controller: **151084 / 1114 / 2256**
- Delta: **+8 / 0 / 0**
- `nm`: `setup` at `0x000b5efc`, size `0xe34`
- `objdump`: comparisons for 60 at `0xb604e`, 0 at `0xb6066`, 10 at `0xb606c`, and 130 at `0xb628a`
- Literal in-worktree discovery: **122/123**. The only failure was the thermal test finding pre-existing Round 1 scratch exports under `build-tmp/wo-2026-11-09-001-stage7-r1/`, which I was not authorized to alter. The thermal test passes on a clean tracked export, giving a source-equivalent **123/123**.
- Both known load-sensitive tests passed on their first run.
- Both new tests pass bare with default 6.6.0 headers.
- Final tracked worktree and index are clean; HEAD and `src/` identity were rechecked.

Model: `gpt-5.6-sol`; reasoning: `xhigh`.

Interpreter/toolchain: zsh 5.9 (arm64-apple-darwin26.0); Python 3.14.6; GNU Arm Embedded 10.2.1; Device OS 6.6.0; buildscripts 1.17.2.

Round 2 of 2, the last — return to the architect. No hardware, flashing, commit, push, merge or PR operation was run.