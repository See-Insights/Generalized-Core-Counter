# WO-2026-10-29-002 Stage 7 verdict

**VERIFIED WITH CONCERNS** — all concerns are **Low**.

Verifier: Codex `gpt-5.6-sol` (probed 2026-10-29), reasoning **high**.

## Evidence

1. **Closed-only gate / unchanged Open and Unknown — verified.** The shared helper is exactly
   `Clock::openness() == Clock::Openness::Closed` (`src/state/State_Common.h:61-63`). The awake
   path still consumes `loop()` and gates only the resulting action (`src/state/State_Modes.cpp:63-71`);
   the PIR-wake path uses the same helper before any start/extension (`:212-218`). `openness()` returns
   Unknown before hour evaluation when trust/config is absent (`src/time/Clock.cpp:567-577`), and 0-24
   is always Open by `hour >= 0 && hour < 24` (`:14-17`, `:579-583`), so neither is clamped
   (`src/state/State_Common.h:355-361`). Open and Unknown start/extend and close unclamped in the real
   extracted code test (`tests/occupancy_report_by_mode_test.cpp:745-764`); `hour_rules_test.sh` also
   passed its real-source 0-24 checks.

2. **COUNTING and cleanup — verified unchanged.** COUNTING remains its separate awake branch
   (`src/state/State_Modes.cpp:29-53`) and PIR-wake branch (`:196-211`); the diff has no hunk in either.
   `State_Report.cpp` has no diff: an occupied session is still explicitly closed through `boundary`
   (`src/state/State_Report.cpp:54-61`), and the `close == 24` restart remains after cleanup
   (`:77-82`). The structural test pins both (`tests/occupancy_ends_at_close_structural_test.py:53-56`).

3. **Closing and next-day fields — verified by trace.** The due session is closed before `publishData`
   (`src/state/State_Report.cpp:54-70`); the helper clears `occupied` after adding the boundary-limited
   duration (`src/state/State_Common.h:395-405`, `:428-430`). Thus `PIRSensor::reportFields` emits
   closing `occupancy:0` with the credited old-day total (`src/sensors/PIRSensor.cpp:25-29`). Only after
   that publish, `dailyCleanup()` invokes the daily reset (`src/state/State_Report.cpp:77-82`, `:306-327`),
   which zeros occupied/session/total fields (`src/sensors/PIRSensor.cpp:14-22`). Closed detections cannot
   repopulate them, so the next report emits `dailyoccupancy:0` through the same `reportFields` lines.

4. **Clamp and callers — verified.** `todayAt(24)` selects next midnight; otherwise it selects today's
   configured close (`src/time/DailyBoundary.cpp:9-23`). `check(now)` subtracts one day whenever that
   candidate is still ahead (`:26-45`): before open it yields yesterday's close, after close today's,
   and for `close == 24` the most recent midnight. The Closed clamp uses that boundary (`src/state/State_Common.h:355-361`).
   Changed default-close callers are `"modes"` (`src/state/State_Modes.cpp:151-154`), `"idle"`
   (`src/state/State_Idle.cpp:58-62`), and `"sleep"` (`src/state/State_Modes.cpp:174-178`).
   `"daily-cleanup"` passes the same explicit boundary and is unchanged (`src/state/State_Report.cpp:56-61`).
   `rg` finds no other production callers.

5. **Required concern rulings.** (a) **Low:** the Closed PIR return precedes the low-battery refresh
   (`src/state/State_Modes.cpp:215-233`), an ancillary behavior change; however the next open-hours wake
   samples and commits policy before event routing (`src/state/State_Sleep.cpp:1016-1053`), so intended
   open-hours behavior is not stale. No dedicated Closed-refresh test exists. (b) **Low, accepted:** the
   untouched carry-over sets occupied=true/start=max(boundary,start) (`src/state/State_Report.cpp:79-82`),
   while the ruled-in clamp makes its eventual post-midnight credit zero; this is within the architect's
   gate-plus-clamp ruling. The closing report is 0 before restart; a later report while carried is correctly
   occupied=1/dailyoccupancy=0. (c) **Low:** awake logging is once per ignored detection (`src/state/State_Modes.cpp:66-70`);
   production uses `SerialLogHandler`, not flash storage (`src/cloud/Particle_Functions.cpp:13-23`), so only
   bounded serial-log volume is at issue, and the dispatch expressly allowed one unrated line. (d) **Low,
   ruled out:** return false resumes the sole caller (`src/state/State_Sleep.cpp:1053-1110`); Closed suppresses
   opportunistic reporting, non-CONNECTED routes back to sleep, CONNECTED falls to Idle, and progress was
   marked before return (`src/state/State_Modes.cpp:212-217`). No new software wake loop is introduced.
   (e) **Low:** the Stage 6 report says +305/−1 tests, but the present diff plus the 86-line untracked test is
   **+291/−1**. This still substantially exceeds the +100 guideline; repetition across five connection modes
   and overlapping behavioral/structural mutants is conservative, not gap-masking.

6. **Production linkage — verified.** In addition to the clamp call (`src/state/State_Common.h:359`), real
   production callers are the awake and wake sites (`src/state/State_Modes.cpp:67`, `:215`). Inlining explains
   the absent helper symbol; the reproduced `nm` caller-size growth is consistent with these call sites.

7. **Test adequacy — sufficient, with Low gaps.** The extracted-real-code cases cover close/end, both gates,
   no extension, awake/sleep clamp, zero night carry, close-24, Open, and Unknown
   (`tests/occupancy_report_by_mode_test.cpp:650-765`). Behavioral mutations catch gate removal, `!= Open`,
   either missing site, and clamp removal (`tests/occupancy_report_by_mode_test.sh:516-531`); structural
   mutations catch leaving the close session open and changing carry-over (`tests/occupancy_ends_at_close_structural_test.py:70-85`).
   The four required mutations therefore prove their targets. Gaps: the named “first report” assertion checks
   backing state rather than serialized payload (`tests/occupancy_report_by_mode_test.cpp:715-724`), and its
   boundary is injected rather than computed (`:634-638`). Existing passing payload/reset and DailyBoundary
   source-fidelity tests plus the code trace above cover both, but a direct end-to-end payload case would be better.

Verification run: occupancy behavior+mutations, new structural+mutations, clock openness, hour rules, daily
boundary, PIR reset+mutations, payload golden+mutations, session restart, and sleep-wake equivalence/structure
all passed; `git diff --check` passed.
