# Boot-storm detector and bench image 5096: rulings needed (9 Nov 2026)

Written by Claude Code at Chip's request, because the question did not reach the architect in chat. A plain file; nothing was committed from this checkout. Evidence is in PR #215 (merged or open: `docs/handover/2026-11-14-v50-bench-checklist.md` and `docs/handover/2026-11-08-v50-readiness-review-notes.md`, item 4) and in the scratchpad probe described below.

## 1. The finding (verified by a real build)
The boot-storm detector in `setup()` (`src/Generalized-Core-Counter.cpp:338-352`, on the integration head `1d1d5af`, and in v49.1) decides whether a reset counts toward a storm with a `switch (reason)`:

```
case RESET_REASON_NONE:
#ifdef RESET_REASON_PANIC
case RESET_REASON_PANIC:
#endif
case RESET_REASON_WATCHDOG:
#ifdef RESET_REASON_UNKNOWN
case RESET_REASON_UNKNOWN:
#endif
#ifdef RESET_REASON_USER_APPLICATION
case RESET_REASON_USER_APPLICATION:
#endif
  qualifiesForStormCount = true;
```

In Device OS 6.6.0 these are **enumerators** (`services/inc/system_defs.h`: `RESET_REASON_PANIC = 130`, `RESET_REASON_UNKNOWN = 10`, `RESET_REASON_WATCHDOG = 60`, `RESET_REASON_NONE = 0`), not macros, and nothing defines macros of the same names. `#ifdef` on an enumerator is false, so those cases are compiled out. `RESET_REASON_USER_APPLICATION` does not exist in 6.6.0 at all.

**Proof:** I built the integration head with the Device OS 6.6.0 toolchain (make rc 0) with `#pragma message` probes at the switch. All six names (`NONE`, `PANIC`, `WATCHDOG`, `UNKNOWN`, `USER_APPLICATION`, `PIN_RESET`) printed `NOT_A_MACRO`. (Scratchpad `probe-storm/`, a scratch export of tracked files; no repo change.)

**Consequence:** in the real firmware **only reasons 0 (NONE) and 60 (WATCHDOG) count** toward a storm. A panic loop (reason 130) or an unknown-reason loop (reason 10) never trips the storm protection (the 600 s holdoff and the alert). This is pre-existing (v49.1 and earlier). The structural test `tests/retained_boot_structural_test.py` passes because it checks only the source text (it asserts the `case RESET_REASON_PANIC:` line exists), not what the build compiles. It does not affect the failure mode seen in the field (the watchdog stalls are reason 60, which does count).

## 2. Rulings needed
1. **Fix it?** Recommended: yes, as a small test-first WO, not necessarily pre-RC1 (your call on priority; it is a gap in a safety net, not a regression). The fix: direct `case RESET_REASON_PANIC:` and `case RESET_REASON_UNKNOWN:` labels (both exist in 6.6.0), drop `USER_APPLICATION`, and a compile-level test (a host test that includes the real Device OS header and asserts the cases compile, or a structural test that fails on an `#ifdef` guarding an enumerator). Budget small (about -4 lines of `#ifdef`, +0 to +2).
2. **Count `UNKNOWN` (reason 10)?** Reason 10 is "unspecified"; the risk of counting it is a false storm only if the previous boot also failed early (the detector already needs that), so I lean to count it; your call. `PANIC` should count.
3. **Behaviour change on the fleet:** after the fix a panic/unknown loop would trip the storm holdoff (600 s ULP sleep). Confirm that is wanted.
4. **Bench:** the early-reset image (below) benches the storm path with reason 60. Do you also want a PANIC variant to show on hardware that a panic loop does **not** trip today (and does after the fix)? Optional; I did not build it.

## 3. Bench image 5096 (already built; no ruling needed unless you object)
Early-reset image, **bench-only, never released**: version 5096 (`v50.1-NativeRTC-EARLYRESET-BENCH-ONLY`), branch `bench/2026-11-09-earlyreset-5096-NEVER-RELEASE` @ `daa5b05cba092c7a908cc66b871ab404d3841fa6` (tree `3dee094f768512441b57a64b2f83d66f52c89a9c`), CI run 42562513244, SHA-256 `98e88b50d8d0d8a05ab650eeb9f1daf68409141a06cb6a1a5e75ea6843dd3088`, in `~/Downloads/v50-bench-kit-2026-11-14/` and its `SHA256SUMS.txt`. One added line after the storm block: while the retained `bootStormTripCount == 0` it calls `system_reset(SYSTEM_RESET_MODE_NORMAL, RESET_REASON_WATCHDOG, 0x5096, SYSTEM_RESET_FLAG_NO_WAIT, nullptr)`. It uses reason **60** because a `PANIC` reset would not count (section 1). Expected: boots 2 to 6 count 1 to 5, boot 7 trips (`BOOT STORM: 6 early resets detected`), the board sleeps about 600 s with the external watchdog stopped and restarted around the sleep (**pass: no reset during the sleep**), then the boot continues and the image stops resetting. The checklist (PR #215) has the step-by-step procedure, the power-cycle before and after, and the expected side effects (watchdogResetCount and alert 19 rise).

## 4. Also decided today (no action needed)
Rate check accepted (v49.1 reason-60 breadcrumb-28 rate 0.036 per device-day against 0.052 and 0.068; ratio 0.61, 95% CI 0.06-3.41). Wave 2 depends only on PCKL1 at the open (11:10Z, 11:30Z): reports and flushes the close = GO; silent = hold, and item 14 becomes pre-RC1 priority. Recorded in recovery plan item 14 (PR #216).
