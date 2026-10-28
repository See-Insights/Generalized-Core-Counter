# Step 0 dispatch: occupancy after the close counts into the next day (recovery item 10)

**To:** Codex (`gpt-5.6-sol`, high), read-only, stdin closed. **From:** Claude Code, for Chip (29 Oct 2026). **Base:** the main checkout of this worktree (`git log -1` will say). **WO:** none yet; this is a Step 0 for `docs/RECOVERY_PLAN_2026-09-26.md` item 10, the next fix after v47 (architect, 29 Oct). **Report:** up to 150 lines. **Do not** edit any file, commit, push or propose a patch; no network.

## The question

When a PIR session is open or starts after the 22:00 (local) close boundary while the device is still awake, its occupied minutes are credited to the **next** day. Establish, from the code on this checkout, exactly how that happens, on which paths, and what each candidate treatment would have to touch. History first (`git log -S`, `git log -L`): was the boundary ever handled by splitting the session, and why is it not?

## Field cases (S3 hooks and serial; context, not proof)

1. Dev-09, v43, 15 Oct: close report 13:59:59Z `dailyoccupancy` 291; PIR at 14:01:55Z during a slow connect that kept the device awake until 14:22Z added 8 min; the 22:00:43Z report read 8, not 0.
2. Dev-14, v46, 26 Oct: close hook 456 (payload 13:59:59Z); a further hook payload 14:05:48Z `occupancy` 0, `dailyoccupancy` 5; the next morning's first report read 5.
3. Dev-09, 23-24 Oct (untrusted clock, items 18/19): stale daily 345 carried for eleven hours past the boundary; a different symptom (the boundary was not processed).
4. **28 Oct (v47, both bench devices):** Dev-09: close hook payload 13:59:59Z daily 458 (published 14:02:37Z after a 151 s close connect with CloudRecover stage 2); the exerciser's 22:00 SGT slot started a PIR session at 14:00:44Z (serial `Occ: state=1 reason=pir`); `occupancy` 1 daily 0 at 14:02:39Z (payload 14:02:35Z), then `occupancy` 0 daily **7** at payload 14:07:52Z (317 s); back to `Idle->Sleep` 14:09:22Z. Dev-14: close hook 14:00:11Z daily 449; PIR session at 14:00:50Z; orphan occ=0 hook payload 14:07:47Z daily **5**, alert 40 (webhook timeout 14:00:28Z); the occ=1 hook is missing. Whether those credits carry into the 22:00Z wake report is being checked separately.
Exerciser: a session every 20 min through the day, including after 22:00 local (the bench does not stop at the close). A real site may have no such activity; treat the bench as a worst case, not the common case.

## What to establish (answer each, with `file:line`)

1. **Every path that adds to `totalOccupiedSeconds` / `dailyoccupancy`** and every path that zeroes it (the daily reset, `PIRSensor.cpp:15-22`, `State_Report.cpp:46,56-59`, the wake reset); the order of the boundary processing relative to a session that opens or closes at or after the boundary.
2. **Awake after the close:** which states keep the device awake after the boundary (a long Connect, Report, a CloudRecover stage, the failsafe) and which keep PIR sampling and the debounce running there; does the device ever sleep with an open session across the boundary, and what does the sleep plan do after the boundary (the wake time, the 22:00Z hibernate)?
3. **The credit:** for a session that starts before and ends after the boundary, and one that starts and ends after it, which day's counter gets the minutes, and is the retained start time or the clock involved? What exactly is zeroed at the 22:00Z wake and could a carried-in value survive it (case 2's morning start from 5)?
4. **Candidate treatments, no recommendation:** (a) split the session at the boundary in `closeOccupancySessionSafely()`; (b) discard or ignore PIR after the close until the wake; (c) credit it to the new day on purpose (it is that day's occupancy) and make the daily reset not erase it; (d) leave the behavior and record it. For each: the code that would have to change, the tests that already exist (`tests/`, `tests/fixtures/`), the risk to the nightly hibernate and to item 15 (occupancy lost across a reset, the boot cap), and the size class.
5. **The orphan occ=0:** why can an occ=0 report go out with no occ=1 report (case 4, Dev-14), is that the same cause as a dropped hook or something in the report path, and does it bear on the credit?
6. **Smallest read-only measurement** to tell the treatments apart on the bench (existing log lines first).

## Report format

(A) The answer in five lines; (B) answers 1-6 with `file:line`; (C) the candidate treatments compared; (D) what the code alone cannot show. No fix proposals beyond the comparison; no edits. If the code shows the new day's credit is correct as designed, say so plainly.
