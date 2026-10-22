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

## Stage 6 result and controller reproduction (22 Oct)

**Stage 6** (Copilot, Sonnet 5.5, high; implementation commit `febcf0b` on `wip/s6-sleep-wake-and-occupancy`, no PR): option D as designed. **Step 0 split rule: 6a + 6b = +12 ≤ +20, one WO.** `src/` net **+12** (`State_Sleep.cpp` 1,383 → 1,275, −108; `State_Modes.cpp` +115 including the three includes; `StateHandlers.h` +5); the three files are byte-identical to Claude Code's compiled prototype. `src/` tree hash (`find src -type f | LC_ALL=C sort | xargs shasum -a 256 | shasum -a 256`) `88d823de…` → `437c9dca2275323293745127b9c0bc37c08185c44af773c7269c9eca93108f56`.
**Controller reproduction** (Device OS 6.4.1 Boron, fresh `BUILD_PATH_BASE`, `-frandom-seed=p6`, "before" is `git archive HEAD`):
- **Fixture:** `--regenerate` from the base `e6d3e69` is byte-identical; `COPY_MISMATCH` fires under `SLEEP_WAKE_TAMPER_COPY=1`; 143 scenarios, 143 passes, 31 distinct event forms; the S-1 to S-5 fixtures are unchanged.
- **Suite 103/103** (sh via zsh, py via python3, serial; baseline 101); `git diff --check` flags only `State_Modes.cpp:176,225,253`, trailing-space blank lines carried from the base.
- **ARM:** text/data/bss `151348/1114/2212 → 151340/1114/2212`; `processSleepWakeEvent(bool)` is an out-of-line 0x250 B symbol, `postWakeRouting` inlines, `handleSleepingState()` 0x14f4 → 0x12a0; **`.backup` unchanged** (retained outcome A).
- **Mutations (git-backed copies):** the four WO mutations (PIR wake before the LED-timeout close; breadcrumb 5 dropped; the PIR-wake `BatteryAuthority` refresh skipped; a CONNECTED PIR wake routed back to sleep) each fail the transcript and the structural test; the control passes. Copilot also ran 22 mutations (the timer-suppress condition, a stray occupancy write, a `retained` probe, one per re-anchored test).

## Stage 7 result and controller edits (22 Oct)

**Round 1** (Codex `gpt-5.6-sol`, high, on `febcf0b`): NOT VERIFIED on **F1 (Medium)**: the new structural test checked for occupancy/counting writes only in the handler and `postWakeRouting`, so a rogue writer elsewhere in `State_Sleep.cpp` passed. Checks 1–5 and 7 PASS (moved code verbatim, ordering, ODR, transcript, re-anchors, ELF; retained outcome A). **Round 2**: F1 narrowed: `strip()` removed `//` before strings, so `Log.info("//"); …set_occupied(false);` hid a write. **Round 3: VERIFIED WITH CONCERNS (Low only)**, suite 103/103, `src/` tree identical to `febcf0b`.
- **Controller edits (§13 (d); test-only, accepted retroactively by the architect, 22 Oct), both in `tests/sleep_wake_structural_test.py`, +8/−4 lines in total (net +4):** (1) `OWNED_WRITES` now scans the whole comment-stripped `State_Sleep.cpp` (base had ten such writes; the candidate has none) plus a probe; (2) `strip()` is one left-to-right pass over comments, strings and character literals plus a probe. Each was checked by git-backed mutants and rechecked by Codex.
- **C1 (Low), accepted by the architect under §12.4 ("lexical evasion of a structural test, not a regression"), not fixed:** a C++ raw string with an embedded quote (`R"tag(" //)tag"`) still hides a write from the lexer. No raw string occurs in the file. **No round 4** (architect, 22 Oct).
- **C2 (Low), accepted by the architect:** non-fixture test code is +960 lines, 9.6× the +100 guideline; the 1,545-line fixture is within the 1,500–3,000 estimate.

## Closing record

| Item | Budget | Actual |
|---|---|---|
| `src/` net | ≤ +20, hard | **+12** (+235/−223; Sleep −108, Modes +115, header +5); only the three authorized files |
| Tests (S-6 alone) | guideline +100 | **+960** non-fixture lines (+1,021/−61, including the +4 controller edit; 9.6×), a **1,545-line fixture** (estimate 1,500–3,000) |
| Suite | baseline 101 | **103/103** (sh via zsh, py via python3, serial); golden fixture sha256 `07de1e19…` unchanged |

- **Test complexity (ruling 9), against main `0c565fa` (cumulative, Claude Code's count):** `src/` 22,306 → 22,390 (**+84**); test lines excluding fixtures 28,893 → 34,043 (**+5,150**); fixture lines 834 → 20,435 (**+19,601**); suite 91 → 103 (**+12**). S-6 alone: +12 / +960 / +1,545 / +2.
- **Design (option D):** `bool processSleepWakeEvent(bool pirWake)` at the end of `State_Modes.cpp` (declared in `StateHandlers.h`), holding the LED-timeout close and PIR-wake synthesis verbatim; `static void postWakeRouting(bool, bool, bool, const char *)` in `State_Sleep.cpp` after `} // namespace` (the one slot outside every harness span), holding the rest of the post-wake routing; the handler calls it last. `logWakeSummary` and the simplify-series duplicates (debounce default four ways, two session-end tests, the PIR event counted twice, occupancy start written twice, the duplicate low-battery `BatteryAuthority` refresh) are moved verbatim and stay.
- **Transcript provenance:** `tests/fixtures/sleep_wake_transcript.txt`, base `e6d3e69`; 143 scenarios; file sha256 `30a2e150…`, body `af9dc4a5…`. Regenerated from the base by Claude Code and Codex: byte-identical.
- **`handleSleepingState()` length:** **1,492 lines at main `0c565fa`, 377 at S-5, 155 after S-6.** Phase functions extracted by the series: `SleepTeardown::run` (S-1), `sleepAndResume` (S-2), `cloudGatePass` (S-3), `sleepPlan` with `nightlyHeapGuard` and `hibernateAttempt` (S-4), `endOfCycleSummary` and `wifiOffGuardPass` (S-5), `postWakeRouting` and `processSleepWakeEvent` (S-6).
- **ELF and retained:** as above; static-init stubs 22/22; no `retained` keyword added; outcome A.
- **Ruling-8 table (final):** ten tests re-anchored and mutation-checked (battery-authority allowlist; the two connection-mode-downgrade tests; clock-resync wiring; hourly-while-occupied; occupancy-report-by-mode; loop-stage prep exclusion; the S-2 harness call site; the S-4 and S-5 structural tests), and the three `wifiGuard.` normalizations narrowed to S-5's four reset lines (S-5 C1 closed). The four WO mutations are pinned by the new transcript.
- **Argon:** not built.
- **Deviations:** the fixture is within the estimate but the test code is 9.6× the guideline; Copilot's report said `src/` hash `decb954c…`/`12dc9de0…` (a different method, not comparable); two controller edits (above).
- **Rounds:** Stage 6 one round; Stage 7 three rounds (NOT VERIFIED, NOT VERIFIED, VERIFIED WITH CONCERNS). Ships in v46 with S-1 to S-5. **The PR is marked "do not merge until v45 is cut and #128, #130, #131, #132 and #136 are merged"** (release/v45 is cut: PR #135).

## For the architect

- C1 (raw-string lexical evasion) is accepted and recorded, not fixed.
- Series state: S-1 to S-6 done; `handleSleepingState()` is 155 lines.
