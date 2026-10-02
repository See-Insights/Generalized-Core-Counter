# WO-2026-10-02-003: stop the per-wake memory leak (v34)

**Goal, in plain language:** free memory stays level across wake cycles (item A), and if memory does run out, the device resets at once with a cause code that says so (item B).

**Status:** **BENCH PASS** (2026-10-02): VERIFIED at Stage 7 (round 1's only finding, P2, a test gap, closed by a pre-authorized narrow edit to the test); the Dev-09 bench passed (check 7 in the bench log). Committed at Stage 8 for Chip's merge.

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`: Stage 5 → Stage 6 (Copilot) → Stage 7 (Codex), §5 model routing, §12 guardrails (plain goal, size budget with no compressed code, two-round rule, facts checked against their source, budget vs actual in the closing record).

**Branch:** `wo/2026-10-02-003-sleep-config-leak`, from `main` at `43b8a69` (v33 merged, PR #55; PR #56 merged). All file:line below were checked against `43b8a69` on 2026-10-02.

**Version:** `v34-SleepConfigLeak`, product version 34. It includes v33.

## Cause

From the Codex investigation (`docs/work-orders/2026-10-02-wake-heap-loss-codex-report.md`), verified against the source by Claude Code.

- **The global:** `SystemSleepConfiguration config` is declared at `Generalized-Core-Counter.cpp:131` and `extern`'d in `StateMachine.h:30`.
- **Where it's reset before each sleep:** `config = SystemSleepConfiguration();` at `State_Sleep.cpp:1116` (hibernate), `:1202` (ultra-low-power), `:1443` and `:1460` (the STOP fallbacks).
- **The Device OS defect:** the move assignment (`system/inc/system_sleep_configuration.h:205–210`) `memcpy`s the new configuration over the old one **without freeing the old `wakeup_sources` list**. Only the destructor (`:213`) frees it, and a global is never destroyed. **Every sleep orphans a list:** 2 GPIO + 1 RTC nodes, plus 1 network node with standby. That's at least 72–104 B, up to about 168 B with allocator overhead.
- **The field evidence:** Dev-09 on v33, 2 Oct, lost 117–141 B per wake cycle (`fh` 73,848 → 51,952 over 3 h and +187 cycles).
- **History:** introduced in `41bc674` (17 Jan 2026); made larger by network standby in `217a4ae`.
- **The same move assignment** is in every installed Device OS version (6.1.1, 6.3.3, 6.3.5, 6.4.1) and in Particle's current `develop` branch, so upgrading Device OS wouldn't fix it.

**Who reads the global** (`git grep`, 2026-10-02): only `State_Sleep.cpp`, at its four sleep sites.

| Site | Reset | Configured | Slept |
|---|---|---|---|
| Hibernate | `:1116` | `:1117` | `:1149` |
| Ultra-low-power | `:1202` | `:1344–1360` (no use between `:1202` and `:1344`) | `:1402` |
| STOP fallback | `:1443` | `:1444–1447` | `:1454` |
| STOP timer-only fallback | `:1460` | `:1461–1462` | `:1469` |

**Nothing reads the configuration after `System.sleep()` returns:** wake handling uses the separate `SystemSleepResult result`.

**Precedent in the code:** the boot-storm sleep already uses a local, `bootStormSleep` (`Generalized-Core-Counter.cpp:882`).

## Change

### A. Stop the per-wake leak. Budget about 12

At each of the four sites, **build a fresh local `SystemSleepConfiguration`, configure it, and pass it to `System.sleep()`**, as Particle's examples do and as `bootStormSleep` does. Never assign to an existing configuration.
- **The ultra-low-power site:** declare the local where it's configured (`:1344`), and remove the reset at `:1202`.
- **Each STOP fallback** gets its own local.
- **The hibernate site** gets its own local. On success HIBERNATE resets the MCU; on failure the code falls back to the ultra-low-power site.
- **Remove the global** (`Generalized-Core-Counter.cpp:131`) **and its `extern`** (`StateMachine.h:30`). Nothing else uses them.

**Unchanged:**
- every wake source at every site: the same pins, edges, RTC duration and network standby flag;
- sleep modes, breadcrumbs, logging and `AwakeCycles::recordSleepReturn()`;
- the order of the pre-sleep sequence.

**Size budget: about 12 net `src/` lines, likely net negative.** No compressed code (§12 guardrail 3). Going over means stop and report.

### B. Restore the original out-of-memory reset. Budget about 5 (separate from A's)

**Added by Chip, 2026-10-02, during Stage 6.**

**Today:**
- the Device OS out-of-memory handler is registered (`System.on(out_of_memory, outOfMemoryHandler)`, `Generalized-Core-Counter.cpp:899`) and stores the size in `outOfMemory` (`:2498–2499`);
- on `outOfMemory >= 0`, the main loop logs, raises alert 14 and transitions to `ERROR_STATE` (`:1686–1694`);
- `ERROR_STATE`'s `resolveErrorAction()` case 14 (`State_Error.cpp:41`) soft-resets with code 6 (`ERROR_STATE_SOFT`) while today's reset count is under 3, but **from 3 on it suppresses the reset and returns to Idle**.

**Why it changes (the Error/Idle bounce risk):** `outOfMemory` is never cleared. So once resets are suppressed, every loop pass goes back to `ERROR_STATE` and back to Idle, and the device keeps running with its memory exhausted and no recovery. Even under 3, the reset is delayed by `resetWait` and the error-state path, and it carries code 6, which other error-state resets share, so an out-of-memory reset can't be identified.

**Change:**
- on `outOfMemory >= 0`, log the size and call **`System.reset(RESET_CAUSE_OUT_OF_MEMORY)`** directly, as the original design did (`Log.info("out of memory occurred size=%d", outOfMemory); delay(100);` then the reset), bypassing `ERROR_STATE` and its suppression;
- add **code 7, "out of memory"**, to `src/ResetCause.h`, and to the structural test that requires every `System.reset()` to carry a distinct, non-zero code;
- **Alert 14 (Stage 5 decision, Chip, 2026-10-02, during Stage 6; replaces the earlier "remove if unused"):**
  - **Bypass the error-state route** for out-of-memory: no `raiseAlert(14)` and no `transitionTo(ERROR_STATE, "out of memory")` in the loop's out-of-memory block. That's the actual change.
  - **Leave as they are:** the boot-time clear of alert 14 (`Generalized-Core-Counter.cpp:1081`) and its severity rank (`MyPersistentData.cpp:864`). They're harmless, and they handle an alert 14 stored by older firmware.
  - **Anything now unused is noted here for a later cleanup, not removed in this round.** That includes `resolveErrorAction()` case 14 (`State_Error.cpp:41`), which nothing reaches once the loop no longer raises 14, apart from a stored 14 that the boot clear normally removes.

### Later cleanup (recorded, not done in v34)

- Review alert 14's remaining uses (the boot-time clear and the severity rank) once older firmware has left the fleet.

**Done in v34 (moved from "Later cleanup", per Chip's advance decision):** `resolveErrorAction()` case 14 (`State_Error.cpp`) was removed by Stage 6 item B, because nothing in `src/` raises 14 any more. A 14 stored by pre-v34 firmware falls to `default`, so the error state returns to Idle without resetting. The boot-time clear (`Generalized-Core-Counter.cpp:1081`) and the severity rank (`MyPersistentData.cpp:864`) were **kept**, unchanged.

## Protected

- Sleep modes, wake sources and their order, and the watchdog pause and restore.
- v32 and v33 behavior (failsafe, `MODEM_OFF`, payload fields, hourly reports while occupied).
- `lib/`.
- The `WITH_ACK` path.

## Non-goals

- Fixing Device OS.
- Filing anything with Particle (a draft report is prepared separately; Chip decides).
- Other heap suspects.

## Acceptance (Stage 7, narrow)

(Items 1–3 are A's; item 4 is B's; item 5 covers both.)

1. **No reused long-lived sleep configuration remains in `src/`:** a structural test that fails if a global or static `SystemSleepConfiguration` exists, or if any `SystemSleepConfiguration` is assigned to again.
2. **A leak check over many cycles:**
   - run a loop of at least 1,000 simulated sleep cycles under a leak checker (LeakSanitizer or a counting allocator), configuring each site's wake sources;
   - use Device OS 6.4.1's real `system_sleep_configuration.h` if it compiles on the host, or a faithful copy of its allocation behavior (constructor, the builder methods' `new`, the move assignment, the destructor) if not;
   - **zero bytes lost per cycle**;
   - **mutation:** restoring the global reuse (`config = SystemSleepConfiguration();` on a long-lived object) makes the test fail.
3. **Wake sources unchanged:** each site still configures the same pins and edges, RTC duration, and network standby (when `useNetworkStandby`) as before.
4. **B, out-of-memory reset:**
   - with `outOfMemory >= 0`, the loop calls `System.reset(7)` directly, with no `ERROR_STATE` transition and no suppression at any reset count;
   - code 7 is in `ResetCause.h`, and the structural test requires it, distinct and non-zero;
   - **nothing outside the out-of-memory path changed behavior:** the boot-time clear of alert 14, its severity rank and `resolveErrorAction()` are untouched (if Stage 6 went further, Stage 7 checks that nothing outside the out-of-memory path changed behavior);
   - **mutation:** restoring the `ERROR_STATE` route fails a test.
5. **Suite and build:**
   - every `tests/*.sh` with zsh, plus every bare `tests/*.py` with python3; the `WITH_ACK` structural test green;
   - local ARM release build and linkage;
   - `v34-SleepConfigLeak`, product 34;
   - the budget met.

## Bench (after the flash, Dev-09; Chip)

Keep Dev-09 occupied for an hour (wave at the sensor at least every 4 minutes), so there are many wake cycles between reports.

**Pass:** across the reports, `fh` changes by much less than 130 B × the change in `cyc`; for example, under 1 KB over 50+ cycles. v33 lost about 7 KB per hour.

### Bench log

**Check 1, 2026-10-02 09:50Z (Claude Code, from the S3 archive; Dev-09 `e00fce68399ee6244a963935`). Result: PENDING, only 21 cycles so far.**

- **Boot:** OTA to v34 at 09:33:35Z. The status event shows `version` `v34-SleepConfigLeak`, `resetReason` 70 (update), `freeHeap` 90320.
- **v33's last report:** published 09:33:33Z but stamped 17:29:27 SGT, before the v34 boot. It was queued under v33 and is left out below.

| Payload stamp (SGT) | occupancy | dailyoccupancy | fh | lfb | cyc | slp |
|---|---|---|---|---|---|---|
| 17:33:41 (first after boot; skipped) | 1 | 393 | 77320 | 75584 | 2 | 1 |
| 17:48:01 | 0 | 408 | 75368 | 74216 | 22 | 21 |
| 17:48:30 | 1 | 408 | 77088 | 75240 | 23 | 22 |

- **Per the rule** (skip the first report after the boot), one stretch remains:
  - 17:48:01 → 17:48:30: Δ`fh` +1720 over Δ`cyc` 1, so +1720 B per cycle. Too short to mean anything.
- **Indicative only**, with the skipped report as the baseline:
  - 17:33:41 → 17:48:01: −1952 over 20 cycles, −98 B per cycle;
  - 17:33:41 → 17:48:30: −232 over 21 cycles, −11 B per cycle.
- **Comparison:** v33 would have lost about 2.5–3 KB over 21 cycles.
- **Why the stretches swing:** `fh` is a point sample, and it moves by ±2 KB with what is allocated at report time. The 17:48:01 session-end report and the 17:48:30 report were 29 s apart.
- **Grading:** needs 50+ cycles, judged from the overall trend, not from one stretch.

**Check 2, 2026-10-02 10:03Z (Claude Code, archive). Result: PENDING, 23 cycles since the first counted report (45 since boot).** There are no status or watchdog events since the boot.

Two new reports:

| Payload stamp (SGT) | occupancy | dailyoccupancy | fh | lfb | cyc | slp |
|---|---|---|---|---|---|---|
| 17:57:33 | 0 | 417 | 75968 | 75240 | 45 | 44 |
| 18:03:13 | 1 | 417 | 75464 | 74312 | 45 | 44 |

- **Stretches** (skipping 17:33:41):

  | Stretch | Δ`fh` | Δ`cyc` | Per cycle |
  |---|---|---|---|
  | 17:48:01 → 17:48:30 | +1720 | 1 | +1720 B |
  | 17:48:30 → 17:57:33 | −1120 | 22 | −51 B |
  | 17:57:33 → 18:03:13 | −504 | 0 | n/a (same awake period) |

- **Total by the rule:** 17:48:01 → 18:03:13, Δ`fh` +96 over Δ`cyc` 23, so +4 B per cycle (level).
- **Like-for-like:**
  - session-end reports: 17:48:01 → 17:57:33, +600 over 23 cycles;
  - occupied reports: 17:48:30 → 18:03:13, −1624 over 22 cycles (−74 B per cycle).
- **Indicative, from the boot report:** 17:33:41 → 18:03:13, −1856 over 43 cycles (−43 B per cycle). v33 would have lost about 5–6 KB over 43 cycles.
- **Reading:** clearly below v33's rate, but report-time noise (about ±2 KB) still swamps a figure in the 0 to −70 B per cycle range at this count.

**Check 3, 2026-10-02 10:51Z (Claude Code, archive). Result: level, but only 39 counted cycles (short of 50). No grade yet.** There are no status or watchdog events since the boot. There has been no report since 10:22Z, and the last one shows occupancy 0, so the waving appears to have stopped.

| Payload stamp (SGT) | occupancy | dailyoccupancy | fh | lfb | cyc | slp |
|---|---|---|---|---|---|---|
| 18:11:00 | 0 | 430 | 75328 | 74312 | 58 | 57 |
| 18:16:50 | 1 | 430 | 77064 | 75240 | 59 | 58 |
| 18:22:03 | 0 | 435 | 76856 | 75240 | 61 | 60 |

- **Stretches:**

  | Stretch | Δ`fh` | Δ`cyc` | Per cycle |
  |---|---|---|---|
  | 18:03:13 → 18:11:00 | −136 | 13 | −10 B |
  | 18:11:00 → 18:16:50 | +1736 | 1 | +1736 B |
  | 18:16:50 → 18:22:03 | −208 | 2 | −104 B |

- **Total by the rule:** 17:48:01 → 18:22:03, Δ`fh` **+1488** over Δ`cyc` **39**, so +38 B per cycle (no loss).
- **Like-for-like:**
  - session-end reports: 22 → 58, −40 over 36 cycles (−1 B per cycle); 22 → 61, +1488 over 39 cycles;
  - occupied reports: 17:48:30 → 18:16:50, −24 over 36 cycles (−0.7 B per cycle).
- **From the boot report:** 17:33:41 → 18:22:03, −464 over 59 cycles (−8 B per cycle).
- **Comparison:** v33 would have lost about 5 KB over 39 cycles, or about 7.7 KB over 59.

**Check 4, 2026-10-02 11:21Z (Claude Code, archive). Result: level; 40 counted cycles (short of 50). No grade yet.** There are no status or watchdog events since the boot. There was one new report, the unoccupied hourly report:

| Payload stamp (SGT) | occupancy | dailyoccupancy | fh | lfb | cyc | slp |
|---|---|---|---|---|---|---|
| 19:00:02 | 0 | 435 | 77296 | 75240 | 62 | 61 |

- **Stretch:** 18:22:03 → 19:00:02, +440 over 1 cycle.
- **Total by the rule:** 17:48:01 → 19:00:02, Δ`fh` **+1928** over Δ`cyc` **40** (no loss).
- **Like-for-like:**
  - unoccupied reports: 22 → 62, +1928 over 40 cycles;
  - occupied reports: unchanged from check 3, −24 over 36 cycles.
- **From the boot report:** 17:33:41 → 19:00:02, **−24 over 60 cycles**.
- **Pace:** with no one at the sensor, `cyc` now rises about once an hour. Reaching 50 counted cycles needs more waving.

**Check 5, 2026-10-02 11:51Z (Claude Code, archive). No change: no new reports, and no status or watchdog events.** The latest is still 19:00:02 SGT (`fh` 77296, `cyc` 62), so there are 40 counted cycles. The next unoccupied hourly report is due at 20:00 SGT (12:00Z).

**Check 6, 2026-10-02 11:58Z (Claude Code, archive, on Chip's request). Result: level; 41 counted cycles. No grade yet.** There are no status or watchdog events. One new report shows Dev-09 occupied again:

| Payload stamp (SGT) | occupancy | dailyoccupancy | fh | lfb | cyc | slp |
|---|---|---|---|---|---|---|
| 19:58:09 | 1 | 435 | 77056 | 75240 | 63 | 62 |

- **Stretch:** 19:00:02 → 19:58:09, −240 over 1 cycle.
- **Total by the rule:** 17:48:01 → 19:58:09, Δ`fh` **+1688** over Δ`cyc` **41** (no loss).
- **Like-for-like, occupied reports:** 17:48:30 → 19:58:09, **−32 over 40 cycles** (−0.8 B per cycle).
- **From the boot report:** −264 over 61 cycles (−4 B per cycle).

**Check 7, 2026-10-02 12:00Z (Claude Code, archive, on Chip's request). Result: PASS, with 51 counted cycles.** There are no status or watchdog events since the boot, so no reset happened during the bench.

| Payload stamp (SGT) | occupancy | dailyoccupancy | fh | lfb | cyc | slp |
|---|---|---|---|---|---|---|
| 20:00:12 | 1 | 435 | 75960 | 75240 | 73 | 72 |

- **Stretch:** 19:58:09 → 20:00:12, −1096 over 10 cycles (−110 B per cycle). This is within the ±2 KB report-time swing seen throughout; the 17:48:30 → 17:48:01 pair alone differed by 1,720 B.
- **Total by the rule:** 17:48:01 → 20:00:12, Δ`fh` **+592** over Δ`cyc` **51**. That is no loss, so it meets the pass (under about 1 KB lost over 50+ cycles).
- **Like-for-like, occupied reports:** 17:48:30 → 20:00:12, −1128 over 50 cycles (−23 B per cycle).
- **From the boot report:** 17:33:41 → 20:00:12, −1360 over 71 cycles (−19 B per cycle).
- **v33 comparison:** at about 130 B per cycle, v33 would have lost about 6.6 KB over 51 cycles, or about 9.2 KB over 71.
- **Reading:** every measure lies between +0.6 KB and −1.4 KB, against v33's 6.6–9.2 KB. The per-wake leak is fixed. A small leftover of up to about 20 B per cycle can't be ruled out at this count. Heap over a longer run, such as overnight or several days of field reports, would settle it. That is not a bench condition.
- **Bench closed.**

## Separate item

A **draft** bug report for Particle (not filed; Chip decides where): `docs/work-orders/WO-2026-10-02-003-particle-bug-report-draft.md`.

## Approval record

- [x] Stage 5 decision in advance of User Gate 2 (Chip, 2026-10-02, third):
  - **If Copilot removed only `resolveErrorAction()` case 14:** keep the removal (nothing can reach it once the loop stops raising 14, and fewer lines is the goal), and move it from "Later cleanup" to done.
  - **If it also removed the boot-time clear or the severity rank:** put them back as a pre-authorized narrow edit (Claude Code), because they protect devices with an alert 14 stored by older firmware during the rollout, and record it here.
- [x] Stage 5 decision (Chip, 2026-10-02, during Stage 6, second): for alert 14, only bypass the error-state route; leave the boot-time clear and the severity rank; note anything unused for a later cleanup rather than remove it. If Stage 6 went further, Stage 7 checks that nothing outside the out-of-memory path changed behavior.
- [x] Stage 5 decision (Chip, 2026-10-02, during Stage 6): add item B, restoring the original out-of-memory reset with code 7 and removing the unused `ERROR_STATE` route; budget about 5, separate from A's. Why: the Error/Idle bounce risk (above).
- [x] Stage 5: Chip and the architect, 2026-10-02, in the opening dispatch (goal, cause, change, budget, Stage 7 checks, version v34 / product 34, bench, the bug-report draft; routing: one Copilot round `claude-opus-5` medium, one narrow Stage 7 Codex `gpt-6-astra` high; not authorized: commits, flashing, filing anything with Particle).
- [x] Stage 6 item A: Copilot `claude-opus-5` medium. Four local `SystemSleepConfiguration`s (`hibernateConfig`, `ulpConfig`, `stopConfig`, `stopTimerConfig`); the global and its `extern` removed. **Net −2** `src/` lines (budget about 12). Leak test compiled against Device OS 6.4.1's real header (counting allocator, 5,000 cycles per site): 0 B with the new pattern; the old pattern loses 80 / 144 / 112 / 32 B per cycle by site (the ultra-low-power site with standby: 144). Structural ownership test, with 4 mutations caught. `sleep_breadcrumb_sequence_test.py` updated for the renamed `System.sleep(...)` argument. Suite 59/59. Release 150692 / 1090 / 2180. Report: `WO-2026-10-02-003-stage6-copilot-report.md`.
- [x] Stage 6 item B: Copilot `claude-opus-5` medium. The loop's out-of-memory block now logs, `delay(100)`, `System.reset(RESET_CAUSE_OUT_OF_MEMORY)` (7); `ResetCause.h` adds code 7; `resolveErrorAction()` case 14 removed (only that; boot clear and severity rank kept). **Net −7** `src/` lines (budget about 5). `reset_cause_structural_test.py` extended (7 sites); new behavioral `oom_immediate_reset_test.sh`; 6 mutations caught. Suite 60/60. Release 150556 / 1090 / 2180. Claude Code checked: item A's 7 files hash-identical; archive unchanged. Report: `WO-2026-10-02-003-stage6-itemB-copilot-report.md`.
- [x] Stage 7 round 1: Codex `gpt-6-astra` high, **NOT VERIFIED, on one test-completeness finding (P2)**.
  - **The finding:** `tests/sleep_config_ownership_structural_test.py:283` only rejects assignments spelled `= SystemSleepConfiguration(...)`. A brace assignment such as `ulpConfig = {};` after the wake sources are added passes both sleep checks, yet invokes the same leaking move assignment (confirmed leaking under the real-header allocator harness).
  - **Smallest fix (test only):** reject any assignment to a discovered configuration variable, whatever the right-hand side, and add that mutation. Codex's temporary correction passed the current source and caught the mutation.
  - **Everything else VERIFIED:**
    - leak check 0 B / 0 blocks over 5,000 cycles per site against the real 6.4.1 header (old pattern: 80 / 144 / 112 / 32 B per cycle);
    - wake sources and sequence identical to `43b8a69` apart from the ownership and names;
    - lifetimes;
    - the direct out-of-memory reset (log → delay(100) → reset(7) at all 256 reset counts);
    - nothing outside the out-of-memory path changed (`MyPersistentData.cpp` unchanged, the boot clear byte-identical, a stored 14 → `default` → Idle, as accepted);
    - the old-route mutation caught.
  - Suite 60/60 (31 sh via zsh, 29 py via python3). Release 150556 / 1090 / 2180, product 34; no configuration object in `.bss`/`.data`; `System.reset(7)` at the out-of-memory site. Budgets: A −2, B −7. Working tree byte-identical. Verdict: `WO-2026-10-02-003-stage7-verdict.md`.
- [x] **Narrow edit closing Stage 7's P2** (Claude Code, pre-authorized by Chip, 2026-10-02): `tests/sleep_config_ownership_structural_test.py` only.
  - **The change:** the test now collects every declared `SystemSleepConfiguration` variable and rejects any assignment to one (`name = …`, whatever follows the `=`, not `==`; declarations with initializers excepted), alongside the existing `= SystemSleepConfiguration(` check. It adds the brace-assignment mutation (`ulpConfig = {};` after the wake sources are added).
  - **Verified:** the test passes on the current source; **all 5 mutations caught** (the global restored, the assignment reset, **the brace assignment**, a static site, a changed wake edge); the suite **60/60 (31 sh via zsh, 29 py via python3)**.
  - **Scope:** the diff touches only that test file; it's the only file under `src/` or `tests/` changed since the Stage 7 verdict.
  - **v34 is VERIFIED.**
- [x] Stage 8: bench on Dev-09, PASS 2026-10-02 12:00Z (51 counted cycles, Δ`fh` +592). Committed by Claude Code on Chip's instruction (firmware and WO; the investigation records separately); PR opened, Chip merges.

## Budget versus actual (closing record)

Per `AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3. Actuals are from the Stage 7 verdict (`WO-2026-10-02-003-stage7-verdict.md`).

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| A. Per-wake leak (local configurations, global removed) | about 12 | — | −2 | not recorded |
| B. Direct out-of-memory reset (code 7; `resolveErrorAction()` case 14 removed) | about 5 | — | −7 | not recorded |
| **Total `src/`** | about 17 | — | **−9** | not recorded |
