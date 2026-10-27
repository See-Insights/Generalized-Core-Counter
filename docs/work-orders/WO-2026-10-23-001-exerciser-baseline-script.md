# WO-2026-10-23-001: the exerciser baseline: measured versus expected occupied minutes

**Author:** architect, 2026-10-23 · **Status:** ready (Step 0 first)
**Branch:** `wo/2026-10-23-001-exerciser-baseline-script`, from main
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.3; §13 (b), (g), (j)
**Spec source:** `docs/handover/2026-10-22-exerciser-interface-answers.md` (the GCC side of the exerciser), and the exerciser V1 pattern approved on 22 Oct (below).

## Plain goal

Turn "can we trust the occupancy data?" into a number per firmware version. A small host script reads a counter's report hooks for a day, computes the occupied minutes the exerciser pattern **should** produce, and reports measured against expected per day and per report window, with the error and its spread.

**No firmware change.** The script lives in `tools/`, and the release soak WOs use it from v46 on.

## The pattern it assumes (exerciser V1, approved 22 Oct)

- **Startup:** power-up at the counter's first open-hours wake, then 6 s quiet.
- **Cycle:** every 20 min, 10 pulses at 0, 30, …, 270 s (each held 2 s high), then silence.
- **Debounce:** the counter's `sensor.setting1`, 300,000 ms in production. 60,000 ms is allowed for bench checks.
- **The rule:** a session runs from its first pulse to the last pulse + debounce, so **expected credit per session = 270 s + debounce**: 570 s (9.5 min) at 5 min, or 330 s at 1 min. The counter credits a session when it closes (`closeOccupancySessionSafely`).

## Inputs and outputs

**Inputs** (local files; the script does no network or AWS calls):
- A directory of S3 hook JSON files for one device, `particle-events/<day>/Ubidots-Sensor-Hook-v1/<id>/*.json`, synced by Claude Code beforehand. It uses each file's `published_at`, and from the payload `timestamp` (ms), `occupancy`, `dailyoccupancy` and `fw_version` when present.
- The `status/<id>/` files for the same days, for resets (`resetReason`) inside a window.
- Parameters: device id, dates, local timezone, open and close hours, `debounce_ms`, `cycle_s` (1200), `pulses` (10), `spacing_s` (30), and an optional `--exclude START..END` list for bench-maintenance windows (for example the P7-A flash on Dev-14).

**The phase for each day:** T0 is the payload `timestamp` of the day's first report with `occupancy` = 1. In INTERMITTENT_KEEP_ALIVE that report is the session-start report fired by the first pulse. The phase comes from the counter, but only the phase does. Expected minutes per session never come from the counter.

**The expected-credit model:**
- Session *k* is expected to close at `T0 + k × cycle_s + (pulses−1) × spacing_s + debounce_s`.
- It is counted if it closes before the close hour, local.
- Expected credit = `(pulses−1) × spacing_s + debounce_s` seconds.

**Outputs** (plain text table and a CSV):
1. **Per day:**
   - measured: the last `dailyoccupancy` of the day, from the close report;
   - expected minutes, error in minutes and error %;
   - session count expected;
   - flags: missing reports, resets in the day, excluded windows.
2. **Per report window** (between consecutive reports):
   - measured: the `dailyoccupancy` difference;
   - expected: the sum of sessions whose expected close falls in the window;
   - the error.
   - `dailyoccupancy` is truncated to whole minutes, so a ±1 min error per window is quantization, and the output says so.
3. **Summary per firmware version:**
   - **accuracy** = the mean daily error;
   - **precision** = the standard deviation of the daily error;
   - the per-window error mean and standard deviation, with the number of days and windows.

## Tests (host, run with the suite)

A synthetic fixture: hand-built hook JSON for one day under each of these conditions, each with a known answer:
- a perfect counter;
- a counter that credits +60 s per session;
- one missing report;
- a reset in the middle of a session.

The script must reproduce each known answer exactly. Mutations, each of which must fail a test:
1. off-by-one in the session count at the close hour;
2. using `published_at` instead of `timestamp` for the phase;
3. dropping the truncation note.

## Step 0 (Claude Code, read-only)

- Confirm the hook payload keys and order from `src/cloud/ReportPublisher.cpp:103–104` and `PIRSensor.cpp:26–29`, and from one real Dev-09 hook file.
- Confirm the `status` reset fields.
- Propose the script's line count. **PROCEED** at ≤ 200 script lines and ≤ 100 test lines.

## Budget

- **`src/`: 0 lines.**
- **Script:** at most 200 lines in `tools/exerciser_baseline.py` (new folder `tools/`). Python 3, standard library only.
- **Tests:** guideline +100, including the fixture.

## Agents and models

- **Step 0:** Claude Code, read-only.
- **Stage 6:** Copilot, Sonnet tier, **medium**. It's a small standalone script with a clear model and known-answer tests.
- **Stage 7:** Codex `gpt-5.6-sol`, **medium**, stdin closed. It checks:
  - the expected-credit arithmetic and close-hour boundary;
  - that the phase uses `timestamp`;
  - that the fixture answers are right by hand;
  - no network calls.
- **Stage 8:** pre-approved on VERIFIED, or on VERIFIED WITH CONCERNS when every concern is Low. Controller test-only fixes follow §13 (d).
- **PR:** no merge hold. It's a tool, not firmware.

## First use

On the first exerciser day on v45 (Dev-09 and Dev-14), Claude Code syncs the hooks and runs the script, then reports the per-day and per-window tables. **That result is the v45 baseline.** The v46 soak WO compares against it.

## Closing record

- Lines (script, tests).
- The fixture answers.
- The first run's v45 numbers, with their source and window (§13 (j)).

## Amendment 1 (23 Oct): the power-up session: DROPPED

It was proposed to flag and exclude the first session after each exerciser power-up (a possible +6 s start-up edge). **Chip dropped it on 23 Oct:** the power-up session and the next one were equally long (Dev-14 first session `session=581s` on serial; the 11:49 local session 580 s by the hooks, S3 start report 03:48:57Z to end report 03:58:37Z, against 570 s expected), so power-up is not the discriminator. Session length: 569–581 s observed; the two long sessions (580–581 s, hooks) ran while Dev-14 was on laptop USB, and the clean serial values after the move to the forwarder were 569 s and 570 s; cause open; Saturday's full day on the forwarder decides (serial `session=` 569 s on Dev-14 at 04:18:31Z and 570 s on Dev-09 at 04:24:15Z, S3 `serialLog`, 23 Oct). **The script stays as Codex verified it** (no change). The exerciser sketch is frozen after 23 Oct except for driving IO1 LOW first in `setup()`.

## Stage 6 and Stage 7 result (23 Oct)

**Stage 6** (Copilot, Sonnet 5.5, medium): round 1 wrote `tools/exerciser_baseline.py` (166 lines) and `tests/exerciser_baseline_test.py` (94 lines); round 2 and round 3 (after Stage 7 round 1) fixed the findings. **Stage 7** (Codex `gpt-5.6-sol`, medium): **round 1 NOT VERIFIED:** `--tz SGT-8` crashed (the project's timezone strings are POSIX, not IANA), a missing payload `timestamp` fell back to `published_at` for the phase, and the tests covered neither. **Round 2 (the last, architect-set): VERIFIED WITH CONCERNS (Low only):** the three defects are fixed (POSIX TZ via `os.environ["TZ"]` + `time.tzset()` + `time.mktime()`; hook reports without a payload `timestamp` are ignored with a note; tests for `SGT-8` across the UTC date line and for `EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00` in October (EDT) and December (EST)). **C1 (Low), accepted:** the test file is 113 lines against the 100-line guideline.
**Controller reproduction** (Claude Code): the test passes; suite **92/92** (91 + the new test); the WO mutations (`<` → `<=` at the close hour, `published_at` for the phase, truncation note dropped) plus the wrong `SGT-8` sign and DST ignored each fail the test, the control passes (git-backed copies); no network/AWS path.

## Closing record

| Item | Budget | Actual |
|---|---|---|
| `src/` | 0 | **0** |
| Script | ≤ 200 lines | **184** (`tools/exerciser_baseline.py`, new) |
| Tests | guideline +100 | **113** (`tests/exerciser_baseline_test.py`, new; fixtures generated in the test) |
| Suite | baseline 91 | **92/92** |

- **Fixture answers (60 s debounce, 330 s = 5.5 min per session, hand-worked in the test's docstring):** a perfect counter measures 33.0 min over 6 sessions (daily 5, 11, 16, 22, 27, 33; window errors ±0.5 from truncation); +60 s per session measures 39 (error +6.0 min, +18.18 %); one missing report (flagged) still measures 33; a reset mid-session (flagged) measures 27 (error −6.0 min). Session 6 would close exactly at the close hour, which is not before it, so 6 sessions.
- **First run (v45 baseline):** pending: today (23 Oct) is shakedown; Saturday 24 Oct (full day) is the baseline on v45, Dev-09 and Dev-14 (Dev-14 `--exclude` 23 Oct 02:50Z..03:20Z). Phase uncertainty (the start report is stamped after the connect completes) is reported by the script's note.
- **Deviations:** none from the design; Stage 6 took three rounds, Stage 7 two. **No merge hold** (a tool, not firmware).

## For the architect

- Amendment 1 (power-up session) is dropped; the script stays as verified.
- Session length: 569–581 s observed; the two long sessions (580–581 s, hooks) ran while Dev-14 was on laptop USB, and the clean serial values after the move to the forwarder were 569 s and 570 s; cause open; Saturday's full day on the forwarder decides. The Saturday run reports the per-session start-to-end distribution from the hooks (and serial `session=` where captured) alongside the day totals.
