# WO-2026-10-25-001: the occupied sleep wakes at the time left in the debounce, not the full debounce (recovery item 20)

**Author:** architect, 2026-10-25 · **Status:** ready (Step 0 first)
**Branch:** `wo/2026-10-25-001-debounce-sleep-remaining`, from main. It ships in **v47** with #142 (P7-A) and #151 (the failsafe fix); the PR is marked "do not merge until v46 is cut".
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.1–12.4; §13 (b), (c), (g), (j)
**Evidence:** recovery item 20 and its Codex Step 0 (in #154); the exerciser baseline for 24 Oct (`2026-10-24-v45-baseline-day1-result.md`). On Dev-09, v45, ten sessions spanning the hourly report were credited 12–13 min instead of 9.5, about +29 min per day.

## Plain goal

**Today:** while a session is open, the sleep plan always sleeps the **full** debounce (`State_Sleep.cpp:607–616` at main, `debounceSeconds = setting1 / 1000`), measured from when it goes to sleep. After any wake that isn't a pulse (most often the hourly report), the device sleeps another full 300 s even though most of the debounce has already run. So the session's end is noticed late, and the late close time is credited.

**After this WO:** an occupied device sleeps only the **time left** before the debounce expires. A session is then credited as first pulse to last pulse + debounce, whatever woke the device in between.

**Also fixed (one line, the same diagnostic):** the `Wake: … led=` log field always reads 0. `signalLEDTimeRemaining()` already returns seconds (`device_pinout.cpp:124–140`), and `logWakeSummary()` divides it by 1000 again (`State_Sleep.cpp:35–40`).

## The change (re-check at Step 0)

1. **The occupied branch of the sleep plan.** The time left is the LED deadline, which is the wake path's close criterion: `ledOffTime` in RTC seconds, set when each pulse is handled. Use `signalLEDTimeRemaining()` when it is greater than 0, plus a 1 s margin so the wake lands just after the deadline. If it is 0 (expired, or no valid time), keep today's full debounce so behavior never gets worse. Keep `min(…, wakeInSeconds)` against the reporting boundary.
2. **`logWakeSummary()`:** drop the `/ 1000UL`.
3. **Nothing else changes:** no change to the awake path (`updateOccupancyState()`, which is `millis()`-based), to `closeOccupancySessionSafely()` or to crediting.

## Step 0 (Claude Code, read-only, §13 (c))

1. Re-check both sites. Confirm the LED deadline is the close criterion on every OCCUPANCY wake path (the LED-timeout close, now in `State_Modes.cpp` after S-6).
2. **Cases to reason through:**
   - an untrusted or invalid time, where `ledOffTime` is 0 and the full debounce must be kept;
   - the COUNTING LED, which must not be affected because the branch is OCCUPANCY-only;
   - a debounce expiring during a connect, where the awake path closes it;
   - a reset mid-session, where the boot cap is unchanged.
3. **The S-4 sleep-plan transcript** (`tests/fixtures/sleep_plan_transcript.txt`) will change in its occupied cases. List the cases expected to change, so the regenerated fixture is checked against that list and not just accepted.
4. **History (§12.1):** was the occupied wake ever based on the time left? (`git log -S` on that branch.)
5. **Size:** expected +3 to +6 `src/` lines. **PROCEED** at ≤ +20.

## Tests

- **The plan harness:** occupied with the LED left at 300, 120, 5, 1 and 0 s, crossed with the reporting boundary closer and further. Expected wake = min(remaining + 1, boundary), or the full debounce when the remaining time is 0.
- **The regenerated fixture** differs from the old one only in the cases Step 0 listed.
- **A test of `logWakeSummary()`'s `led=` value.**
- **Mutations,** each of which must fail:
  1. restore the full debounce;
  2. drop the +1 margin;
  3. use the remaining time when it is 0.

## Bench (the exerciser does it)

On the v47 soak, hour-straddling sessions should come in at **570 s ± a few seconds**, as normal sessions do, against 12–13 min credited on v45 and v46. The baseline script's per-category report is the pass test.

## Budget

- **`src/`:** +3 to +6 net, hard limit +20.
- **Tests:** guideline +100. Record ruling 9's numbers.

## Agents and models

- **Step 0:** Claude Code, read-only.
- **Stage 6:** Copilot, Sonnet tier, **medium**: a small, well-specified change on an existing harness.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**, stdin closed. This is a behavior change to the session close. It checks:
  - the fallback cases;
  - the fixture diff against the Step 0 list;
  - no change to COUNTING or the awake path.
- **Stage 8:** pre-approved on VERIFIED, or on VERIFIED WITH CONCERNS when every concern is Low. Controller test-only fixes follow §13 (d).
- **PR:** marked "do not merge until v46 is cut"; ships in v47.

## Closing record (Claude Code, 25 Oct 2026)

- **Budget versus actual.** `src/`: **+4 net** (7 added, 3 removed, one file, `State_Sleep.cpp`; 1,275 to 1,279 lines) against the WO's +3 to +6 and the Step 0 estimate of +7; limit +20. Tests: **+189** non-fixture lines against the +100 guideline (about 1.9x, at the flag threshold; accepted: independent expected values, copy checks, three structural re-anchors, one new diagnostic test), fixture +124 lines. Review rounds: Stage 7 round 1 only. Controller edits (§13 (d)): none.
- **Verification.** Step 0: PROCEED. Stage 6: Copilot (claude-sonnet-5.5, medium), accepted after the controller reproduced the suite (105/105, serial) and the four mutations in place (each caught by the check aimed at it). Stage 7: Codex (gpt-5.6-sol, high, stdin closed): **VERIFIED WITH CONCERNS, all Low** (`WO-2026-10-25-001-stage7-verdict.md`): the ARM build on Device OS 6.6.0 is text 151572 to 151580 B, data 1114, bss 2200, `.backup` 152 unchanged; `sleepPlan()` is inlined into `handleSleepingState()` (4768 to 4784 B) and the chain to `signalLEDTimeRemaining` is linked; two mutations of its own were caught. `src/` tree hash `1c5659d45f8544bee15dcc5665dc64f0811b6270aca3a0730a4d88d02e186dc8`.
- **The fixture-diff list.** Expected and confirmed independently by the controller and by Codex: S55-S67 each gain one `CALL signalLEDTimeRemaining` line with no plan value changed; S1-S80 otherwise byte-identical; S81-S90 appended (ledRem 300, 120, 5, 1, 0, each with the boundary nearer and further); header counts 98 scenarios (release 91) and the body hash updated.
- **History (§12.1).** `git log -S'debounceSeconds'` on the sleep file finds only `eda6b7e` (v3.24 Occupancy Mode, 9 Feb 2026), which introduced the occupied sleep as the full debounce. It was never based on the time left, so this adds a behavior; there was none to restore.
- **Ruling 9's numbers** (base `bf4e628` plus the WO commits; test lines excluding `tests/fixtures/`, fixture lines, suite, `src/` lines): 34,143 / 20,435 / 104 / 22,378 before; 34,332 / 20,559 / 105 / 22,382 after.
- **Particle cloud compile: service failure on 25 Oct; per-PR requirement replaced (ruling, Chip, 25 Oct: no hold on #155).** `particle compile boron . --target 6.6.0` on a clean copy of the candidate (`src/`, `lib/`, `project.properties`; same `src/` hash) failed three times with "Compile failed: Compiler timed out or encountered an error" after about 2 minutes and no compiler diagnostics; the **unmodified base** (`git archive HEAD`) failed identically, so the failure is the cloud service or target, not the candidate. The local ARM build (Codex and Copilot, separate fresh `BUILD_PATH_BASE`) passed. **The requirement is now one passing `particle compile boron --target 6.6.0` on the v47 release branch head (covering #142, #151 and #155), with its flash and RAM numbers in the v47 release record.** If the service is still down at v47, record that and release from the verified local build as usual.
- **Low concerns carried (Codex).** (1) A backward clock step while occupied can defer the close to the next report or awake opportunity (bounded by the reporting boundary; no cap, by the WO's rule); recorded in recovery item 20 as a separate clock-step question. (2) Two cited evidence files were missing on this branch because it was cut before #154; the branch was merged with main. (3) Test growth, accepted above.
- **Bench (the v47 soak).** Hour-straddling sessions should read 570 s plus or minus a few seconds, against 12-13 min credited on v45 and v46; the baseline script's per-category report is the pass test.
