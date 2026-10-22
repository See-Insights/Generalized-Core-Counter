# WO-2026-10-22-001: Sleep S-6: wake-event processing to `State_Modes.cpp`, post-wake routing to one function (behavior-neutral)

**Author:** architect, 2026-10-22 · **Status:** ready (Step 0 first)
**Branch:** `wo/2026-10-22-001-sleep-s6-wake-and-occupancy`, cut from the S-5 branch tip `e6d3e69` (PR #136). The PR's base is `wo/2026-10-21-004-sleep-s5-preflight`, retargeted as the earlier PRs merge.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:**
- `WO-2026-10-20-002-step0-addendum-report.md` (round 2) Q9 row 9 and sequence item 6;
- WO-2026-10-20-002 ruling 2: the 14 Oct PIR ruling reversed; wake processing moves to `State_Modes.cpp`, called synchronously, with no hop through Idle; split into 6a/6b if over +20;
- rulings 7–9;
- WO-2026-10-21-001's option D ruling;
- the one-build rule in WO-2026-10-21-003.

**This WO cites `src/state/State_Sleep.cpp` at `e6d3e69` (1,383 lines) and `src/state/State_Modes.cpp` (160 lines).** Re-check every range.

## Plain goal

**Today:** when the device wakes from sleep, `State_Sleep.cpp` counts a PIR wake (COUNTING), starts or extends an occupied session (OCCUPANCY), and ends a session when the LED timer has run out. That is the same job `State_Modes.cpp` does while awake.

**After this WO:**
- The sensor-event work moves to `State_Modes.cpp`, so **one file owns counting and occupancy events, awake or waking.**
- The sleep file keeps only the routing: button, open hours, timer, PIR, and which state comes next. That routing becomes one named function, and `handleSleepingState()` becomes a short script.

**No behavior change:** the same counts, occupancy fields, LED calls, log lines, breadcrumbs, `ThrashGuard` marks, battery-policy calls and transitions, in the same order.

## What changes (at `e6d3e69`; re-check every range)

**6a: wake-event processing moves to `State_Modes.cpp`.** One new function there, for example `bool processSleepWakeEvent(bool pirWake)`, returns true when it has transitioned. It covers:
- the LED-timeout session close, `State_Sleep.cpp:1217–1235` (OCCUPANCY; `closeOccupancySessionSafely("sleep")`, and the debounce report and transition);
- the PIR-wake synthesis, `:1237–1324`:
  - COUNTING: the counts, `lastCountTime`, the LED flash, the log and serial line;
  - OCCUPANCY: `markProgress("PIR_WAKE_OCCUPANCY")`, the low-battery `BatteryAuthority` refresh, the session start and LED, breadcrumb 5, the report transition, the restart during debounce, and `lastOccupancyEvent`.

The new function is declared next to `handleOccupancyMode()` in `StateHandlers.h:43–53`, and is called from the same point in `handleSleepingState()`. The moved text keeps its `"sleep"`/`"pir-wake"` labels and its own log strings byte-for-byte. **It does not merge into `handleCountingMode()`/`handleOccupancyMode()`.**

**6b: post-wake routing becomes one file-local function in `State_Sleep.cpp`** (option D). This is everything after `sleepAndResume` (`:1160–1215`, `:1326–1381`), taking the `WakeContext`:
- the button wake;
- the open-hours sensor re-enable, post-wake battery sample, low-battery refresh and CONNECTED reconnect;
- `logWakeSummary`;
- the COUNTING LED-off;
- the timer, overdue-PIR and return-to-sleep routing, and the Idle fallback.

`logWakeSummary` (`:34–40`) stays in the sleep file.

## Design (do not widen)

1. **Event work moves only.** No logic changes and no deduplication with the awake handlers. Those duplicates are recorded below for the simplify series.
2. **The call is synchronous.** `handleSleepingState()` calls `processSleepWakeEvent(pirWake)` exactly where the blocks were, and returns if it transitioned. There is no hop through Idle.
3. **No new file and no new state.** `WakeContext` stays in the sleep file, because 6a needs only `pirWake`. Add only the includes `State_Modes.cpp` needs for the moved calls.
4. **Ownership.** `BatteryAuthority`, `ModemHealth` and `HibernateCycle` ownership is unchanged. The `BatteryAuthority::commit` site that moves keeps its allowlist entry, updated to the new file.

## Step 0 (Claude Code, read-only, §13 (c))

- **Re-check every range;** mark each holds, moved or wrong. Confirm by bare-name search that `logWakeSummary`, `pirWake`, `buttonWake`, `timerWake` and `wakeReasonLabel` have no readers outside the ranges.
- **Size:** prototype and count the change as before. A cross-file move pays for includes and a declaration, so count them.
  - **6a + 6b ≤ +20:** one WO; dispatch Stage 6 without coming back.
  - **Over +20:** split. **6a first** (the move to `State_Modes.cpp`), then 6b as its own WO on top. Each must be ≤ +20 to dispatch.
  - **6a alone over +20:** stop and report.
- **Fixture growth:** state the expected growth.
- **Ruling 8:** run the suite on the prototype; list and re-anchor every test anchored on text or line order in the changed code (in both files), giving each one's mutation. Likely readers:
  - `occupancy_report_by_mode_test.{cpp,sh}`;
  - `sleep_breadcrumb_sequence_test.py` (breadcrumb 5);
  - the `battery_authority_*` structural tests (commit-site allowlist);
  - the `ThrashGuard` PIR tests;
  - `config_window_hold_test.sh`.

  **S-5 C1:** if the three structural tests that strip `wifiGuard.` are touched, narrow them to S-5's four edits.
- **Report:** at most 150 lines, committed on the WO branch.

## Equivalence (freeze the output, not the code)

- **Extend the shared harness** (ruling 7). Record `tests/fixtures/sleep_wake_transcript.txt` once from `e6d3e69`, with that hash in its header. The harness extracts the moved and new functions verbatim by marker (`COPY_MISMATCH`).
- **Builds:** one build.
- **Scenarios** (cover the edges, not a full cross-product):
  - **Wake source:** button, PIR or timer.
  - **Openness:** Open, Closed or Unknown.
  - **Mode:** COUNTING or OCCUPANCY.
  - **Connection mode:** KEEP_ALIVE, INTERMITTENT, CONNECTED or DISCONNECTED.
  - **OCCUPANCY state:** occupied or not; LED on with time remaining, or expired; `setting1` 0 or set; session still open after close.
  - **Report due or not:** by `reportDueThisInterval`, and by `lastReport` age at and around the interval.
  - **Low-battery mode:** on or off.
  - **CONNECTED:** connected or not.
- **Freeze, per pass:**
  - the counters and occupancy fields;
  - the LED calls;
  - the `BatteryAuthority` calls;
  - `ThrashGuard` marks and breadcrumbs;
  - the log lines, character for character;
  - `session` flags;
  - the transition and reason.

## Budget

- **`src/`: at most +20 net non-move lines per WO, hard** (the split rule above). If Stage 6 goes over, stop and report; don't compress (§12.3).
- **Tests:** guideline +100. Record the actual and flag it at about twice the guideline. Record ruling 9's three numbers against `0c565fa`.

## Tests and checks

- The transcript matches exactly, the golden payload test is unchanged, and the suite reports `N/N`, run serially. The S-1 to S-5 fixtures are unchanged.
- Re-anchored tests each fail their mutation.
- **Mutations.** Each must fail the transcript or the named test aimed at it:
  1. Process the PIR wake before the LED-timeout close.
  2. Drop breadcrumb 5.
  3. Skip the `BatteryAuthority` refresh on an OCCUPANCY PIR wake.
  4. Route a CONNECTED PIR wake back to sleep.
- **Structural:**
  - no occupancy or counting field is written in `State_Sleep.cpp` after the move;
  - the `BatteryAuthority::commit` allowlist still totals four sites (main, `State_Report.cpp`, and the two in the wake path, now `State_Sleep.cpp` ×1 and `State_Modes.cpp` ×1);
  - no `retained` keyword is added.
- **`nm` retained check:** expect `.backup` unchanged. ELF delta and any layout difference explained.

## Agents and models

- **Step 0:** Claude Code (controller), read-only, in its own session.
- **Stage 6:** Copilot, Sonnet tier, **high**. It's a cross-file move with transitions inside, an allowlist update, and the largest scenario matrix in the series.
- **Controller, before Stage 7 (§13 (g)):** regenerate the transcript and confirm it is byte-identical; reproduce the build, `nm`, the mutations and the suite, serially.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. It checks:
  - provenance and edge coverage;
  - the moved text is byte-identical apart from the function boundary;
  - the same order of checks, returns and transitions;
  - one owner of occupancy and counting writes;
  - the allowlist;
  - the ruling-8 list is complete.
- **Stage 8:** pre-approved on VERIFIED, or on VERIFIED WITH CONCERNS when every concern is Low and already closed by another test, with net `src/` ≤ +20 per WO. Any High or Medium finding comes to the architect. At most two rounds.
- **PR:** marked **"do not merge until v45 is cut and #128, #130, #131, #132 and #136 are merged"**. Each half's PR says so, if split.

## For the simplify series (record, don't do)

These are the duplicates this move puts side by side. Each is a necessity-review candidate; several change log text or behavior, so each needs its own WO and evidence.

1. **Debounce default written four ways:**
   - `State_Common.h:318` (`occupancyDebounceMs()`);
   - `State_Modes.cpp` (setting1 / 1000, or 60 s);
   - the sleep plan (`:612`, the same in seconds);
   - the moved wake code (`Config::occupancyDebounceMsForRuntime()`).

   All resolve to 60,000 ms by default (`Config.h:34`). One helper would do.
2. **Two session-end tests:** the awake path ends a session on `millis() - lastOccupancyEvent > debounce`. The wake path ends it when the LED timer has expired. Decide which is the rule.
3. **Counting a PIR event twice:** `handleCountingMode()` and the wake synthesis each increment counts, set `lastCountTime`, flash the LED and log.
4. **Occupancy start written twice:** the awake handler and the wake synthesis each set `occupied`, the start time, the LED and the event log.
5. **The low-battery `BatteryAuthority` refresh** appears twice in the wake path (`:1197–1206`, `:1266–1275`), identical.

## Closing record

- Budget versus actual, per WO if split.
- Ruling 9's numbers.
- The transcript hash and its provenance.
- ELF by symbol, and any layout difference.
- The retained outcome.
- The ruling-8 table.

**Plus a series closing note:** `handleSleepingState()`'s line count at `0c565fa` and after S-6, and the list of phase functions.
