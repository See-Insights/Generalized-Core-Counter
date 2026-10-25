# Step 0 dispatch: why does a session spanning the hourly report credit 12-13 min instead of 9.5? (recovery item 20)

**To:** Codex (`gpt-5.6-sol`, high), read-only. **From:** Claude Code, for Chip (25 Oct 2026). **Base:** main at `bf4e628` (this worktree). **WO:** none yet; this is a Step 0 for `docs/RECOVERY_PLAN_2026-09-26.md` item 20. **Report:** up to 150 lines. **Do not** edit any file, commit, push, or propose a fix. No network.

## The question

Find the **mechanism in the report and sleep path** by which an occupancy session that spans the hourly (:00) report is credited 12-13 minutes where a session that does not span it is credited 9 or 10. Read the code on this checkout; the bench data below are context, not proof.

## What was observed (Dev-09, firmware v45, 24 Oct; two agents checked the numbers in S3)

- The exerciser (a repeatable occupancy test) starts a session every 20 minutes (:13, :33, :53 on Dev-09), a PIR pulse every 30 s for about 570 s, then stops. The configured debounce is 300 s. A normal session closes about 570 s after it starts (serial `Occ: state=0 reason=debounce session=570s`) and credits 9 or 10 min.
- Dev-09's :53 session spans the hourly report at :00. All ten from 04:06Z to 13:05Z credit **12 or 13 min** (four at 13, six at 12). The hooks show: occ=1 at :53:2x, an **occ=1 report at :00:0x**, then the occ=0 report at :05:4x to :06:4x carrying the whole increment. Serial `session=` for five of them reads **734-810 s** (810 at 04:06:54Z, 737 at 06:05:47Z, 740 at 08:05:59Z, 734 at 12:06:04Z, 736 at 13:06:10Z): the session ends **165-240 s later** than a normal one. The device stays awake or in session longer when the hourly report falls inside a session.
- Dev-14 (v46) does not show this, because its phase puts :00 between sessions; there is no v46 data either way.
- Context lines that show up around a report during a session: `Wake: reason=TIMER`, `Occ: state=1 reason=pir-wake led=300s`, `Report: occ=1 totalMin=... ok=1 ledger=req`, `StateReq: Report->Connect reason=...`, `LoopStage: stage=SLEEP_PREP ...`. Connects are normally 1-6 s on Dev-09 (`ConnSummary`), and the :00 report does a connect in the middle of the session.

## What to establish (answer each, with `file:line` on this checkout)

1. **Where is the credit computed and added to the daily total?** Every path that adds to `dailyoccupancy` / `totalMin` (the close, a periodic report, a flush on a state change); what it measures (session start, `millis()` or the wall clock, the retained start time, the last PIR time, the debounce); and when the occ=0 report is generated.
2. **What does the hourly report (and its connect) do to an open session?** Does it close and reopen the session, re-arm the debounce, restart the session timer, change `led=`, or delay the close until after the report and connect? Trace one cycle: PIR wake at :53, awake in session, the :00 TIMER wake or report due, report, connect, back to sleep or to counting, the debounce expiry, the close, the occ=0 report. Say which states and transitions run in what order (include the sleep path: does the device sleep with an open session, and what wakes it?).
3. **Which mechanism produces the 165-240 s of extra session length?** Candidates to test against the code, not to assume: the connect during the session blocks the loop and the PIR samples (so the debounce restarts or counts from a later time); the report re-arms or extends the debounce; the close waits for a state (Report/Connect) to finish; the sleep-wake cycle changes how long `led=` / the counting window runs after a report; a restart of the session start time at the report. Rank them by what the code supports; name any that the code rules out.
4. **Does the extra length equal the extra credit?** (Extra credit about 2.5-3.5 min = 150-210 s; extra serial length 165-240 s.) If the credit tracks wall-clock span from the start time and the end is late, say so; if the credit is added twice (once at the report, once at the close), say so with the arithmetic.
5. **Does it depend on this bench?** The exerciser's phase, `--debounce-ms 300000`, the 20-min cadence, and any site setting; would a real occupant's session spanning a report be overcredited the same way? Which of these changes would make the effect larger, smaller or absent?
6. **What is the smallest read-only measurement that would confirm the mechanism** on the bench (a serial line that exists, a hook field, a test-mode log) so that a later bench check can tell it from the alternatives? Name existing log lines before any new ones.

## Report format

Up to 150 lines: (A) the answer in five lines; (B) the answers to 1-6 with `file:line`; (C) the ranked mechanisms and what rules out the rest; (D) what you could not determine from the code alone, and the measurement for it. **No fix proposals and no code edits.** If the code shows the credit is correct and the extra length is real occupancy behavior on the bench, say that plainly.
