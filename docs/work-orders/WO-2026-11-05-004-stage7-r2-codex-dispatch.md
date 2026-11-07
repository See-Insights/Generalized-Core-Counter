AGENT: Codex · MODEL: gpt-5.6-sol · REASONING: xhigh
AUTHORIZATION SCOPE: read the repository; run host tests (zsh for .sh, python3 for .py), the local Device OS 6.6.0 toolchain build and nm/objdump; scratch `build-tmp/wo-2026-11-05-004-stage7-r2/` (create it; use only that); read Device OS from `~/.particle/toolchains/deviceOS/6.6.0` / NOT authorized: editing any tracked file, committing, pushing, merging, flashing, creating PRs or tags, deleting anything you did not create (including `build-tmp/` itself). Your complete verdict goes in your final message (saved with `-o`).
UNDER REVIEW: wo/2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake @ edcebbcd75513f928467ce4aa8009918fe8d4ed1
TIME-BOX: 90 minutes from dispatch (deadline 13:20:39Z); a run without a verdict at the deadline is stopped, its partial output archived uncounted. This is round 2 of 2, the last: a failure goes to the architect.

# Stage 7 dispatch, round 2: WO-2026-11-05-004 (v50 piece S2)
**First line of your verdict: quote the `UNDER REVIEW` line above, with the full SHA.** Check `git rev-parse HEAD` equals it first. A verdict without the exact SHA is invalid.
Binding spec: `docs/work-orders/WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md` **including Amendments 1-4** (Amendment 4 holds the architect's rulings on your round 1 findings F1-F6), your round 1 verdict `docs/work-orders/WO-2026-11-05-004-stage7-r1-verdict.md`, and the Stage 6 round 3 report `WO-2026-11-05-004-stage6-r3-copilot-report.md` (reasoning level not observable: review as if written without that assurance). Base: `integration/v50-ab1805` @ 3b5cfb8. Diff: `git diff 3b5cfb8 HEAD -- src tests`; fixes only: `git diff 86b11e0 HEAD -- src tests`. **Budget: net `src/` <= +20** (reported -133; recount).

## Judge each (PASS / FAIL / NOT VERIFIED, file:line)
1. **F1** sentinel 0xA5 compared exactly; legacy 2, 0 and other values are "no record"; named constant; test and mutation.
2. **F2** flush immediately after the boot clear; after each `deepPowerDown()` returns, evidence cleared and flushed; failsafe caller also downgrades stage 3; the calls themselves unchanged; check the downgrade value really removes stage-3 evidence and has no side effect on recovery staging; a power loss between the boot clear and the flush.
3. **F3** reason 30 = successful HIBERNATE wake everywhere (code, comments, tests, names); no timing arm; the 1,800 s test restated; the GPIO-wake case; the residual (early button wake passes) stated plainly.
4. **F4** stage-3 evidence rejected when `Time` is invalid.
5. **F5** `ab1805WakeReason\":\"N/A\"` restored in the status payload; buffer size adequate for the worst case (check the truncation path); `watchdogSource` values unchanged from Amendment 1.
6. **F6** the equivalence fixture and the structural tests compare the base `hibernateAttempt()` again with only the approved S2 substitutions normalised (`tests/s2_hibernate_normalise.py`, `tests/s2_transcript_normalise.py`): judge that the normalisers are narrow and cannot hide an unapproved change; the fixture regeneration (137 -> 121 scenarios, the 16 dropped are retired scenarios): confirm that claim; the four new case families.
7. **Regression since round 1:** anything the fixes broke in items 1-9 of your round 1 table (alarm, chip watchdog at every sleep path, wake classification, linkage, S1/S1b untouched, `SysData`/`retained` layouts).
8. **Budget, build, suite, mutations.** Recount net `src/`. Build (fresh `BUILD_PATH_BASE` as a make argument; Copilot r3: 151508 / 1114 / 2240; r1: 151468). Host suite N/N (controller: 118/118; thermal test only on a clean export of tracked files). At least four of Copilot's mutations plus two of your own not used before (the controller used: sentinel `== 0xA5` -> `!= 0`, boot flush removed, invalid-Time arm `timeValid &&` -> `!timeValid ||`; all caught).
9. **Anything else** at boot, sleep, wake or the S3 hand-off.

## Verdict format
First line: the `UNDER REVIEW` line. Then **PASS or NOT VERIFIED for S2 as scoped**; a table of 1-9; for each finding: severity, file:line, what breaks, the smallest fix and whether it fits +20; the model and reasoning level you actually ran; the interpreter line; the build numbers. Round 2 of 2. Say plainly what you could not run (no hardware: the 8 h nights, the watchdog proof and the post-POWER_OFF reason are Chip's bench).
