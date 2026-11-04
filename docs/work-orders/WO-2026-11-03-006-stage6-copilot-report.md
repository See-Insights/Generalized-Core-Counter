# WO-2026-11-03-006 Stage 6 report — item 19 C2 (untrusted split, credit and report on millis)

Model: Claude Sonnet 5.5 (Copilot CLI). Branch base `2703390`. Nothing committed, pushed or merged. Dev-14 bench not done and not claimed.

## Step 0 (re-read on this base)

- **Budget:** `src/` is **+15 net** (24 added, 9 removed; Generalized-Core-Counter.cpp +3/-1, StateMachine.h +2, State_Common.h +14/-7, State_Modes.cpp +5/-1). The design fits ≤ +16, no stop.
- **`DailyBoundary::check()` is trust-gated** (`src/time/DailyBoundary.cpp:36`, `if (Clock::isTrusted())`; due=false, boundary=0 otherwise). No boundary can close an untrusted-open piece while the clock is untrusted.
- **Boot with a persisted open session, untrusted:** `setup()` loads `CurrentReadings`, then (`Generalized-Core-Counter.cpp:~497-507`) clears `lastOccupancyEvent` and sets the B3 anchor. Trust is false at that point (no sync this boot). Now: the session becomes an untrusted piece (`pieceUntrusted`, `occupancyStartMs` = 0 = boot), one `Occ: untrusted straddler, pre-boot part dropped` log line, and the B3 anchor is set for a **trusted** boot only (otherwise the stale anchor would cap the next trusted close).
- **Expectations that encode the old untrusted merge** (listed before editing):
  1. `occupancy_session_restart_test.sh` tests 7, 8, 11: an untrusted close keeps the session open, re-arms. They model a piece opened trusted that then loses trust; B2 is unchanged for that case. `bootMidSession()` now sets `pieceUntrusted=false` explicitly; assertions are untouched. The untrusted case is the new test.
  2. `occupancy_report_by_mode_test.cpp`: no assertion encoded the merge. Only the `SessionState` stub gained the two fields, plus a new scenario (below).
- **Mechanical test edits (no expectation changed):** `SessionState` stubs (`tests/stubs/sleep_teardown/state/State_Common.h`, the two cpp harnesses) gained the two fields; `sleep_wake_equivalence_test.cpp` gained `Clock::isTrusted()`; the old wake-gate text in `debounce_expiry_c1_test.sh`, `occupancy_report_by_mode_test.sh`, `sleep_wake_equivalence_test.sh`, `sleep_wake_structural_test.py` was replaced by the new gate text, and the structural test maps the two added open-site lines back to the base. `tests/sleep_wake_transcript.txt` and the S-4/S-6 fixtures are **untouched**; the S-6 equivalence run still passes.

## LED-gate gap (C1 verdict note 6)

Reachable: OCCUPANCY, occupied, LED low, debounce expired. LED goes low via sleep prep (`State_Sleep.cpp:~517`, Closed branch only), after any reset (pin low at boot), or `signalLEDUpdate()` on the wall deadline. The **awake Idle path** (`State_Idle.cpp:48-49`) and `updateOccupancyState()` close on the predicate alone, so they are not LED-gated; only `processSleepWakeEvent` (`State_Modes.cpp:~156`) was, and there a PIR wake extended the session.
**Fixed inside the budget** by gating on `signalLEDStatus() || (session.pieceUntrusted && occupied)`: an open untrusted piece closes on the predicate alone; the trusted path (LED-gated, as v48) is unchanged. **Not fixed (out of scope, stated):** the same gap for a *trusted* piece with the LED low.

## Change

- `SessionState` (RAM only): `occupancyStartMs`, `pieceUntrusted`. Set at both open sites (`handleOccupancyMode`, `processSleepWakeEvent`) from `millis()` and `!Clock::isTrusted()`.
- `closeOccupancySessionSafely`: `piece = occupied && pieceUntrusted`. B2 re-arm remains only for a trusted piece that loses trust. A piece skips the wall-clock validity checks and credits `(lastOccupancyEvent - occupancyStartMs + debounceMs) / 1000` (unsigned, wrap-safe, whole-second truncation like the existing arithmetic) through the **existing** credit/validation path; `pieceUntrusted` clears on close. The caller path (log, `occupancyChangeTriggered`, `REPORTING_STATE`) is the trusted one, unchanged. One `Occ: untrusted piece credit=<s> start=<ms>` log line per credit.
- A piece stays on `millis()` when trust returns. Persisted/retained layout: no change (`persisted_layout_preservation_test.py` passes).

## Tests

Host suite: **109/109** (sh via zsh, py via python3; counted with `p=$((p+1))`): C1's 108 + the new `tests/untrusted_piece_c2_test.sh`. Logs in `build-tmp/wo-2026-11-03-006-stage6/`.

New (`untrusted_piece_c2_test.sh`, real extracted close/predicate/`updateOccupancyState`): control 10 pieces each 570 s (5700 s), every close raises the report; Dev-09 replay with the reset; straddler; trust returns mid-piece (wall step +10 h → credit 360 s on millis, not 36 360); trusted piece still credits wall (460 vs millis 360); next piece starts clean; quiet mode credits with no report; explicit-closeAt close of an untrusted piece. Added also in `occupancy_report_by_mode_test`: wake close of an untrusted piece with the LED low (and the trusted LED-low path unchanged), with its own mutation.
**Mutations, all caught:** keep B2 re-arm; credit wall span; credit from persisted wall start; one-debounce fallback at boot; start a trusted piece on millis (two ways); drop the report call; reuse the old start (open-site removed); straddler not marked at boot; wake close gated on the LED again (twice).

### Numbers, and two points where the WO's figures do not reproduce — for the controller

- **Dev-09 replay (set1 300 s, pulses 30 s apart for 270 s, session every 1200 s, untrusted):** control = 10 closed pieces by 14:00 = **5700 s = 95.0 min**; with the reset 334 s into the first piece: **301 + 9×570 = 5431 s = 90.5 min**; v45 credited 205 min. Both are far from 205, but **neither lands in "96-98 min"**: that band comes from a 47.5 % duty estimate (97.21 min) of the window, and the exact credited figure depends on how many whole pieces have closed by 14:00 (the eleventh opens 13:55:21 and closes 14:04:51). The closing count, not the algorithm, decides it. Each piece is exactly 570 s.
- **Straddler "about 236 s":** with the WO's formula a boot re-arms `lastOccupancyEvent` at the first loop pass, so with a 300 s debounce and no PIR after the reboot the straddler credits **≈ 301 s** (boot + 1 s + 300 s), not 236 s. 236 s is exactly reproduced when the debounce is 60 s and PIRs continue until 176 s after boot (test asserts 236; a one-debounce fallback would give 296). The bench expectation (236 vs 536) therefore depends on the debounce and on whether pulses continue after the reset; please check it against Dev-14's `setting1` before Chip's run. This is the formula working as specified, not a fallback.
- **Boundary overshoot:** if trust returns mid-piece and a boundary close arrives before the debounce runs out, the piece credits `(last + debounce) - start`, i.e. up to one debounce beyond the boundary instant (bounded; tested).
- **B3 behavior change:** at an untrusted boot the B3 cap no longer applies (the straddler is an untrusted piece); B3 stays for a trusted boot (item 21 will make that case reachable).

## Build and linkage

Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE=build-tmp/wo-2026-11-03-006-stage6/obj-release`, rc=0, text/data/bss **152036 / 1114 / 2208** (C1: 151692/1114/2200). `nm` on `tree.elf`: `closeOccupancySessionSafely` (W, `c1d38`), `updateOccupancyState` (`c25bc`), `processSleepWakeEvent` (`c2780`), `debounceRemainingMs` (`c2520`); `bl c1d38` from `handleIdleState`, `updateOccupancyState`, `processSleepWakeEvent` and `handleReportingState` (the boundary close). `State_Modes.o` references `session` (U) and `Generalized-Core-Counter.o` defines it (B); the credit and straddler log strings are in the ELF.

Scratch left in `build-tmp/wo-2026-11-03-006-stage6/` (git-ignored). Not done: Dev-14 exerciser bench (Chip's step).
