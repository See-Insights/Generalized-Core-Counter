# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `1ad7094c-910d-4cef-9271-da08de411491`  
> - **Started:** 11/7/2026, 11:25:58 AM  
> - **Duration:** 3m 12s  
> - **Exported:** 11/7/2026, 11:29:10 AM  

---

<sub>7s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-05-004-stage6-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>9s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-004-stage6-copilot-dispatch.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-004-stage6-copilot-dispatch.md`

<details>
<summary>12 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/` (the S2 sites listed in the WO), tests under `tests/` (new S2 tests; existing hibernate/sleep tests and `tests/stubs/` only as the change forces), `docs/` only for the report; run host tests (zsh for .sh, python3 for .py), the local Device OS toolchain build and nm/objdump; scratch `build-tmp/wo-2026-11-05-004-stage6/` (create it; use only that) / NOT authorized: committing, pushing, merging, flashing, creating PRs or tags, touching `lib/` (the vendored `AB1805_RK` stays for the S3 callers), the two `deepPowerDown()` callers (S3), S1/S1b code (`time/ExternalRtc.h`, `time/PreBindRtc.h`, the bind and pre-bind calls), changing the persisted or `retained` layout beyond what Amendments 1-2 allow (a new `watchdogSource` value `inferred` only if the WO's option (b) applies; no layout change; stop and ask if no spare byte exists), deleting anything you did not create (including `build-tmp/` itself).

# Stage 6 dispatch: WO-2026-11-05-004 (v50 piece S2: sleep, alarm, watchdog and wake classification)
Binding spec: `docs/work-orders/WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md` **including Amendments 1 and 2 and the Grok follow-up V2**, and its parent `docs/work-orders/WO-2026-11-05-002-v50-ab1805-switch.md` (rulings 1-6 and the 6 Nov merge gate). Base: branch `wo/2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake` on `integration/v50-ab1805` at 3b5cfb8 (S1 #195 and S1b #196 merged). **The WO's line numbers are at the v49.1 tag and S1/S1b have since changed `src/`: re-cite every one at Step 0.** **Budget: `src/` at most +20 net** (removals count); stop and report the cut rather than trim style. Leave changes uncommitted. **Run at the dispatched reasoning level (medium); the report states the level actually run, or that it could not be observed.** This is a sleep/watchdog/wake change: be conservative, and stop and report on any Step 0 answer that breaks the plan.

## Do
1. **Step 0 first, answered in the report with file:line** (the WO's six questions), reading Device OS 6.6.0 from `~/.particle/toolchains/deviceOS/6.6.0`. The open attribution question is ruled by Amendment 1: (a) if Step 0 finds a public readback, else (b) with the new `inferred` source and the derived evidence of Amendment 2 (the persisted failsafe stage/action and the Error state's recorded hard-reset action; never `retained`). **If the window cannot be evaluated because `retained` is lost at a reason-20 power-up, or no spare byte exists, STOP and report.** Report whether a stop condition applies before editing.
2. **Implement "The change" items 1-5 with the Amendments.** The two `deepPowerDown()` callers stay (S3).
3. **Tests and mutations** exactly as the WO's Tests section and Amendments 1-2 (each mutation must fail; back up the file, mutate, restore from the backup; never use `git checkout`/`git restore` while your own uncommitted work is present). **Verify:** host suite `N/N (sh via zsh, py via python3)` (count with `p=$((p+1))`, never `((p++))`; `thermal_coupling_structural_test.py` also on a clean export of tracked files; some tests need `git`, so run them in the checkout; the baseline is 117/117); local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE` as a make argument) with text/data/bss (baseline for this base 153100 / 1114 / 2240) and `nm`/objdump showing the new call sites linked; `git diff --numstat 3b5cfb8 -- src` net <= +20 (state the number); layouts and fixtures untouched.
## Stop and report if
the budget cannot hold; `System.sleep()` HIBERNATE with `duration()` plus GPIO wakes does not arm the chip alarm as the WO assumes; `ExternalWatchdog` cannot represent the 124 s timeout; `retained` or the persisted evidence is not available at classification time; any S1/S1b behaviour would have to change.
## Report
`docs/work-orders/WO-2026-11-05-004-stage6-copilot-report.md`: model and reasoning level actually used, interpreter line, Step 0 answers with file:line, net `src/` lines per file, test names and mutation results, build and `nm` evidence, deviations, open questions. The bench (Chip, from 14 Nov, on the integrated head) is not yours: claim nothing about hardware.
```

</details>


---

<sub>12s</sub>

### `bash`

**Check WO sizes and repo state**

`$ cd docs/work-orders && wc -c WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md WO-2026-11-05-002-v50-ab1805-switch.md; git status --short | head; git log --oneline -1`

```
14880 WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md
   13609 WO-2026-11-05-002-v50-ab1805-switch.md
   28489 total
ebbf07f WO-2026-11-05-004 (S2): work order (with Amendments 1-2, V2) and Stage 6 Copilot dispatch
<shellId: 0 completed with exit code 0>
```


---

<sub>15s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md`

<details>
<summary>52 lines</summary>

```
# WO-2026-11-05-004: v50 piece S2 — sleep, alarm, watchdog and wake classification on the built-in AM18x5 driver

**Author:** Claude Code, 5 Nov 2026, DRAFT for the architect's review; not dispatched · **Status:** DRAFT; architect's ruling on the attribution question recorded (Amendment 1, 5 Nov); dispatch when the gate opens
**Parent:** `WO-2026-11-05-002-v50-ab1805-switch.md` (Plan B, rulings of 5 Nov); **S1:** `WO-2026-11-05-003-v50-s1-boot-config-time.md` (Amendment 1, ruling 3: `ab1805.loop()` removal is S2's) · **Branch (when dispatched):** `wo/…-v50-s2-…` stacked on `integration/v50-ab1805` after S1, from `main` after v49.1 is the product default and C1/C2 have merged
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (sleep, watchdog and wake logic: Codex xhigh); §12; §13 (c), (j) · **Agents:** Stage 6 Copilot standard/medium; Stage 7 Codex `gpt-5.6-sol` **xhigh**, at most two rounds · **Budget:** `src/` at most **+20 net** (removals count); stop and report rather than trim style.
**Dispatch gate:** only once v49.1 is the product default and C1/C2 have merged (architect, 5 Nov). **S2 never merges to `main` alone** (merge as a set with S1 and S3).

## Plain goal
After S1, Device OS owns the chip's binding and time, but GCC still drives the same chip through the vendored `AB1805_RK` for the **hibernate alarm, the chip watchdog and the wake classification**: two owners in one build. S2 moves those three to Device OS: the hibernate alarm is armed by `System.sleep()` itself, the chip watchdog is driven by `ExternalWatchdog`, and a wake is classified from the **OS reset reason plus the retained request and time window**, not from the chip's raw flags. After S2 no `ab1805.` call remains except the two `deepPowerDown()` callers (S3). Behaviour the fleet sees (sleep durations, wake verdicts, the `hibernate_wake` event, the watchdog timeout) is unchanged.

## Rulings that bind this piece (parent WO; not reopened)
1. **Wake classification (ruling 2):** reason 30 (`RESET_REASON_POWER_MANAGEMENT`) with the alarm flag set is a native RTC hibernate wake (spike P5, both rounds). A power-down by the vendored `deepPowerDown()` came back with **reason 20 (`PIN_RESET`)** (spike P7). **Reason 20 at a power-up is a wake only inside a due wake window** (retained request + RTC time); **otherwise a pin reset** (architect, 5 Nov, ruling 5). The reason after a native `POWER_OFF` is unobserved: S3's bench records it and this table is corrected if it differs.
2. **One time API (ruling 4):** `hal_rtc_*` and `TimeClass(EXTERNAL)` from S1's helpers; S2 reuses them, adds none.
3. **Hibernate proof (ruling 6):** two real 8 h nights in the v50 bench soak with serial attached; the fallback is the vendored library (the guard returns). The 8 h alarm and P5 round 2's timing are unproven.

## The change (lines at the v49.1 tag; re-check every one at Step 0)
1. **Alarm.** `State_Sleep.cpp:699` `ab1805.interruptAtTime(wakeTime)` and the whole pre-arm block: `SystemSleepConfiguration().mode(HIBERNATE).duration(wakeInSeconds * 1000)` plus the existing `.gpio(WAKEUP_PIN, FALLING)`/`.gpio(BUTTON_PIN, FALLING)`; Device OS arms the chip's one-shot alarm. The "failed to program AB1805 alarm" fallback (`:747`) goes (a native sleep error is already handled by the existing return path: `HibernateCycle::abandon()`, resume the watchdog, ULTRA_LOW_POWER fallback). Keep `shouldUseBoronRtcAlarmHibernate()` (the 900-36000 s window, `HibernateEligibility.cpp:~58-95`) with its RTC read moved to S1's helpers; `armForSleep(rtcNow, wakeInSeconds)` keeps the RTC-before value.
2. **Stale-alarm clearing deleted.** `clearAb1805StaleAlarmInterrupts()` (`HibernateEligibility.cpp:24-51`, header `:5`, the call at `State_Sleep.cpp:697`) writes STATUS, INTMASK and the timer control by raw register. Device OS owns those registers now. Delete it; the stale-alarm risk is proven by the two real nights, not by code. The `digitalRead(WAKEUP_PIN) == HIGH` check before the sleep (`:695`) stays as the cheap guard.
3. **Watchdog.** `ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS)` (`Generalized-Core-Counter.cpp:809`), `stopWDT()` (`State_Sleep.cpp:709,785`) and `resumeWDT()` (`:742,898,926`) → `ExternalWatchdog.init/start/stop/refresh` (instance 2, DONE on D5, the timeout equal to today's 124 s ceiling; distinct from GCC's internal `Watchdog`). Refresh happens where `ab1805.loop()` serviced the chip today (`:1174`), which is **deleted here** (S1 Amendment 1, ruling 3). `checkClockResync()` on the next line stays. Keep `pauseAwakeWatchdogForSleep()`/`restoreAwakeWatchdogAfterWake()` as they are.
4. **Wake classification.** `HibernateCycle::classifyWake(int osResetReason, AB1805 &rtc)` (`HibernateCycle.cpp:44`, header `:101`, caller `Generalized-Core-Counter.cpp:789`) loses its `AB1805&` parameter (the `class AB1805;` forward declaration at `HibernateCycle.h:45` goes). The six-arm gate keeps its order and its `GateArm` values; arm `kWakeReason` is evaluated from the **OS reason and the retained window**: reason 30, or reason 20 inside a due window (`rtcAtWake` within 0 to +60 s of `rtcBefore + requestedSleepSec`, the existing `dpdOnTime` rule, now applied to reason 20). `ab1805WakeReasonName()` (`ClockDiagnostics.cpp:14-32`, header `:2-3`) and the `AB1805_RK.h` includes in `HibernateCycle.cpp:4`, `ClockDiagnostics.h:2`, `StateMachine.h:4`, `State_Sleep.cpp:21`, `State_Error.cpp:13` and `Clock.cpp:10` (if S1 left it) go once nothing needs them; the `extern AB1805 ab1805` at `StateMachine.h:31` stays until S3 (the two `deepPowerDown()` callers).
5. **The chip-watchdog attribution (open question, see below).** Today a `PIN_RESET` is confirmed as the chip's watchdog from the chip's wake reason (`:789-806`, `startupAb1805ConfirmedWatchdog`, the `ab1805_watchdog` forensic event, `RecoveryState` source `AB1805_PIN`, reset cause 19 in `MyPersistentData.cpp:792`). Without the vendored `setup()` that source of truth is gone.

## Open question for the architect (decide before dispatch)
**How is a chip-watchdog reset attributed after S2?** The soak and fleet checks count `watchdogResetCount`, and the AB1805-confirmed `PIN_RESET` path is how a chip-watchdog reset is told from a manual pin reset today. Options:
- **(a)** Step 0 finds a public readback on Device OS 6.6.0 (`Am18x5.status()` / an `ExternalWatchdog` flag / an `ExternalTime.onEvent` watchdog event) and the confirmation is kept on it. Preferred if it exists.
- **(b)** Infer it: a `PIN_RESET` that is neither a due wake window nor within the boot-after-flash/power-up cases is counted `watchdog-suspected` with a new `watchdogSource` value (a payload change; `RecoveryState` has room, `MyPersistentData.h:176`).
- **(c)** Drop the attribution and keep only the OS `RESET_REASON_WATCHDOG` path (loses the count the soaks use; **not recommended**).
Step 0 answers (a); the architect rules (b) or (c) if (a) is absent. Until ruled, S2 is not dispatched.

## Step 0 (read-only, inside Stage 6, before editing; stop and report if any answer breaks the plan)
1. **`System.sleep()` with HIBERNATE and `duration()` on 6.6.0:** which alarm source does it arm on a bound AM18x5 (the chip alarm, not the nRF RTC)? Does a `duration()` plus `.gpio()` wake mix work (both wake sources armed at once)? The maximum duration (our window reaches 36,000 s). What the call does if the chip is not ready (error code, no sleep?). File:line in `system_sleep` / `hal` for the `POWER_MANAGEMENT` reason on a chip alarm wake.
2. **`ExternalWatchdog`:** the exact API (`init`, `start`, `stop`, `refresh`, instance number), the timeout range (is 124 s representable?), what happens at `System.sleep()` (does Device OS stop it for the sleep, or must we, as `stopWDT()` does today?), and the arming state across a reset (the chip keeps counting; the v50 → v49 rollback bench item 2 depends on this). Whether Device OS itself refreshes it, and from where (so the refresh we add is not a second source).
3. **Attribution:** answer the open question (a) with file:line, or state that none exists.
4. **The window arithmetic for reason 20:** where in `setup()` is the RTC readable (S1 binds before anything) so `rtcAtWake` exists when `classifyWake()` runs; confirm `retainedHibernatePending` survives a reason-20 power-up (it is RAM `retained`: a power-down clears it, spike P7 lost the RTC). **If `retained` is lost at a power-up, the window cannot be evaluated; state what the rule becomes and stop.**
5. **Dead code and symbols:** list every `AB1805`-type use that remains after the change; the test `nm`/grep list for `HibernateCycle`, `ClockDiagnostics` and the headers.
6. **Count.** The net `src/` lines per file; if over +20, stop and report the cut (candidates: move the classification-table rename into S3's cleanup; keep `ab1805WakeReasonName()` until L).

## Tests (host; the real functions)
Fakes for `System.sleep`/`ExternalWatchdog`/`hal_rtc_*` driving the real call sites: the `classifyWake()` table (reason 30 + window → wake; reason 30 + early/late → each failing arm as today; reason 20 + due window → wake; reason 20 + no wake due → pin reset; reason 20 + `retained` cleared → pin reset; other reasons); the hibernate attempt arms exactly one duration of `wakeInSeconds` and both GPIO wakes; a native sleep error falls back to ULTRA_LOW_POWER with `abandon()` and a watchdog restart; the watchdog is stopped before and started after each sleep path (structural, in order, all five sites); the refresh sits where `ab1805.loop()` was; `clearAb1805StaleAlarmInterrupts` and `interruptAtTime` have no remaining reference; no `ab1805.` call remains outside the two S3 callers (structural). The existing `HibernateWakeDiagnostics` and hibernate tests keep their expectations (their inputs change, not their outputs). Mutations (each must fail): reason 20 always a wake; reason 20 never a wake; the window widened or dropped; the alarm armed twice; no watchdog stop before a sleep path; refresh missing; the HIBERNATE fallback removed. `nm` on the local ELF for linkage.

## Bench (Chip, from 14 Nov; **not benched alone**: the integrated head carries the bench with S1 and S3)
The parent WO's pre-ship items 2, 3 and 5, in particular: **two real 8 h nights, serial attached**: per wake the actual versus the requested time by GCC's own method, the reset reason, the `STATUS` at boot, the teardown time before the sleep call, and the boot clock versus the first sync (P5 lacked these). Plus a **watchdog proof**: stop refreshing on a bench build and confirm the chip resets the board in about 124 s and that the reset is attributed as ruled.

## Protected / not in scope
S1 (boot, binding, time), S3 (`deepPowerDown()` callers), L, I21; the clock-trust rule, the failsafe ladder and cooldowns, the hibernate verdict policy (eligibility window, the six `GateArm` conditions), item 19's credit logic, the persisted/`retained` layouts (a new `watchdogSource` value only if option (b) is ruled), M-SoM.

## Rollback
Revert the S2 commit on the integration branch before the set merges. After the set ships, rollback is to v49.1 (the guard deletes the stored binding; bench item 2 proves the armed chip watchdog does not reset the board first).

## Amendment 1 (architect ruling, 5 Nov 2026) — chip-watchdog attribution; S2 may be dispatched when the gate opens
1. **(a) if Step 0 finds a public readback** (confirm the attribution on it, as today). **Otherwise (b):** keep counting in `watchdogResetCount` with a **new `watchdogSource` value `inferred`**. **(c) is rejected.**
2. **Condition for (b):** a reason-20 boot counts as a watchdog reset **only if there is no due wake window and no power-off was requested**, the request read from the **persisted** failsafe or recovery record (written by v49's `flushAllBeforeReset()` before stage 3 and before the ERROR_STATE hard power-down), **not from `retained` RAM** (a power-down clears it). This binds S3: its two callers must persist the "power-off requested" record before the call, and that record is cleared once the boot has classified the wake.
3. **Tests added:** the (b) table (reason 20 + due window → wake; reason 20 + persisted power-off record → not a watchdog; reason 20 + neither → `inferred` watchdog and the count; the record absent because `retained`-only → not accepted as evidence); mutations (record read from `retained`; count without the check; window ignored).
4. **Bench:** the **watchdog proof** (stop refreshing on a bench build; the chip resets the board in about 124 s; the reset is attributed as `inferred` or by the readback) is a required pre-ship item, in addition to the items above.
5. Budget unchanged (≤ +20 net); if the persisted-record read or the new `watchdogSource` value pushes it over, stop and report the cut.

## Amendment 2 (architect, 5 Nov 2026) — the "power-off requested" evidence is derived, not new
No new flag. The evidence is **derived from existing persisted fields:** the failsafe's persisted **stage and action** (`persistConnectivityFailsafeState(3, now, true)`, written before the stage-3 power-off) and the Error state's **recorded hard-reset action** (the persisted recovery/alert record the ERROR_STATE hard path writes before `flushAllBeforeReset()`). Step 0 names the exact fields and file:line for each caller. If a path persists nothing usable, **use an existing spare byte** (no layout change); **only if there is none, stop and ask.** `retained` RAM is never evidence. Tests: the (b) table reads the derived fields; a mutation that reads `retained` instead must fail.

## Grok follow-up V2 (Chip, 6 Nov 2026): reason 20 and the post-`POWER_OFF` wake
Grok's V2 (`docs/reviews/2026-11-grok-progress/grok-response-2.md`): the rule "reason 20 inside a due window is a power-up wake" is unverified against a real native power-off; a power-off that returns with reason 20 and a cleared retained window would be classified as a pin reset. Ruled **already gated** (no added scope). The gates are:
- **Step 0 item 4** (the window arithmetic for reason 20, and that `retainedHibernatePending` and the RTC can be lost on a power-down, spike P7);
- **the parent's ruling 5** and **S3's ruling 2:** the post-`POWER_OFF` wake reason is unobserved; **S3's bench must record and confirm it before v50.1 releases**, and **this WO's classification table is corrected if the reason or the retained state differs** from the table;
- the (b) table in Amendment 1 (reason 20 with neither a due window nor a persisted power-off record counts as `inferred`; `retained`-only evidence is not accepted).
**Gate timing (Chip, 6 Nov): the stricter gate is ruled.** The S-set does not merge to `main` until all five pre-ship bench items pass on the integration branch's CI binaries (parent WO, "Release and merge rule"); the S3 bench result is one of them.
```

</details>


---

<sub>15s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-002-v50-ab1805-switch.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-002-v50-ab1805-switch.md`

<details>
<summary>50 lines</summary>

````
# WO-2026-11-05-002: v50 — switch GCC from the vendored AB1805_RK to the built-in AM18x5 driver (sequence, pieces, dependencies)

**Author:** Claude Code, 5 Nov 2026, DRAFT for the architect's review; not dispatched · **Status:** DRAFT; architect's rulings of 5 Nov recorded below (Plan B; S1 drafted as `WO-2026-11-05-003-v50-s1-boot-config-time.md`)
**Branch (when approved):** one WO branch per piece, from `main` after v49.1 is the fleet default and C1/C2 have merged; the pieces stack on `integration/v50-ab1805`
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (boot-time hardware, time and sleep: Codex xhigh); §12; §13 (c), (j). **Agents** for every piece: Stage 6 Copilot standard/medium; Stage 7 Codex `gpt-5.6-sol` **xhigh**, at most two rounds each.
**Inputs:** `WO-2026-10-31-001-step0-report.md` (Step 0, Rulings 1-9, 1 Nov); `WO-2026-11-01-001-ab1805-native-driver-spike.md` (spike result record and the architect's final ruling, 4 Nov); `WO-2026-11-04-002-v49-binding-delete-guard.md`; the "v50 record" in the Step 0 WO; recovery plan items 18, 19, 21.

## Plain goal
GCC stops driving the carrier-board AB1805 itself (the vendored `lib/AB1805_RK`, 2,068 lines, one I2C owner today) and lets Device OS 6.6.0's built-in AM18x5 driver own the chip: one binding, one set of time/alarm/watchdog/power-off calls, the correct, protected `PWGT` key sequence for the timed power-off, and the shared driver for M-SoM later. (**Not a known item-18 defect:** the spike's P7 showed the vendored `deepPowerDown()` does power down on Dev-14: LED dark and USB gone for 120 s, then a boot with reset reason 20. The vendored code lacks the key write the built-in driver performs, but that has not been seen to fail on this hardware.) The application keeps every policy (clock trust, hibernate verdicts, failsafe ladder). Target release: **v50**, with item 19 C1/C2 (#185, #184) and item 21.

## Rulings that bind this WO (not reopened here)
1. **One fleet-wide configuration:** `particle::Am18x5Configuration`: `Wire` (address 0x69), `.interruptPin(WKP)`, `.watchdogPin(D5)`, RC fallback on (`RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL`), always-on RC **off** (default). Seen on the spike: `bound=1 present=1 ready=1 clkSrc=2 caps_en=0x20`. `Am18x5.enable(cfg)` stores the binding (`EXRTC_CONFIG_DATA`) and binds; later boots bind before `setup()`.
2. **Wake classification:** OS **reset reason plus time-window evidence**, not the raw WDT/ALM/TIM/SLST flags. Observations to build on: a native RTC hibernate wake boots with **reason 30** (`RESET_REASON_POWER_MANAGEMENT`) and the alarm flag set (`STATUS` 0x04) (spike P5, both rounds); a power-down by the vendored `deepPowerDown()` came back with **reason 20 (`PIN_RESET`)**, not a power-down reason (spike P7; likely the AB1805 asserting nRST at power-up: a guess), the RTC unset and the session lost. **Reason 20 at a power-up must therefore be classified by the time window (a wake due at that time), not read as a manual pin reset.** The wake reason after a native `POWER_OFF` has not been observed: S3 records it.
3. **Oscillator faults (item 21):** event-based evidence ("fault seen since bind", via `ExternalTime.onEvent`); the boot-sticky OF flag is not needed.
4. **One time API,** used throughout: the spike read the chip with `TimeClass(TimeSource::EXTERNAL)` and `hal_rtc_get_time(HAL_RTC_SOURCE_EXTERNAL)` and the two agreed to the second (`r=0`). **Pick `hal_rtc_*` for both get and set** (one API, an error result the existing `Clock` code maps to its bool success); `TimeClass(EXTERNAL).isValid()` replaces `isRTCSet()`. (`ExternalTime` itself has no `isValid()`/`setTime()`; Step 0 correction.) The spike's `settime ext|hal` write results are not in the logs reviewed: confirm in S1's Step 0 before relying on the setter.
5. **Clock-trust rule unchanged** in the switch (item 21 follows).
6. **Hibernate:** the architect's final spike ruling: the AB1805 switch stays in v50; proof is **two real nights in the v50 bench soak with serial attached**; fallback as planned if that fails. P5 on Dev-14 (two 4 h hibernates) showed no early or stale-alarm wake; the 8 h alarm and round 2's timing are not proven.
7. **Rollback floor:** **v49 is the rollback floor and the fleet default before v50 ships.** v48 and older have no binding guard.

## Sequence, pieces and dependencies (each piece ≤ +20 net `src/`; removals count)
```
v49 is the fleet default (gate)   →  hold lifted: C1 (#185), C2 (#184) merge to main
        →  S1 → S2 → S3 (one integration branch, merged as a set)  →  pre-ship bench (Chip)
        →  v50.1 tag  →  soak (two real nights, serial attached)
        →  L (library removal) and I21 (item 21)  →  v50.2
```
**Plan B is ruled (architect, 5 Nov): S1, S2 and S3 are separate WOs** (the Step 0 estimate of −6 to +19 for the whole switch no longer holds: the scope grew with the guard replacement, the register-clearing removal, reason-and-window wake classification, native sleep/power-off with an abort fallback, and event telemetry).
**The pieces.** They stack on one integration branch `integration/v50-ab1805` and **merge to `main` together**: merging any one alone leaves **two owners** (native for time, vendored `Wire` calls for alarm or watchdog), the hazard the guard exists for. Each piece gets its own Stage 6 and Stage 7; the integrated head gets a final Stage 7 and the bench.
- **S1: boot, configuration and time** (≤ +20; drafted in full: `WO-2026-11-05-003-v50-s1-boot-config-time.md`). Replace `ab1805.withFOUT(WKP).setup()` (`Generalized-Core-Counter.cpp:667`) and the v49 guard by the configuration and a bind-if-not-bound `Am18x5.enable(cfg)` (check the result; check a stored config matches); startup oscillator snapshot (`usingRCOscillator()`, the raw OSC registers, `:684-686`) → `Am18x5.status().clockSource()` and `getConfig().capabilities()`; `Clock.cpp` get/set (`:402-403`) and `ClockTrust`/RTC-skew test hook → `hal_rtc_*`; `isRTCSet()` (`:868`, `HibernateEligibility.cpp:75`) → `TimeClass(EXTERNAL).isValid()`; delete `ab1805.loop()` (`:1174`; the app-side write-back `checkClockResync()` stays); Device OS now seeds system time from the chip before `setup()`, so the future-date clamp and the "setup seeds Time" comments are re-read. **Remove the v49 binding guard** (this piece owns it).
- **S2: sleep, alarm, watchdog and wake classification** (≤ +20; depends on S1; **also removes `ab1805.loop()`** (`Generalized-Core-Counter.cpp:~1174`), which services the chip watchdog S2 replaces: S1 amendment, 5 Nov). `interruptAtTime()` (`State_Sleep.cpp:699`) → `SystemSleepConfiguration().mode(HIBERNATE).duration()` (Device OS arms the one-shot alarm); the raw STATUS/AIE/TIE/RPT clearing in `HibernateEligibility.cpp:24-51` is deleted (the stale-alarm risk is proven by the two real nights, not by code); `setWDT/stopWDT/resumeWDT` (`Generalized-Core-Counter.cpp:809`, `State_Sleep.cpp:709,742,785,898,926`) → `ExternalWatchdog.init/start/stop/refresh` (instance 2, DONE on D5; distinct from GCC's internal `Watchdog`); `HibernateCycle` wake classification by reset reason plus the retained request/window (reason 30 = wake; reason 20 inside a due window = a power-up wake; a reason-20 with no wake due stays a pin reset).
- **S3: timed power-off** (≤ +20; depends on S1). `ab1805.deepPowerDown()` at the two callers (`ConnectivityFailsafe.cpp:316`, stage 3; `State_Error.cpp:178`, hard) → `System.sleep()` with `SystemSleepMode::POWER_OFF` and a duration, after the v49 `flushAllBeforeReset()`; a native abort (it returns `ABORTED`, not a silent success) falls back to `System.reset()` with the existing reset cause. Bench: the **#151 TEST_MODE stage 2 → 3** run watching the power rail, the modem, the off-time and the reset reason after the wake (this also records the post-`POWER_OFF` reason that classification needs).
- **L: library removal** (≤ 0 `src/` lines; about −2,068 repo lines; depends on S1-S3 merged and one clean soak day). Delete `lib/AB1805_RK`, every `#include "AB1805_RK.h"` and the `AB1805 ab1805` object; verify no `AB1805` symbol or include remains (`nm`, grep, `project.properties`), build every supported target, and run a Dev-14 smoke cycle. Run CI's library inventory after it.
- **I21: item 21, trust survives a restart and is withdrawn on evidence** (≤ +20; depends on S1-S3, the event telemetry and C1/C2). Needs its own Step 0 first (the recovery plan lists it: every decision gated on `isClockTrusted()`/`Clock::openness()`; the fleet's `correctionSec` to size the threshold; where the last-sync epoch is persisted, which touches the persistence layout rules). Evidence: a large `correctionSec` at sync, an invalid RTC, or an oscillator-fault **event** since bind.

## Protected / not in scope
Clock-trust rule (until I21), the failsafe ladder and cooldowns, hibernate verdict policy, item 19's credit logic (C1/C2 merge first and are not rebased onto the new driver by this WO), the persisted and `retained` layouts (unless I21 says otherwise), M-SoM bring-up, `lib/` other than `AB1805_RK`.

## Tests (host; the real functions) — common to S1-S3
Fakes for `Am18x5`/`ExternalTime`/`ExternalWatchdog`/`hal_rtc_*` driving the real call sites: bind-if-not-bound; a stored binding with a different config; every get/set error path maps to the existing bool outcomes; wake classification table (reason 30 + window; reason 20 + window; reason 20 without a wake due; other reasons); native sleep abort falls back to the reset; watchdog stop before and start after each sleep path (structural, in order); no `AB1805_RK` symbol referenced by S-set code. Mutations per piece as in the v49 guard WO (each must fail). `nm` on the local ELF for linkage.

## Pre-ship bench (Chip, Dev-14 then Dev-09; the v50 pre-ship checklist, moved from the guard WO)
1. **Binding present, v49 over a bound device:** flash `ab1805-spike.bin` (`~/Downloads/spike-2026-11-03/`), `bind`, reboot, `status` bound=1; flash the v49 build: the deletion line once, reset data 8, vendored setup works in the same session, cloud connects, no loop, no deletion line on the second boot.
2. **Rollback v50 → v49:** run v50 until the chip is bound **and the external watchdog is armed**, then flash v49 over it: v49 deletes the binding; **the chip's own armed watchdog must not reset the board before v49's first watchdog service** (`Generalized-Core-Counter.cpp:~809`; the Stage 7 note on the guard: not source-proven bounded). Confirm a single owner afterwards.
3. **v50 on a bound device and on an unbound one:** first boot binds once; later boots do not re-enable; **on the first v50 boot the RTC keeps valid time** (S1b's pre-bind seed; S1 Amendment 4 reverses Amendment 3's accepted exposure: Device OS's bind overwrites the chip with the year-2000 internal clock otherwise); the stored config is the fleet configuration.
4. **#151 stage 2 → 3** (S3) and the error-state power-off, watching the rail.
5. **Two real 8 h nights with serial attached** (the soak): per wake record `actual − req` by GCC's own method, the reset reason, the AM18x5 `STATUS` at boot, the teardown time before the sleep call, and the boot-clock-versus-first-sync reading (the spike's P5 lacked these). Pass: each wake within 5 s of the request, the second not early.
Release rule: v50.1 releases only on Stage 7 of every piece and the integrated head, items 1-4 passed, and the two nights clean; fallback as planned if the nights fail (keep the vendored library; the guard returns).

## Rollback
Revert the integrated PR (v49 behavior returns; the stored binding, if any, is deleted by v49's guard). Rolling back past v49 is not supported once v50 is out.

## Rulings (architect, 5 Nov 2026) and timeline
1. **Plan B now:** S1, S2, S3 separate WOs on `integration/v50-ab1805`. 2. **Merge as a set:** agreed (no piece merges to `main` alone). 3. **v50.1 = C1 + C2 + the S-set; v50.2 = L + I21.** 4. **The setter is `hal_rtc_set_time(EXTERNAL)`**, confirmed in S1's Step 0 **with a test**, or stop and name the alternative. 5. **The reason-20 rule as drafted** (a power-up wake with reason 20 only inside a due wake window; otherwise a pin reset); **S3's bench must confirm the reason after a native `POWER_OFF` before v50.1 releases** (S2's table is corrected if it differs).
- **Dispatch gate:** Stage 6 of S1-S3 is dispatched only once **v49.1 is the product default** and C1/C2 have merged (#185, #184; the "merge after v49 is default" hold).
- **Timeline (architect, 5 Nov):** the **pre-ship bench needs Chip from 14 Nov**; **RC1 moves to about 20-23 Nov** (v50.1 bench and the two real nights first, then the soak, then v50.2 and the RC1 call).

## Rulings of 5 Nov (later): S2 attribution and the I21 latch
S2 Amendment 1 (watchdog attribution: readback if present, else `inferred` from no due window and no persisted power-off record; (c) rejected; S3 persists that record; watchdog bench proof required). S1 Amendment 2 (the "fault seen since bind" latch is the S-set's: S1 if it fits, else S4 ≤ +10). S2 may be dispatched when the gate opens.

## S1b added (Chip, 6 Nov): `WO-2026-11-06-002-v50-s1b-pre-bind-rtc-access.md`
Sequence: S1 → **S1b** → S2 → S3 (+ S4) on `integration/v50-ab1805`. S1b (at most +15 `src/`) does the pre-bind chip access (seed `Time` from a plausible chip time, set WRTC) that S1's Stage 7 round 1 showed is needed; bench: on Dev-14 `settime … hal` fails without S1b and succeeds with it, and the first v50 boot keeps valid time. It is part of the set and subject to the same merge gate.
````

</details>


---

<sub>17s</sub>

### `bash`

**Locate AB1805 uses, tests, toolchain**

`$ git log --oneline -6; git branch --show-current; grep -rn "ab1805\|AB1805" src --include=*.cpp --include=*.h | grep -v "^src/.*://" | head -120; ls tests | head -150; ls ~/.particle/toolchains/deviceOS/`

<details>
<summary>282 lines</summary>

```
ebbf07f WO-2026-11-05-004 (S2): work order (with Amendments 1-2, V2) and Stage 6 Copilot dispatch
3b5cfb8 Merge pull request #196 from See-Insights/wo/2026-11-06-002-v50-s1b-pre-bind-rtc-access
deb2368 WO-2026-11-06-002 (S1b): fix two persistence comments (boot order); architect ruling closes Stage 7 without a third Codex pass; +17 net src/ unchanged
5a56aaf WO-2026-11-06-002 (S1b): Stage 7 round 2 verdict (NOT VERIFIED on c0e4b27: runtime PASS; two persistence comments state the boot order backwards; architect ruling)
3031803 WO-2026-11-06-002 (S1b): Stage 7 round 2 Codex dispatch (UNDER REVIEW c0e4b27; 90-minute time-box, deadline 05:20:40Z)
c0e4b27 WO-2026-11-06-002 (S1b): Amendment 3 — clear WRTC before the time read, always write normalised CONTROL1, return false on any failed transaction; comments; +17 net src/; suite 117/117
wo/2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake
src/MyPersistentData.cpp:792:        case 19: // watchdog reset (Device-OS-detected or AB1805-confirmed PIN_RESET)
src/power/ConnectivityFailsafe.cpp:316:  ab1805.deepPowerDown();
src/sensors/SensorManager.cpp:418:  // Guard against interference with other I2C users (AB1805, FRAM, etc.).
src/ResetCause.h:12: * sites (the AB1805 library fallback, a cloud reset, or Device OS itself).
src/device_pinout.cpp:84:    // owned by the carrier hardware and the AB1805 wiring.
src/device_pinout.cpp:86:    pinMode(WAKEUP_PIN, INPUT_PULLUP);    // AB1805 FOUT/nIRQ (open-drain, active-LOW, needs pull-up)
src/MyPersistentData.h:176:		uint8_t lastWatchdogSource;                       // WatchdogSource enum: which mechanism reported the last watchdog reset (DEVICE_OS vs AB1805_PIN)
src/BuildProfile.h:199: * classification block: it reads the current AB1805 RTC value, writes back
src/BuildProfile.h:214: * AB1805 is Boron hardware). After the bench test completes, set back to 0
src/BuildProfile.h:271:    {"net.pppncp", LOG_LEVEL_WARN}, {"app.ab1805", LOG_LEVEL_WARN}
src/state/StateMachine.h:4:#include "AB1805_RK.h"
src/state/StateMachine.h:31:extern AB1805 ab1805;
src/state/State_Sleep.cpp:21:#include "AB1805_RK.h"
src/state/State_Sleep.cpp:699:          if (ab1805.interruptAtTime(wakeTime)) {
src/state/State_Sleep.cpp:709:            const bool watchdogStopped = ab1805.stopWDT();
src/state/State_Sleep.cpp:717:            // ab1805.interruptAtTime() only - nothing reads it back on
src/state/State_Sleep.cpp:742:            ab1805.resumeWDT();
src/state/State_Sleep.cpp:747:            Log.warn("HIBERNATE bypass: failed to program AB1805 alarm - falling back to ULTRA_LOW_POWER");
src/state/State_Sleep.cpp:785:  const bool watchdogStopped = ab1805.stopWDT();
src/state/State_Sleep.cpp:898:        ab1805.resumeWDT();
src/state/State_Sleep.cpp:926:  ab1805.resumeWDT();
src/state/State_Sleep.cpp:1258:  // NO AB1805 alarms - AB1805 is only used for watchdog + RTC time sync.
src/state/State_Common.h:225: *          SYNC's future->get()) - the same hazard the vendored AB1805_RK
src/state/State_Error.cpp:13:#include "AB1805_RK.h"
src/state/State_Error.cpp:84:    // Start with a soft reset, then escalate to an AB1805 deep
src/state/State_Error.cpp:172:    // Hard recovery using AB1805 deep power down after delay. This fully
src/state/State_Error.cpp:178:      ab1805.deepPowerDown();
src/time/HibernateCycle.cpp:4:#include "AB1805_RK.h"
src/time/HibernateCycle.cpp:9:extern const char *ab1805WakeReasonName(AB1805::WakeReason reason);
src/time/HibernateCycle.cpp:44:WakeVerdict classifyWake(int osResetReason, AB1805 &rtc) {
src/time/HibernateCycle.cpp:47:  // ===== PIN_RESET / AB1805 WATCHDOG CONFIRMATION =====
src/time/HibernateCycle.cpp:49:  // AB1805 watchdog firing via the reset pin) as PIN_RESET, not WATCHDOG -
src/time/HibernateCycle.cpp:50:  // so the only way to confirm the AB1805 was the cause is to ask it
src/time/HibernateCycle.cpp:54:  // IMPORTANT: this relies on AB1805::getWakeReason() being a plain getter
src/time/HibernateCycle.cpp:55:  // (returns a cached member, see AB1805_RK.h) onto the result of the ONE
src/time/HibernateCycle.cpp:56:  // destructive updateWakeReason() call AB1805::setup() already made
src/time/HibernateCycle.cpp:65:    const AB1805::WakeReason pinResetWakeReason = rtc.getWakeReason();
src/time/HibernateCycle.cpp:66:    v.pinResetWakeReasonName = ab1805WakeReasonName(pinResetWakeReason);
src/time/HibernateCycle.cpp:68:    if (pinResetWakeReason == AB1805::WakeReason::WATCHDOG) {
src/time/HibernateCycle.cpp:70:    } else if (pinResetWakeReason == AB1805::WakeReason::UNKNOWN) {
src/time/HibernateCycle.cpp:71:      // Explicitly inconclusive - do NOT treat UNKNOWN as "not the AB1805".
src/time/HibernateCycle.cpp:91:    const AB1805::WakeReason wakeReason = rtc.getWakeReason();
src/time/HibernateCycle.cpp:92:    v.wakeReasonName = ab1805WakeReasonName(wakeReason);
src/time/HibernateCycle.cpp:100:    // WO-2026-10-04-001 item C: the AB1805 library labels every wake
src/time/HibernateCycle.cpp:106:    in.wakeReasonIsAlarm = (wakeReason == AB1805::WakeReason::ALARM) ||
src/time/HibernateCycle.cpp:107:                           (wakeReason == AB1805::WakeReason::DEEP_POWER_DOWN && dpdOnTime);
src/time/HibernateWakeDiagnostics.h:22: *          No Particle/AB1805 dependency, so this compiles and tests on the
src/time/HibernateWakeDiagnostics.h:41:  kWakeReason,     // AB1805 wake reason was not ALARM
src/time/HibernateWakeDiagnostics.h:42:  kRtcRead,        // ab1805.getRtcAsTime() failed
src/time/RtcSkewTest.h:11: *          This header has zero Particle/AB1805 dependencies (no
src/time/RtcSkewTest.h:12: *          Particle.h, AB1805_RK.h, or other project headers - only
src/time/RtcSkewTest.h:107: *          `retained` RAM survives an MCU reset - soft reset, AB1805
src/time/RtcSkewTest.h:114: *          - that clears the latch along with the AB1805 RTC. This is
src/time/RtcSkewTest.h:133: *          Per Amendment A.3, the hook writes the AB1805 RTC only -
src/time/RtcSkewTest.h:134: *          `ab1805.setRtcFromTime()` - and does not also call
src/time/ClockDiagnostics.h:2:#include "AB1805_RK.h"
src/time/ClockDiagnostics.h:3:const char *ab1805WakeReasonName(AB1805::WakeReason reason);
src/time/HibernateEligibility.cpp:31:  if (!ab1805.readRegister(kRegIntMask, intMask)) {
src/time/HibernateEligibility.cpp:36:  if (!ab1805.writeRegister(kRegStatus, 0x00)) {
src/time/HibernateEligibility.cpp:41:  if (!ab1805.writeRegister(kRegIntMask, intMask)) {
src/time/HibernateEligibility.cpp:45:  if (!ab1805.maskRegister(
src/time/HibernateEligibility.cpp:46:        AB1805::REG_TIMER_CTRL,
src/time/HibernateEligibility.cpp:47:        (uint8_t)~AB1805::REG_TIMER_CTRL_RPT_MASK,
src/time/HibernateEligibility.cpp:48:        AB1805::REG_TIMER_CTRL_RPT_DIS)) {
src/time/Clock.cpp:172: *          lib/AB1805_RK/src/AB1805_RK.cpp:51-53 documents this exact hazard
src/time/Clock.cpp:178: *          could delay the watchdog refresh past the 60s app / 124s AB1805
src/time/Clock.cpp:282: *          gated by `AB1805::loop()`'s per-boot `timeSet` latch, and picks
src/time/Clock.cpp:292: *          ONLY after `ab1805.setRtcFromSystem()` returns true. Previously it
src/time/Clock.cpp:308: *          a PERMANENT AB1805/Wire fault would retry `setRtcFromSystem()`
src/time/Clock.cpp:341: *          the vendored `AB1805::loop()` (lib/AB1805_RK, untouched) has its
src/time/Clock.cpp:346: *          `ab1805.loop()` runs immediately before `checkClockResync()` in
src/time/Clock.cpp:353: *          `lib/AB1805_RK`, which is out of scope. "Exactly once" elsewhere
src/time/Clock.cpp:375:  // so a permanent AB1805/Wire fault cannot retry ab1805.setRtcFromSystem()
src/time/ExternalRtc.h:44:    Log.info("AB1805: enable r=%d", r);
src/time/ExternalRtc.h:50:      Log.warn("AB1805: stored config differs, re-bound r=%d", r);
src/time/HibernateCycle.h:19: *          `classifyWake()` also absorbs the PIN_RESET / AB1805-watchdog
src/time/HibernateCycle.h:22: *          same `AB1805::getWakeReason()` getter (safe to call more than
src/time/HibernateCycle.h:24: *          destructive `updateWakeReason()` call `AB1805::setup()` already
src/time/HibernateCycle.h:45:class AB1805; // full definition only needed in HibernateCycle.cpp
src/time/HibernateCycle.h:56:  // ---- PIN_RESET / AB1805-watchdog confirmation ----
src/time/HibernateCycle.h:101:WakeVerdict classifyWake(int osResetReason, AB1805 &rtc);
src/time/ClockDiagnostics.cpp:14:const char *ab1805WakeReasonNameLocal(AB1805::WakeReason reason) {
src/time/ClockDiagnostics.cpp:16:  case AB1805::WakeReason::WATCHDOG:
src/time/ClockDiagnostics.cpp:18:  case AB1805::WakeReason::DEEP_POWER_DOWN:
src/time/ClockDiagnostics.cpp:20:  case AB1805::WakeReason::COUNTDOWN_TIMER:
src/time/ClockDiagnostics.cpp:22:  case AB1805::WakeReason::ALARM:
src/time/ClockDiagnostics.cpp:24:  case AB1805::WakeReason::UNKNOWN:
src/time/ClockDiagnostics.cpp:31:const char *ab1805WakeReasonName(AB1805::WakeReason reason) {
src/time/ClockDiagnostics.cpp:32:  return ab1805WakeReasonNameLocal(reason);
src/time/ClockTrust.h:27: *          has no Particle/AB1805 dependency and compiles/tests on the host,
src/time/ClockTrust.h:61: *          (e.g. `AB1805::loop()`'s `millis() - lastWatchdogMillis`).
src/time/ClockTrust.h:158: * @param lastAttemptMs `millis()` at the time `ab1805.setRtcFromSystem()`
src/time/ClockTrust.h:219: *          that is `AB1805::setRtcFromSystem()`'s I2C result, which is
src/Generalized-Core-Counter.cpp:47:#include "AB1805_RK.h"               // hardware watchdog (S2) and power-off (S3)
src/Generalized-Core-Counter.cpp:117:void publishWatchdogForensics(bool ab1805Confirmed = false); // One-time watchdog forensic snapshot at boot
src/Generalized-Core-Counter.cpp:127:AB1805 ab1805(Wire);   // AB1805 RTC / Watchdog
src/Generalized-Core-Counter.cpp:296:  // v50: Device OS owns the AB1805; bind once before anything reads Time.
src/Generalized-Core-Counter.cpp:297:  if (!Am18x5.status().bound() && !PreBindRtc::run(ab1805, Wire)) Log.warn("PreBindRtc: chip access failed");
src/Generalized-Core-Counter.cpp:536:  // AB1805-confirmed PIN_RESET cases) is handled after AB1805 wake-reason
src/Generalized-Core-Counter.cpp:537:  // classification below, once the AB1805 wake reason is available -
src/Generalized-Core-Counter.cpp:748:  // ===== AB1805 WAKE-REASON CLASSIFICATION (HibernateCycle) =====
src/Generalized-Core-Counter.cpp:749:  // WO-2026-09-16 Step 2: PIN_RESET/AB1805-watchdog confirmation and the
src/Generalized-Core-Counter.cpp:750:  // hibernate wake-validation gate both used to read ab1805.getWakeReason()
src/Generalized-Core-Counter.cpp:754:  // updateWakeReason() call AB1805::setup() already made) - but there is no
src/Generalized-Core-Counter.cpp:758:  const HibernateCycle::WakeVerdict wakeVerdict = HibernateCycle::classifyWake(reason, ab1805);
src/Generalized-Core-Counter.cpp:760:  bool ab1805ConfirmedWatchdog = false;
src/Generalized-Core-Counter.cpp:766:      ab1805ConfirmedWatchdog = true;
src/Generalized-Core-Counter.cpp:767:      Log.warn("PIN_RESET confirmed as AB1805 watchdog reset");
src/Generalized-Core-Counter.cpp:769:      // Explicitly inconclusive - do NOT treat UNKNOWN as "not the AB1805".
src/Generalized-Core-Counter.cpp:770:      Log.info("PIN_RESET with AB1805 wake reason UNKNOWN - inconclusive, not ruling out AB1805 watchdog");
src/Generalized-Core-Counter.cpp:772:      Log.info("PIN_RESET with AB1805 wake reason=%s - not an AB1805 watchdog reset",
src/Generalized-Core-Counter.cpp:775:    startupAb1805ConfirmedWatchdog = ab1805ConfirmedWatchdog;
src/Generalized-Core-Counter.cpp:778:  ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS); // Enable watchdog
src/Generalized-Core-Counter.cpp:781:  // Moved here (from immediately after SystemConfig::setup(), before AB1805 init)
src/Generalized-Core-Counter.cpp:782:  // so it can act on the AB1805 classification above. The publish queue was
src/Generalized-Core-Counter.cpp:785:  // (reason == RESET_REASON_WATCHDOG) and the AB1805-confirmed PIN_RESET
src/Generalized-Core-Counter.cpp:789:  const bool watchdogClassified = watchdogResetDetected || ab1805ConfirmedWatchdog;
src/Generalized-Core-Counter.cpp:798:    RecoveryState::set_lastWatchdogSource(watchdogResetDetected ? RecoveryState::WATCHDOG_SOURCE_DEVICE_OS : RecoveryState::WATCHDOG_SOURCE_AB1805_PIN);
src/Generalized-Core-Counter.cpp:811:    publishWatchdogForensics(ab1805ConfirmedWatchdog);
src/Generalized-Core-Counter.cpp:1143:  ab1805.loop(); // Keeps the RTC synchronized with the device clock
README.md
SensorManagerBatteryAuthorityTests.cpp
alert19_clears_after_report_test.sh
awake_cycle_counters_test.sh
battery_authority_commit_sites_structural_test.py
battery_authority_seam_structural_test.py
battery_authority_structural_test.py
battery_health_test.cpp
battery_health_test.sh
battery_tier_guard_test.cpp
battery_tier_guard_test.sh
boron_soc_commit_test.py
build_flags_witness_test.sh
bump_version_scheme_test.sh
charge_inhibit_thermal_test.cpp
charge_inhibit_thermal_test.sh
clock_hibernate_helpers_move_structural_test.py
clock_openness_test.cpp
clock_openness_test.sh
clock_owner_structural_test.py
clock_resync_wiring_test.py
clock_rtc_writeback_test.cpp
clock_status_republish_test.cpp
clock_status_republish_test.sh
clock_trust_standard_structural_test.py
clock_trust_test.cpp
close_before_night_sleep_test.py
config_window_hold_test.cpp
config_window_hold_test.sh
conn_phase_modem_off_test.sh
connect_attempt_counter_test.sh
connect_no_modem_wait_structural_test.py
connected_online_last_connection_test.cpp
connected_online_last_connection_test.sh
connection_mode_downgrade_structural_test.py
connection_mode_downgrade_test.cpp
connection_mode_downgrade_test.sh
connection_signal_validity_test.sh
connectivity_failsafe_move_structural_test.py
connectivity_failsafe_open_hours_test.cpp
connectivity_failsafe_open_hours_test.sh
connectivity_failsafe_stage1_retired_test.py
connectivity_failsafe_unknown_openness_test.cpp
connectivity_failsafe_unknown_openness_test.sh
daily_cleanup_boundary_test.py
debounce_expiry_c1_test.sh
default_timezone_test.cpp
default_timezone_test.sh
deliberate_reset_flush_test.py
exerciser_baseline_test.py
external_rtc_structural_test.py
external_rtc_test.cpp
facade_header_isolation_test.py
failsafe_cadence_rule_test.cpp
failsafe_cadence_rule_test.sh
firmware_update_dwell_test.sh
firmware_update_wiring_structural_test.py
fixtures
hibernate_cycle_single_owner_structural_test.py
hibernate_wake_diagnostics_test.cpp
hibernate_wake_diagnostics_test.sh
hour_rules_test.cpp
hour_rules_test.sh
hourly_while_occupied_test.cpp
hourly_while_occupied_test.sh
idle_timediag_on_change_test.cpp
idle_timediag_on_change_test.sh
ledger_no_retry_test.cpp
ledger_no_retry_test.sh
loop_forensics_move_structural_test.py
loop_stage_sleep_prep_exclusion_test.py
modem_health_equivalence_test.cpp
modem_health_equivalence_test.sh
modem_health_owner_structural_test.py
modem_teardown_confirmation_logging_test.py
nightly_heap_guard_flush_test.sh
occupancy_ends_at_close_structural_test.py
occupancy_report_by_mode_test.cpp
occupancy_report_by_mode_test.sh
occupancy_session_restart_test.sh
occupancy_start_time_clamp_test.cpp
occupancy_start_time_clamp_test.sh
oom_immediate_reset_test.sh
persisted_layout_preservation_test.py
persistence_facade_behavior_test.cpp
persistence_facade_behavior_test.sh
persistence_facade_fanin_structural_test.py
pir_sensor_lifecycle_test.cpp
pir_sensor_lifecycle_test.sh
pir_sensor_reset_totals_test.cpp
pir_sensor_reset_totals_test.sh
pmic_fault_monitor_test.cpp
pmic_fault_monitor_test.sh
pmic_latch_call_site_structural_test.py
power_composition_test.cpp
power_composition_test.sh
power_constants_structural_test.py
power_constants_values_test.cpp
power_constants_values_test.sh
power_consumer_regression_test.sh
power_diagnostics_batch_test.cpp
power_diagnostics_batch_test.sh
power_diagnostics_log_guard_test.cpp
power_diagnostics_log_guard_test.sh
power_manager_accessors_test.cpp
power_source_override_test.cpp
power_source_override_test.sh
power_tier_test.cpp
power_tier_test.sh
pre_bind_rtc_test.cpp
pre_bind_rtc_test.py
publish_with_ack_structural_test.py
release_upload_job_test.py
report_payload_fields_test.py
report_payload_golden_test.cpp
report_payload_golden_test.sh
report_publisher_move_structural_test.py
reporting_interval_cap_test.cpp
reporting_interval_cap_test.sh
reporting_interval_store_seam_structural_test.py
reporting_policy_adapter_test.cpp
reporting_policy_adapter_test.sh
reporting_policy_clock_owner_structural_test.py
reporting_policy_test.cpp
reset_cause_structural_test.py
reset_wiring_test.cpp
reset_wiring_test.sh
retained_boot_structural_test.py
retained_boot_test.cpp
retained_boot_test.sh
rtc_skew_test.cpp
rtc_skew_test.sh
sensor_type_source_test.cpp
sensor_type_source_test.sh
sensor_type_validation_test.cpp
sensor_wake_source_test.cpp
sensor_wake_source_test.sh
serial_settle_test.py
sleep_breadcrumb_sequence_test.py
sleep_cloud_gate_equivalence_test.cpp
sleep_cloud_gate_equivalence_test.sh
sleep_cloud_gate_structural_test.py
sleep_config_leak_test.cpp
sleep_config_leak_test.sh
sleep_config_ownership_structural_test.py
sleep_duration_consumes_openness_test.py
sleep_execution_equivalence_test.cpp
sleep_execution_equivalence_test.sh
sleep_execution_structural_test.py
sleep_gate_modem_off_test.sh
2.1.0
6.3.4
6.4.1
6.6.0
<shellId: 1 completed with exit code 0>
```

</details>


---

<sub>19s</sub>

### `bash`

**Read HibernateCycle**

`$ cat src/time/HibernateCycle.cpp; sed -n 1,140p src/time/HibernateCycle.h`

<details>
<summary>213 lines</summary>

```
#include "Particle.h"
#include "time/HibernateCycle.h"

#include "AB1805_RK.h"

// Defined in Generalized-Core-Counter.cpp - not Boron/hibernate-specific
// (the AB1805 chip is used on every supported platform), so it stays
// defined in one place rather than duplicated here.
extern const char *ab1805WakeReasonName(AB1805::WakeReason reason);

namespace HibernateCycle {

namespace {

// These four retained fields lived in Generalized-Core-Counter.cpp before
// this step, keeping their original identifiers (not renamed - this is a
// move, not a rewrite) so a repo-wide search for retainedHibernate* still
// finds them, now confined to this one file.
// retainedHibernateWakeTime (unused by any reader anywhere in the tree -
// confirmed by a full-tree search during this step's baseline
// investigation) was not carried forward.
retained time_t retainedHibernateRtcBefore = 0;
retained uint32_t retainedHibernateRequestedSleep = 0;
retained uint32_t retainedHibernateCount = 0;
retained bool retainedHibernatePending = false;

} // namespace

void armForSleep(time_t rtcNow, uint32_t requestedSec) {
  retainedHibernateRtcBefore = rtcNow;
  retainedHibernateRequestedSleep = requestedSec;
  retainedHibernateCount++;
  retainedHibernatePending = true;
}

void abandon() {
  retainedHibernatePending = false;
}

uint32_t currentHibernateCount() {
  return retainedHibernateCount;
}

WakeVerdict classifyWake(int osResetReason, AB1805 &rtc) {
  WakeVerdict v{};

  // ===== PIN_RESET / AB1805 WATCHDOG CONFIRMATION =====
  // Device OS reports an external MCU reset (e.g. the carrier board's
  // AB1805 watchdog firing via the reset pin) as PIN_RESET, not WATCHDOG -
  // so the only way to confirm the AB1805 was the cause is to ask it
  // directly, via the same getWakeReason() getter the hibernate gate below
  // also uses.
  //
  // IMPORTANT: this relies on AB1805::getWakeReason() being a plain getter
  // (returns a cached member, see AB1805_RK.h) onto the result of the ONE
  // destructive updateWakeReason() call AB1805::setup() already made
  // earlier in setup() - updateWakeReason() itself clears the status bit
  // it classifies, so a second call to IT (not getWakeReason()) would
  // silently overwrite a correct WATCHDOG classification on a combined-bit
  // status read. Calling getWakeReason() here and again below for the
  // hibernate gate is safe and idempotent because neither call is
  // updateWakeReason() - both just read the one already-classified result.
  if (osResetReason == RESET_REASON_PIN_RESET) {
    v.pinResetChecked = true;
    const AB1805::WakeReason pinResetWakeReason = rtc.getWakeReason();
    v.pinResetWakeReasonName = ab1805WakeReasonName(pinResetWakeReason);

    if (pinResetWakeReason == AB1805::WakeReason::WATCHDOG) {
      v.pinResetConfirmedWatchdog = true;
    } else if (pinResetWakeReason == AB1805::WakeReason::UNKNOWN) {
      // Explicitly inconclusive - do NOT treat UNKNOWN as "not the AB1805".
      v.pinResetWakeReasonUnknown = true;
    }
  }

  // ===== RTC READ - every boot, independent of the above =====
  time_t rtcTime = 0;
  v.rtcReadOk = rtc.getRtcAsTime(rtcTime);
  v.rtcAtWake = (int64_t)rtcTime;

  // ===== HIBERNATE WAKE-VALIDATION GATE =====
  // This IS the classification now - not a call into a separately
  // maintained mirror. All six conditions below are evaluated directly,
  // in the same short-circuit order as before this step (WO-2026-09-14-002
  // Step 1 first consolidated a hand-written `if` chain and a mirrored
  // classifyGateArm() into one call site; this step consolidates further,
  // absorbing the classification itself so there is exactly one place this
  // logic is written, not a function elsewhere being wrapped).
  if (retainedHibernatePending) {
    v.hibernateGateEvaluated = true;
    const AB1805::WakeReason wakeReason = rtc.getWakeReason();
    v.wakeReasonName = ab1805WakeReasonName(wakeReason);

    HibernateWakeDiagnostics::GateInputs &in = v.gateInputs;
    in.resetReasonIsPowerManagement = (osResetReason == RESET_REASON_POWER_MANAGEMENT);
    in.rtcReadOk = v.rtcReadOk;
    in.rtcBefore = (int64_t)retainedHibernateRtcBefore;
    in.requestedSleepSec = retainedHibernateRequestedSleep;
    in.rtcAtWake = v.rtcAtWake;
    // WO-2026-10-04-001 item C: the AB1805 library labels every wake
    // DEEP_POWER_DOWN once SLST is set and never clears it, so accept that
    // label when the RTC shows the wake 0 to +60 s after the requested time.
    // An early wake (a button) or a late wake still fails the gate.
    const int64_t dpdExpectedWake = in.rtcBefore + (int64_t)in.requestedSleepSec;
    const bool dpdOnTime = in.rtcReadOk && in.rtcAtWake >= dpdExpectedWake && in.rtcAtWake <= dpdExpectedWake + 60;
    in.wakeReasonIsAlarm = (wakeReason == AB1805::WakeReason::ALARM) ||
                           (wakeReason == AB1805::WakeReason::DEEP_POWER_DOWN && dpdOnTime);

    using HibernateWakeDiagnostics::GateArm;
    if (!in.resetReasonIsPowerManagement) {
      v.gateArm = GateArm::kResetReason;
    } else if (!in.wakeReasonIsAlarm) {
      v.gateArm = GateArm::kWakeReason;
    } else if (!in.rtcReadOk) {
      v.gateArm = GateArm::kRtcRead;
    } else if (!(in.rtcBefore > 0)) {
      v.gateArm = GateArm::kRtcBeforeZero;
    } else if (!(in.requestedSleepSec > 0)) {
      v.gateArm = GateArm::kRequestedZero;
    } else if (!(in.rtcAtWake >= in.rtcBefore)) {
      v.gateArm = GateArm::kRtcOrder;
    } else {
      v.gateArm = GateArm::kNone;
    }

    v.requestedSleepSec = retainedHibernateRequestedSleep;
    v.hibernateCount = retainedHibernateCount;

    if (v.gateArm == GateArm::kNone) {
      v.actualSleepSec = (uint32_t)(rtcTime - retainedHibernateRtcBefore);
      v.sleepErrorSec = (int32_t)v.actualSleepSec - (int32_t)retainedHibernateRequestedSleep;
    }
  }

  return v;
}

} // namespace HibernateCycle
#pragma once

/**
 * @file HibernateCycle.h
 * @brief Single owner of the hibernate-sleep/wake retained-state lifecycle
 *        (WO-2026-09-16, Step 2 of the Structural Ownership Map roadmap).
 *
 * @details Before this step, five retained fields
 *          (`retainedHibernateRtcBefore`, `retainedHibernateWakeTime`,
 *          `retainedHibernateRequestedSleep`, `retainedHibernateCount`,
 *          `retainedHibernatePending`) were declared in
 *          `Generalized-Core-Counter.cpp`, armed by `State_Sleep.cpp`
 *          immediately before a HIBERNATE sleep, and read back by
 *          `Generalized-Core-Counter.cpp` on the wake boot - three files
 *          sharing one lifecycle with no single owner. This header/its
 *          `.cpp` now own all five (four - `retainedHibernateWakeTime` had
 *          no readers anywhere in the tree and was not carried forward).
 *
 *          `classifyWake()` also absorbs the PIN_RESET / AB1805-watchdog
 *          confirmation classification that used to live in `setup()`
 *          immediately before the hibernate gate: both consumers read the
 *          same `AB1805::getWakeReason()` getter (safe to call more than
 *          once - it is a plain accessor onto the result of the ONE
 *          destructive `updateWakeReason()` call `AB1805::setup()` already
 *          made; see `HibernateCycle.cpp` for the full citation), so one
 *          call now serves both instead of two independently-timed reads.
 *
 *          The former `HibernateWakeDiagnostics::classifyGateArm()` is not
 *          called from here - its six-condition decision is this file's own
 *          implementation now, not a separate function being wrapped.
 *          `HibernateWakeDiagnostics.h` still owns `GateInputs`/`GateArm`/
 *          `EventFields`/`buildEventFields()`/`buildEventPayload()` - the
 *          shared, Particle-free data and cloud-event-rendering types both
 *          this file and `Generalized-Core-Counter.cpp` use.
 */

#ifndef __HIBERNATE_CYCLE_H
#define __HIBERNATE_CYCLE_H

#include <cstdint>
#include <ctime>

#include "time/HibernateWakeDiagnostics.h"

class AB1805; // full definition only needed in HibernateCycle.cpp

namespace HibernateCycle {

/// Everything a wake boot needs to know, in one call. Populated in two
/// independent halves - PIN_RESET fields only when `osResetReason` was
/// `RESET_REASON_PIN_RESET`; hibernate-gate fields only when a hibernate
/// was actually pending this boot - a boot can populate neither, either,
/// but never both (a hibernate wake reports `RESET_REASON_POWER_MANAGEMENT`,
/// not `PIN_RESET`).
struct WakeVerdict {
  // ---- PIN_RESET / AB1805-watchdog confirmation ----
  bool pinResetChecked = false;
  bool pinResetConfirmedWatchdog = false;
  bool pinResetWakeReasonUnknown = false;
  const char *pinResetWakeReasonName = "N/A";

  // ---- RTC read outcome, populated every boot regardless of the above -
  // needed both for hibernate-gate classification and, independently, by
  // the caller's own "did system time just get restored" check. ----
  bool rtcReadOk = false;
  int64_t rtcAtWake = 0;

  // ---- Hibernate wake-validation gate, populated only when a hibernate
  // was pending this boot. `gateInputs` is exposed so the caller can still
  // build the `hibernate_wake` forensic event via
  // `HibernateWakeDiagnostics::buildEventFields()` without this file
  // needing to know anything about cloud events. ----
  bool hibernateGateEvaluated = false;
  HibernateWakeDiagnostics::GateArm gateArm = HibernateWakeDiagnostics::GateArm::kNone;
  HibernateWakeDiagnostics::GateInputs gateInputs{};
  const char *wakeReasonName = "UNKNOWN";
  uint32_t requestedSleepSec = 0;
  uint32_t hibernateCount = 0;
  uint32_t actualSleepSec = 0;
  int32_t sleepErrorSec = 0;
};

/// Arms the retained state for an upcoming HIBERNATE sleep. Called
/// immediately before `System.sleep(config)` with a HIBERNATE
/// configuration. Replaces the inline
/// `retainedHibernateRtcBefore/RequestedSleep/Count++/Pending` sequence
/// formerly in `State_Sleep.cpp`.
void armForSleep(time_t rtcNow, uint32_t requestedSec);

/// Un-arms a hibernate attempt without processing a wake: either the
/// HIBERNATE `System.sleep()` call returned instead of resetting the MCU
/// (the ULTRA_LOW_POWER fallback path), or the wake boot has finished
/// processing `classifyWake()`'s verdict and the pending flag should not
/// carry into the next cycle. Replaces both
/// `retainedHibernatePending = false;` sites.
void abandon();

/// Classifies this boot's wake in one call - see the struct doc above.
/// `osResetReason` is the OS reset reason (`System.resetReason()`'s
/// result, e.g. `RESET_REASON_PIN_RESET`/`RESET_REASON_POWER_MANAGEMENT`).
WakeVerdict classifyWake(int osResetReason, AB1805 &rtc);

/// Current hibernate count, for the one confirmed consumer outside the
/// wake-classification path: `publishStartupStatus()`'s `status` event.
uint32_t currentHibernateCount();

} // namespace HibernateCycle

#endif
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>19s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/src/state/State\_Sleep.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/src/state/State_Sleep.cpp (lines 640-940)`

<details>
<summary>276 lines</summary>

```
      Log.warn("Nightly heap guard: freeHeap=%lu <= %lu before overnight ULTRA_LOW_POWER fallback - resetting for clean next-day start",
               freeHeap,
               ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES);
      // Reset drops RAM - flush the diagnostics batch now, since this is
      // the only chance for this cycle's accumulated entries.
#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
      PowerDiagnostics::flushDiagBatch();
#endif
      flushAllBeforeReset();
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);
      return true;
    }

    if (freeHeap <= ConnectivityPolicy::NIGHTLY_HEAP_WARN_THRESHOLD_BYTES) {
      Log.warn("Nightly heap guard: freeHeap=%lu <= %lu before overnight ULTRA_LOW_POWER fallback - monitoring only",
               freeHeap,
               ConnectivityPolicy::NIGHTLY_HEAP_WARN_THRESHOLD_BYTES);
    }
  }
  return false;
}

struct WakeContext { bool pirWake, buttonWake, timerWake; const char *wakeReasonLabel; };
struct { bool preserveAfterNormalReturn = false; bool forceModemOff = false; int error = SYSTEM_ERROR_NONE; } standbyReturn;

bool effectiveStandby(bool useNetworkStandbyRequested) {
  bool preserveStandbyForThisPass = false;
#if HAL_PLATFORM_CELLULAR
  if (standbyReturn.preserveAfterNormalReturn &&
      (!useNetworkStandbyRequested || session.modemUnstable || Cellular.isOff())) {
    standbyReturn.preserveAfterNormalReturn = false;
  }
  preserveStandbyForThisPass = standbyReturn.preserveAfterNormalReturn;
#endif
  bool useNetworkStandbyEffective =
      useNetworkStandbyRequested &&
      (preserveStandbyForThisPass ||
       Observability::cycleStats().connect_result ==
           Observability::WakeCycleStats::ConnectResult::SUCCESS);
#if HAL_PLATFORM_CELLULAR
  useNetworkStandbyEffective = useNetworkStandbyEffective && !Cellular.isOff();
  if (session.modemUnstable || standbyReturn.forceModemOff) {
    useNetworkStandbyEffective = false;
  }
#else
  useNetworkStandbyEffective = false;
#endif
  return useNetworkStandbyEffective;
}

void hibernateAttempt(bool overnightFallbackSleep, int wakeInSeconds) {
#if PLATFORM_ID == PLATFORM_BORON
  if (overnightFallbackSleep) {
    time_t rtcNow = 0;
    if (shouldUseBoronRtcAlarmHibernate((uint32_t)wakeInSeconds, rtcNow)) {
      pinMode(WAKEUP_PIN, INPUT_PULLUP);
      if (digitalRead(WAKEUP_PIN) == HIGH) {
        if (clearAb1805StaleAlarmInterrupts()) {
          const time_t wakeTime = rtcNow + (time_t)wakeInSeconds;
          if (ab1805.interruptAtTime(wakeTime)) {
            setAppBreadcrumb(24); // BREADCRUMB_SLEEP_CONFIG_START (was stale literal 20, colliding with BREADCRUMB_REPORT_EXIT)
            // Fresh local configuration per sleep: Device OS move-assigns
            // without freeing the previous wake-source list, so a reused
            // long-lived object orphans that list on every cycle.
            SystemSleepConfiguration hibernateConfig;
            hibernateConfig.mode(SystemSleepMode::HIBERNATE)
              .gpio(WAKEUP_PIN, FALLING)
              .gpio(BUTTON_PIN, FALLING);

            const bool watchdogStopped = ab1805.stopWDT();
            pauseAwakeWatchdogForSleep("hibernate");
            if (!watchdogStopped) {
              Log.warn("SleepCall: watchdog-stop failed mode=HIBERNATE dur=%ds", wakeInSeconds);
            }

            // WO-2026-09-16 Step 2: HibernateCycle now owns the retained
            // hibernate-cycle fields; wakeTime itself is used above for
            // ab1805.interruptAtTime() only - nothing reads it back on
            // wake (confirmed dead as a retained field), so it is not
            // passed through here.
            HibernateCycle::armForSleep(rtcNow, (uint32_t)wakeInSeconds);

            Log.info("ModemTeardown: radioOn=%d point=hibernate", (int)Connectivity::isRadioPoweredOn());
            Log.info("Sleep: HIBERNATE reason=closed dur=%ds wakePin=%d", wakeInSeconds, (int)WAKEUP_PIN);
            PowerDiagnostics::logPowerState("pre-hibernate");
            thrashGuard.markProgress("SLEEP_ATTEMPT");
            Cloud::instance().logLedgerSleepState();
            setAppBreadcrumb(25); // BREADCRUMB_SLEEP_SYSTEM_CALL (was stale literal 21, colliding with BREADCRUMB_IDLE_ENTRY) - entering final pre-sleep sequence
            drainSerialBeforeSleep();
            setAppBreadcrumb(26); // BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE
            // HIBERNATE may reset the MCU without returning - flush the
            // diagnostics batch now, since this is the only chance.
#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
            PowerDiagnostics::flushDiagBatch();
            setAppBreadcrumb(27); // BREADCRUMB_SLEEP_DIAG_FLUSH_DONE
#endif
            setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
            const SystemSleepResult hibernateResult = System.sleep(hibernateConfig);
            AwakeCycles::recordSleepReturn(hibernateResult.error() == SYSTEM_ERROR_NONE);

            // HIBERNATE should reset the MCU. If we return here, treat it as failed and fall back.
            HibernateCycle::abandon();
            ab1805.resumeWDT();
            restoreAwakeWatchdogAfterWake("hibernate");
            Log.warn("HIBERNATE sleep failed err=%d - falling back to ULTRA_LOW_POWER",
                     (int)hibernateResult.error());
          } else {
            Log.warn("HIBERNATE bypass: failed to program AB1805 alarm - falling back to ULTRA_LOW_POWER");
          }
        } else {
          Log.warn("HIBERNATE bypass: failed stale interrupt cleanup - falling back to ULTRA_LOW_POWER");
        }
      } else {
        Log.warn("HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER");
      }
    }
  }
#endif
}

// Returns true when the caller must return (all sleep attempts failed).
bool sleepAndResume(int wakeInSeconds, bool useNetworkStandby, const char *sleepReason, WakeContext &wake) {
  const SensorWakeSource sensorWake = SensorManager::instance().activeSensor()->wakeSource();
  const bool sensorCanWake = (sensorWake.pin != PIN_INVALID);
  SystemSleepConfiguration ulpConfig;
  ulpConfig.mode(SystemSleepMode::ULTRA_LOW_POWER)
    .gpio(BUTTON_PIN, FALLING);    // Service button wake (active-low)
  if (sensorCanWake) ulpConfig.gpio(sensorWake.pin, sensorWake.edge);  // Sensor wake
    
  // Add network standby for occupancy mode to avoid reconnection delays
  // Note: Network standby is only supported/needed on cellular devices to prevent
  // carrier blacklisting. WiFi reconnection is already fast enough (~2-5s).
  if (useNetworkStandby) {
#if HAL_PLATFORM_CELLULAR
    // Keep cellular modem in low-power standby so we can quickly reconnect
    // when occupancy changes. This prevents 30-60s reconnection overhead and
    // potential carrier blacklisting from rapid disconnect/reconnect cycles.
    ulpConfig.network(NETWORK_INTERFACE_CELLULAR, SystemSleepNetworkFlag::INACTIVE_STANDBY);
#endif
  }
  
  ulpConfig.duration((uint32_t)wakeInSeconds * 1000UL);  // Timer-based wake at reporting boundary

  // Stop watchdog immediately before sleep (after config is fully built)
  // to minimize time between I2C transaction and sleep entry
  const bool watchdogStopped = ab1805.stopWDT();
  pauseAwakeWatchdogForSleep("ulp");
#if ENABLE_SLEEP_TRACE
  Log.info("SleepCall: standby=%d dur=%d wdt=%s gate=ok",
           useNetworkStandby ? 1 : 0,
           wakeInSeconds,
           watchdogStopped ? "off" : "err");
#else
  if (!watchdogStopped) {
    Log.warn("SleepCall: watchdog-stop failed standby=%d dur=%d",
             useNetworkStandby ? 1 : 0,
             wakeInSeconds);
  }
#endif
#if CONNECTIVITY_FAILSAFE_TEST_MODE
  Log.info("Sleep: ULP standby=%d reason=%s dur=%ds occ=%d soc=%.1f test=1",
           useNetworkStandby ? 1 : 0,
           sleepReason,
           wakeInSeconds,
           CurrentReadings::get_occupied() ? 1 : 0,
           (double)PowerManager::instance().soc());
#else
  Log.info("Sleep: ULP standby=%d reason=%s dur=%ds occ=%d soc=%.1f",
           useNetworkStandby ? 1 : 0,
           sleepReason,
           wakeInSeconds,
           CurrentReadings::get_occupied() ? 1 : 0,
           (double)PowerManager::instance().soc());
#endif

  thrashGuard.markProgress("SLEEP_ATTEMPT");
  PowerDiagnostics::logPowerState("pre-sleep-ulp");
  const unsigned long sleepCallStartMs = millis();
  Cloud::instance().logLedgerSleepState();
  setAppBreadcrumb(25); // BREADCRUMB_SLEEP_SYSTEM_CALL (was stale literal 21, colliding with BREADCRUMB_IDLE_ENTRY) - entering final pre-sleep sequence
  drainSerialBeforeSleep();
  setAppBreadcrumb(26); // BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE
  setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
  SystemSleepResult result = System.sleep(ulpConfig);
  AwakeCycles::recordSleepReturn(result.error() == SYSTEM_ERROR_NONE);
  const unsigned long sleepElapsedMs = millis() - sleepCallStartMs;
  pin_t wakePin = result.wakeupPin();
  const char *wakeReturnReason = (result.error() != SYSTEM_ERROR_NONE) ? "ERROR" :
      ((wakePin == BUTTON_PIN) ? "BUTTON" : ((wakePin == sensorWake.pin) ? "PIR" : "TIMER"));

#if HAL_PLATFORM_CELLULAR
  if (useNetworkStandby) {
    if (result.error() == SYSTEM_ERROR_NONE) {
      standbyReturn.preserveAfterNormalReturn = true;
      standbyReturn.forceModemOff = false;
      standbyReturn.error = SYSTEM_ERROR_NONE;
      if (SystemConfig::get_verboseMode()) {
        Log.info("SLEEP: standby sleep returned normally; preserving modem standby");
      }
    } else {
      standbyReturn.preserveAfterNormalReturn = false;
      standbyReturn.forceModemOff = true;
      standbyReturn.error = (int)result.error();
      Log.warn("SLEEP: standby sleep failed; forcing modem-off reason=sleep_error err=%d",
               standbyReturn.error);
    }
  } else {
    standbyReturn.preserveAfterNormalReturn = false;
    standbyReturn.forceModemOff = false;
    standbyReturn.error = SYSTEM_ERROR_NONE;
  }
#endif

  // If Device OS rejects the sleep configuration, System.sleep() returns
  // immediately and the wake pin will be invalid (65535). Without handling
  // that error, the state machine will interpret it as a timer wake and can
  // thrash in a tight loop.
  if (result.error() != SYSTEM_ERROR_NONE) {
    thrashGuard.markProgress("SLEEP_RETURNED");
    Log.error("ULTRA_LOW_POWER sleep failed err=%d (wakeIn=%d sec, button=%d pir=%d) - falling back to STOP", (int)result.error(), wakeInSeconds, (int)BUTTON_PIN, (int)sensorWake.pin);
    RecoveryState::raiseAlert(16);

   // STOP generally supports a wider set of wake pins on some platforms.
    setAppBreadcrumb(24); // BREADCRUMB_SLEEP_CONFIG_START (was stale literal 20, colliding with BREADCRUMB_REPORT_EXIT)
    SystemSleepConfiguration stopConfig;
    stopConfig.mode(SystemSleepMode::STOP)
      .gpio(BUTTON_PIN, FALLING);
    if (sensorCanWake) stopConfig.gpio(sensorWake.pin, sensorWake.edge);
    stopConfig.duration((uint32_t)wakeInSeconds * 1000UL);
    PowerDiagnostics::logPowerState("pre-sleep-stop-fallback");
    Cloud::instance().logLedgerSleepState();
    setAppBreadcrumb(25); // BREADCRUMB_SLEEP_SYSTEM_CALL (was stale literal 21, colliding with BREADCRUMB_IDLE_ENTRY) - entering final pre-sleep sequence
    drainSerialBeforeSleep();
    setAppBreadcrumb(26); // BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE
    setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
    result = System.sleep(stopConfig);
    AwakeCycles::recordSleepReturn(result.error() == SYSTEM_ERROR_NONE);

    if (result.error() != SYSTEM_ERROR_NONE) {
      Log.error("STOP sleep fallback failed err=%d (wakeIn=%d sec) - using timer-only STOP sleep", (int)result.error(), wakeInSeconds);
      setAppBreadcrumb(24); // BREADCRUMB_SLEEP_CONFIG_START
      SystemSleepConfiguration stopTimerConfig;
      stopTimerConfig.mode(SystemSleepMode::STOP)
        .duration((uint32_t)wakeInSeconds * 1000UL);
      PowerDiagnostics::logPowerState("pre-sleep-stop-timer-only");
      Cloud::instance().logLedgerSleepState();
      setAppBreadcrumb(25); // BREADCRUMB_SLEEP_SYSTEM_CALL - entering final pre-sleep sequence
      drainSerialBeforeSleep();
      setAppBreadcrumb(26); // BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE
      setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
      result = System.sleep(stopTimerConfig);
      AwakeCycles::recordSleepReturn(result.error() == SYSTEM_ERROR_NONE);

      if (result.error() != SYSTEM_ERROR_NONE) {
        // All sleep modes failed - this indicates a device state corruption
        // that requires immediate reset (alert 16 already raised on first failure).
        Log.info("ModemTeardown: radioOn=%d point=sleep-attempts-failed-reset", (int)Connectivity::isRadioPoweredOn());
        Log.error("All sleep attempts failed err=%d - immediate reset required", (int)result.error());
        ab1805.resumeWDT();

        // Brief delay to allow log output to flush before reset
        delay(2000);

        // Reset device to clear corrupted state
        Log.info("Resetting device to clear sleep failure state");
        // Reset drops RAM - flush the diagnostics batch now, since this is
        // the only chance for this cycle's cascade of sleep-attempt entries.
#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
        PowerDiagnostics::flushDiagBatch();
#endif
        flushAllBeforeReset();
        System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);
        
        // Should never reach here, but set ERROR_STATE as fallback
        transitionTo(ERROR_STATE, "sleep-all-attempts-failed");
        return true;
      }
    }
  }
  
  // Clear any pending interrupts on wake pins after sleep to ensure clean state
  // This is critical for proper interrupt handling after wake
  pinMode(BUTTON_PIN, INPUT);    // Reset pin mode
  if (sensorCanWake) pinMode(sensorWake.pin, INPUT);  // Reset pin mode
  
  // Resume hardware watchdog immediately after wake
  ab1805.resumeWDT();
  restoreAwakeWatchdogAfterWake("ulp");

  // Mark progress immediately after wake to reset ThrashGuard timer
  thrashGuard.markProgress("WAKE_FROM_SLEEP");
  setAppBreadcrumb(4);

  // Single convergence point for the ULP/STOP-fallback/STOP-timer-only sleep
  // cascade - whichever attempt actually succeeded lands here, so this flushes
  // the whole cycle's diagnostics batch exactly once regardless of how many
  // fallback attempts fired. (Hibernate and the total-sleep-failure reset are
  // flushed separately at their own non-returning call sites, above.)
#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
  PowerDiagnostics::flushDiagBatch();
#endif
```

</details>


---

<sub>28s</sub>

### `bash`

**Find ExternalWatchdog in Device OS**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; ls | head -30; grep -rln "ExternalWatchdog" --include=*.h --include=*.cpp . 2>/dev/null | head -20`

<details>
<summary>34 lines</summary>

```
CHANGELOG.md
CONTRIBUTING.md
Doxyfile
ISSUE_TEMPLATE.md
LICENSE
PULL_REQUEST_TEMPLATE.md
README.md
STYLE_GUIDE.md
bootloader
build
ci
communication
communication-dynalib
crypto
crypto-dynalib
docs
dynalib
gsm0710muxer
hal
hal-dynalib
main
makefile
modules
newlib_nano
platform
proto_defs
rt-dynalib
scripts
services
services-dynalib
./user/tests/wiring/watchdog/watchdog.cpp
./user/tests/wiring/api/watchdog.cpp
./wiring/inc/spark_wiring_watchdog.h
<shellId: 3 completed with exit code 0>
```

</details>


---

<sub>31s</sub>

### `bash`

**Read watchdog wiring header**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; grep -n "" wiring/inc/spark_wiring_watchdog.h | sed -n 1,400p`

<details>
<summary>362 lines</summary>

```
1:/*
2: * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
3: *
4: * This library is free software; you can redistribute it and/or
5: * modify it under the terms of the GNU Lesser General Public
6: * License as published by the Free Software Foundation, either
7: * version 3 of the License, or (at your option) any later version.
8: *
9: * This library is distributed in the hope that it will be useful,
10: * but WITHOUT ANY WARRANTY; without even the implied warranty of
11: * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
12: * Lesser General Public License for more details.
13: *
14: * You should have received a copy of the GNU Lesser General Public
15: * License along with this library; if not, see <http://www.gnu.org/licenses/>.
16: */
17:
18:#pragma once
19:
20:#include <chrono>
21:
22:#include "spark_wiring_thread.h"
23:#include "spark_wiring_platform.h"
24:#include "delay_hal.h"
25:#include "timer_hal.h"
26:
27:#if PLATFORM_THREADING
28:
29:class ApplicationWatchdog
30:{
31:	volatile system_tick_t timeout;
32:	static volatile system_tick_t last_checkin;
33:
34:	std::function<void(void)> timeout_fn;
35:
36:	Thread* thread;
37:
38:	static void start(void* pointer);
39:
40:	void loop();
41:
42:public:
43:    static const unsigned DEFAULT_STACK_SIZE = 512;
44:
45:	ApplicationWatchdog(unsigned timeout_ms, std::function<void(void)> fn, unsigned stack_size=DEFAULT_STACK_SIZE) :
46:		timeout(timeout_ms),
47:		timeout_fn(fn)
48:	{
49:		checkin();
50:		thread = new Thread("appwdt", start, this, OS_THREAD_PRIORITY_CRITICAL, stack_size);
51:	}
52:	ApplicationWatchdog(std::chrono::milliseconds ms, std::function<void(void)> fn, unsigned stack_size=DEFAULT_STACK_SIZE) : ApplicationWatchdog(ms.count(), fn, stack_size) {}
53:
54:    // This constuctor helps to resolve overloaded function types, such as System.reset(), which is not always
55:    // possible in case of std::function
56:    ApplicationWatchdog(unsigned timeout_ms, void (*fn)(), unsigned stack_size=DEFAULT_STACK_SIZE) :
57:        ApplicationWatchdog(timeout_ms, std::function<void()>(fn), stack_size)
58:    {
59:    }
60:    ApplicationWatchdog(std::chrono::milliseconds ms, void (*fn)(), unsigned stack_size=DEFAULT_STACK_SIZE) : ApplicationWatchdog(ms.count(), fn, stack_size) {}
61:
62:	~ApplicationWatchdog() {
63:		dispose();
64:		if (thread) {
65:			// NOTE: this may have to wait up to original timeout for the thread to exit
66:			delete thread;
67:		}
68:	}
69:
70:	bool isComplete()
71:	{
72:		return !thread || !thread->isRunning();
73:	}
74:
75:	static inline system_tick_t current_time()
76:	{
77:		return HAL_Timer_Get_Milli_Seconds();
78:	}
79:
80:	bool has_timedout()
81:	{
82:		return (current_time()-last_checkin)>=timeout;
83:	}
84:
85:	/**
86:	 * Dispose of this thread next time it wakes up.
87:	 */
88:	void dispose()
89:	{
90:		timeout = 0;
91:	}
92:
93:	/**
94:	 * Lifesign that the application is still working normally.
95:	 */
96:	static void checkin()
97:	{
98:		last_checkin = current_time();
99:	}
100:
101:};
102:
103:inline void application_checkin() { ApplicationWatchdog::checkin(); }
104:
105:#else
106:
107:inline void application_checkin() {  }
108:
109:#endif // PLATFORM_THREADING
110:
111:
112:#if Wiring_Watchdog
113:
114:#include "watchdog_hal.h"
115:#include "enumflags.h"
116:#include "enumclass.h"
117:
118:namespace particle {
119:
120:enum class WatchdogCap : uint32_t {
121:    NONE            = HAL_WATCHDOG_CAPS_NONE,
122:    RESET           = HAL_WATCHDOG_CAPS_RESET,              /** The watchdog can be configured to not hard resetting device on expired. */
123:    NOTIFY          = HAL_WATCHDOG_CAPS_NOTIFY,             /** The watchdog can generate an interrupt on expired. */
124:    NOTIFY_ONLY     = HAL_WATCHDOG_CAPS_NOTIFY_ONLY,        /** The watchdog can generate an interrupt on expired without resetting the device. */
125:    RECONFIGURABLE  = HAL_WATCHDOG_CAPS_RECONFIGURABLE,     /** The watchdog can be re-configured after started. */
126:    STOPPABLE       = HAL_WATCHDOG_CAPS_STOPPABLE,          /** The watchdog can be stopped after started. */
127:    SLEEP_RUNNING   = HAL_WATCHDOG_CAPS_SLEEP_RUNNING,       /** The watchdog will be paused in sleep mode. */
128:    DEBUG_RUNNING   = HAL_WATCHDOG_CAPS_DEBUG_RUNNING,       /** The watchdog will be paused in debug mode. */
129:    ALL             = HAL_WATCHDOG_CAPS_ALL
130:};
131:ENABLE_ENUM_CLASS_BITWISE(WatchdogCap);
132:
133:using WatchdogCaps = EnumFlags<WatchdogCap>;
134:
135:enum class WatchdogState : uint8_t {
136:    DISABLED = HAL_WATCHDOG_STATE_DISABLED,
137:    CONFIGURED = HAL_WATCHDOG_STATE_CONFIGURED,
138:    STARTED = HAL_WATCHDOG_STATE_STARTED,
139:    SUSPENDED = HAL_WATCHDOG_STATE_SUSPENDED,
140:    STOPPED = HAL_WATCHDOG_STATE_STOPPED,
141:};
142:
143:using WatchdogOnExpiredStdFunction = std::function<void(void)>;
144:typedef void (*WatchdogOnExpiredCallback)(void* context);
145:
146:class WatchdogConfiguration {
147:public:
148:    WatchdogConfiguration()
149:            : config_() {
150:        config_.size = sizeof(hal_watchdog_config_t);
151:        config_.version = HAL_WATCHDOG_VERSION;
152:        config_.enable_caps = HAL_WATCHDOG_CAPS_RESET | HAL_WATCHDOG_CAPS_SLEEP_RUNNING;
153:    }
154:
155:    WatchdogConfiguration(hal_watchdog_config_t conf)
156:            : config_(conf) {
157:    }
158:
159:    ~WatchdogConfiguration() = default;
160:
161:    WatchdogConfiguration& timeout(system_tick_t ms) {
162:        config_.timeout_ms = ms;
163:        return *this;
164:    }
165:
166:    system_tick_t timeout() const {
167:        return config_.timeout_ms;
168:    }
169:
170:    WatchdogConfiguration& timeout(std::chrono::milliseconds ms) {
171:        return timeout(ms.count());
172:    }
173:
174:    WatchdogConfiguration& capabilities(WatchdogCaps caps) {
175:        config_.enable_caps = caps.value();
176:        return *this;
177:    }
178:
179:    WatchdogCaps capabilities() const {
180:        return WatchdogCaps::fromUnderlying(config_.enable_caps);
181:    }
182:
183:    const hal_watchdog_config_t* halConfig() const {
184:        return &config_;
185:    }
186:
187:private:
188:    hal_watchdog_config_t config_;
189:
190:    static constexpr uint32_t WATCHDOG_DEFAULT_TIMEOUT_MS = 5000;
191:};
192:
193:class WatchdogInfo : public hal_watchdog_info_t {
194:public:
195:    WatchdogInfo()
196:            : info_{} {
197:        info_.size = sizeof(hal_watchdog_info_t);
198:        info_.version = HAL_WATCHDOG_VERSION;
199:    }
200:
201:    WatchdogInfo(hal_watchdog_info_t info)
202:            : info_(info) {
203:    }
204:
205:    WatchdogCaps mandatoryCapabilities() const {
206:        return WatchdogCaps::fromUnderlying(info_.mandatory_caps);
207:    }
208:
209:    WatchdogCaps capabilities() const {
210:        return WatchdogCaps::fromUnderlying(info_.optional_caps);
211:    }
212:
213:    WatchdogConfiguration configuration() const {
214:        return WatchdogConfiguration(info_.config);
215:    }
216:
217:    system_tick_t minTimeout() const {
218:        return info_.min_timeout_ms;
219:    }
220:
221:    system_tick_t maxTimeout() const {
222:        return info_.max_timeout_ms;
223:    }
224:
225:    WatchdogState state() const {
226:        return WatchdogState(info_.state);
227:    }
228:
229:    hal_watchdog_info_t* halInfo() {
230:        return &info_;
231:    }
232:private:
233:    hal_watchdog_info_t info_;
234:};
235:
236:/**
237: * @note For nRF52-based platforms,
238: *  - the watchdog will be paused during hibernate mode.
239: *  - When device wakes up from the hibernate mode, the watchdog won't restart automatically.
240: *  - When device wakes up from the ultra-low power mode or stop mode, the watchdog will
241: *    continue running if the watchdog has started previously. Use Watchdog.started()
242: *    to check the status of watchdog.
243: *  - System.reset() will reset the watchdog.
244: * 
245: * For RTL872x-based platforms,
246: *  - Either interrupt mode or reset mode can be enabled at the same time.
247: *  - The watchdog keeps running when expires when the watchdog is configured as interrupt mode, unless it is stopped explicitly.
248: *  - When device wakes up from the hibernate mode, the watchdog won't restart automatically.
249: *  - When device wakes up from the ultra-low power mode or stop mode, the watchdog will continue running if
250: *    the watchdog has started previously. Use Watchdog.started() to check the status of watchdog.
251: *  - System.reset() will reset the watchdog.
252: */
253:class WatchdogClass {
254:public:
255:    /**
256:     * @brief Initialize the hardware watchdog so that it is ready to be started.
257:     * 
258:     * @note Some hardware watchdog don't support re-configuring once started. @ref WatchdogCaps::RECONFIGURABLE
259:     * 
260:     * @param[in] config The watchdog configuration @ref WatchdogConfiguration
261:     * @return SYSTEM_ERROR_NONE if succeeded, otherwise failed.
262:     */
263:    int init(const WatchdogConfiguration& config);
264:
265:    /**
266:     * @brief Start the hardware watchdog. Only if the watchdog is initialized, it can be started. When the
267:     * watchdog is running, calling this method makes no effect, i.e. it won't refresh the watchdog.
268:     * 
269:     * @return SYSTEM_ERROR_NONE if succeeded, otherwise failed.
270:     */
271:    int start();
272:
273:    /**
274:     * @brief Check if the watchdog is started.
275:     * 
276:     * @return true if started, otherwise false.
277:     */
278:    bool started();
279:
280:    /**
281:     * @brief Stop the hardware watchdog.
282:     * 
283:     * @note Some hardware watchdog don't support stopping once started. @ref WatchdogCaps::HAL_WATCHDOG_CAPS_STOPPABLE
284:     * 
285:     * @return SYSTEM_ERROR_NONE if succeeded, otherwise failed.
286:     */
287:    int stop();
288:
289:    /**
290:     * @brief Refresh the hardware watchdog counter, as like it is re-started. The watchdog should be refreshed
291:     * before it expires, otherwise, a callback is invoked (if supported) followed by hard resetting the device.
292:     * 
293:     * @return SYSTEM_ERROR_NONE if succeeded, otherwise failed.
294:     */
295:    int refresh();
296:
297:    /**
298:     * @brief Get the hardware watchdog infomations. @ref WatchdogInfo
299:     * 
300:     * @param[in,out] info A left-value reference for storing the retrieved watchdog informations.
301:     * @return SYSTEM_ERROR_NONE if succeeded, otherwise failed.
302:     */
303:    int getInfo(WatchdogInfo& info);
304:
305:    /**
306:     * @brief Set the callback to be invoked when watchdog expires.
307:     * 
308:     * @note Some hardware watchdog don't support callback when expired. @ref WatchdogCaps::INT
309:     * 
310:     * @param[in] callback The callback function to be set.
311:     * @param[in] context The context to be propagated when the callback is invoked.
312:     * @return SYSTEM_ERROR_NONE if succeeded, otherwise failed.
313:     */
314:    int onExpired(WatchdogOnExpiredCallback callback, void* context = nullptr);
315:
316:    int onExpired(const WatchdogOnExpiredStdFunction& callback);
317:    
318:    template<typename T>
319:    int onExpired(void(T::*callback)(void), T* instance) {
320:        return onExpired((callback && instance) ? std::bind(callback, instance) : (WatchdogOnExpiredStdFunction)nullptr);
321:    }
322:
323:    /**
324:     * @brief Get the hardware watchdog instance.
325:     * 
326:     * @return The hardware watchdog singlton.
327:     */
328:    static WatchdogClass& getInstance(hal_watchdog_instance_t instance = HAL_WATCHDOG_INSTANCE1) {
329:        static WatchdogClass watchdogs[] = {
330:            WatchdogClass(HAL_WATCHDOG_INSTANCE1),
331:            WatchdogClass(HAL_WATCHDOG_INSTANCE2)
332:        };
333:        return watchdogs[instance];
334:    }
335:
336:private:
337:    WatchdogClass() = delete;
338:    explicit WatchdogClass(hal_watchdog_instance_t instance)
339:            : callback_(nullptr),
340:              instance_(instance) {
341:    }
342:    ~WatchdogClass() = default;
343:
344:    static void onWatchdogExpiredCallback(void* context) {
345:        auto instance = reinterpret_cast<WatchdogClass*>(context);
346:        if (instance->callback_) {
347:            instance->callback_();
348:        }
349:    }
350:
351:    WatchdogOnExpiredStdFunction callback_;
352:    hal_watchdog_instance_t instance_;
353:};
354:
355:#define Watchdog WatchdogClass::getInstance(HAL_WATCHDOG_INSTANCE1)
356:#define ExternalWatchdog WatchdogClass::getInstance(HAL_WATCHDOG_INSTANCE2)
357:
358:} // namespace particle
359:
360:#endif // Wiring_Watchdog
361:
<shellId: 4 completed with exit code 0>
```

</details>


---

<sub>33s</sub>

### Copilot

Now HAL for the external watchdog and sleep.

---

<sub>34s</sub>

### `bash`

**Find external watchdog implementation**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; grep -rln "INSTANCE2\|hal_watchdog_.*external\|ExternalWatchdog\|ExternalRtc\|Am18x5" hal services system platform wiring --include=*.cpp --include=*.h | head -30`

<details>
<summary>9 lines</summary>

```
hal/inc/watchdog_hal.h
hal/shared/am18x5.cpp
hal/shared/am18x5.h
hal/shared/watchdog_hal.cpp
hal/shared/exrtc_hal.cpp
hal/src/nRF52840/exrtc_hal_deprecated.cpp
wiring/inc/spark_wiring_exrtc.h
wiring/inc/spark_wiring_watchdog.h
<shellId: 5 completed with exit code 0>
```

</details>


---

<sub>36s</sub>

### `bash`

**Read watchdog HAL and am18x5 watchdog parts**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0/hal/shared; cat watchdog_hal.cpp | sed -n 1,260p; grep -n "atchdog\|WDT\|wdt" am18x5.cpp | head -80`

<details>
<summary>126 lines</summary>

```
/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "watchdog_hal_impl.h"
#include "static_recursive_mutex.h"

#if HAL_PLATFORM_HW_WATCHDOG || HAL_PLATFORM_EXTERNAL_RTC

#if HAL_PLATFORM_EXTERNAL_RTC
#include "am18x5.h"
using namespace particle;
#endif

class WatchdogLock {
public:
    WatchdogLock() {
        mutex_.lock();
    }

    ~WatchdogLock() {
        mutex_.unlock();
    }

    WatchdogLock(const WatchdogLock&) = delete;
    WatchdogLock& operator=(const WatchdogLock&) = delete;
    WatchdogLock(WatchdogLock&&) = delete;
    WatchdogLock& operator=(WatchdogLock&&) = delete;

private:
    static StaticRecursiveMutex mutex_;
};

StaticRecursiveMutex WatchdogLock::mutex_;

static WatchdogBase* getWatchdogInstance(hal_watchdog_instance_t instance) {
    static WatchdogBase* watchdogs[] = {
#if HAL_PLATFORM_NRF52840
        Nrf52Watchdog::instance(),
#endif
#if HAL_PLATFORM_RTL872X
        RtlWatchdog::instance(),
#endif
#if HAL_PLATFORM_EXTERNAL_RTC
        Am18x5Watchdog::instance(),
#endif
        // Add pointer to new watchdog here.
    };
    CHECK_TRUE(instance < sizeof(watchdogs) / sizeof(watchdogs[0]), nullptr);
    return watchdogs[instance];
}

int hal_watchdog_set_config(hal_watchdog_instance_t instance, const hal_watchdog_config_t* config, void* reserved) {
    WatchdogLock lk;
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    return pInstance->init(config);
}

int hal_watchdog_on_expired_callback(hal_watchdog_instance_t instance, hal_watchdog_on_expired_callback_t callback, void* context, void* reserved) {
    WatchdogLock lk;
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    return pInstance->setOnExpiredCallback(callback, context);
}

int hal_watchdog_start(hal_watchdog_instance_t instance, void* reserved) {
    WatchdogLock lk;
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    return pInstance->start();
}

int hal_watchdog_stop(hal_watchdog_instance_t instance, void* reserved) {
    WatchdogLock lk;
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    return pInstance->stop();
}

int hal_watchdog_refresh(hal_watchdog_instance_t instance, void* reserved) {
    WatchdogLock lk;
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    return pInstance->refresh();
}

int hal_watchdog_get_info(hal_watchdog_instance_t instance, hal_watchdog_info_t* info, void* reserved) {
    WatchdogLock lk;
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    // Update info.state according to the status register.
    pInstance->started();
    return pInstance->getInfo(info);
}

// Backward compatibility
bool hal_watchdog_reset_flagged_deprecated(void) {
    return false;
}

void hal_watchdog_refresh_deprecated() {
    hal_watchdog_refresh(HAL_WATCHDOG_INSTANCE1, nullptr);
}

#endif // HAL_PLATFORM_HW_WATCHDOG || HAL_PLATFORM_EXTERNAL_RTC
900:int Am18x5::enableWatchdog(system_tick_t ms) {
904:    Am18x5WatchdogFrequency frequency;
906:        frequency = Am18x5WatchdogFrequency::HZ_16;
909:        frequency = Am18x5WatchdogFrequency::HZ_4;
912:        frequency = Am18x5WatchdogFrequency::HZ_1;
915:        frequency = Am18x5WatchdogFrequency::HZ_1_4;
921:    uint8_t regValue = WDT_REGISTER_WDS_MASK; // Generate a reset when it times out
922:    regValue |= (value << WDT_REGISTER_BMB_SHIFT);
924:    watchdogValue_ = regValue;
925:    return writeRegister(Am18x5Register::WDT, regValue);
928:int Am18x5::disableWatchdog() const {
931:    return writeRegister(Am18x5Register::WDT, 0, false, true, WDT_REGISTER_BMB_MASK, WDT_REGISTER_BMB_SHIFT);
934:int Am18x5::feedWatchdog() const {
939:        return writeRegister(Am18x5Register::WDT, watchdogValue_);
949:void Am18x5::getWatchdogLimits(system_tick_t* low, system_tick_t* high) const {
951:        *low = 63; // round(Am18x5WatchdogFrequency::HZ_16 * 1)
954:        *high = 124000; // // round(Am18x5WatchdogFrequency::HZ_1_4 * 31)
958:bool Am18x5::isWatchdogStarted() const {
962:    readRegister(Am18x5Register::WDT, &bmb, false, WDT_REGISTER_BMB_MASK, WDT_REGISTER_BMB_SHIFT);
1053:    bool watchdogWasStarted = isWatchdogStarted();
1054:    if (watchdogWasStarted) {
1055:        CHECK(disableWatchdog());
1069:        if (watchdogWasStarted) {
1070:            writeRegister(Am18x5Register::WDT, watchdogValue_);
<shellId: 6 completed with exit code 0>
```

</details>


---

<sub>38s</sub>

### `bash`

**Read enableWatchdog and find Am18x5Watchdog**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0/hal/shared; sed -n 895,970p am18x5.cpp; grep -n "Am18x5Watchdog" -r . ../ | head; `

<details>
<summary>80 lines</summary>

```
uint8_t alm;
    CHECK(readRegister(Am18x5Register::TIMER_CONTROL, &alm, false, TIMER_CONTROL_RPT_MASK, TIMER_CONTROL_RPT_SHIFT));
    return alm;
}

int Am18x5::enableWatchdog(system_tick_t ms) {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t value; // Maximum 31.
    Am18x5WatchdogFrequency frequency;
    if (ms < 1937) { // 31 * 1000 / 16
        frequency = Am18x5WatchdogFrequency::HZ_16;
        value = ms * 16 / 1000;
    } else if (ms <= 7750) {
        frequency = Am18x5WatchdogFrequency::HZ_4;
        value = ms * 4 / 1000;
    } else if (ms <= 31000) {
        frequency = Am18x5WatchdogFrequency::HZ_1;
        value = ms / 1000;
    } else if (ms <= 124000) {
        frequency = Am18x5WatchdogFrequency::HZ_1_4;
        value = ms / 4 / 1000;
    } else {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK_TRUE(value > 0 && value < 32, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint8_t regValue = WDT_REGISTER_WDS_MASK; // Generate a reset when it times out
    regValue |= (value << WDT_REGISTER_BMB_SHIFT);
    regValue |= static_cast<uint8_t>(frequency);
    watchdogValue_ = regValue;
    return writeRegister(Am18x5Register::WDT, regValue);
}

int Am18x5::disableWatchdog() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::WDT, 0, false, true, WDT_REGISTER_BMB_MASK, WDT_REGISTER_BMB_SHIFT);
}

int Am18x5::feedWatchdog() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);

    if (config_.wdi_pin == PIN_INVALID) {
        return writeRegister(Am18x5Register::WDT, watchdogValue_);
    }
    if (hal_gpio_read(config_.wdi_pin) == 1) {
        hal_gpio_write(config_.wdi_pin, 0);
    } else {
        hal_gpio_write(config_.wdi_pin, 1);
    }
    return SYSTEM_ERROR_NONE;
}

void Am18x5::getWatchdogLimits(system_tick_t* low, system_tick_t* high) const {
    if (low) {
        *low = 63; // round(Am18x5WatchdogFrequency::HZ_16 * 1)
    }
    if (high) {
        *high = 124000; // // round(Am18x5WatchdogFrequency::HZ_1_4 * 31)
    }
}

bool Am18x5::isWatchdogStarted() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, false);
    uint8_t bmb = 0;
    readRegister(Am18x5Register::WDT, &bmb, false, WDT_REGISTER_BMB_MASK, WDT_REGISTER_BMB_SHIFT);
    return (bmb != 0);
}

int Am18x5::enableClkOut(Am18x5SqwFrequency freq) {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (to_underlying(freq) > 0x1F) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
./am18x5.cpp:904:    Am18x5WatchdogFrequency frequency;
./am18x5.cpp:906:        frequency = Am18x5WatchdogFrequency::HZ_16;
./am18x5.cpp:909:        frequency = Am18x5WatchdogFrequency::HZ_4;
./am18x5.cpp:912:        frequency = Am18x5WatchdogFrequency::HZ_1;
./am18x5.cpp:915:        frequency = Am18x5WatchdogFrequency::HZ_1_4;
./am18x5.cpp:951:        *low = 63; // round(Am18x5WatchdogFrequency::HZ_16 * 1)
./am18x5.cpp:954:        *high = 124000; // // round(Am18x5WatchdogFrequency::HZ_1_4 * 31)
./am18x5.h:117:enum class Am18x5WatchdogFrequency : uint8_t {
./am18x5.h:377:class Am18x5Watchdog : public WatchdogBase {
./am18x5.h:430:    static Am18x5Watchdog* instance() {
<shellId: 7 completed with exit code 0>
```

</details>


---

<sub>40s</sub>

### `bash`

**Am18x5Watchdog class**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0/hal; sed -n 370,440p shared/am18x5.h; ls shared | grep -i watchdog; grep -rn "Am18x5Watchdog::" --include=*.cpp . | head -20`

<details>
<summary>62 lines</summary>

```
private:
    bool locked_;
};

} // namespace particle

class Am18x5Watchdog : public WatchdogBase {
public:
    int init(const hal_watchdog_config_t* config) {
        CHECK_TRUE(particle::Am18x5::getInstance().isPresent(), SYSTEM_ERROR_NOT_FOUND);
        if (started()) {
            stop();
        }

        system_tick_t minTimeout, maxTimeout;
        particle::Am18x5::getInstance().getWatchdogLimits(&minTimeout, &maxTimeout);

        CHECK_TRUE(config && (config->size > 0), SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(config->timeout_ms >= minTimeout, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(config->timeout_ms <= maxTimeout, SYSTEM_ERROR_INVALID_ARGUMENT);
        
        memcpy(&info_.config, config, std::min(info_.config.size, config->size));
        info_.min_timeout_ms = minTimeout;
        info_.max_timeout_ms = maxTimeout;
        info_.state = HAL_WATCHDOG_STATE_CONFIGURED;
        initialized_ = true;
        return SYSTEM_ERROR_NONE;
    }

    int start() {
        CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
        CHECK_FALSE(started(), SYSTEM_ERROR_NONE);
        CHECK(particle::Am18x5::getInstance().enableWatchdog(info_.config.timeout_ms));
        CHECK_TRUE(started(), SYSTEM_ERROR_INTERNAL);
        return SYSTEM_ERROR_NONE;
    }

    bool started() override {
        if (particle::Am18x5::getInstance().isWatchdogStarted()) {
            info_.state = HAL_WATCHDOG_STATE_STARTED;
        }
        return info_.state == HAL_WATCHDOG_STATE_STARTED;
    }

    int stop() override {
        CHECK_TRUE(started(), SYSTEM_ERROR_NONE);
        CHECK(particle::Am18x5::getInstance().disableWatchdog());
        info_.state = HAL_WATCHDOG_STATE_STOPPED;
        CHECK_FALSE(started(), SYSTEM_ERROR_INTERNAL);
        return SYSTEM_ERROR_NONE;
    }

    int refresh() {
        CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(started(), SYSTEM_ERROR_INVALID_STATE);
        CHECK(particle::Am18x5::getInstance().feedWatchdog());
        return SYSTEM_ERROR_NONE;
    }

    static Am18x5Watchdog* instance() {
        static Am18x5Watchdog watchdog(HAL_WATCHDOG_CAPS_RESET,
                                    HAL_WATCHDOG_CAPS_RECONFIGURABLE | HAL_WATCHDOG_CAPS_STOPPABLE |
                                    HAL_WATCHDOG_CAPS_SLEEP_RUNNING | HAL_WATCHDOG_CAPS_DEBUG_RUNNING,
                                    0, 0);
        return &watchdog;
    }

private:
    Am18x5Watchdog(uint32_t mandatoryCaps, uint32_t optionalCaps, uint32_t minTimeout, uint32_t maxTimeout)
            : WatchdogBase(mandatoryCaps, optionalCaps, minTimeout, maxTimeout),
watchdog_hal.cpp
./shared/watchdog_hal.cpp:58:        Am18x5Watchdog::instance(),
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>44s</sub>

### `bash`

**Watchdog instances and OS callers**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; grep -n "INSTANCE" hal/inc/watchdog_hal.h; grep -n "refresh\|WatchdogClass::" wiring/src/spark_wiring_watchdog.cpp | head -30; grep -rn "hal_watchdog_refresh\|hal_watchdog_stop\|HAL_WATCHDOG_INSTANCE2" --include=*.cpp --include=*.c system hal/src services wiring/src 2>/dev/null | head -20`

<details>
<summary>22 lines</summary>

```
31:    HAL_WATCHDOG_INSTANCE1 = 0,
32:    HAL_WATCHDOG_INSTANCE2 = 1,
33:    HAL_WATCHDOG_INSTANCE_ALL = 0xFF,
30:int WatchdogClass::init(const WatchdogConfiguration& config) {
34:int WatchdogClass::start() {
38:bool WatchdogClass::started() {
44:int WatchdogClass::stop() {
48:int WatchdogClass::refresh() {
49:    return hal_watchdog_refresh(instance_, nullptr);
52:int WatchdogClass::getInfo(WatchdogInfo& info) {
56:int WatchdogClass::onExpired(WatchdogOnExpiredCallback callback, void* context) {
61:int WatchdogClass::onExpired(const WatchdogOnExpiredStdFunction& callback) {
hal/src/template/watchdog_hal.cpp:35:int hal_watchdog_stop(hal_watchdog_instance_t instance, void* reserved)
hal/src/template/watchdog_hal.cpp:40:int hal_watchdog_refresh(hal_watchdog_instance_t instance, void* reserved)
hal/src/template/watchdog_hal.cpp:57:void hal_watchdog_refresh_deprecated()
hal/src/gcc/watchdog_hal.cpp:35:int hal_watchdog_stop(hal_watchdog_instance_t instance, void* reserved)
hal/src/gcc/watchdog_hal.cpp:40:int hal_watchdog_refresh(hal_watchdog_instance_t instance, void* reserved)
hal/src/gcc/watchdog_hal.cpp:57:void hal_watchdog_refresh_deprecated()
hal/src/nRF52840/delay_hal.cpp:46:        // hal_watchdog_refresh_deprecated();
wiring/src/spark_wiring_watchdog.cpp:45:    return hal_watchdog_stop(instance_, nullptr);
wiring/src/spark_wiring_watchdog.cpp:49:    return hal_watchdog_refresh(instance_, nullptr);
<shellId: 9 completed with exit code 0>
```

</details>


---

<sub>57s</sub>

### `bash`

**Watchdog use in sleep paths**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; grep -rn "atchdog" hal/src/nRF52840/sleep_hal.cpp hal/src/nRF52840/exrtc_hal*.cpp system/src/system_sleep.cpp hal/shared/exrtc_hal.cpp 2>/dev/null | head -20; grep -rln "feedWatchdog\|hal_watchdog_" --include=*.cpp --include=*.h . | head -20`

<details>
<summary>24 lines</summary>

```
hal/src/nRF52840/exrtc_hal_deprecated.cpp:29:void hal_exrtc_get_watchdog_limits_deprecated(system_tick_t* low, system_tick_t* high, void* reserved) {
hal/src/nRF52840/exrtc_hal_deprecated.cpp:30:    Am18x5::getInstance().getWatchdogLimits(low, high);
hal/src/nRF52840/exrtc_hal_deprecated.cpp:33:int hal_exrtc_enable_watchdog_deprecated(system_tick_t ms, void* reserved) {
hal/src/nRF52840/exrtc_hal_deprecated.cpp:34:    return Am18x5::getInstance().enableWatchdog(ms);
hal/src/nRF52840/exrtc_hal_deprecated.cpp:37:int hal_exrtc_disable_watchdog_deprecated(void* reserved) {
hal/src/nRF52840/exrtc_hal_deprecated.cpp:38:    return Am18x5::getInstance().disableWatchdog();
hal/src/nRF52840/exrtc_hal_deprecated.cpp:41:int hal_exrtc_feed_watchdog_deprecated(void* reserved) {
hal/src/nRF52840/exrtc_hal_deprecated.cpp:42:    return Am18x5::getInstance().feedWatchdog();
./hal/inc/watchdog_base.h
./hal/inc/hal_dynalib_watchdog.h
./hal/inc/watchdog_hal.h
./hal/inc/hal_dynalib_core.h
./hal/shared/am18x5.cpp
./hal/shared/am18x5.h
./hal/shared/watchdog_hal.cpp
./hal/src/template/watchdog_hal.cpp
./hal/src/gcc/watchdog_hal.cpp
./hal/src/nRF52840/delay_hal.cpp
./hal/src/nRF52840/watchdog_hal_impl.h
./hal/src/nRF52840/exrtc_hal_deprecated.cpp
./hal/src/rtl872x/watchdog_hal_impl.h
./wiring/inc/spark_wiring_watchdog.h
./wiring/src/spark_wiring_watchdog.cpp
<shellId: 10 completed with exit code 0>
```

</details>


---

<sub>48s</sub>

### `bash`

**Hibernate handling in nRF sleep HAL**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; grep -n "HIBERNATE\|ExternalRtc\|exrtc\|Am18x5\|hal_exrtc\|setWakeup\|duration\|ALARM\|alarm" hal/src/nRF52840/sleep_hal.cpp | head -80`

<details>
<summary>20 lines</summary>

```
48:#include "exrtc_hal_internal.h"
758:    if (mode == HAL_SLEEP_MODE_HIBERNATE || mode == HAL_SLEEP_MODE_POWER_OFF) {
763:    if (mode == HAL_SLEEP_MODE_HIBERNATE || mode == HAL_SLEEP_MODE_POWER_OFF) {
767:        hal_exrtc_status_t status = {};
770:        if (hal_exrtc_get_status(HAL_EXRTC_INSTANCE_DEFAULT, &status, nullptr, nullptr) < 0 || !(status.status & HAL_EXRTC_STATUS_READY)) {
782:    if (mode == HAL_SLEEP_MODE_HIBERNATE) {
799:        if (mode == HAL_SLEEP_MODE_HIBERNATE) {
807:    if (mode == HAL_SLEEP_MODE_HIBERNATE) {
1024:    // WARNING: This is going to fix the issue that when both .ble() and .duration()
1137:         * As a workaround, for the duration of the sleep, we'll be bumping the priority of SD_EVT_IRQn, so that it can wake
1310:    hal_exrtc_sleep_config_t sleep = {};
1331:            CHECK_TRUE(sleep.duration == 0, SYSTEM_ERROR_NOT_SUPPORTED);
1332:            sleep.duration = rtc->ms / 1000;
1333:            CHECK_TRUE(sleep.duration > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
1340:    return hal_exrtc_command(HAL_EXRTC_INSTANCE_DEFAULT, HAL_EXRTC_COMMAND_SLEEP, &sleep, 0, nullptr);
1362:            CHECK(hal_rtc_set_alarm(&tv, HAL_RTC_ALARM_FLAG_IN, nullptr, nullptr, nullptr));
1466:            hal_exrtc_device_t device = {};
1468:            CHECK(hal_exrtc_get_device(HAL_EXRTC_INSTANCE_DEFAULT, &device, nullptr));
1529:        case HAL_SLEEP_MODE_HIBERNATE:
<shellId: 11 completed with exit code 0>
```

</details>


---

<sub>1m 0s</sub>

### `bash`

**Read hibernate sleep path**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; sed -n 745,830p hal/src/nRF52840/sleep_hal.cpp; sed -n 1295,1380p hal/src/nRF52840/sleep_hal.cpp; sed -n 1455,1490p hal/src/nRF52840/sleep_hal.cpp; sed -n 1520,1570p hal/src/nRF52840/sleep_hal.cpp`

<details>
<summary>211 lines</summary>

```
if (mode == HAL_SLEEP_MODE_POWER_OFF) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    if (gpio->pin >= TOTAL_PINS) {
        return SYSTEM_ERROR_LIMIT_EXCEEDED;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateRtcWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_rtc_t* rtc) {
    if (rtc->ms == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (mode == HAL_SLEEP_MODE_HIBERNATE || mode == HAL_SLEEP_MODE_POWER_OFF) {
        if ((rtc->ms / 1000) == 0) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
    }
    if (mode == HAL_SLEEP_MODE_HIBERNATE || mode == HAL_SLEEP_MODE_POWER_OFF) {
#if !HAL_PLATFORM_EXTERNAL_RTC
        return SYSTEM_ERROR_NOT_SUPPORTED;
#elif HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
        hal_exrtc_status_t status = {};
        status.size = sizeof(status);
        status.version = HAL_EXRTC_API_VERSION;
        if (hal_exrtc_get_status(HAL_EXRTC_INSTANCE_DEFAULT, &status, nullptr, nullptr) < 0 || !(status.status & HAL_EXRTC_STATUS_READY)) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
#endif
    }
    return SYSTEM_ERROR_NONE;
}

static int validateUsartWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_usart_t* usart) {
    if (!hal_usart_is_enabled(usart->serial)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (mode == HAL_SLEEP_MODE_HIBERNATE) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateNetworkWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_network_t* network) {
    if (!(network->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
#if HAL_PLATFORM_CELLULAR
        if (network->index == NETWORK_INTERFACE_CELLULAR &&
                !hal_usart_is_enabled(HAL_PLATFORM_CELLULAR_SERIAL)) {
#elif HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_NCP_SDIO
        if (network->index == NETWORK_INTERFACE_WIFI_STA &&
                !hal_usart_is_enabled(HAL_PLATFORM_WIFI_SERIAL)) {
#endif
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (mode == HAL_SLEEP_MODE_HIBERNATE) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
    }
    return SYSTEM_ERROR_NONE;
}

static int validateBleWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_base_t* base) {
    if (mode == HAL_SLEEP_MODE_HIBERNATE) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateLpcompWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_lpcomp_t* lpcomp) {
    if (hal_pin_validate_function(lpcomp->pin, PF_ADC) != PF_ADC) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (lpcomp->trig > HAL_SLEEP_LPCOMP_CROSS) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_base_t* base) {
    if (base->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
        return validateGpioWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_gpio_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
        return validateRtcWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_rtc_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
        return validateUsartWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_usart_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
            ret = constructBleWakeupReason(wakeupReason);
        } else if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
            ret = constructLpcompWakeupReason(wakeupReason, wakeupPin);
        } else if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
            ret = constructNetworkWakeupReason(wakeupReason, netif);
        } else {
            ret = SYSTEM_ERROR_INTERNAL;
        }
    }

    return ret;
}

static int enterPowerOffMode(const hal_sleep_config_t* config) {
#if HAL_PLATFORM_EXTERNAL_RTC
    hal_exrtc_sleep_config_t sleep = {};
    sleep.version = HAL_EXRTC_API_VERSION;
    sleep.size = sizeof(sleep);

    auto source = config->wakeup_sources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            auto gpio = reinterpret_cast<const hal_wakeup_source_gpio_t*>(source);
            CHECK_TRUE(gpio->type == HAL_PIN_TYPE_RTC, SYSTEM_ERROR_NOT_SUPPORTED);
            switch (gpio->mode) {
                case FALLING:
                case RISING: {
                    sleep.exti_mode = gpio->mode;
                    break;
                }
                default: {
                    return SYSTEM_ERROR_NOT_SUPPORTED;
                }
            }
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            auto rtc = reinterpret_cast<const hal_wakeup_source_rtc_t*>(source);
            CHECK_TRUE(sleep.duration == 0, SYSTEM_ERROR_NOT_SUPPORTED);
            sleep.duration = rtc->ms / 1000;
            CHECK_TRUE(sleep.duration > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
        } else {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        source = source->next;
    }

    return hal_exrtc_command(HAL_EXRTC_INSTANCE_DEFAULT, HAL_EXRTC_COMMAND_SLEEP, &sleep, 0, nullptr);
#else
    return SYSTEM_ERROR_NOT_SUPPORTED;
#endif
}

static int enterHibernateMode(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeupReason) {
#if HAL_PLATFORM_EXTERNAL_RTC
    if (config->mode == HAL_SLEEP_MODE_POWER_OFF) {
        return enterPowerOffMode(config);
    }
#endif
#if HAL_PLATFORM_EXTERNAL_RTC
    auto source = config->wakeup_sources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            auto rtcWakeup = reinterpret_cast<const hal_wakeup_source_rtc_t*>(source);
            auto seconds = rtcWakeup->ms / 1000;
            struct timeval tv = {
                .tv_sec = seconds,
                .tv_usec = 0
            };
            CHECK(hal_rtc_set_alarm(&tv, HAL_RTC_ALARM_FLAG_IN, nullptr, nullptr, nullptr));
        }
        source = source->next;
    }
#endif

    // Make sure we acquire exflash lock BEFORE going into a critical section
    hal_exflash_lock();

    // Disable thread scheduling
    os_thread_scheduling(false, nullptr);

    // Suspend USARTs
    for (int usart = 0; usart < HAL_PLATFORM_USART_NUM; usart++) {
        hal_usart_sleep(static_cast<hal_usart_interface_t>(usart), true, nullptr);
    }
    // Suspend SPIs
    for (int spi = 0; spi < HAL_PLATFORM_SPI_NUM; spi++) {
        hal_spi_sleep(static_cast<hal_spi_interface_t>(spi), true, nullptr);
                } else {
                    nrf_gpio_cfg_sense_set(nrfPin, NRF_GPIO_PIN_SENSE_HIGH);
                }
            } else {
                nrf_gpio_cfg_sense_input(nrfPin, wakeupPinMode, wakeupPinSense);
            }
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
            configLpcompWakeupSource(wakeupSource);
        }
#if HAL_PLATFORM_EXTERNAL_RTC
        else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            hal_exrtc_device_t device = {};
            device.size = sizeof(device);
            CHECK(hal_exrtc_get_device(HAL_EXRTC_INSTANCE_DEFAULT, &device, nullptr));
            CHECK_TRUE(device.transport == HAL_EXRTC_TRANSPORT_I2C, SYSTEM_ERROR_NOT_SUPPORTED);
            CHECK_TRUE(device.i2c.pin_int != PIN_INVALID, SYSTEM_ERROR_NOT_SUPPORTED);
            hal_pin_info_t* halPinMap = hal_pin_map();
            uint32_t nrfPin = NRF_GPIO_PIN_MAP(halPinMap[device.i2c.pin_int].gpio_port, halPinMap[device.i2c.pin_int].gpio_pin);
            nrf_gpio_cfg_sense_input(nrfPin, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
        }
#endif
        wakeupSource = wakeupSource->next;
    }

    // RAM retention is configured on early boot in Set_System()

    SPARK_ASSERT(sd_power_system_off() == NRF_SUCCESS);
    while (1);
    return SYSTEM_ERROR_NONE;
}

int hal_sleep_validate_config(const hal_sleep_config_t* config, void* reserved) {
    // Checks the sleep mode.
    if (config->mode == HAL_SLEEP_MODE_NONE || config->mode >= HAL_SLEEP_MODE_MAX) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    int ret = SYSTEM_ERROR_NONE;

    switch (config->mode) {
        case HAL_SLEEP_MODE_STOP:
        case HAL_SLEEP_MODE_ULTRA_LOW_POWER: {
            ret = enterStopBasedSleep(config, wakeup_source);
            break;
        }
        case HAL_SLEEP_MODE_HIBERNATE:
        case HAL_SLEEP_MODE_POWER_OFF: {
            ret = enterHibernateMode(config, wakeup_source);
            break;
        }
        default: {
            ret = SYSTEM_ERROR_NOT_SUPPORTED;
            break;
        }
    }

    return ret;
}
<shellId: 12 completed with exit code 0>
```

</details>


---

<sub>1m 5s</sub>

### `bash`

**RTC alarm HAL and reset reason sources**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; grep -n "hal_rtc_set_alarm" -r hal/src/nRF52840/*.cpp hal/shared/*.cpp | head; grep -rn "RESET_REASON_POWER_MANAGEMENT" --include=*.cpp --include=*.h hal system | head; sed -n 1,60p wiring/inc/spark_wiring_exrtc.h | head -5; grep -n "ms\b\|duration" wiring/inc/spark_wiring_system_sleep.h 2>/dev/null | head -20`

<details>
<summary>9 lines</summary>

```
hal/src/nRF52840/rtc_hal.cpp:133:int hal_rtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved) {
hal/src/nRF52840/sleep_hal.cpp:1362:            CHECK(hal_rtc_set_alarm(&tv, HAL_RTC_ALARM_FLAG_IN, nullptr, nullptr, nullptr));
system/src/system_cloud_internal.cpp:965:    case RESET_REASON_POWER_MANAGEMENT:
/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
<shellId: 13 completed with exit code 0>
```

</details>


---

<sub>1m 13s</sub>

### `bash`

**Alarm HAL detail and reset reason, exrtc wiring**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; sed -n 120,200p hal/src/nRF52840/rtc_hal.cpp; grep -rn "POWER_MANAGEMENT" --include=*.cpp --include=*.h --include=*.c . 2>/dev/null | grep -v "docs/\|tests" | head; grep -n "bound\|class\|status\|clockSource" wiring/inc/spark_wiring_exrtc.h | head -40`

<details>
<summary>117 lines</summary>

```
// Update both is possible
    int r = 0;
    if (!opt || opt->source != HAL_RTC_SOURCE_INTERNAL) {
        r = hal_exrtc_set_time_internal(tv);
    }
    if (opt && opt->source == HAL_RTC_SOURCE_EXTERNAL) {
        return r;
    }
#endif // HAL_PLATFORM_EXTERNAL_RTC

    return setUnixTime(tv);
}

int hal_rtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved) {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);

#if HAL_PLATFORM_EXTERNAL_RTC
    if (hal_exrtc_is_default() && !hal_exrtc_set_alarm(tv, flags, handler, context)) {
        return 0;
    }
#endif // HAL_PLATFORM_EXTERNAL_RTC

    struct timeval alarm = *tv;
    if (flags & HAL_RTC_ALARM_FLAG_IN) {
        struct timeval now;
        CHECK(hal_rtc_get_time(&now, nullptr));
        timeradd(&alarm, &now, &alarm);
    }
    auto unixTimeMs = getUsUnixTime() / 1000;
    auto alarmTimeMs = usUnixtimeFromTimeval(&alarm) / 1000;
    if (alarmTimeMs <= unixTimeMs) {
        // Too late to set such an alarm
        return SYSTEM_ERROR_TIMEOUT;
    }

    // This implementation is only used for System.sleep(seconds) (network sleep)
    // on Gen3 devices.
    if (!s_alarm_timer) {
        os_timer_create(&s_alarm_timer, 0, [](os_timer_t timer) {
            if (s_alarm_handler) {
                s_alarm_handler(s_alarm_context);
            }
        }, nullptr, true, nullptr);
        SPARK_ASSERT(s_alarm_timer);
    }

    hal_rtc_cancel_alarm();

    s_alarm_context = context;
    s_alarm_handler = handler;

    unsigned diffMs = (unsigned)(alarmTimeMs - unixTimeMs);
    // NOTE: changing the period of a timer in a dormant state will also
    // start the timer.
    int r = os_timer_change(s_alarm_timer, OS_TIMER_CHANGE_PERIOD, false, diffMs,
            0xffffffff, nullptr);

    if (r != 0) {
        return SYSTEM_ERROR_INTERNAL;
    }

    return r;
}

void hal_rtc_cancel_alarm(void) {
#if HAL_PLATFORM_EXTERNAL_RTC
    if (hal_exrtc_is_default() && !hal_exrtc_cancel_alarm()) {
        return;
    }
#endif // HAL_PLATFORM_EXTERNAL_RTC

    // This implementation is only used for System.sleep(seconds) (network sleep)
    // on Gen3 devices.
    if (s_alarm_timer) {
        os_timer_change(s_alarm_timer, OS_TIMER_CHANGE_STOP, false, 0, 0xffffffff, nullptr);
    }
}

// These are deprecated due to time_t size changes
void hal_rtc_set_unixtime_deprecated(time32_t value) {
    struct timeval tv = {
./hal/inc/power_hal.h:50:    HAL_POWER_MANAGEMENT_DISABLE = 0x100,
./hal/inc/core_hal.h:54:    POWER_MANAGEMENT_RESET = 0x04,
./hal/inc/hal_platform.h:203:#ifndef HAL_PLATFORM_POWER_MANAGEMENT
./hal/inc/hal_platform.h:204:#define HAL_PLATFORM_POWER_MANAGEMENT (0)
./hal/inc/hal_platform.h:205:#endif /* HAL_PLATFORM_POWER_MANAGEMENT */
./hal/inc/hal_platform.h:235:#ifndef HAL_PLATFORM_POWER_MANAGEMENT_STACK_SIZE
./hal/inc/hal_platform.h:236:#define HAL_PLATFORM_POWER_MANAGEMENT_STACK_SIZE (1536)
./hal/inc/hal_platform.h:237:#endif /* HAL_PLATFORM_POWER_MANAGEMENT_STACK_SIZE */
./hal/inc/hal_platform.h:311:#ifndef HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
./hal/inc/hal_platform.h:312:#define HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL (0)
27:#include "enumclass.h"
39:enum class RtcType : uint32_t {
47:enum class RtcCap : uint32_t {
64:enum class RtcStatusFlag : uint8_t {
73:enum class RtcClockSource : uint8_t {
79:enum class Am18x5AutoCalibration : uint8_t {
85:enum class RtcEvent : uint32_t {
98:class RtcEventSubscription {
116:class RtcConfigurationTag {
122:class RtcConfigurationBase: public RtcConfigurationTag {
249:    Concrete& clockSource(RtcClockSource source) {
254:    RtcClockSource clockSource() const {
320:class RtcConfiguration : public RtcConfigurationBase<RtcConfiguration> {
330:class Am18x5Configuration : public RtcConfigurationBase<Am18x5Configuration> {
394:class RtcStatus {
397:    explicit RtcStatus(hal_exrtc_status_t status)
398:            : status_(status),
421:        return static_cast<RtcType>(status_.type);
425:        return RtcStatusFlags::fromUnderlying(status_.status);
429:        return RtcCaps::fromUnderlying(status_.caps_supported);
433:        return RtcCaps::fromUnderlying(status_.caps_optional);
437:        return RtcCaps::fromUnderlying(status_.caps_enabled);
440:    RtcClockSource clockSource() const {
441:        return static_cast<RtcClockSource>(status_.clock_source);
445:        return status_.xtal_calibration;
449:        return status_.status & HAL_EXRTC_STATUS_BUILT_IN;
452:    bool bound() const {
453:        return status_.status & HAL_EXRTC_STATUS_BOUND;
457:        return status_.status & HAL_EXRTC_STATUS_PRESENT;
461:        return status_.status & HAL_EXRTC_STATUS_READY;
465:    hal_exrtc_status_t status_ = {};
470:class ExRtcBase: public TimeClass {
481:    RtcStatus status() const {
482:        hal_exrtc_status_t stat = {};
485:        int r = hal_exrtc_get_status(instance_, &stat, nullptr, nullptr);
581:class ExRtcT : public ExRtcBase {
638:class ExRtc : public ExRtcT<RtcConfiguration> {
656:class Am18x5Rtc : public ExRtcT<Am18x5Configuration> {
<shellId: 14 completed with exit code 0>
```

</details>


---

<sub>1m 18s</sub>

### `bash`

**RTC status/events and reset reasons**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; sed -n 36,115p wiring/inc/spark_wiring_exrtc.h; grep -n "EXRTC_STATUS\|EXRTC_EVENT" hal/inc/exrtc_hal.h; grep -n "RESET_REASON" system/inc/system_defs.h hal/inc/core_hal.h | head -30`

<details>
<summary>75 lines</summary>

```
namespace particle {

enum class RtcType : uint32_t {
    NONE = HAL_EXRTC_TYPE_NONE,
    UNKNOWN = HAL_EXRTC_TYPE_UNKNOWN,
#if HAL_PLATFORM_AM18X5
    AM18X5 = HAL_EXRTC_TYPE_AM18X5,
#endif // HAL_PLATFORM_AM18X5
};

enum class RtcCap : uint32_t {
    NONE = HAL_EXRTC_CAPS_NONE,
    POWER_GATE = HAL_EXRTC_CAPS_POWER_GATE,
    CLOCK_SOURCE = HAL_EXRTC_CAPS_CLOCK_SOURCE,
    CLOCK_OUTPUT = HAL_EXRTC_CAPS_CLOCK_OUTPUT,
    AUTO_CALIBRATION = HAL_EXRTC_CAPS_AUTO_CALIBRATION,
    AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY = HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY,
    AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL = HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL,
    SLEEP = HAL_EXRTC_CAPS_SLEEP,
    EXTI = HAL_EXRTC_CAPS_EXTI,
    EXTI_LEVEL_TRIGGER = HAL_EXRTC_CAPS_EXTI_LEVEL_TRIGGER,
    WATCHDOG = HAL_EXRTC_CAPS_WATCHDOG,
};
ENABLE_ENUM_CLASS_BITWISE(RtcCap);

using RtcCaps = EnumFlags<RtcCap>;

enum class RtcStatusFlag : uint8_t {
    NONE = HAL_EXRTC_STATUS_NONE,
    BUILT_IN = HAL_EXRTC_STATUS_BUILT_IN,
    BOUND = HAL_EXRTC_STATUS_BOUND,
    PRESENT = HAL_EXRTC_STATUS_PRESENT,
    READY = HAL_EXRTC_STATUS_READY,
};
ENABLE_ENUM_CLASS_BITWISE(RtcStatusFlag);

enum class RtcClockSource : uint8_t {
    NONE = HAL_EXRTC_CLOCK_SOURCE_NONE,
    INTERNAL = HAL_EXRTC_CLOCK_SOURCE_INTERNAL,
    EXTERNAL = HAL_EXRTC_CLOCK_SOURCE_EXTERNAL,
};

enum class Am18x5AutoCalibration : uint8_t {
    DISABLE = HAL_EXRTC_AM18X5_AUTO_CAL_DISABLE,
    EVERY_1024_SEC = HAL_EXRTC_AM18X5_AUTO_CAL_EVERY_1024_SEC,
    EVERY_512_SEC = HAL_EXRTC_AM18X5_AUTO_CAL_EVERY_512_SEC,
};

enum class RtcEvent : uint32_t {
    NONE = HAL_EXRTC_EVENT_NONE,
    CALIBRATION_FAILURE = HAL_EXRTC_EVENT_CALIBRATION_FAILURE,
    CLOCK_SOURCE_EXTERNAL_FAILURE = HAL_EXRTC_EVENT_CLOCK_SOURCE_EXTERNAL_FAILURE,
};
ENABLE_ENUM_CLASS_BITWISE(RtcEvent);

using RtcEvents = EnumFlags<RtcEvent>;
using ExRtcOnEventCallback = void (*)(RtcEvents events, void* context);
using ExRtcOnEventStdFunction = std::function<void(RtcEvents events)>;

using RtcStatusFlags = EnumFlags<RtcStatusFlag>;

class RtcEventSubscription {
public:
    explicit RtcEventSubscription(void* cookie = nullptr)
            : cookie_(cookie) {
    }

    explicit operator bool() const {
        return cookie_ != nullptr;
    }

    void* getCookie() const {
        return cookie_;
    }

private:
    void* cookie_;
};

57:    HAL_EXRTC_STATUS_NONE = 0,
58:    HAL_EXRTC_STATUS_BUILT_IN = 0x01,
59:    HAL_EXRTC_STATUS_BOUND = 0x02,
60:    HAL_EXRTC_STATUS_PRESENT = 0x04,
61:    HAL_EXRTC_STATUS_READY = 0x08
86:    HAL_EXRTC_EVENT_NONE = 0,
87:    HAL_EXRTC_EVENT_CALIBRATION_FAILURE = 0x01,
88:    HAL_EXRTC_EVENT_CLOCK_SOURCE_EXTERNAL_FAILURE = 0x02
grep: system/inc/system_defs.h: No such file or directory
<shellId: 15 completed with exit code 0>
```

</details>


---

<sub>1m 23s</sub>

### `bash`

**Sleep and STATUS handling in am18x5**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0/hal/shared; sed -n 1040,1080p am18x5.cpp; grep -n "STATUS" am18x5.cpp | head -40; grep -n "init\b\|::init(" am18x5.cpp | head`

<details>
<summary>55 lines</summary>

```
CHECK(writeRegister(Am18x5Register::OUTPUT_CTRL, outputCtrl));

    uint8_t control1 = 0x00;
    CHECK(readRegister(Am18x5Register::CONTROL1, &control1));
    bool pwr2Enabled = control1 & CONTROL1_PWR2_MASK;

    // Set PSW/nIRQ2 to be working in SLEEP mode as the power switch output.
    CHECK(writeRegister(Am18x5Register::CONTROL2, 6, false, true, CONTROL2_OUT2S_MASK, CONTROL2_OUT2S_SHIFT));
    CHECK(writeRegister(Am18x5Register::CONTROL1, 1, false, true, CONTROL1_PWR2_MASK, 1));
    // When 1, the I/O interface will be disabled when the power switch is active and disabled 
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
    CHECK(writeRegister(Am18x5Register::OSC_CONTROL, 1, false, true, OSC_CONTROL_PWGT_MASK, OSC_CONTROL_PWGT_SHIFT));

    bool watchdogWasStarted = isWatchdogStarted();
    if (watchdogWasStarted) {
        CHECK(disableWatchdog());
    }

    struct timeval alarmTv = {};
    int enabled = CHECK(getAlarm(&alarmTv));

    // This function should not return if succeeded.
    SCOPE_GUARD ({
        // Restore alarm config
        setAlarm(enabled > 0 ? true : false, 0, &alarmTv, alarmHandler_, alarmHandlerContext_, false);
        // Disable EXTI interrupt, as it is only used for sleep wakeup for now
        intMask &= ~INTERRUPT_EX1E_MASK;
        writeRegister(Am18x5Register::INT_MASK, intMask);
        writeRegister(Am18x5Register::CONTROL1, pwr2Enabled ? 1 : 0, false, true, CONTROL1_PWR2_MASK, 1);
        if (watchdogWasStarted) {
            writeRegister(Am18x5Register::WDT, watchdogValue_);
        }
    });

    if (config->duration > 0) {
        struct timeval tv = {
            .tv_sec = config->duration,
            .tv_usec = 0
        };
        CHECK(setAlarm(true, HAL_RTC_ALARM_FLAG_IN, &tv, nullptr, nullptr, false));
        intMask |= INTERRUPT_AIE_MASK;
362:    exrtcStatus.status = HAL_EXRTC_STATUS_BOUND;
378:        exrtcStatus.status |= HAL_EXRTC_STATUS_PRESENT | HAL_EXRTC_STATUS_READY;
435:    CHECK(readRegister(Am18x5Register::OSC_STATUS, &oscStatus));
436:    const uint8_t xtcal = (oscStatus & OSC_STATUS_XTCAL_MASK) >> OSC_STATUS_XTCAL_SHIFT;
598:        CHECK(writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK, OSC_STATUS_OF_SHIFT));
625:    CHECK(readRegister(Am18x5Register::OSC_STATUS, &oscStatus));
626:    *source = (oscStatus & OSC_STATUS_OMODE_MASK) ? Am18x5Oscillator::INTERNAL_RC : Am18x5Oscillator::EXTERNAL_CRYSTAL;
646:    CHECK(writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK, 0));
985:    CHECK(writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_LKO2_MASK, OSC_STATUS_LKO2_SHIFT));
1092:    CHECK(readRegister(Am18x5Register::STATUS, &status));
1314:    CHECK(writeRegister(Am18x5Register::OSC_STATUS, xtcal, false, true, OSC_STATUS_XTCAL_MASK, OSC_STATUS_XTCAL_SHIFT));
1418:            if (instance->readRegister(Am18x5Register::STATUS, &alm, false, STATUS_ALM_MASK) != SYSTEM_ERROR_NONE) {
1445:            if (instance->readRegister(Am18x5Register::OSC_STATUS, &oscStatus) != SYSTEM_ERROR_NONE) {
1457:                    if (oscStatus & OSC_STATUS_OF_MASK) {
1463:                if (oscStatus & OSC_STATUS_ACF_MASK) {
1467:            if (oscStatus & (OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK)) {
1469:                instance->writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK, 0);
492:            hal_i2c_init(config_.i2c_if, nullptr);
501:        // hal_i2c_lock/hal_i2c_unlock, which do not function unless hal_i2c_init is called.
<shellId: 16 completed with exit code 0>
```

</details>


---

<sub>1m 27s</sub>

### `bash`

**setAlarm arming details**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0/hal/shared; grep -n "int Am18x5::setAlarm" -A50 am18x5.cpp | head -80; grep -n "hal_exrtc_set_alarm" -A15 exrtc_hal.cpp | head -40`

<details>
<summary>68 lines</summary>

```
829:int Am18x5::setAlarm(bool enable, uint32_t flags, const struct timeval* tv, AlarmHandler handler, void* context, bool requireInterruptPin) {
830-    Am18x5Lock lock;
831-    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
832-    if (enable) {
833-        if (requireInterruptPin) {
834-            CHECK_TRUE(config_.int_pin != PIN_INVALID, SYSTEM_ERROR_NOT_SUPPORTED);
835-        }
836-        if (tv) {
837-            struct timeval alarm = *tv;
838-            struct timeval now;
839-            CHECK(getTime(&now));
840-            if (flags & HAL_RTC_ALARM_FLAG_IN) {
841-                timeradd(tv, &now, &alarm);
842-            }
843-
844-            if (alarm.tv_sec <= now.tv_sec) {
845-                // Too late to set such an alarm
846-                return SYSTEM_ERROR_TIMEOUT;
847-            }
848-
849-            struct tm calendar;
850-            CHECK(canonicalizeTimeval(&alarm));
851-            CHECK(timevalToCalendar(&alarm, &calendar));
852-            uint8_t buff[7] = {0};
853-            buff[0] = CHECK(decToBcd(alarm.tv_usec / MICROS_IN_HUNDREDTH));
854-            buff[1] = CHECK(decToBcd(calendar.tm_sec));
855-            buff[2] = CHECK(decToBcd(calendar.tm_min));
856-            buff[3] = CHECK(decToBcd(calendar.tm_hour));
857-            buff[4] = CHECK(decToBcd(calendar.tm_mday));
858-            buff[5] = CHECK(decToBcd(calendar.tm_mon + 1)); // Month in tm structure ranges from 0 - 11.
859-            alarmYear_ = calendar.tm_year % 100;
860-            buff[6] = CHECK(decToBcd(calendar.tm_wday));
861-            CHECK(writeContinuousRegisters(Am18x5Register::HUNDREDTHS_ALARM, buff, sizeof(buff)));
862-        }
863-        alarmHandler_ = handler;
864-        alarmHandlerContext_ = context;
865-        // Hundredths, seconds, minutes, hours, date and month match (once per year)
866-        // TODO: allow configuring repeat mode
867-        CHECK(writeRegister(Am18x5Register::TIMER_CONTROL, 1, false, true, TIMER_CONTROL_RPT_MASK, TIMER_CONTROL_RPT_SHIFT));
868-    } else {
869-        CHECK(writeRegister(Am18x5Register::TIMER_CONTROL, 0, false, true, TIMER_CONTROL_RPT_MASK, TIMER_CONTROL_RPT_SHIFT));
870-    }
871-    return writeRegister(Am18x5Register::INT_MASK, enable, false, true, INTERRUPT_AIE_MASK, INTERRUPT_AIE_SHIFT);
872-}
873-
874-int Am18x5::getAlarm(struct timeval* tv) const {
875-    Am18x5Lock lock;
876-    if (tv) {
877-        uint8_t buff[7] = {};
878-        CHECK(readContinuousRegisters(Am18x5Register::HUNDREDTHS_ALARM, buff, sizeof(buff)));
879-
419:int hal_exrtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context) {
420-    if (Am18x5::getInstance().isPresent()) {
421-        return Am18x5::getInstance().setAlarm(true, flags, tv, handler, context, true);
422-    }
423-    return SYSTEM_ERROR_NOT_FOUND;
424-}
425-
426-int hal_exrtc_cancel_alarm(void) {
427-    if (Am18x5::getInstance().isPresent()) {
428-        return Am18x5::getInstance().setAlarm(false);
429-    }
430-    return SYSTEM_ERROR_NOT_FOUND;
431-}
432-
433-bool hal_exrtc_is_default(void) {
434-    return Am18x5::getInstance().isDefault();
<shellId: 17 completed with exit code 0>
```

</details>


---

<sub>1m 32s</sub>

### `bash`

**Find reset reason mapping**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; grep -rn "RESET_REASON_POWER\|RESET_REASON_PIN" --include=*.cpp --include=*.h --include=*.c hal services system wiring 2>/dev/null | head; grep -n "wakeup_source\b" hal/src/nRF52840/sleep_hal.cpp | head -3; grep -n "sd_power_reset\|RESETREAS\|resetreas" -r hal/src/nRF52840/core_hal.c | head -20`

<details>
<summary>20 lines</summary>

```
hal/network/ncp_client/sara/sara_ncp_client.cpp:3048:                    (reason == RESET_REASON_POWER_DOWN || reason == RESET_REASON_POWER_BROWNOUT)) {
hal/src/nRF52840/core_hal.c:670:            last_reset_info.reason = RESET_REASON_POWER_MANAGEMENT; // Reset generated when entering standby mode (nRST_STDBY: 0)
hal/src/nRF52840/core_hal.c:674:            last_reset_info.reason = RESET_REASON_POWER_DOWN;
hal/src/nRF52840/core_hal.c:678:            last_reset_info.reason = RESET_REASON_POWER_BROWNOUT;
hal/src/nRF52840/core_hal.c:682:            last_reset_info.reason = RESET_REASON_PIN_RESET;
hal/src/rtl872x/core_hal.c:607:            last_reset_info.reason = RESET_REASON_POWER_MANAGEMENT; // Reset generated when entering standby mode (nRST_STDBY: 0)
hal/src/rtl872x/core_hal.c:611:            last_reset_info.reason = RESET_REASON_POWER_DOWN;
hal/src/rtl872x/core_hal.c:615:            last_reset_info.reason = RESET_REASON_POWER_BROWNOUT;
hal/src/rtl872x/core_hal.c:619:            last_reset_info.reason = RESET_REASON_PIN_RESET;
services/inc/system_defs.h:55:    RESET_REASON_PIN_RESET = 20, ///< Reset from the reset pin.
1515:int hal_sleep_enter(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source, void* reserved) {
1526:            ret = enterStopBasedSleep(config, wakeup_source);
1531:            ret = enterHibernateMode(config, wakeup_source);
hal/src/nRF52840/core_hal.c:620:        sd_power_reset_reason_get(&reset_reason);
hal/src/nRF52840/core_hal.c:624:            return reset_reason == NRF_POWER_RESETREAS_RESETPIN_MASK;
hal/src/nRF52840/core_hal.c:630:            return reset_reason == NRF_POWER_RESETREAS_SREQ_MASK;
hal/src/nRF52840/core_hal.c:633:            return reset_reason == NRF_POWER_RESETREAS_DOG_MASK;
hal/src/nRF52840/core_hal.c:637:            return (reset_reason == NRF_POWER_RESETREAS_OFF_MASK) || (reset_reason == NRF_POWER_RESETREAS_LPCOMP_MASK);
hal/src/nRF52840/core_hal.c:695:    sd_power_reset_reason_clr(0xFFFFFFFF);
<shellId: 18 completed with exit code 0>
```

</details>


---

<sub>1m 33s</sub>

### `bash`

**Reset reason mapping**

`$ cd ~/.particle/toolchains/deviceOS/6.6.0; sed -n 610,700p hal/src/nRF52840/core_hal.c`

<details>
<summary>86 lines</summary>

```
return SYSTEM_ERROR_DEPRECATED;
}

void HAL_Core_Execute_Standby_Mode(void) {
    // Deprecated. See: hal/src/nRF52840/sleep_hal.cpp
}

bool HAL_Core_System_Reset_FlagSet(RESET_TypeDef resetType) {
    uint32_t reset_reason = SYSTEM_FLAG(RCC_CSR_SysFlag);
    if (reset_reason == 0xffffffff) {
        sd_power_reset_reason_get(&reset_reason);
    }
    switch(resetType) {
        case PIN_RESET: {
            return reset_reason == NRF_POWER_RESETREAS_RESETPIN_MASK;
        }
        case SOFTWARE_RESET: {
            if (HAL_Core_Read_Backup_Register(BKP_DR_04) == 0xDEADBEEF) {
                return true;
            }
            return reset_reason == NRF_POWER_RESETREAS_SREQ_MASK;
        }
        case WATCHDOG_RESET: {
            return reset_reason == NRF_POWER_RESETREAS_DOG_MASK;
        }
        case POWER_MANAGEMENT_RESET: {
            // SYSTEM OFF Mode
            return (reset_reason == NRF_POWER_RESETREAS_OFF_MASK) || (reset_reason == NRF_POWER_RESETREAS_LPCOMP_MASK);
        }
        // If none of the reset sources are flagged, this indicates that
        // the chip was reset from the on-chip reset generator,
        // which will indicate a power-on-reset or a brownout reset.
        case POWER_DOWN_RESET:
        case POWER_BROWNOUT_RESET: {
            return reset_reason == 0;
        }
        default:
            return false;
    }
}

static void Init_Last_Reset_Info()
{
    if (HAL_Core_System_Reset_FlagSet(SOFTWARE_RESET))
    {
        // Load reset info from backup registers
        last_reset_info.reason = HAL_Core_Read_Backup_Register(BKP_DR_02);
        last_reset_info.data = HAL_Core_Read_Backup_Register(BKP_DR_03);
        // Clear backup registers
        HAL_Core_Write_Backup_Register(BKP_DR_02, 0);
        HAL_Core_Write_Backup_Register(BKP_DR_03, 0);
    }
    else // Hardware reset
    {
        if (HAL_Core_System_Reset_FlagSet(WATCHDOG_RESET))
        {
            last_reset_info.reason = RESET_REASON_WATCHDOG;
        }
        else if (HAL_Core_System_Reset_FlagSet(POWER_MANAGEMENT_RESET))
        {
            last_reset_info.reason = RESET_REASON_POWER_MANAGEMENT; // Reset generated when entering standby mode (nRST_STDBY: 0)
        }
        else if (HAL_Core_System_Reset_FlagSet(POWER_DOWN_RESET))
        {
            last_reset_info.reason = RESET_REASON_POWER_DOWN;
        }
        else if (HAL_Core_System_Reset_FlagSet(POWER_BROWNOUT_RESET))
        {
            last_reset_info.reason = RESET_REASON_POWER_BROWNOUT;
        }
        else if (HAL_Core_System_Reset_FlagSet(PIN_RESET)) // Pin reset flag should be checked in the last place
        {
            last_reset_info.reason = RESET_REASON_PIN_RESET;
        }
        // TODO: Reset from USB, NFC, LPCOMP...
        else
        {
            last_reset_info.reason = RESET_REASON_UNKNOWN;
        }
        last_reset_info.data = 0; // Not used
    }

    HAL_Core_Write_Backup_Register(BKP_DR_04, 0);

    // Clear Reset info register
    sd_power_reset_reason_clr(0xFFFFFFFF);
}

int HAL_Core_Get_Last_Reset_Info(int *reason, uint32_t *data, void *reserved) {
    if (HAL_Feature_Get(FEATURE_RESET_INFO)) {
        if (reason) {
<shellId: 19 completed with exit code 0>
```

</details>


---

<sub>1m 49s</sub>

### `bash`

**Read setup classification caller**

`$ sed -n 740,860p src/Generalized-Core-Counter.cpp`

<details>
<summary>111 lines</summary>

```
"succeed; RTC left unchanged (before(raw=%ld readOk=%d) attemptedAfter=%s)",
                  (long)rtcSkewRawBefore, rtcSkewReadBeforeOk ? 1 : 0,
                  Time.format(rtcSkewAfter, TIME_FORMAT_DEFAULT).c_str());
      }
    }
  }
#endif

  // ===== AB1805 WAKE-REASON CLASSIFICATION (HibernateCycle) =====
  // WO-2026-09-16 Step 2: PIN_RESET/AB1805-watchdog confirmation and the
  // hibernate wake-validation gate both used to read ab1805.getWakeReason()
  // independently, at two different points in setup(). Both are safe to
  // call more than once - see HibernateCycle.cpp's own comment for why
  // (getWakeReason() is a plain getter onto the one destructive
  // updateWakeReason() call AB1805::setup() already made) - but there is no
  // longer a reason to keep them apart: HibernateCycle::classifyWake() does
  // both in one call, here, before setWDT() re-arms the watchdog, exactly
  // where the PIN_RESET classification used to run alone.
  const HibernateCycle::WakeVerdict wakeVerdict = HibernateCycle::classifyWake(reason, ab1805);

  bool ab1805ConfirmedWatchdog = false;
  if (wakeVerdict.pinResetChecked) {
    startupPinResetAb1805Checked = true;
    startupAb1805WakeReasonName = wakeVerdict.pinResetWakeReasonName;

    if (wakeVerdict.pinResetConfirmedWatchdog) {
      ab1805ConfirmedWatchdog = true;
      Log.warn("PIN_RESET confirmed as AB1805 watchdog reset");
    } else if (wakeVerdict.pinResetWakeReasonUnknown) {
      // Explicitly inconclusive - do NOT treat UNKNOWN as "not the AB1805".
      Log.info("PIN_RESET with AB1805 wake reason UNKNOWN - inconclusive, not ruling out AB1805 watchdog");
    } else {
      Log.info("PIN_RESET with AB1805 wake reason=%s - not an AB1805 watchdog reset",
               startupAb1805WakeReasonName);
    }
    startupAb1805ConfirmedWatchdog = ab1805ConfirmedWatchdog;
  }

  ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS); // Enable watchdog

  // ===== WATCHDOG RESET PERSISTENCE / FORENSICS =====
  // Moved here (from immediately after SystemConfig::setup(), before AB1805 init)
  // so it can act on the AB1805 classification above. The publish queue was
  // already initialized above, so this is a reordering, not a new
  // dependency. Covers both the Device-OS-detected case
  // (reason == RESET_REASON_WATCHDOG) and the AB1805-confirmed PIN_RESET
  // case classified above; both reuse the same lastWatchdog* fields, tagged
  // with lastWatchdogSource. An inconclusive or non-watchdog PIN_RESET must
  // not overwrite a previously persisted watchdog's forensic data.
  const bool watchdogClassified = watchdogResetDetected || ab1805ConfirmedWatchdog;
  if (watchdogClassified) {
    const uint16_t priorWatchdogResetCount = RecoveryState::get_watchdogResetCount();
    if (priorWatchdogResetCount < 0xFFFF) {
      RecoveryState::set_watchdogResetCount((uint16_t)(priorWatchdogResetCount + 1));
    }
    RecoveryState::set_lastWatchdogBreadcrumb(previousBreadcrumb);
    RecoveryState::set_lastWatchdogUptimeMs(previousBreadcrumbMs);
    RecoveryState::set_lastWatchdogResetReasonData(reasonData);
    RecoveryState::set_lastWatchdogSource(watchdogResetDetected ? RecoveryState::WATCHDOG_SOURCE_DEVICE_OS : RecoveryState::WATCHDOG_SOURCE_AB1805_PIN);

    // New watchdog-reset alert code (its own tier, strictly above tier-3;
    // see getAlertSeverity() in MyPersistentData.cpp - this must outrank
    // any already-active tier-3 alert like thrash detection or a boot
    // storm). WO-2026-10-06-001: reported once, in the first report after
    // the watchdog reboot, then cleared by isAutoClearAfterReportAlert() so
    // it no longer hides later alerts. The forensics persist elsewhere:
    // watchdogResetCount, the lastWatchdog* fields and the watchdog event.
    RecoveryState::raiseAlert(19);

    // Queue watchdog forensic event only after PublishQueuePosix setup above
    // initialized its mutex.
    publishWatchdogForensics(ab1805ConfirmedWatchdog);
  }

  // wakeVerdict.rtcReadOk/rtcAtWake were computed inside classifyWake()
  // (called above, before setWDT()) - nothing between that call and here
  // touches Time's validity (confirmed: the RtcSkewTest bench hook
  // explicitly does not call Time.setTime(), and neither setWDT() nor the
  // watchdog forensics block touches Time), so sampling Time.isValid() here
  // observes the same transition it always did (seeded by Device OS
  // earlier in setup(), not by this read itself).
  const time_t rtcTime = (time_t)wakeVerdict.rtcAtWake;
  const bool rtcReadOk = wakeVerdict.rtcReadOk;
  const bool timeValidAfterRtc = Time.isValid();
  if (!timeValidBeforeRtc && timeValidAfterRtc) {
    if (rtcReadOk) {
      if (SystemConfig::get_verboseMode()) {
        Log.info("RTC restored system time: %s (rtc=%s)",
                 Time.timeStr().c_str(),
                 Time.format(rtcTime, TIME_FORMAT_DEFAULT).c_str());
      }
    } else if (SystemConfig::get_verboseMode()) {
      Log.info("RTC restored system time: %s (rtc read failed)",
               Time.timeStr().c_str());
    }
  } else if (!timeValidAfterRtc) {
    Log.warn("RTC did not restore time (rtcSet=%s rtcReadOk=%s)",
             ExternalRtc::isSet() ? "true" : "false",
             rtcReadOk ? "true" : "false");
  }

  startupHibernateStatusReady = false;
  startupHibernateWakeReason = "UNKNOWN";
  startupHibernateActualSleepSec = 0;
  startupHibernateSleepErrorSec = 0;
#if PLATFORM_ID == PLATFORM_BORON
  if (wakeVerdict.hibernateGateEvaluated) {
    startupHibernateWakeReason = wakeVerdict.wakeReasonName;

    if (wakeVerdict.gateArm == HibernateWakeDiagnostics::GateArm::kNone) {
      startupHibernateActualSleepSec = wakeVerdict.actualSleepSec;
      startupHibernateSleepErrorSec = wakeVerdict.sleepErrorSec;
      startupHibernateStatusReady = true;

      Log.info("HibernateWake: reason=%s req=%lu actual=%lu err=%ld count=%lu",
               startupHibernateWakeReason,
               (unsigned long)wakeVerdict.requestedSleepSec,
               (unsigned long)startupHibernateActualSleepSec,
               (long)startupHibernateSleepErrorSec,
               (unsigned long)wakeVerdict.hibernateCount);
    } else {
<shellId: 20 completed with exit code 0>
```

</details>


---

<sub>1m 54s</sub>

### `bash`

**Read the two deepPowerDown callers**

`$ sed -n 255,335p src/power/ConnectivityFailsafe.cpp; sed -n 70,200p src/state/State_Error.cpp`

<details>
<summary>160 lines</summary>

```
time_t requiredDelay = ConnectivityPolicy::CONNECTIVITY_FAILSAFE_COOLDOWN_SEC;
  if (nextStage >= 2) {
    requiredDelay += (time_t)connectivityFailsafeJitterSec(nextStage);
  }

  if (currentStage != 0 && lastAction != 0 && (now - lastAction) < requiredDelay) {
    return;
  }

  // WO-2026-10-08-001: a connection is not overdue until one cadence after it
  // was due, so with a cadence of 3 h or more the threshold is cadence + 3 h.
  // Compiled out of the test build, where the 5 min threshold must stay a
  // 5 min threshold for the bench reset.
#if !CONNECTIVITY_FAILSAFE_TEST_MODE
  const time_t cadenceSec = (time_t)ReportingPolicyResolver::resolveRuntime(
      PowerManager::instance().soc(), now).effectiveIntervalSec;
  if (cadenceSec >= ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC &&
      connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC + cadenceSec) {
    return;
  }
#endif

  if (activeConnectAttemptWithinBudget()) {
    return;
  }

  const BatteryTier tier = currentBatteryTierForFailsafe();
  const bool externalPowerPresent = connectivityFailsafeHasExternalPower();
  const bool lowBatteryHardActionBlocked =
      nextStage >= 2 &&
      !externalPowerPresent &&
      (PowerManager::instance().downgradeActive() || tier == TIER_SURVIVAL);

  if (lowBatteryHardActionBlocked) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferLowBatteryHardStageSuppressed(nextStage, tier);
#else
    Log.info("Failsafe: defer reason=low-battery stage=%u tier=%s",
             (unsigned)nextStage,
             batteryTierShortName(tier));
#endif
    persistConnectivityFailsafeState(currentStage, now, false);
    SystemConfig::flushNow();
    return;
  }

  if (nextStage == 2) {
    setAppBreadcrumb(BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD);
    persistConnectivityFailsafeState(2, now, true);
    Log.info("Failsafe: stage=2 action=system-reset age=%lds",
             (long)connectionAgeSec);
    flushAllBeforeReset();
    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);
    return;
  }

  setAppBreadcrumb(BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD);
  persistConnectivityFailsafeState(3, now, true);
  Log.info("Failsafe: stage=3 action=deep-powerdown age=%lds",
           (long)connectionAgeSec);
  flushAllBeforeReset();
  ab1805.deepPowerDown();
}

    time_t now = Time.now();
    if ((now - lastHook) > (3 * 3600L)) {
      Log.info("Alert 40 - no successful webhook response for >3 hours, scheduling soft reset");
      return 2; // soft reset to try to recover integration path
    }

    Log.info("Alert 40 active but webhook response is recent - no reset needed");
    return 0;
  }

  case 16: { // repeated sleep failures (HIBERNATE / ULTRA_LOW_POWER)
    // If both HIBERNATE and ULTRA_LOW_POWER are failing to honour
    // long sleep requests, treat this as a platform-level fault.
    // Start with a soft reset, then escalate to an AB1805 deep
    // power-down, and finally stop resetting to avoid thrash.
    if (resets >= 4) {
      Log.info("Alert 16 with reset count=%u; suppressing further resets", resets);
      return 0;
    }
    if (resets >= 1) {
      return 3; // after first reset, try a hard power-cycle
    }
    return 2;   // start with a soft reset
  }

  case 17: // boot storm detected during setup/early boot
    // Boot-storm logic has already enforced holdoff and surfaced the event.
    // No additional recovery action needed here; just report via webhook.
    Log.info("Alert 17 (boot storm) - holdoff logic already handled; no additional action");
    return 0;

  case 18: // state machine thrash detected by ThrashGuard
    // ThrashGuard has already taken corrective action (tier 2: force sleep, tier 3: reset).
    // No additional recovery action needed here; just report via webhook.
    Log.info("Alert 18 (thrash) - ThrashGuard already handled; no additional action");
    return 0;

  default:
    // Unknown or less critical alert: don't take drastic action here.
    return 0;
  }
}

// ERROR_STATE: Error supervisor: decide recovery action
void flushAllBeforeReset() {
  CurrentReadings::flushNow();
  SystemConfig::flushNow();
  SystemConfig::SensorSettings::flushNow();
  PublishQueuePosix::instance().writeQueueToFiles();
}

void handleErrorState() {
  static unsigned long resetTimer = 0;
  static int resolution = 0;

  if (state != oldState) {
    publishStateTransition();

    // Safety: regardless of recovery choice, do not leave radio/modem powered
    // while we sit in ERROR_STATE waiting for reset.
    Connectivity::requestFullDisconnectAndRadioOff();

    // In INTERMITTENT or DISCONNECTED modes, avoid reset loops for connectivity/sleep alerts.
    if (PowerManager::instance().effectiveConnectionMode() != SystemConfig::CONNECTED) {
      int8_t alert = RecoveryState::get_alertCode();
      if (alert == 15 || alert == 16 || alert == 31) {
        Log.warn("Low-power mode: clearing alert %d to avoid reset loop", alert);
        RecoveryState::set_alertCode(0);
        RecoveryState::set_lastAlertTime(0);
        resolution = 0;
      } else {
        resolution = resolveErrorAction();
      }
    } else {
      resolution = resolveErrorAction();
    }
    Log.info("Entering ERROR_STATE with alert=%d, resetCount=%u, resolution=%d",
             RecoveryState::get_alertCode(), RecoveryState::get_resetCount(), resolution);
    resetTimer = millis();
  }

  switch (resolution) {
  case 0:
    // No automatic recovery; return to IDLE so the normal state machine
    // can continue and we rely on future hourly reports to surface the
    // issue.
    transitionTo(IDLE_STATE, "error-no-recovery");
    break;

  case 2:
    // Soft reset after a short delay to allow any queued publishes to
    // flush.
    if (millis() - resetTimer > resetWait) {
      Log.info("ModemTeardown: radioOn=%d point=error-state-soft-reset", (int)Connectivity::isRadioPoweredOn());
      Log.info("Executing soft reset from ERROR_STATE");
      flushAllBeforeReset();
      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);
    }
    break;

  case 3:
    // Hard recovery using AB1805 deep power down after delay. This fully
    // power-cycles the device and modem but is limited by resolveErrorAction
    // avoid thrashing.
    if (millis() - resetTimer > resetWait) {
      Log.info("Executing deep power down from ERROR_STATE (alert=%d)", RecoveryState::get_alertCode());
      flushAllBeforeReset();
      ab1805.deepPowerDown();
    }
    break;

  default:
    // Should not happen, but don't get stuck here.
    transitionTo(IDLE_STATE, "error-unknown-resolution");
    break;
  }
}
<shellId: 21 completed with exit code 0>
```

</details>


---

<sub>1m 57s</sub>

### `bash`

**Persist failsafe state, resolve error action, layout fields**

`$ grep -n "persistConnectivityFailsafeState" -B2 -A25 src/power/ConnectivityFailsafe.cpp | sed -n 1,70p; sed -n 20,70p src/state/State_Error.cpp; grep -n "ailsafe\|resetCount\|lastAlert\|WATCHDOG_SOURCE\|spare\|reserved\|lastWatchdog" src/MyPersistentData.h | head -50`

<details>
<summary>135 lines</summary>

```
75-}
76-
77:void persistConnectivityFailsafeState(uint8_t stage, time_t actionTime, bool incrementCount) {
78-  RecoveryState::set_connectivityRecoveryStage(stage);
79-  RecoveryState::set_lastConnectivityRecoveryAction(actionTime);
80-  if (incrementCount) {
81-    uint8_t count = RecoveryState::get_connectivityRecoveryCount();
82-    if (count < 0xFF) {
83-      RecoveryState::set_connectivityRecoveryCount(count + 1);
84-    }
85-  }
86-}
87-
88-const char *failsafeDeferReasonNameLocal(FailsafeDeferReason reason) {
89-  switch (reason) {
90-  case FAILSAFE_DEFER_INVALID_TIME:
91-    return "invalid-time";
92-  case FAILSAFE_DEFER_NO_LAST_CONNECTION:
93-    return "no-last-connection";
94-  case FAILSAFE_DEFER_DISCONNECTED_MODE:
95-    return "disconnected-mode";
96-  case FAILSAFE_DEFER_UPDATE_PENDING:
97-    return "update-pending";
98-  case FAILSAFE_DEFER_LOW_BATTERY_HARD_STAGE_SUPPRESSED:
99-    return "low-battery-hard-stage-suppressed";
100-  case FAILSAFE_DEFER_CLOSED_HOURS_LONG_SLEEP:
101-    return "closed-hours-long-sleep";
102-  default:
--
294-             batteryTierShortName(tier));
295-#endif
296:    persistConnectivityFailsafeState(currentStage, now, false);
297-    SystemConfig::flushNow();
298-    return;
299-  }
300-
301-  if (nextStage == 2) {
302-    setAppBreadcrumb(BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD);
303:    persistConnectivityFailsafeState(2, now, true);
304-    Log.info("Failsafe: stage=2 action=system-reset age=%lds",
305-             (long)connectionAgeSec);
306-    flushAllBeforeReset();
307-    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);
308-    return;
309-  }
310-
311-  setAppBreadcrumb(BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD);
312:  persistConnectivityFailsafeState(3, now, true);
313-  Log.info("Failsafe: stage=3 action=deep-powerdown age=%lds",
314-           (long)connectionAgeSec);
315-  flushAllBeforeReset();
316-  ab1805.deepPowerDown();
317-}

// Decide what corrective action to take for the current alert.
//
// Uses the current alert code and resetCount to choose between:
//  - 0: No action (return to IDLE and try again later)
//  - 2: Soft reset via System.reset()
//  - 3: Hard recovery using AB1805 deep power down
//
// The mapping is intentionally conservative to avoid thrashing:
//  - Modem/disconnect failure (15) and connect timeout (31):
//    a couple of soft resets, then a hard power-cycle, then stop.
//  - Sleep failures (16): soft reset, then hard power-cycle, then stop.
static int resolveErrorAction() {
  int8_t alert   = RecoveryState::get_alertCode();
  uint8_t resets = RecoveryState::get_resetCount();

  if (alert <= 0) {
    return 0;
  }

  switch (alert) {
  // Alert 14 (out-of-memory) has no case here: WO-2026-10-02-003 item B made
  // loop() reset directly with RESET_CAUSE_OUT_OF_MEMORY, so nothing raises 14.
  // A 14 persisted by pre-v34 firmware falls to `default` and returns to Idle
  // without resetting - the same outcome the old suppression branch gave.
  case 15: // modem or disconnect failure
  case 31: // failed to connect to cloud
  case 44: // ledger sync timeout before sleep
    if (resets >= 4) {
      Log.info("Connectivity alert %d with reset count=%u; suppressing further resets", alert, resets);
      return 0;
    }
    if (resets >= 2) {
      return 3; // escalate to hard power-cycle after a few soft resets
    }
    return 2;   // start with soft reset

  case 40: { // repeated webhook failures
    // WO-2026-09-19 Step 3b: Clock::isTrusted(), not isTimeValid() - the
    // age check below (now - lastHook) must not run on an untrusted clock.
    if (!Clock::isTrusted()) {
      Log.info("Alert 40 set but clock is untrusted - deferring corrective action");
      return 0;
    }

    time_t lastHook = SystemConfig::get_lastHookResponse();
    if (lastHook == 0) {
      Log.info("Alert 40 set but no recorded lastHookResponse - deferring corrective action");
      return 0;
    }

73:	 * connectivity failsafe stage counters.
121:		uint8_t reservedUint8_0;                   // Reserved: was structuresVersion; kept to preserve the stored layout
124:		bool reservedBool0;                     // Reserved: was lowPowerMode; kept to preserve the stored layout
126:		uint8_t resetCount;                               // reset count of device (0-256)
134:		uint8_t reservedUint8_1;                   // Reserved: was sensorType; kept to preserve the stored layout
135:		bool reservedBool1;                     // Reserved: was updatesPending; kept to preserve the stored layout
137:		bool reservedBool2;                     // Reserved: was disconnectedMode; kept to preserve the stored layout
140:		time_t reservedTime0;                     // Reserved: was verboseModeStartTime; kept to preserve the stored layout
158:		float reservedFloat0;                             // Reserved to preserve persistent layout after battery test removal
160:		uint8_t reservedByte0;                            // Stored as enableHibernateSleep flag (0/1) while preserving persistent layout
167:		time_t lastConnectivityRecoveryAction;           // Last time the connectivity failsafe acted or deferred
168:		uint8_t connectivityRecoveryCount;               // Count of connectivity failsafe recovery actions since last good connect
170:		uint8_t lastWatchdogBreadcrumb;                  // Last retained app breadcrumb observed on a watchdog boot
171:		uint32_t lastWatchdogUptimeMs;                   // millis() value recorded with the last watchdog breadcrumb
172:		uint32_t lastWatchdogResetReasonData;            // System.resetReasonData() from the last watchdog boot
176:		uint8_t lastWatchdogSource;                       // WatchdogSource enum: which mechanism reported the last watchdog reset (DEVICE_OS vs AB1805_PIN)
230:	uint8_t get_resetCount() const;
231:	void set_resetCount(uint8_t value);
330:	 * @brief Returns the last timestamp when the connectivity failsafe acted or deferred.
336:	 * @brief Persists the timestamp of the last connectivity failsafe action or defer.
355:	uint8_t get_lastWatchdogBreadcrumb() const;
356:	void set_lastWatchdogBreadcrumb(uint8_t value);
357:	uint32_t get_lastWatchdogUptimeMs() const;
358:	void set_lastWatchdogUptimeMs(uint32_t value);
359:	uint32_t get_lastWatchdogResetReasonData() const;
360:	void set_lastWatchdogResetReasonData(uint32_t value);
365:	uint8_t get_lastWatchdogSource() const;
366:	void set_lastWatchdogSource(uint8_t value);
658:		uint16_t reservedUint16_0;                  // Reserved: was faceNumber; kept to preserve the stored layout
659:		uint16_t reservedUint16_1;                  // Reserved: was faceScore; kept to preserve the stored layout
660:		uint16_t reservedUint16_2;                  // Reserved: was gestureType; kept to preserve the stored layout
661:		uint16_t reservedUint16_3;                  // Reserved: was gestureScore; kept to preserve the stored layout
664:		float reservedFloat1;                    // Reserved: was externalTempC; kept to preserve the stored layout
666:		time_t lastAlertTime;
725:	time_t get_lastAlertTime() const;
726:	void set_lastAlertTime(time_t value);
<shellId: 22 completed with exit code 0>
```

</details>


---

<sub>2m 7s</sub>

### `bash`

**Recovery struct and stage usage**

`$ sed -n 150,200p src/MyPersistentData.h; grep -n "connectivityRecoveryStage\|WATCHDOG_SOURCE\|set_resetCount\|lastAlertTime" -r src --include=*.cpp --include=*.h | grep -v "^src/MyPersistentData.h" | head -40`

<details>
<summary>84 lines</summary>

```
uint16_t cloudDisconnectBudgetSec;                // Max seconds to wait for cloud disconnect before error
		uint16_t modemOffBudgetSec;                       // Max seconds to wait for modem power-down before error
		
		// ********** Battery-Aware Back-off Configuration **********
		uint8_t currentBatteryTier;                       // Current battery tier (0=HEALTHY, 1=CONSERVING, 2=CRITICAL, 3=SURVIVAL)
		uint8_t connectionAttemptCounter;                 // Counter for periodic deep connection attempts (0-3, resets to 0 after reaching 3)
		
		// ********** Test Mode Overrides **********
		float reservedFloat0;                             // Reserved to preserve persistent layout after battery test removal
		uint16_t testConnectionDurationOverride;          // Test mode: connection duration override (0xFFFF = disabled, 0-999 = override value in seconds)
		uint8_t reservedByte0;                            // Stored as enableHibernateSleep flag (0/1) while preserving persistent layout

		// ********** Webhook Configuration **********
		char webhookName[64];                             // Webhook event name from cloud config
		bool webhookEnabled;                              // Enable/disable webhook publishing
		uint32_t webhookTimeoutMs;                        // Webhook response timeout in milliseconds
		uint8_t connectivityRecoveryStage;               // 0=none, 1=radio reset, 2=system reset, 3=deep power down
		time_t lastConnectivityRecoveryAction;           // Last time the connectivity failsafe acted or deferred
		uint8_t connectivityRecoveryCount;               // Count of connectivity failsafe recovery actions since last good connect
		uint16_t watchdogResetCount;                     // Count of watchdog-related resets observed across boots
		uint8_t lastWatchdogBreadcrumb;                  // Last retained app breadcrumb observed on a watchdog boot
		uint32_t lastWatchdogUptimeMs;                   // millis() value recorded with the last watchdog breadcrumb
		uint32_t lastWatchdogResetReasonData;            // System.resetReasonData() from the last watchdog boot
		bool hasValidLedgerConfig;                       // True once at least one complete, valid ledger configuration has been applied
		uint8_t configSource;                            // Config::Source enum persisted for diagnostics
		// ***** Records are frozen: no field is appended while a rollback target reads them (WO-2026-10-22-003) *****
		uint8_t lastWatchdogSource;                       // WatchdogSource enum: which mechanism reported the last watchdog reset (DEVICE_OS vs AB1805_PIN)

		// ********** F2a Thermal Charge Inhibit Configuration (WO-2026-08-25-001) **********
		// Per-device ledger-configurable overrides for ChargeInhibitPolicy::ThermalThresholds.
		// initialize() does not run again for already-provisioned devices after these fields
		// were appended, so StorageHelperRK::validate() zero-pads them for a migrated device
		// instead. The get_ accessors resolve the raw stored set AS A WHOLE via
		// ChargeInhibitPolicy::isValidThermalThresholds() (see
		// sysStatusData::resolveThermalThresholds()) and fall back to the complete compiled
		// default set (37/0/35/3) atomically when the stored set is invalid - a zero-padded
		// {0,0,0,0} record fails this (armHighC > releaseHighC is false), while an individual
		// field being 0.0f (armLowC's own default) inside an otherwise-valid set is honoured.
		float thermalChargeArmHighC;                     // Arm (inhibit) at/above this temperature, C (default 37.0)
		float thermalChargeArmLowC;                       // Arm (inhibit) at/below this temperature, C (default 0.0)
		float thermalChargeReleaseHighC;                  // Release (resume) once at/below this temperature, C (default 35.0)
		float thermalChargeReleaseLowC;                   // Release (resume) once at/above this temperature, C (default 3.0)

	};

	SysData sysData;

	// 	******************* Get and Set Functions for each variable in the storage object ***********
    
	/**
	 * @brief For the Get functions, used to retrieve the value of the variable
src/MyPersistentData.cpp:107:        if (sysStatus.get_connectivityRecoveryStage() > 3) {
src/MyPersistentData.cpp:108:            Log.info("data not valid connectivity recovery stage =%d", sysStatus.get_connectivityRecoveryStage());
src/MyPersistentData.cpp:150:    sysStatus.set_connectivityRecoveryStage(0);
src/MyPersistentData.cpp:159:    sysStatus.set_lastWatchdogSource((uint8_t)RecoveryState::WATCHDOG_SOURCE_DEVICE_OS);
src/MyPersistentData.cpp:187:void sysStatusData::set_resetCount(uint8_t value) {
src/MyPersistentData.cpp:389:uint8_t sysStatusData::get_connectivityRecoveryStage() const {
src/MyPersistentData.cpp:390:    return getValue<uint8_t>(offsetof(SysData,connectivityRecoveryStage));
src/MyPersistentData.cpp:392:void sysStatusData::set_connectivityRecoveryStage(uint8_t value) {
src/MyPersistentData.cpp:393:    setValue<uint8_t>(offsetof(SysData,connectivityRecoveryStage), value);
src/MyPersistentData.cpp:649:  sysStatus.set_resetCount(0);                                          // Reset the reset count as well
src/MyPersistentData.cpp:756:time_t currentStatusData::get_lastAlertTime() const {
src/MyPersistentData.cpp:757:    // lastAlertTime is stored as time_t in CurrentData; retrieve with correct type
src/MyPersistentData.cpp:758:    return getValue<time_t>(offsetof(CurrentData,lastAlertTime));
src/MyPersistentData.cpp:761:void currentStatusData::set_lastAlertTime(time_t value) {
src/MyPersistentData.cpp:762:    setValue<time_t>(offsetof(CurrentData,lastAlertTime),value);
src/MyPersistentData.cpp:821:        // reads get_lastAlertTime() as `lastEscalation` and treats
src/MyPersistentData.cpp:829:        set_lastAlertTime(Time.now());
src/MyPersistentData.cpp:1042:void set_resetCount(uint8_t value) { sysStatus.set_resetCount(value); }
src/MyPersistentData.cpp:1044:uint8_t get_connectivityRecoveryStage() { return sysStatus.get_connectivityRecoveryStage(); }
src/MyPersistentData.cpp:1045:void set_connectivityRecoveryStage(uint8_t value) { sysStatus.set_connectivityRecoveryStage(value); }
src/MyPersistentData.cpp:1074:time_t get_lastAlertTime() { return current.get_lastAlertTime(); }
src/MyPersistentData.cpp:1075:void set_lastAlertTime(time_t value) { current.set_lastAlertTime(value); }
src/power/PmicFaultMonitor.cpp:181:          RecoveryState::set_lastAlertTime(0);
src/power/PmicFaultMonitor.cpp:379:        RecoveryState::set_lastAlertTime(0);
src/power/PmicFaultMonitor.cpp:388:      RecoveryState::set_lastAlertTime(0);
src/power/ConnectivityFailsafe.cpp:78:  RecoveryState::set_connectivityRecoveryStage(stage);
src/power/ConnectivityFailsafe.cpp:136:      RecoveryState::get_connectivityRecoveryStage() != 0 ||
src/power/ConnectivityFailsafe.cpp:145:  RecoveryState::set_connectivityRecoveryStage(0);
src/power/ConnectivityFailsafe.cpp:152:    RecoveryState::set_lastAlertTime(0);
src/power/ConnectivityFailsafe.cpp:237:  uint8_t currentStage = RecoveryState::get_connectivityRecoveryStage();
src/state/State_Connect.cpp:563:        RecoveryState::set_lastAlertTime(0);
src/state/State_Report.cpp:144:          time_t lastEscalation = RecoveryState::get_lastAlertTime();
src/state/State_Report.cpp:148:            // Repurpose lastAlertTime as our escalation timestamp for alert 40.
src/state/State_Report.cpp:149:            RecoveryState::set_lastAlertTime(now);
src/state/State_Error.cpp:139:        RecoveryState::set_lastAlertTime(0);
src/Generalized-Core-Counter.cpp:546:    // Same shared lastAlertTime field/consumer as currentStatusData::
src/Generalized-Core-Counter.cpp:550:    RecoveryState::set_lastAlertTime(Time.now());
src/Generalized-Core-Counter.cpp:584:    RecoveryState::set_lastAlertTime(0);
src/Generalized-Core-Counter.cpp:607:    RecoveryState::set_lastAlertTime(0);
src/Generalized-Core-Counter.cpp:620:    RecoveryState::set_resetCount(RecoveryState::get_resetCount() + 1);
<shellId: 23 completed with exit code 0>
```

</details>


---

<sub>2m 11s</sub>

### `bash`

**Setup reset-reason handling**

`$ sed -n 520,640p src/Generalized-Core-Counter.cpp`

<details>
<summary>111 lines</summary>

```
// the first coupled measureTemperatureAndApplyChargeDecision() call
  // below, which re-establishes the DCT from a REAL measurement in the same
  // synchronous span of code - see that call site for the rest of this
  // rationale. Nothing between here and there reads
  // PowerManager::instance()'s report/capabilities (verified: the
  // constructor already populates report_.capabilities independently of
  // setup(), and no other PowerManager::instance() call site exists in this
  // range).

  if (SystemConfig::get_hasValidLedgerConfig()) {
    Config::markStorageConfigurationLoaded();
  } else {
    Config::markFactoryDefaultsActive();
  }

  // Watchdog-reset persistence/forensics-publish (Device-OS-detected and
  // AB1805-confirmed PIN_RESET cases) is handled after AB1805 wake-reason
  // classification below, once the AB1805 wake reason is available -
  // see the "WATCHDOG RESET CLASSIFICATION" block further down in setup().

  // If a boot storm holdoff was triggered on this or the prior boot, surface
  // it as an alert now that persistent current status storage is initialized.
  if (bootStormAlertPending) {
    // Force explicit boot-storm alert visibility in startup/report payloads.
    RecoveryState::set_alertCode(17);
    // WO-2026-08-29-002 item 8: deliberately NOT gated on isClockTrusted().
    // Same shared lastAlertTime field/consumer as currentStatusData::
    // raiseAlert() (MyPersistentData.cpp) - see that function's comment for
    // why writing a 0 sentinel here would incorrectly bypass State_Report.cpp's
    // alert-40 escalation cooldown.
    RecoveryState::set_lastAlertTime(Time.now());
    bootStormAlertPending = false;
    Log.warn("Boot storm holdoff detected - raising alert 17");
  }

  // Configure serial logging based on serial flag
  // Note: Particle firmware on this platform doesn't support Log.level()
  if (SystemConfig::get_serialConnected() || (ALLOW_BLOCKING_SERIAL_WAITS != 0)) {
    Serial.begin(9600);

    // Honor the persisted serialConnected flag with a bounded wait so
    // USB serial can attach before a low-power device drops back to sleep.
    // In developer builds, the build profile can force this wait even when
    // cloud config has not enabled serial logging yet.
    const unsigned long serialWaitStart = millis();
    while (!Serial.isConnected() && (millis() - serialWaitStart) < ConnectivityPolicy::DEBUG_SERIAL_WAIT_TIMEOUT_MS) {
      serviceAwakeWatchdog();
      Particle.process();
      delay(ConnectivityPolicy::DEBUG_SERIAL_WAIT_POLL_DELAY_MS);
    }

    if (Serial.isConnected()) {
      delay(ConnectivityPolicy::DEBUG_SERIAL_POST_CONNECT_DELAY_MS);
    }
  }

  if (SystemConfig::get_testConnectionDurationOverride() == 0) {
    SystemConfig::set_testConnectionDurationOverride(0xFFFF);  // Disabled (max uint16_t)
  }

  // Testing: clear sticky sleep-failure alert to avoid reset/deep-power loops.
  if (RecoveryState::get_alertCode() == 16) {
    Log.info("Clearing alert 16 on boot");
    RecoveryState::set_alertCode(0);
    RecoveryState::set_lastAlertTime(0);
  }

  // Clear sticky OOM alert after a reset-driven recovery attempt so field
  // reports can distinguish a one-off OOM from repeated reset/OOM cycles.
  bool clearOomAlertOnBoot = false;
  switch (reason) {
  case RESET_REASON_PIN_RESET:
  case RESET_REASON_USER:
  case RESET_REASON_WATCHDOG:
    clearOomAlertOnBoot = true;
    break;
#ifdef RESET_REASON_USER_APPLICATION
  case RESET_REASON_USER_APPLICATION:
    clearOomAlertOnBoot = true;
    break;
#endif
  default:
    break;
  }
  if (RecoveryState::get_alertCode() == 14 && clearOomAlertOnBoot) {
    Log.info("Clearing alert 14 on boot after reset-driven recovery");
    RecoveryState::set_alertCode(0);
    RecoveryState::set_lastAlertTime(0);
  }

  // Track how often the device has been resetting so the error supervisor
  // can apply backoffs and avoid permanent reset loops. Only count resets
  // that are likely to be recoverable by firmware (pin/user/watchdog).
  switch (reason) {
  case RESET_REASON_PIN_RESET:
  case RESET_REASON_USER:
  case RESET_REASON_WATCHDOG:
#ifdef RESET_REASON_USER_APPLICATION
  case RESET_REASON_USER_APPLICATION:
#endif
    RecoveryState::set_resetCount(RecoveryState::get_resetCount() + 1);
    break;
  case RESET_REASON_UPDATE:
    // After OTA firmware update, force connection to reload
    // configuration from ledger. This ensures device-settings
    // (operatingMode, etc.) override any stale FRAM values.
    Log.info("OTA update detected - forcing connection to reload config");
    transitionTo(CONNECTING_STATE, "OTA update");
    break;
  case RESET_REASON_POWER_MANAGEMENT:
    // Waking from sleep. Alert 40 suppression for overnight hibernate
    // is handled later in setup() after timezone configuration is complete.
    break;
  default:
    break;
  }

  // Ensure sensor-board LED power default matches configured sensor type
  // TODO: Consider moving this sensor-specific logic to device_pinout.cpp or SensorManager
  pinMode(ledPower, OUTPUT);
  SensorType configuredType = static_cast<SensorType>(SystemConfig::SensorSettings::get_sensorType());
<shellId: 24 completed with exit code 0>
```

</details>


---

<sub>2m 23s</sub>

### `bash`

**Search docs for P7 retained evidence**

`$ grep -rIl "P7" docs | head; grep -rIn -i "retained" docs/work-orders/WO-2026-11-01-001-ab1805-native-driver-spike.md | head -20`

<details>
<summary>13 lines</summary>

```
docs/work-orders/WO-2026-10-22-002-step7-persistence-layout-step0.md
docs/work-orders/WO-2026-10-22-003-bench-dev14-prestate.md
docs/work-orders/WO-2026-11-05-003-stage6-copilot-transcript.md
docs/work-orders/2026-10-27-v47-release-and-soak.md
docs/work-orders/WO-2026-10-22-003-stage6-copilot-dispatch.md
docs/work-orders/WO-2026-11-04-002-v49-binding-delete-guard.md
docs/work-orders/WO-2026-10-31-001-step0-report.md
docs/work-orders/WO-2026-11-04-002-stage6-copilot-transcript.md
docs/work-orders/WO-2026-11-01-001-ab1805-native-driver-spike.md
docs/work-orders/WO-2026-10-22-003-stage7-round2-verdict.md
docs/work-orders/WO-2026-11-01-001-ab1805-native-driver-spike.md:31:- A copy of the then-current `main` with `AB1805_RK` removed from the wake, time, watchdog and power-off paths and the native driver in its place, only as far as each proof needs. Diagnostics may be `Log` lines; the payload, `retained` layout and persisted layout stay untouched.
docs/work-orders/WO-2026-11-01-001-ab1805-native-driver-spike.md:96:- `retained` RAM and the persisted layout are untouched.
<shellId: 25 completed with exit code 0>
```

</details>


---

<sub>2m 33s</sub>

### `bash`

**Gather remaining line cites**

`$ grep -n "AB1805_RK\|ab1805\.\|ab1805)" -r src | grep -v "^src/.*:\s*//" | grep -v "^\S*:[0-9]*:\s*\*" | cut -c1-140 | head -50; sed -n 15,100p src/time/HibernateEligibility.cpp | head -110; sed -n 1136,1148p src/Generalized-Core-Counter.cpp; git status --short | head`

<details>
<summary>100 lines</summary>

```
src/power/ConnectivityFailsafe.cpp:316:  ab1805.deepPowerDown();
src/state/StateMachine.h:4:#include "AB1805_RK.h"
src/state/State_Sleep.cpp:21:#include "AB1805_RK.h"
src/state/State_Sleep.cpp:699:          if (ab1805.interruptAtTime(wakeTime)) {
src/state/State_Sleep.cpp:709:            const bool watchdogStopped = ab1805.stopWDT();
src/state/State_Sleep.cpp:742:            ab1805.resumeWDT();
src/state/State_Sleep.cpp:785:  const bool watchdogStopped = ab1805.stopWDT();
src/state/State_Sleep.cpp:898:        ab1805.resumeWDT();
src/state/State_Sleep.cpp:926:  ab1805.resumeWDT();
src/state/State_Error.cpp:13:#include "AB1805_RK.h"
src/state/State_Error.cpp:178:      ab1805.deepPowerDown();
src/time/HibernateCycle.cpp:4:#include "AB1805_RK.h"
src/time/HibernateWakeDiagnostics.h:42:  kRtcRead,        // ab1805.getRtcAsTime() failed
src/time/ClockDiagnostics.h:2:#include "AB1805_RK.h"
src/time/HibernateEligibility.cpp:31:  if (!ab1805.readRegister(kRegIntMask, intMask)) {
src/time/HibernateEligibility.cpp:36:  if (!ab1805.writeRegister(kRegStatus, 0x00)) {
src/time/HibernateEligibility.cpp:41:  if (!ab1805.writeRegister(kRegIntMask, intMask)) {
src/time/HibernateEligibility.cpp:45:  if (!ab1805.maskRegister(
src/Generalized-Core-Counter.cpp:47:#include "AB1805_RK.h"               // hardware watchdog (S2) and power-off (S3)
src/Generalized-Core-Counter.cpp:758:  const HibernateCycle::WakeVerdict wakeVerdict = HibernateCycle::classifyWake(reason, ab1805);
src/Generalized-Core-Counter.cpp:778:  ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS); // Enable watchdog
src/Generalized-Core-Counter.cpp:1143:  ab1805.loop(); // Keeps the RTC synchronized with the device clock
src/Generalized-Core-Counter.cpp:1144:  checkClockResync(); // WO-2026-08-29-002: recurring app-side RTC write-back, not gated by ab1805.loo

  if (major > 6) {
    return true;
  }
  if (major < 6) {
    return false;
  }
  return minor >= 4;
}

bool clearAb1805StaleAlarmInterrupts() {
  static const uint8_t kRegStatus = 0x0f;
  static const uint8_t kRegIntMask = 0x12;
  static const uint8_t kMaskAieTie = 0x0c;

  uint8_t intMask = 0;
  if (!ab1805.readRegister(kRegIntMask, intMask)) {
    return false;
  }

  // Clear stale interrupt flags before arming the next one-shot alarm.
  if (!ab1805.writeRegister(kRegStatus, 0x00)) {
    return false;
  }

  intMask = (uint8_t)(intMask & (uint8_t)~kMaskAieTie);
  if (!ab1805.writeRegister(kRegIntMask, intMask)) {
    return false;
  }

  if (!ab1805.maskRegister(
        AB1805::REG_TIMER_CTRL,
        (uint8_t)~AB1805::REG_TIMER_CTRL_RPT_MASK,
        AB1805::REG_TIMER_CTRL_RPT_DIS)) {
    return false;
  }

  return true;
}

bool shouldUseBoronRtcAlarmHibernate(uint32_t requestedSleepSec, time_t &rtcNow) {
  Log.info("HibernateDiag: check enabled=%d requested=%lu",
           SystemConfig::get_enableHibernateSleep() ? 1 : 0,
           (unsigned long)requestedSleepSec);
  if (!SystemConfig::get_enableHibernateSleep()) {
    Log.info("HibernateDiag: fail=disabled");
    return false;
  }

  if (!isDeviceOsAtLeast6400()) {
    Log.info("HibernateDiag: fail=device_os");
    return false;
  }

  static const uint32_t kMinHibernateSleepSec = 900;
  static const uint32_t kMaxHibernateSleepSec = 36000;
  if (requestedSleepSec < kMinHibernateSleepSec || requestedSleepSec > kMaxHibernateSleepSec) {
    Log.info("HibernateDiag: fail=duration requested=%lu", (unsigned long)requestedSleepSec);
    return false;
  }

  if (!ExternalRtc::isSet()) {
    Log.info("HibernateDiag: fail=rtc_not_set");
    return false;
  }

  if (!ExternalRtc::get(rtcNow)) {
    Log.info("HibernateDiag: fail=rtc_read");
    return false;
  }

  if (!Clock::isPlausibleEpoch(rtcNow)) {
    Log.info("HibernateDiag: fail=rtc_invalid epoch=%ld", (long)rtcNow);
    return false;
  }

  Log.info("HibernateDiag: pass epoch=%ld", (long)rtcNow);
  return true;
}
#endif
    break;
  }
  noteLoopStageDuration(false);

  setLoopStage(LOOP_STAGE_DIAGNOSTICS);
  thrashGuard.loop(state, millis());

  ab1805.loop(); // Keeps the RTC synchronized with the device clock
  checkClockResync(); // WO-2026-08-29-002: recurring app-side RTC write-back, not gated by ab1805.loop()'s per-boot latch

  // Housekeeping for each transit of the main loop
  CurrentReadings::loop();
  SystemConfig::loop();
<shellId: 26 completed with exit code 0>
```

</details>


---

<sub>3m 8s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-004-stage6-copilot-report.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-004-stage6-copilot-report.md`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-004-stage6-copilot-report.md with 9505 characters
```


---

<sub>3m 12s</sub>

### Copilot

I stopped at Step 0 and made no `src/` edits, so net `src/` change is 0. I ran no tests, build or `nm`, and committed nothing. The report is in `docs/work-orders/WO-2026-11-05-004-stage6-copilot-report.md`.

Two stop conditions apply:

1. **`retained` is lost at a reason-20 power-up.** A power-down clears it, so the "reason 20 inside a due window" rule can never be true. A native HIBERNATE keeps RAM and wakes with reason 30, so S2's own sleep path never needs that rule. The report proposes that reason 20 be classified from persisted evidence only.
2. **The Error-state hard path persists nothing usable.** Writing a spare byte for it means editing `State_Error.cpp`, which is an S3 `deepPowerDown()` caller and not authorised here. Failsafe stage 3 is usable, but it needs a recency check.

The other Step 0 answers are cited with file:line in the report:
- HIBERNATE with `duration()` arms the chip alarm, and mixing it with GPIO wakes works.
- `ExternalWatchdog` can represent 124 s, and the app must stop it before each sleep, as it does today.
- No public watchdog readback exists, so Amendment 1 option (b) applies.

The architect's answers to the three open questions in the report are what S2 is waiting on.

I could not observe the reasoning level actually run; it was dispatched at medium.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>
