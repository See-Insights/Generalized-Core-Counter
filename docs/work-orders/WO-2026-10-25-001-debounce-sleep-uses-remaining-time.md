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

## Closing record

- Budget versus actual.
- The fixture-diff list.
- History.
- Ruling 9's numbers.
