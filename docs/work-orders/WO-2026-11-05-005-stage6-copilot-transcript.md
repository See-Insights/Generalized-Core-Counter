# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `5b7aef15-34dc-462c-b32c-26fd77dac471`  
> - **Started:** 11/7/2026, 7:24:42 PM  
> - **Duration:** 27m 59s  
> - **Exported:** 11/7/2026, 7:52:42 PM  

---

<sub>7s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-05-005-stage6-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>9s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/docs/work-orders/WO-2026-11-05-005-stage6-copilot-dispatch.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/docs/work-orders/WO-2026-11-05-005-stage6-copilot-dispatch.md`

<details>
<summary>13 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/` (the helper beside the failsafe in `src/power/`, its header, `src/power/ConnectivityFailsafe.cpp`, `src/state/State_Error.cpp`, and removal of the AB1805 include/extern use at those two callers), tests under `tests/` (new S3 tests; existing failsafe/sleep/deepPowerDown tests and `tests/stubs/` only as the change forces), and write the report; run host tests, the local Device OS toolchain build and nm/objdump; scratch `build-tmp/wo-2026-11-05-005-stage6/` (create it; use only that) / NOT authorized: committing, pushing, merging, flashing, creating PRs or tags, touching `lib/`, S1/S1b/S2 behaviour (the sentinel write, the boot clear, `HibernateWakeDiagnostics`), the failsafe decision logic/cooldowns/thresholds, the reset-cause enum, changing the persisted or `retained` layout, deleting anything you did not create (including `build-tmp/` itself).

# Stage 6 dispatch: WO-2026-11-05-005 (v50 piece S3, timed power-off)
Binding spec: `docs/work-orders/WO-2026-11-05-005-v50-s3-timed-power-off.md` **including Amendments 1-3 (read Amendment 3 first)** and the parent `docs/work-orders/WO-2026-11-05-002-v50-ab1805-switch.md`. Base: this branch (`integration/v50-ab1805` at 62e3dbc: S1, S1b and S2 merged; line numbers in the WO are at v49.1: re-check every one). **Budget: `src/` at most +20 net** (removals count); stop and report the cut rather than trim style. Leave changes uncommitted. **Run at the dispatched reasoning level (medium); the report states the level actually run, or that it could not be observed.** This is power/failsafe logic: be conservative.

## Do
1. **Step 0 first** (WO items 1-6 plus Amendment 3 item 3): read-only, answered in the report with file:line against Device OS 6.6.0 (`~/.particle/toolchains/deviceOS/6.6.0`) and the vendored `lib/AB1805_RK`. **Stop and report if any answer breaks the plan** (notably: POWER_OFF with a duration is not available on this Device OS/board, USB attached prevents power-off, the watchdog interaction, or a caller needs a layout change).
2. Implement the change: one shared helper (smallest home, beside the failsafe) building `SystemSleepConfiguration` `POWER_OFF` with the off time (today's vendored value or a named constant) and **no GPIO wake**; it **returns** on any non-success (logging the error); both callers keep their existing log lines verbatim, their `flushAllBeforeReset()`, their S2 after-return block (clear evidence, flush; stage 3 also downgrades), and then call `System.reset(<cause>)` as the fallback. Remove `#include "AB1805_RK.h"` and the `ab1805` use at the two callers (keep the object definition: L deletes it).
3. **Tests and mutations** per the WO: both callers reach the helper once, after the flush, in order; `POWER_OFF` + duration + no GPIO wake; return/ABORTED/other error -> the after-return clear then `System.reset(cause)`, never back into the caller's loop; verbatim log lines; failsafe tests unchanged; no `deepPowerDown`/`ab1805.` left in `src/` outside the object definition. Mutations (each must fail; back up, mutate, restore from the backup; never `git checkout`/`git restore` while your work is uncommitted): flush after the sleep; fallback removed; GPIO wake added; duration zero; wrong cause; reset before the evidence clear.
4. **Verify (workflow §10, binding):** run CI's discovery exactly: `tests/*.sh` via zsh and `tests/*.py` via python3 (count `p=$((p+1))`, never `((p++))`; report `N/N (sh via zsh, py via python3)`; baseline at the integration head is 120/120); any helper you add under `tests/` must exit 0 when run bare; `thermal_coupling_structural_test.py` also on a clean export of tracked files; local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE` as a make argument): text/data/bss (integration head: 151508 / 1114 / 2240) and `nm` showing the helper linked; `git diff --numstat 62e3dbc -- src` net <= +20 (state the number).
## Stop and report if
the budget cannot hold; Step 0 breaks the plan; the persisted-field/evidence logic of S2 would have to change; `lib/` or S1/S1b/S2 behaviour would have to change.
## Report
`docs/work-orders/WO-2026-11-05-005-stage6-copilot-report.md`: model and reasoning level actually used, interpreter line, Step 0 answers, net `src/` lines per file, tests and mutation results, build and `nm` evidence, deviations, open questions. The bench (Chip, from 14 Nov) is not yours: claim nothing about hardware.
```

</details>


---

<sub>12s</sub>

### `bash`

**Read the WO**

`$ cd docs/work-orders && ls && wc -c WO-2026-11-05-005-v50-s3-timed-power-off.md && cat WO-2026-11-05-005-v50-s3-timed-power-off.md`

<details>
<summary>15 lines</summary>

```
Output too large to read at once (38.4 KB). Saved to: /var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794059694465-copilot-tool-output-79628-e7474eaf-3221-4c15-8d87-72dc3b57e84d.txt
Consider using tools like grep (for searching), head/tail (for viewing start/end), view with view_range (for specific sections), or jq (for JSON) to examine portions of the output.

Preview (first 500 chars):
2026-09-29-config-inventory-codex-dispatch.md
2026-09-29-config-inventory-codex-report.md
2026-10-01-connectivity-investigation-codex-dispatch.md
2026-10-01-connectivity-investigation-codex-report.md
2026-10-02-wake-heap-loss-codex-dispatch.md
2026-10-02-wake-heap-loss-codex-report.md
2026-10-04-pre-step6-known-issues-codex-dispatch.md
2026-10-04-pre-step6-known-issues-codex-report.md
2026-10-06-step6-ownership-codex-dispatch.md
2026-10-06-step6-ownership-codex-report.md
2026-10-07-step6-extensi
<shellId: 0 completed with exit code 0>
```

</details>


---

<sub>14s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/docs/work-orders/WO-2026-11-05-005-v50-s3-timed-power-off.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/docs/work-orders/WO-2026-11-05-005-v50-s3-timed-power-off.md`

<details>
<summary>49 lines</summary>

```
# WO-2026-11-05-005: v50 piece S3 — timed power-off (failsafe stage 3 and the ERROR_STATE hard recovery) on the built-in AM18x5 driver

**Author:** Claude Code, 5 Nov 2026 · **Status:** DISPATCHED 7 Nov 2026 (after S2 #201 merged, 62e3dbc); Amendment 3 below is binding where it differs
**Parent:** `WO-2026-11-05-002-v50-ab1805-switch.md` (Plan B, rulings of 5 Nov) · **Branch (when dispatched):** `wo/…-v50-s3-…` stacked on `integration/v50-ab1805`, from `main` after v49.1 is the product default and C1/C2 have merged (S3 depends on S1 only; S2 is independent of it)
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (failsafe and power logic: Codex xhigh); §12; §13 (c), (j) · **Agents:** Stage 6 Copilot standard/medium; Stage 7 Codex `gpt-5.6-sol` **xhigh**, at most two rounds · **Budget:** `src/` at most **+20 net** (removals count); stop and report rather than trim style.
**Dispatch gate:** only once v49.1 is the product default and C1/C2 have merged (architect, 5 Nov). **S3 never merges to `main` alone** (merge as a set with S1 and S2).

## Plain goal
The two "last resort" recoveries cut the board's power and let the chip bring it back: failsafe stage 3 (no cloud connection for a long time) and the ERROR_STATE hard recovery. Both call the vendored `ab1805.deepPowerDown()`. S3 replaces both with Device OS's own timed power-off (`System.sleep()` in `SystemSleepMode::POWER_OFF` with a duration), after the same `flushAllBeforeReset()`, and falls back to `System.reset()` if the native call returns. (**Not a known defect:** spike P7 showed the vendored call does power down on Dev-14, with reason 20 on the way back. The built-in driver adds the protected key sequence and the shared M-SoM path.) After S3 the only `ab1805` references left are in the object's definition, which L deletes.

## Rulings that bind this piece (parent WO; not reopened)
1. **POWER_OFF with a duration** (parent S3): a native abort returns `ABORTED`, never a silent success; the abort falls back to `System.reset()` with the existing reset cause for that caller.
2. **The post-`POWER_OFF` wake reason is unobserved** (architect, 5 Nov, ruling 5): S3's bench must **record it and confirm it before v50.1 releases**; S2's classification table is corrected if it differs from the assumption (reason 20 inside a due window).
3. **Failsafe ladder, cooldowns and thresholds unchanged.** Stage 3 still follows the same decision path; only the final action changes.

## The change (lines at the v49.1 tag; re-check every one at Step 0)
1. **Stage 3.** `ConnectivityFailsafe.cpp:316` `ab1805.deepPowerDown()` (after `persistConnectivityFailsafeState(3, …)`, the "Failsafe: stage=3 action=deep-powerdown" log and `flushAllBeforeReset()`, `:309-315`) → a small shared helper (one place, both callers use it): build `SystemSleepConfiguration` with `mode(SystemSleepMode::POWER_OFF).duration(<off time>)`, call `System.sleep(cfg)`, and on any return log the error and `System.reset(<caller's cause>)`. Stage 3 uses a new cause only if the architect rules one; otherwise `RESET_CAUSE_CONNECTIVITY_FAILSAFE` (the stage-2 value), distinguished by the persisted stage 3 and a log line.
2. **ERROR_STATE hard recovery.** `State_Error.cpp:178` `ab1805.deepPowerDown()` (after `flushAllBeforeReset()`, `:177`) → the same helper; the fallback reset uses `RESET_CAUSE_ERROR_STATE_SOFT` (or a ruled new value).
3. **Off time.** What the vendored call used (`deepPowerDown()` default, seconds) is read at Step 0; the native duration is that value, or a constant named beside the helper. **The off time must be long enough for the rail to collapse and the modem to drop** (the point of the recovery) and short enough that a device is not dark for long; Step 0 reports today's value and the P7 observation (120 s).
4. **The helper's home.** The smaller of: a function in an existing power file beside the failsafe (`power/…`) declared in a header both callers include; no new file unless Step 0 shows a test needs it. The `#include "AB1805_RK.h"` in `State_Error.cpp:13` and the `extern AB1805 ab1805` use at the callers go.
5. **Test-mode hook.** The #151 TEST_MODE (stage 2 → 3) must still be able to drive stage 3 on the bench; whatever it uses to skip or shorten delays is kept, and the helper's duration is the real one unless the hook asks otherwise (Step 0 lists the hook points).

## Step 0 (read-only, inside Stage 6, before editing; stop and report if any answer breaks the plan)
1. **`POWER_OFF` on 6.6.0:** `SystemSleepMode::POWER_OFF` requirements (the bound AM18x5, the minimum and maximum duration, whether `duration()` is seconds or ms), the exact PWGT key sequence the driver writes (file:line in `hal/shared/am18x5.cpp`), what makes it return (`ABORTED`: which conditions — a pending wake source, USB power, a battery fault?), and the 6.5.0 race fix (#2927). **Whether power is cut at all while USB is attached**: on a USB-powered bench unit the rail may not collapse; state what the vendored call did and what P7 showed (the LED went dark on USB: the AB1805 cuts the nPWR path).
2. **Which wake sources are armed:** does `POWER_OFF` accept `.gpio()` wake sources (the button) or only the RTC? We want the RTC only (a recovery must not be cut short by noise) unless the architect rules otherwise.
3. **The vendored baseline:** `deepPowerDown()` in `lib/AB1805_RK/src/AB1805_RK.cpp`: the off time, the register sequence, what it does on an I2C failure (returns? loops?), so "at least as good as the version replaced" is stated against the real behaviour. The `retained`/persisted effect of a power-down (P7: `retained` and the RTC lost) is the same under the native call: state which fields the failsafe needs afterward (`failsafeStage`, last-connection) and that they are persisted, not `retained`.
4. **Callers and tests:** every caller and every test that mentions `deepPowerDown`/`deep-powerdown` (bare-name search, §13 (c)); the log strings the fleet checks (`Failsafe: stage=3 action=deep-powerdown`, `Executing deep power down from ERROR_STATE`) stay verbatim.
5. **Reset-reason handling on the way back:** how the code at boot treats reason 20 after S2 (inside a due wake window → wake; otherwise a pin reset); a power-off recovery has **no hibernate window pending**, so it arrives as a pin reset: confirm that is harmless (the `RESET_REASON_PIN_RESET` branch, `watchdogClassified`, the forensic event) and that it is **not** counted as a chip-watchdog reset under S2's attribution ruling.
6. **Count.** The net `src/` lines per file; if over +20, stop and report the cut.

## Tests (host; the real functions)
Fakes for `System.sleep` returning success, `ABORTED` and other errors: both callers reach the helper exactly once, after `flushAllBeforeReset()` (structural, in order); the helper builds `POWER_OFF` with the intended duration and no GPIO wake; an error/abort falls back to `System.reset(<cause>)`, never returns silently to the caller's loop; the verbatim log lines; the failsafe stage progression, cooldowns and the low-battery deferral unchanged (the existing failsafe tests keep their expectations); no `deepPowerDown`/`ab1805.` reference remains in `src/` outside the object definition. Mutations (each must fail): the flush after the sleep; the fallback removed; a GPIO wake added; the duration zero; the wrong cause on the fallback. `nm` on the local ELF for linkage.

## Bench (Chip, from 14 Nov; **not benched alone**; the parent WO's pre-ship item 4)
The **#151 TEST_MODE stage 2 → 3** run, watching the power rail, the modem, the off time and the reset reason after the wake; then the ERROR_STATE hard recovery. Record: the off time actually measured, the reset reason after the wake (**the open ruling-5 observation**), the RTC state after the wake (valid or lost, and the first-sync time), and that the next status is a normal boot. Pass: the board is dark for the intended time (±5 s), comes back, connects, and the failsafe state clears as it does today. If the reason is not 20, S2's table changes before release.

## Protected / not in scope
S1, S2 (alarm, watchdog, classification), L, I21; the failsafe decision logic, cooldowns and thresholds, the ERROR_STATE ladder before stage 3, `flushAllBeforeReset()` itself, the reset-cause enum (unless a new value is ruled), M-SoM.

## Rollback
Revert the S3 commit on the integration branch before the set merges. After the set ships, rollback is to v49.1 (vendored `deepPowerDown()`).

## Amendment 1 (follows the S2 attribution ruling, 5 Nov 2026) — status: ruled, with S2 Amendment 1
Both callers write the **persisted "power-off requested" record** (failsafe or recovery record, not `retained`) and flush it in the same `flushAllBeforeReset()` that precedes the call; S2's `inferred` watchdog rule reads it to keep a power-off recovery from being counted as a watchdog reset. Step 0 item 5 confirms the record's home and that the boot clears it after classification. The helper's fallback `System.reset()` clears it too. Counted in the +20 budget; if it does not fit, stop and report the cut.

## Amendment 2 (architect, 5 Nov 2026) — no new flag; derive the record from existing fields
Replaces the "persisted power-off record" wording of Amendment 1: the stage-3 caller relies on the failsafe's already-persisted **stage and action** (written by `persistConnectivityFailsafeState(3, now, true)` before the call), and the ERROR_STATE caller on the Error state's **recorded hard-reset action**. **No new flag.** Step 0 (item 5) finds, per caller, the exact persisted field and confirms it is flushed before the power-off and cleared (or superseded) once the boot has classified the wake. If a caller persists nothing usable, **use an existing spare byte**; **only if none exists, stop and ask.** Counted in the +20 budget.

## Grok follow-up V3 (Chip, 6 Nov 2026): the "power-off requested" evidence
Grok's V3 (`docs/reviews/2026-11-grok-progress/grok-response-2.md`): deriving the evidence from existing persisted fields works only if every path that can reach the power-off has already written a surviving field and flushed it; a path that does not (or whose write is lost in the 250 ms window) will be counted as an inferred watchdog. Ruled **already gated** (no added scope). The gates are:
- **Amendment 2 (and S2 Amendment 1 item 2 and S2 Amendment 2):** Step 0 **names the exact persisted field and its file:line for each caller** (stage 3 and the ERROR_STATE hard recovery); if a path persists nothing usable, **use an existing spare byte (no layout change); only if there is none, stop and ask**; `retained` RAM is never evidence; the mutation that reads `retained` must fail;
- **the flush site:** both callers call `flushAllBeforeReset()` before the power-off (`ConnectivityFailsafe.cpp:~315`, `State_Error.cpp:177`). That call forces the save (`flushNow()`), it does not rely on the deferred 100/250 ms save. **Step 0 confirms for each caller that its field is written before that call**, which is the "250 ms window" case Grok raised.
- **Step 0 item 5** (reset-reason handling on the way back).

## Amendment 3 (Claude Code, 7 Nov 2026, from S2's merged state at 62e3dbc and S2 Amendments 3-4; supersedes the evidence wording of Amendments 1-2 and V3 where it differs)
S2 (#201) is merged. The evidence for a reason-20 boot is now **already written by S2**, so S3 changes only the action:
1. **Do not repeat the evidence write.** `State_Error.cpp` already sets `RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel)` (0xA5, exact match) before `flushAllBeforeReset()`; the stage-3 caller relies on the persisted failsafe stage 3 with a 600 s recency check on a valid clock. Both are read by `HibernateWakeDiagnostics::isInferredWatchdog()` and cleared at the next boot with an immediate flush. Leave all of that unchanged.
2. **The two S2 after-return lines stay.** Both callers currently have an after-return block (State_Error: clear the byte, `flushNow()`; ConnectivityFailsafe: clear the byte, set the stage to 2, `flushNow()`) because the old `deepPowerDown()` "should not return". Under the native call the same holds: if `System.sleep()` returns (any error or `ABORTED`), the evidence must be cleared and flushed **before** the fallback `System.reset(<cause>)`. So the helper **returns** on failure (it never resets inside), the caller keeps its after-return block, and then calls `System.reset(<cause>)` (stage 3: `RESET_CAUSE_CONNECTIVITY_FAILSAFE`; ERROR_STATE: `RESET_CAUSE_ERROR_STATE_SOFT`; no new cause). A mutation that resets before the clear must fail.
3. **Reason 20 and the chip watchdog.** S2 runs `ExternalWatchdog` (124 s) and stops it around sleep. Step 0 must state, for `POWER_OFF`, (a) whether the watchdog must be stopped before the call (the MCU is off; what the chip does with a running watchdog while the rail is cut) and (b) the off time versus the 124 s window: a device must not be reset by a still-armed watchdog during or just after the off period, and the first boot after the wake must re-init the watchdog as S2 does.
4. **Reason 30 / 20 table.** The bench must record the post-`POWER_OFF` reset reason (open observation, parent ruling 5). S2's table (reason 20 with the persisted evidence -> power-up wake; without -> `inferred` watchdog) is corrected before release if the reason differs.
5. **Test-suite rule (workflow §10).** The local suite runs CI's discovery: `tests/*.sh` via zsh and `tests/*.py` via python3 (not `*_test.*`); any helper under `tests/` must exit 0 when run bare; confirm CI green on the PR before saying "ready".
6. **Budget unchanged: `src/` at most +20 net** (S2's baseline is the integration head). Stop and report the cut if it does not fit.
```

</details>


---

<sub>17s</sub>

### `bash`

**Locate callers and files**

`$ cd . && git status --short | head; git log --oneline | head -3; grep -rn "deepPowerDown\|deep-powerdown\|ab1805\|AB1805" src --include=* | grep -v "^lib" | head -40; ls src/power tests | head -150`

<details>
<summary>193 lines</summary>

```
67134b6 WO-2026-11-05-005 (S3): dispatch with Amendment 3 (post-S2 state); Stage 6 Copilot dispatch
62e3dbc Merge pull request #201 from See-Insights/wo/2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake
787179d WO-2026-11-05-004 (S2): CI runs every tests/*.py; the transcript normaliser helper exits 0 when run bare (test-side only)
src/power/ConnectivityFailsafe.cpp:313:  Log.info("Failsafe: stage=3 action=deep-powerdown age=%lds",
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
src/state/StateMachine.h:29:/// AB1805 RTC/watchdog shared across state handlers. Sleep configurations are
src/state/StateMachine.h:31:extern AB1805 ab1805;
src/state/State_Common.h:225: *          SYNC's future->get()) - the same hazard the vendored AB1805_RK
src/state/State_Error.cpp:26://  - 3: Hard recovery using AB1805 deep power down
src/state/State_Error.cpp:84:    // Start with a soft reset, then escalate to an AB1805 deep
src/state/State_Error.cpp:172:    // Hard recovery using AB1805 deep power down after delay. This fully
src/state/State_Error.cpp:179:      ab1805.deepPowerDown();
src/time/HibernateWakeDiagnostics.h:22: *          No Particle/AB1805 dependency, so this compiles and tests on the
src/time/HibernateWakeDiagnostics.h:50:/// `wakeReasonIsHibernateWake` from Device-OS/AB1805 types before populating this,
src/time/RtcSkewTest.h:11: *          This header has zero Particle/AB1805 dependencies (no
src/time/RtcSkewTest.h:12: *          Particle.h, AB1805_RK.h, or other project headers - only
src/time/RtcSkewTest.h:107: *          `retained` RAM survives an MCU reset - soft reset, AB1805
src/time/RtcSkewTest.h:114: *          - that clears the latch along with the AB1805 RTC. This is
src/time/RtcSkewTest.h:133: *          Per Amendment A.3, the hook writes the AB1805 RTC only -
src/time/RtcSkewTest.h:134: *          `ab1805.setRtcFromTime()` - and does not also call
src/time/RtcSkewTest.h:355:// A safe, in-range anchor to skew from when the AB1805's own pre-hook
src/time/RtcSkewTest.h:362:// Below this, a raw AB1805 reading is treated as not sane enough to anchor
src/time/RtcSkewTest.h:374:// Picks the anchor to skew from: the raw pre-hook AB1805 reading if it
src/time/PreBindRtc.h:2:// v50 S1b (after Particle.h, time/Clock.h): before enable() on an unbound boot, seed Time from the chip and set WRTC. L: replace the AB1805_RK calls with raw I2C.
src/time/Clock.cpp:79:// that ever wrote the RTC in this build, `AB1805::loop()`, is latched by a
src/time/Clock.cpp:82:// every later correction for that boot. `AB1805::setRtcFromSystem()`
src/time/Clock.cpp:83:// (`lib/AB1805_RK`, not modified here - vendored) exists for exactly this
src/time/Clock.cpp:97:// its only path to `ab1805.setRtcFromSystem()` for that sync - silently
src/time/Clock.cpp:172: *          lib/AB1805_RK/src/AB1805_RK.cpp:51-53 documents this exact hazard
src/time/Clock.cpp:178: *          could delay the watchdog refresh past the 60s app / 124s AB1805
src/time/Clock.cpp:282: *          gated by `AB1805::loop()`'s per-boot `timeSet` latch, and picks
src/time/Clock.cpp:292: *          ONLY after `ab1805.setRtcFromSystem()` returns true. Previously it
src/time/Clock.cpp:308: *          a PERMANENT AB1805/Wire fault would retry `setRtcFromSystem()`
src/time/Clock.cpp:341: *          the vendored `AB1805::loop()` (lib/AB1805_RK, untouched) has its
src/power:
BatteryAuthority.cpp
BatteryAuthority.h
BatteryAuthorityCommand.cpp
BatteryAuthorityPolicy.cpp
BatteryAuthorityPolicy.h
BatteryHealth.cpp
BatteryHealth.h
ChargeInhibit.cpp
ChargeInhibit.h
ChargeInhibitPolicy.h
Connectivity.h
ConnectivityFailsafe.cpp
ConnectivityFailsafe.h
ConnectivityPolicy.h
ModemHealth.cpp
ModemHealth.h
PmicFaultMonitor.cpp
PmicFaultMonitor.h
PowerDiagnostics.cpp
PowerDiagnostics.h
PowerManager.cpp
PowerManager.h
PowerPlatform.cpp
PowerPlatform.h
PowerTier.cpp
PowerTier.h

tests:
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
<shellId: 1 completed with exit code 0>
```

</details>


---

<sub>20s</sub>

### `bash`

**View callers**

`$ sed -n 1,40p src/power/ConnectivityFailsafe.cpp; echo ------; sed -n 260,345p src/power/ConnectivityFailsafe.cpp; echo -----; cat src/power/ConnectivityFailsafe.h | head -80; echo ----; sed -n 1,30p src/state/State_Error.cpp; sed -n 150,215p src/state/State_Error.cpp`

<details>
<summary>163 lines</summary>

```
#include "power/ConnectivityFailsafe.h"
#include "state/StateMachine.h"
#include "power/ConnectivityPolicy.h"
#include "power/PowerPlatform.h"
#include "power/BatteryAuthority.h"
#include "diagnostics/ConnectivityFailsafeTest.h"
#include "observability/LoopForensics.h"
#include "reporting/ReportingPolicy.h"
#include "time/DailyBoundary.h"
#include "persist/PowerConfig.h"
#include "ResetCause.h"
namespace {
const char *batteryTierShortName(BatteryTier tier) {
  switch (tier) {
  case TIER_HEALTHY:
    return "H";
  case TIER_CONSERVING:
    return "C";
  case TIER_CRITICAL:
    return "CR";
  case TIER_SURVIVAL:
    return "S";
  default:
    return "?";
  }
}

BatteryTier currentBatteryTierForFailsafe() {
  const uint8_t tierValue = PowerConfig::get_currentBatteryTier();
  if (tierValue <= TIER_SURVIVAL) {
    return static_cast<BatteryTier>(tierValue);
  }
  // WO-2026-09-21 Step 4 (corrected same day, twice):
  // BatteryAuthority::evaluateCurrent() - the pure guarded pipeline (vcell
  // floor + trust substitution), fed by the one input-gathering path all
  // read paths share - not the deleted, unguarded Cloud::calculateBatteryTier().
  // Read-only: this fallback never commits. The failsafe's low-battery block
  // must be reachable only through this guarded path.
  return BatteryAuthority::evaluateCurrent(PowerManager::instance().soc()).tier;
}
------
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
  RecoveryState::set_powerOffRequested(0); RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow(); // not expected to return: no recent stage-3 evidence
}
-----
#pragma once
#include "state/State_Common.h"
void connectivityFailsafeSupervisor();
----
#include "state/State_Common.h"
#include "ResetCause.h"
#include "../Config.h"
#include "cloud/Cloud.h"
#include "LocalTimeRK.h"
#include "persist/RecoveryState.h"
#include "persist/SystemConfig.h"
#include "power/PowerManager.h"
#include "PublishQueuePosixRK.h"
#include "sensors/SensorManager.h"
#include "device_pinout.h"
#include "sensors/SensorDefinitions.h"
#include "power/Connectivity.h"
#include "time/Clock.h"
#include "time/HibernateWakeDiagnostics.h"

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

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
      RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog
      flushAllBeforeReset();
      ab1805.deepPowerDown();
      RecoveryState::set_powerOffRequested(0); SystemConfig::flushNow(); // not expected to return: drop the evidence
    }
    break;

  default:
    // Should not happen, but don't get stuck here.
    transitionTo(IDLE_STATE, "error-unknown-resolution");
    break;
  }
}
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>21s</sub>

### `bash`

**Find references and vendored baseline**

`$ grep -rln "deepPowerDown\|deep-powerdown\|deep power down" tests docs/*.md . --include=* 2>/dev/null | grep -v "^./lib\|^./build\|\.git/" | head -30; ls ~/.particle/toolchains/deviceOS/; grep -n "deepPowerDown" -A60 lib/AB1805_RK/src/AB1805_RK.cpp | head -120`

<details>
<summary>100 lines</summary>

```
tests/deliberate_reset_flush_test.py
tests/connectivity_failsafe_unknown_openness_test.cpp
tests/s2_alarm_watchdog_wake_structural_test.py
tests/watchdog_inferred_classification_test.cpp
tests/modem_teardown_confirmation_logging_test.py
tests/connectivity_failsafe_stage1_retired_test.py
docs/RECOVERY_PLAN_2026-09-26.md
docs/architecture-overview.md
docs/recovery-architecture.md
./tests/deliberate_reset_flush_test.py
./tests/connectivity_failsafe_unknown_openness_test.cpp
./tests/s2_alarm_watchdog_wake_structural_test.py
./tests/watchdog_inferred_classification_test.cpp
./tests/modem_teardown_confirmation_logging_test.py
./tests/connectivity_failsafe_stage1_retired_test.py
./docs/work-orders/WO-2026-11-05-003-stage6-copilot-transcript.md
./docs/work-orders/2026-10-27-v47-release-and-soak.md
./docs/work-orders/WO-2026-11-05-003-stage7-r1-verdict.md
./docs/work-orders/WO-2026-11-05-004-stage6-copilot-report.md
./docs/work-orders/WO-2026-11-03-005-stage6-copilot-report.md
./docs/work-orders/WO-2026-10-24-001-stage7-recheck-codex-dispatch.md
./docs/work-orders/2026-10-04-pre-step6-known-issues-codex-report.md
./docs/work-orders/WO-2026-11-04-002-v49-binding-delete-guard.md
./docs/work-orders/WO-2026-11-05-004-stage6-r3-copilot-transcript.md
./docs/work-orders/WO-2026-11-03-005-stage7-r1-verdict.md
./docs/work-orders/WO-2026-10-31-001-step0-report.md
./docs/work-orders/WO-2026-11-04-002-stage6-copilot-transcript.md
./docs/work-orders/WO-2026-11-01-001-ab1805-native-driver-spike.md
./docs/work-orders/WO-2026-11-05-004-stage7-r2-verdict.md
./docs/work-orders/WO-2026-11-05-004-stage6-r2-copilot-report.md
2.1.0
6.3.4
6.4.1
6.6.0
496:bool AB1805::deepPowerDown(int seconds) {
497:    static const char *errorMsg = "failure in deepPowerDown %d";
498-    bool bResult;
499-
500:    _log.info("deepPowerDown %d", seconds);
501-
502-    // Disable watchdog
503-    bResult = setWDT(0);
504-    if (!bResult) {
505-        _log.error(errorMsg, __LINE__);
506-        return false;
507-    }
508-
509-#ifdef SET_D8_LOW
510-    // With FeatherAB1905v1 board, setting D8 low prior to sleep is necessary
511-    // to prevent current leakage. In V1, D8 is pulled up to 3V3R. In V2 and
512-    // later, it's pulled up to 3V3, so it won't be pulled in sleep and this
513-    // code is not necessary.
514-
515-    // Set Output Control Register 1 (0x30)
516-    // O1EN to 1 to enable FOUT/nIRQ in sleep mode.
517-    bResult = setRegisterBit(REG_OCTRL, REG_OCTRL_O1EN);
518-    if (!bResult) {
519-        _log.error(errorMsg, __LINE__);
520-        return false;
521-    }
522-
523-    // Set OUT in Control1 to 0 so the FOUT/nIRQ pin goes low
524-    bResult = clearRegisterBit(REG_CTRL_1, REG_CTRL_1_OUT);
525-    if (!bResult) {
526-        _log.error(errorMsg, __LINE__);
527-        return false;
528-    }
529-
530-    // Make sure SQW is disabled
531-    bResult = writeRegister(REG_SQW, REG_SQW_DEFAULT);
532-    if (!bResult) {
533-        _log.error(errorMsg, __LINE__);
534-        return false;
535-    }
536-
537-    // Set OUT1S in Control2 to 01 so FOUT/nIRQ is set from SQW or OUT. Since SQW is off, this means OUT only.
538-    // Use this mode so FOUT/nIRQ (D8) won't be affected by the countdown timer nIRQ.
539-    bResult = maskRegister(REG_CTRL_2, ~REG_CTRL_2_OUT1S_MASK, REG_CTRL_2_OUT1S_SQW);
540-    if (!bResult) {
541-        _log.error(errorMsg, __LINE__);
542-        return false;
543-    }
544-#endif
545-
546-    bResult = setCountdownTimer(seconds, false);
547-    if (!bResult) {
548-        _log.error(errorMsg, __LINE__);
549-        return false;
550-    }
551-
552-    // Make sure STOP (stop clocking system is 0, otherwise sleep mode cannot be entered)
553-    // PWR2 = 1 (low resistance power switch)
554-    // (also would probably work with PWR2 = 0, as nIRQ2 should be high-true for sleep mode)
555-    bResult = maskRegister(REG_CTRL_1, (uint8_t)~(REG_CTRL_1_STOP | REG_CTRL_1_RSP), REG_CTRL_1_PWR2);
556-    if (!bResult) {
557-        _log.error(errorMsg, __LINE__);
558-        return false;
559-    }
560-
<shellId: 3 completed with exit code 0>
```

</details>


---

<sub>24s</sub>

### `bash`

**Vendored tail, Device OS POWER\_OFF grep**

`$ sed -n 560,640p lib/AB1805_RK/src/AB1805_RK.cpp; grep -n "deepPowerDown" lib/AB1805_RK/src/AB1805_RK.h; D=~/.particle/toolchains/deviceOS/6.6.0; ls $D | head; grep -rn "POWER_OFF" $D/hal/src/nRF52840/sleep_hal.cpp $D/hal/src/nRF52840/*.cpp 2>/dev/null | head -20`

<details>
<summary>90 lines</summary>

```
// Disable the I/O interface in sleep
    bResult = setRegisterBit(REG_OSC_CTRL, REG_OSC_CTRL_PWGT);
    if (!bResult) {
        _log.error(errorMsg, __LINE__);
        return false;
    }

    // OUT2S = 6 to enable sleep mode
    bResult = maskRegister(REG_CTRL_2, (uint8_t)~REG_CTRL_2_OUT2S_MASK, REG_CTRL_2_OUT2S_SLEEP);
    if (!bResult) {
        _log.error(errorMsg, __LINE__);
        return false;
    }

    // Enter sleep mode and set nRST low
    bResult = writeRegister(REG_SLEEP_CTRL, REG_SLEEP_CTRL_SLP | REG_SLEEP_CTRL_SLRES);
    if (!bResult) {
        _log.error(errorMsg, __LINE__);
        return false;
    }

    // _log.trace("delay in case we didn't power down");   
    unsigned long start = millis();
    while(millis() - start < (unsigned long) (seconds * 1000)) {
        _log.info("REG_SLEEP_CTRL=0x%2x", readRegister(REG_SLEEP_CTRL));
        delay(1000);
    }

    _log.error("didn't power down");
    System.reset();

    return true;
}

bool AB1805::setTrickle(uint8_t diodeAndRout) {
    static const char *errorMsg = "failure in setTrickle %d";
    bool bResult;

    // Set the key register to enable writes to the trickle regster.
    // Automatically resets to 0 so no need to clear it afterwards
    bResult = writeRegister(REG_CONFIG_KEY, REG_CONFIG_KEY_OTHER);
    if (!bResult) {
        _log.error(errorMsg, __LINE__);
        return false;
    }

    uint8_t regValue = ((diodeAndRout != 0) ? REG_TRICKLE_TCS_ENABLE | diodeAndRout : 0x00);

    bResult = writeRegister(REG_TRICKLE, regValue);
    if (!bResult) {
        _log.error(errorMsg, __LINE__);
        return false;
    }

    return true;
}

bool AB1805::checkVBAT(uint8_t mask, bool &isAbove) {
    static const char *errorMsg = "failure in checkVBAT %d";
    bool bResult;

    isAbove = false;

    // mask Either REG_ASTAT_BBOD (compare againt BREF) or REG_ASTAT_BMIN (compare against minimum, 1.2V)
    uint8_t trickleValue;
    bResult = readRegister(REG_TRICKLE, trickleValue);
    if (!bResult) {
        _log.error(errorMsg, __LINE__);
        return false;
    }

    if (trickleValue != 0) {
        // Disable trickle before checking voltage
        setTrickle(0);

        // Do we need a delay here?   
    }

    uint8_t aStatus;
    bResult = readRegister(REG_ASTAT, aStatus);
24:        DEEP_POWER_DOWN,    //!< The deepPowerDown() function was used (RTC was in sleep mode)
325:    bool deepPowerDown(int seconds = 30);
328:     * @brief Used internally by interruptCountdownTimer and deepPowerDown.
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
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:737:        if (mode != HAL_SLEEP_MODE_POWER_OFF) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:745:    if (mode == HAL_SLEEP_MODE_POWER_OFF) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:758:    if (mode == HAL_SLEEP_MODE_HIBERNATE || mode == HAL_SLEEP_MODE_POWER_OFF) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:763:    if (mode == HAL_SLEEP_MODE_HIBERNATE || mode == HAL_SLEEP_MODE_POWER_OFF) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:1348:    if (config->mode == HAL_SLEEP_MODE_POWER_OFF) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:1530:        case HAL_SLEEP_MODE_POWER_OFF: {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:737:        if (mode != HAL_SLEEP_MODE_POWER_OFF) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:745:    if (mode == HAL_SLEEP_MODE_POWER_OFF) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:758:    if (mode == HAL_SLEEP_MODE_HIBERNATE || mode == HAL_SLEEP_MODE_POWER_OFF) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:763:    if (mode == HAL_SLEEP_MODE_HIBERNATE || mode == HAL_SLEEP_MODE_POWER_OFF) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:1348:    if (config->mode == HAL_SLEEP_MODE_POWER_OFF) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp:1530:        case HAL_SLEEP_MODE_POWER_OFF: {
<shellId: 4 completed with exit code 0>
```

</details>


---

<sub>28s</sub>

### `bash`

**Read POWER\_OFF HAL**

`$ D=~/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/sleep_hal.cpp; sed -n 725,775p $D; sed -n 1335,1370p $D; sed -n 1520,1600p $D`

<details>
<summary>102 lines</summary>

```
case CHANGE: {
            break;
        }
        default: {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
    }
    if (gpio->type == HAL_PIN_TYPE_RTC) {
#if HAL_PLATFORM_EXTERNAL_RTC
        if (gpio->mode == CHANGE) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (mode != HAL_SLEEP_MODE_POWER_OFF) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        return SYSTEM_ERROR_NONE;
#else
        return SYSTEM_ERROR_NOT_SUPPORTED;
#endif
    }
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
<shellId: 5 completed with exit code 0>
```

</details>


---

<sub>32s</sub>

### `bash`

**Read enterPowerOffMode and am18x5 sleep**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; sed -n 1295,1342p $D/hal/src/nRF52840/sleep_hal.cpp; grep -n "SLEEP\|PWGT\|KEY\|ABORT\|USB\|Sleep" $D/hal/shared/am18x5.cpp | head -80; grep -rn "EXTERNAL_RTC" $D/hal/inc/hal_platform.h $D/hal/inc/*boron* $D/hal/shared/*.h 2>/dev/null | head; grep -n "POWER_OFF" -B3 -A12 $D/wiring/inc/spark_wiring_system.h | head -40`

<details>
<summary>87 lines</summary>

```
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
55:        HAL_EXRTC_CAPS_SLEEP |
268:        case HAL_EXRTC_COMMAND_SLEEP: {
320:    if (exrtcConfigFlags_ & HAL_EXRTC_CONFIG_SLEEP_EXTI_CHECK) {
321:        exrtcConfig.flags |= HAL_EXRTC_CONFIG_SLEEP_EXTI_CHECK;
563:    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_SOFTWARE_RESET));
602:    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
647:    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
1008:    // CONFIG_KEY_PRIMARY enables access to BATMODE_IO and OUTPUT_CTRL registers
1009:    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_PRIMARY));
1017:    // 1, the WDI input is disabled when the AM18X5 is in Sleep Mode
1023:        // 0, the EXTI input is enabled when the AM18X5 is in Sleep Mode
1027:            CHECK(writeRegister(Am18x5Register::SLEEP_CONTROL, 1, false, true, SLEEP_CONTROL_EX1P_MASK, SLEEP_CONTROL_EX1P_SHIFT));
1029:            CHECK(writeRegister(Am18x5Register::SLEEP_CONTROL, 0, false, true, SLEEP_CONTROL_EX1P_MASK, SLEEP_CONTROL_EX1P_SHIFT));
1032:        // 1, the EXTI input is disabled when the AM18X5 is in Sleep Mode
1037:    // CONFIG_KEY resets on each write, redo for OUTPUT_CTRL
1038:    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_PRIMARY));
1046:    // Set PSW/nIRQ2 to be working in SLEEP mode as the power switch output.
1050:    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
1051:    CHECK(writeRegister(Am18x5Register::OSC_CONTROL, 1, false, true, OSC_CONTROL_PWGT_MASK, OSC_CONTROL_PWGT_SHIFT));
1085:    // Read SLEEP_CONTROL to maintain config but set the SLP bit for immediate sleep when written back out.
1087:    CHECK(readRegister(Am18x5Register::SLEEP_CONTROL, &sleepControl));
1088:    sleepControl |= (1 << SLEEP_CONTROL_SLP_SHIFT);
1107:        if ((exrtcConfigFlags_ & HAL_EXRTC_CONFIG_SLEEP_EXTI_CHECK) && config->exti_polarity != Am18x5ExtiPolarity::NONE) {
1113:                return SYSTEM_ERROR_ABORTED;
1117:        // Transfer to SLEEP state without any delay
1118:        CHECK(writeRegister(Am18x5Register::SLEEP_CONTROL, sleepControl));
1125:     * should continue to poll it until the Sleep actually occurs, in order to handle the case where a trigger occurs
1126:     * before the AM18X5AB18XX enters Sleep Mode.
1129:    CHECK(readRegister(Am18x5Register::SLEEP_CONTROL, &isSlpSet, false, SLEEP_CONTROL_SLP_MASK, SLEEP_CONTROL_SLP_SHIFT));
1131:        return SYSTEM_ERROR_ABORTED;
1134:        CHECK(readRegister(Am18x5Register::SLEEP_CONTROL, &isSlpSet, false, SLEEP_CONTROL_SLP_MASK, SLEEP_CONTROL_SLP_SHIFT));
1137:    return SYSTEM_ERROR_ABORTED;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/inc/hal_platform.h:404:#ifndef HAL_PLATFORM_EXTERNAL_RTC
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/inc/hal_platform.h:405:#define HAL_PLATFORM_EXTERNAL_RTC (0)
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/inc/hal_platform.h:406:#endif // HAL_PLATFORM_EXTERNAL_RTC
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/inc/hal_platform.h:408:#ifndef HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/inc/hal_platform.h:409:#define HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL (1)
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/inc/hal_platform.h:410:#endif // HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/inc/hal_platform.h:412:#ifndef HAL_PLATFORM_EXTERNAL_RTC_CAL_XT
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/inc/hal_platform.h:413:#define HAL_PLATFORM_EXTERNAL_RTC_CAL_XT (-45)
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/inc/hal_platform.h:414:#endif // HAL_PLATFORM_EXTERNAL_RTC_CAL_XT
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/am18x5.h:23:#if HAL_PLATFORM_EXTERNAL_RTC
<shellId: 6 completed with exit code 0>
```

</details>


---

<sub>35s</sub>

### `bash`

**Read am18x5 sleep code**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; sed -n 1000,1140p $D/hal/shared/am18x5.cpp; grep -n "define CONFIG_KEY" -r $D/hal/shared/am18x5*; grep -rn "HAL_PLATFORM_EXTERNAL_RTC" $D/hal/src/nRF52840/hal_platform_config.h $D/hal/inc/hal_platform_*.h $D/hal/network/ncp 2>/dev/null| head -5; grep -rn "HAL_PLATFORM_EXTERNAL_RTC\b" $D/build/platform-config* $D/hal/src/boron $D/hal/src/b5som 2>/dev/null | head`

<details>
<summary>128 lines</summary>

```
uint8_t oscControl = 0x00;
        CHECK(readRegister(Am18x5Register::OSC_CONTROL, &oscControl));
        if (!(oscControl & OSC_CONTROL_OSEL_MASK) && !(oscControl & OSC_CONTROL_FOS_MASK)) {
            // If we're using XTAL and RC fallback is not enabled, we will end up in infinite sleep if OSC failure occurs.
            return SYSTEM_ERROR_INVALID_STATE;
        }
    }

    // CONFIG_KEY_PRIMARY enables access to BATMODE_IO and OUTPUT_CTRL registers
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_PRIMARY));
    // If 1, the AM18X5 will not disable the I/O interface even if VCC goes away and VBAT is still present.
    // This allows external access while the AM18X5 is powered by VBAT.
    CHECK(writeRegister(Am18x5Register::BATMODE_IO, 0x00));

    uint8_t outputCtrl = 0x00;
    uint8_t intMask = 0xE0;

    // 1, the WDI input is disabled when the AM18X5 is in Sleep Mode
    outputCtrl |= OUTPUT_CTRL_WDDS_MASK;

    if (config->exti_polarity != Am18x5ExtiPolarity::NONE) {
        // 1, the EXTI input is enabled when the AM18X5 is powered from VBAT
        outputCtrl |= OUTPUT_CTRL_EXBM_MASK;
        // 0, the EXTI input is enabled when the AM18X5 is in Sleep Mode
        outputCtrl &= ~OUTPUT_CTRL_EXDS_MASK;
        intMask |= INTERRUPT_EX1E_MASK;
        if (config->exti_polarity == Am18x5ExtiPolarity::RISING) {
            CHECK(writeRegister(Am18x5Register::SLEEP_CONTROL, 1, false, true, SLEEP_CONTROL_EX1P_MASK, SLEEP_CONTROL_EX1P_SHIFT));
        } else {
            CHECK(writeRegister(Am18x5Register::SLEEP_CONTROL, 0, false, true, SLEEP_CONTROL_EX1P_MASK, SLEEP_CONTROL_EX1P_SHIFT));
        }
    } else {
        // 1, the EXTI input is disabled when the AM18X5 is in Sleep Mode
        outputCtrl |= OUTPUT_CTRL_EXDS_MASK;
        intMask &= ~INTERRUPT_EX1E_MASK;
    }

    // CONFIG_KEY resets on each write, redo for OUTPUT_CTRL
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_PRIMARY));
    // Configure RTC pins to minimize power leakage
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
    } else {
        intMask &= ~INTERRUPT_AIE_MASK;
    }

    // Read SLEEP_CONTROL to maintain config but set the SLP bit for immediate sleep when written back out.
    uint8_t sleepControl = 0x00;
    CHECK(readRegister(Am18x5Register::SLEEP_CONTROL, &sleepControl));
    sleepControl |= (1 << SLEEP_CONTROL_SLP_SHIFT);

    // Read status to clear the interrupt flags
    uint8_t status = 0x00;
    CHECK(readRegister(Am18x5Register::STATUS, &status));
    // Enable interrupt
    CHECK(writeRegister(Am18x5Register::INT_MASK, intMask));

    /*
     * Critical window between checking EXTI for sleep abort and entering sleep needs to be minimized to handle
     * possible race condition around EXTI as AM1805 enters sleep. AM1805 appears to handle this case but ONLY
     * when the EXTI transition occurs within a very narrow window prior.
     * Acquire lock and disable threading to eliminate unexpected delay from task switch extending the window.
     */
    WITH_LOCK(*this)
    {
        SINGLE_THREADED_SECTION();

        // Optionally require the EXTI wake input to be inactive before arming sleep.
        if ((exrtcConfigFlags_ & HAL_EXRTC_CONFIG_SLEEP_EXTI_CHECK) && config->exti_polarity != Am18x5ExtiPolarity::NONE) {
            uint8_t exin;
            CHECK(readRegister(Am18x5Register::EXTENSION_RAM_ADDRESS, &exin));
            bool isExtiHigh = exin & EXTENSION_RAM_EXIN_MASK;
            if((config->exti_polarity == Am18x5ExtiPolarity::RISING && isExtiHigh) ||
                (config->exti_polarity == Am18x5ExtiPolarity::FALLING && !isExtiHigh)) {
                return SYSTEM_ERROR_ABORTED;
            }
        }

        // Transfer to SLEEP state without any delay
        CHECK(writeRegister(Am18x5Register::SLEEP_CONTROL, sleepControl));
    }

    /*
     * In addition, SLP cannot be set if there is an interrupt pending. Software should read the SLP bit after
     * attempting to set it. If SLP is not asserted, the attempt to set SLP was unsuccessful either because a
     * correct trigger was not enabled or because an interrupt was already pending. Once SLP is set, software
     * should continue to poll it until the Sleep actually occurs, in order to handle the case where a trigger occurs
     * before the AM18X5AB18XX enters Sleep Mode.
     */
    uint8_t isSlpSet = 0;
    CHECK(readRegister(Am18x5Register::SLEEP_CONTROL, &isSlpSet, false, SLEEP_CONTROL_SLP_MASK, SLEEP_CONTROL_SLP_SHIFT));
    if (!isSlpSet) {
        return SYSTEM_ERROR_ABORTED;
    }
    do {
        CHECK(readRegister(Am18x5Register::SLEEP_CONTROL, &isSlpSet, false, SLEEP_CONTROL_SLP_MASK, SLEEP_CONTROL_SLP_SHIFT));
    } while (isSlpSet);

    return SYSTEM_ERROR_ABORTED;
}

int Am18x5::setHundredths(uint8_t hundredths) const {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/am18x5_defines.h:179:#define CONFIG_KEY_SOFTWARE_RESET   0x3C
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/am18x5_defines.h:180:#define CONFIG_KEY_PRIMARY          0x9D
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/am18x5_defines.h:181:#define CONFIG_KEY_OSC_CONTROL      0xA1
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/boron/hal_platform_config.h:103:#define HAL_PLATFORM_EXTERNAL_RTC (1)
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/b5som/hal_platform_config.h:49:#define HAL_PLATFORM_EXTERNAL_RTC (1)
<shellId: 7 completed with exit code 0>
```

</details>


---

<sub>40s</sub>

### `bash`

**More Step 0 evidence**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; grep -n "^int Am18x5::.*[Ss]leep\|isWatchdogStarted\|disableWatchdog" $D/hal/shared/am18x5.cpp | head; sed -n 262,275p $D/hal/shared/am18x5.cpp; sed -n 985,1000p $D/hal/shared/am18x5.cpp; grep -n "2927\|power.off\|POWER_OFF\|ABORT" -i $D/CHANGELOG.md | head; grep -rn "POWER_OFF" $D/system/src/*.cpp $D/wiring/src/*.cpp | head; grep -rln "ExternalWatchdog" src | head`

<details>
<summary>45 lines</summary>

```
928:int Am18x5::disableWatchdog() const {
958:bool Am18x5::isWatchdogStarted() const {
991:int Am18x5::sleep(const am18x5_sleep_config_t* config) {
1053:    bool watchdogWasStarted = isWatchdogStarted();
1055:        CHECK(disableWatchdog());
    return begin();
}

int Am18x5::command(hal_exrtc_command_t cmd, void* arg, uint32_t arg1) {
    (void)arg1;
    switch (cmd) {
        case HAL_EXRTC_COMMAND_SLEEP: {
            auto params = static_cast<hal_exrtc_sleep_config_t*>(arg);
            CHECK_TRUE(params && params->size >= sizeof(uint16_t) * 2, SYSTEM_ERROR_INVALID_ARGUMENT);
            am18x5_sleep_config_t config = {};
            config.version = HAL_EXRTC_API_VERSION;
            config.size = sizeof(config);
            config.duration = params->duration;
            switch (params->exti_mode) {
    CHECK(writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_LKO2_MASK, OSC_STATUS_LKO2_SHIFT));
    CHECK(writeRegister(Am18x5Register::CONTROL1, val, false, true, CONTROL1_OUTB_MASK, CONTROL1_OUTB_SHIFT));
    LOG(TRACE, "PSW set to %d", val);
    return SYSTEM_ERROR_NONE;
}

int Am18x5::sleep(const am18x5_sleep_config_t* config) {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
    if ((config->exti_polarity == Am18x5ExtiPolarity::NONE) && (config->duration == 0)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if ((config->exti_polarity == Am18x5ExtiPolarity::NONE) && (config->duration > 0)) {
        // Only the Alarm can wake up the system
        uint8_t oscControl = 0x00;
91:- Fix race condition on entering `POWER_OFF` sleep mode with AM18x5 (external RTC) [#2927](https://github.com/particle-iot/device-os/pull/2927)
102:- [third_party] `lwip`: `SO_LINGER` fixes (abort on close with zero linger timeout, do not abort sockets in LISTEN state), per-pcb RX frame counter for adaptive LCP echo, map `ERR_IF` to `EIO`, `tcp_pcb` double-free fix [#2932](https://github.com/particle-iot/device-os/pull/2932)
147:- `POWER_OFF` sleep mode support through AM18x5 [#2906](https://github.com/particle-iot/device-os/pull/2906)
1387:- [Electron] Fix Sleep 2.0 APIs taking up to 10 minutes to power-off the cellular modem while it's attempting network registration [#2284](https://github.com/particle-iot/device-os/pull/2284)
1422:- [Electron] Fix Sleep 2.0 APIs taking up to 10 minutes to power-off the cellular modem while it's attempting network registration [#2284](https://github.com/particle-iot/device-os/pull/2284)
1638:- [Electron] Fix Sleep 2.0 APIs taking up to 10 minutes to power-off the cellular modem while it's attempting network registration [#2284](https://github.com/particle-iot/device-os/pull/2284)
1681:- [Electron] Fix Sleep 2.0 APIs taking up to 10 minutes to power-off the cellular modem while it's attempting network registration [#2284](https://github.com/particle-iot/device-os/pull/2284)
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/system/src/main.cpp:268:    LED_SIGNAL_START(POWER_OFF, CRITICAL);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/system/src/system_led_signal.cpp:78:    DEFAULT_SIGNAL_DATA(GRAY, SOLID, NORMAL) // LED_SIGNAL_POWER_OFF
src/state/State_Sleep.cpp
src/Generalized-Core-Counter.cpp
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>44s</sub>

### `bash`

**Look at ExternalWatchdog and sleep sites**

`$ grep -n "ExternalWatchdog\|System.sleep\|SystemSleepConfiguration\|POWER_OFF\|HIBERNATE" src/state/State_Sleep.cpp src/Generalized-Core-Counter.cpp | head -40; grep -rn "class ExternalWatchdog" src lib | head -3; D=~/.particle/toolchains/deviceOS/6.6.0; grep -n "HAL_SLEEP_MODE_POWER_OFF\|POWER_OFF" -r $D/system/src/system_sleep.cpp $D/wiring/inc/spark_wiring_system_sleep*.h $D/wiring/src/spark_wiring_system_sleep*.cpp 2>/dev/null| head; ls $D/wiring/inc | grep -i sleep`

<details>
<summary>35 lines</summary>

```
src/state/State_Sleep.cpp:700:        SystemSleepConfiguration hibernateConfig;
src/state/State_Sleep.cpp:701:        hibernateConfig.mode(SystemSleepMode::HIBERNATE)
src/state/State_Sleep.cpp:706:        const bool watchdogStopped = (ExternalWatchdog.stop() == SYSTEM_ERROR_NONE);
src/state/State_Sleep.cpp:709:          Log.warn("SleepCall: watchdog-stop failed mode=HIBERNATE dur=%ds", wakeInSeconds);
src/state/State_Sleep.cpp:717:        Log.info("Sleep: HIBERNATE reason=closed dur=%ds wakePin=%d", wakeInSeconds, (int)WAKEUP_PIN);
src/state/State_Sleep.cpp:724:        // HIBERNATE may reset the MCU without returning - flush the
src/state/State_Sleep.cpp:730:        setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
src/state/State_Sleep.cpp:731:        const SystemSleepResult hibernateResult = System.sleep(hibernateConfig);
src/state/State_Sleep.cpp:734:        // HIBERNATE should reset the MCU. If we return here, treat it as failed and fall back.
src/state/State_Sleep.cpp:736:        ExternalWatchdog.start();
src/state/State_Sleep.cpp:738:        Log.warn("HIBERNATE sleep failed err=%d - falling back to ULTRA_LOW_POWER",
src/state/State_Sleep.cpp:741:        Log.warn("HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER");
src/state/State_Sleep.cpp:752:  SystemSleepConfiguration ulpConfig;
src/state/State_Sleep.cpp:773:  const bool watchdogStopped = (ExternalWatchdog.stop() == SYSTEM_ERROR_NONE);
src/state/State_Sleep.cpp:810:  setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
src/state/State_Sleep.cpp:811:  SystemSleepResult result = System.sleep(ulpConfig);
src/state/State_Sleep.cpp:841:  // If Device OS rejects the sleep configuration, System.sleep() returns
src/state/State_Sleep.cpp:852:    SystemSleepConfiguration stopConfig;
src/state/State_Sleep.cpp:862:    setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
src/state/State_Sleep.cpp:863:    result = System.sleep(stopConfig);
src/state/State_Sleep.cpp:869:      SystemSleepConfiguration stopTimerConfig;
src/state/State_Sleep.cpp:877:      setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
src/state/State_Sleep.cpp:878:      result = System.sleep(stopTimerConfig);
src/state/State_Sleep.cpp:886:        ExternalWatchdog.start();
src/state/State_Sleep.cpp:914:  ExternalWatchdog.start();
src/Generalized-Core-Counter.cpp:218:// HIBERNATE wake) as long as system power is maintained - see
src/Generalized-Core-Counter.cpp:318:  // SLEEP_PREP span cannot survive any real reset (HIBERNATE success,
src/Generalized-Core-Counter.cpp:389:    SystemSleepConfiguration bootStormSleep;
src/Generalized-Core-Counter.cpp:392:    System.sleep(bootStormSleep);
src/Generalized-Core-Counter.cpp:765:  if (ExternalWatchdog.init(WatchdogConfiguration().timeout(124000)) != SYSTEM_ERROR_NONE || ExternalWatchdog.start() != SYSTEM_ERROR_NONE) {
src/Generalized-Core-Counter.cpp:766:    Log.warn("ExternalWatchdog: init/start failed");
src/Generalized-Core-Counter.cpp:916:  // immediately after every boot/HIBERNATE wake and would force this branch
src/Generalized-Core-Counter.cpp:925:  // guarantee a resync opportunity around every HIBERNATE wake. That term
src/Generalized-Core-Counter.cpp:1132:  ExternalWatchdog.refresh(); // the only refresh source (Device OS does not refresh instance 2)
<shellId: 9 completed with exit code 1>
```

</details>


---

<sub>51s</sub>

### `bash`

**Sleep config style, stubs, tests mentioning**

`$ sed -n 695,735p src/state/State_Sleep.cpp; ls tests/stubs; grep -rn "SystemSleepConfiguration\|SystemSleepMode\|SystemSleepResult" tests --include=*.h --include=*.cpp -l | head; grep -n "deepPowerDown\|deep-powerdown\|deep power" tests/*.cpp tests/*.py`

<details>
<summary>86 lines</summary>

```
if (digitalRead(WAKEUP_PIN) == HIGH) {
        setAppBreadcrumb(24); // BREADCRUMB_SLEEP_CONFIG_START (was stale literal 20, colliding with BREADCRUMB_REPORT_EXIT)
        // Fresh local configuration per sleep: Device OS move-assigns
        // without freeing the previous wake-source list, so a reused
        // long-lived object orphans that list on every cycle.
        SystemSleepConfiguration hibernateConfig;
        hibernateConfig.mode(SystemSleepMode::HIBERNATE)
          .gpio(WAKEUP_PIN, FALLING)
          .gpio(BUTTON_PIN, FALLING)
          .duration((uint32_t)wakeInSeconds * 1000UL);

        const bool watchdogStopped = (ExternalWatchdog.stop() == SYSTEM_ERROR_NONE);
        pauseAwakeWatchdogForSleep("hibernate");
        if (!watchdogStopped) {
          Log.warn("SleepCall: watchdog-stop failed mode=HIBERNATE dur=%ds", wakeInSeconds);
        }

        // HibernateCycle owns the retained hibernate-cycle fields; Device OS
        // arms the chip alarm itself from duration().
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
Config.h
MyPersistentData.h
Particle.h
clock_status_republish_overrides
connection_mode_overrides
diag_overrides
external_rtc
hour_rules_overrides
ledger_no_retry_overrides
modem_health
persist
persistence_backend
pir_lifecycle
pmic_fault_monitor_overrides
power
power_composition_overrides
power_source_override_overrides
report_golden
reporting_policy_adapter_overrides
sleep_teardown
tests/stubs/sleep_teardown/SleepExecutionFakes.h
tests/sleep_config_leak_test.cpp
tests/connectivity_failsafe_unknown_openness_test.cpp:98:  void deepPowerDown() { flushedAfterRecordBeforeAction = flushed; lastActionTaken = DEEP_POWER_DOWN; ++actionCount; persistedStageAtAction = persistedStage; throw ActionStop{}; }
tests/connectivity_failsafe_unknown_openness_test.cpp:163:  return a == RESET ? "System.reset" : a == DEEP_POWER_DOWN ? "deepPowerDown" : "no action";
tests/connectivity_failsafe_unknown_openness_test.cpp:196:  check("Unknown, stage 2, past cooldown + jitter: stage 3 deep power-down",
tests/connectivity_failsafe_unknown_openness_test.cpp:214:  check("Open, stage 2, past cooldown + jitter: stage 3 deep power-down",
tests/watchdog_inferred_classification_test.cpp:18:  // The Error-path byte was persisted before deepPowerDown(): a power-up, not a watchdog.
tests/connectivity_failsafe_stage1_retired_test.py:100:    check("ab1805.deepPowerDown()" in supervisor,
tests/connectivity_failsafe_stage1_retired_test.py:101:          "stage 3 is still the AB1805 deep power-down")
tests/deliberate_reset_flush_test.py:35:    ("error hard", "error", "ab1805.deepPowerDown();", "Executing deep power down from ERROR_STATE"),
tests/deliberate_reset_flush_test.py:39:    ("failsafe stage 3", "failsafe", "ab1805.deepPowerDown();\n  RecoveryState::set_powerOffRequested(0);", "persistConnectivityFailsafeState(3, now, true);"),
tests/deliberate_reset_flush_test.py:178:    fs = fs.replace(f"  {CALL}\n  ab1805.deepPowerDown();", f"  {CALL}\n  {rec}\n  ab1805.deepPowerDown();", 1) if stage == 3 else \
tests/modem_teardown_confirmation_logging_test.py:33:  5. State_Error.cpp's Case 3 (`ab1805.deepPowerDown()` block) was NOT
tests/modem_teardown_confirmation_logging_test.py:258:    # ---- Check 6: State_Error.cpp Case 3 (ab1805.deepPowerDown()) block
tests/modem_teardown_confirmation_logging_test.py:262:        i for i, l in enumerate(error_lines) if "ab1805.deepPowerDown();" in l
tests/modem_teardown_confirmation_logging_test.py:265:        fail(f"expected exactly 1 ab1805.deepPowerDown() call in "
tests/modem_teardown_confirmation_logging_test.py:272:            fail(f"State_Error.cpp Case 3 (ab1805.deepPowerDown(), line "
tests/modem_teardown_confirmation_logging_test.py:275:    print("PASS: State_Error.cpp Case 3 (ab1805.deepPowerDown() block) "
tests/s2_alarm_watchdog_wake_structural_test.py:55:            if m.group(1) != "deepPowerDown":
tests/s2_alarm_watchdog_wake_structural_test.py:80:    # The byte is persisted before the deep power-down and the flush, never after.
tests/s2_alarm_watchdog_wake_structural_test.py:83:    order = [case3.find(s) for s in ("set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel)", "flushAllBeforeReset()", "ab1805.deepPowerDown()")]
tests/s2_alarm_watchdog_wake_structural_test.py:85:        p.append("State_Error case 3 must write the sentinel, then flush, then deepPowerDown")
tests/s2_alarm_watchdog_wake_structural_test.py:86:    if not re.search(r"deepPowerDown\(\);\s*RecoveryState::set_powerOffRequested\(0\);\s*SystemConfig::flushNow\(\);", case3):
tests/s2_alarm_watchdog_wake_structural_test.py:87:        p.append("State_Error case 3 must clear the byte and flush after a returning deepPowerDown()")
tests/s2_alarm_watchdog_wake_structural_test.py:88:    if not re.search(r"deepPowerDown\(\);\s*RecoveryState::set_powerOffRequested\(0\);\s*RecoveryState::set_connectivityRecoveryStage\(2\);\s*SystemConfig::flushNow\(\);", fs):
tests/s2_alarm_watchdog_wake_structural_test.py:89:        p.append("the failsafe caller must clear the byte, downgrade stage 3 and flush after a returning deepPowerDown()")
tests/s2_alarm_watchdog_wake_structural_test.py:157:    "byte written after the call": mutated("err", "RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog\n      flushAllBeforeReset();\n      ab1805.deepPowerDown();",
tests/s2_alarm_watchdog_wake_structural_test.py:158:                                           "flushAllBeforeReset();\n      ab1805.deepPowerDown();\n      RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel);"),
<shellId: 10 completed with exit code 0>
```

</details>


---

<sub>55s</sub>

### `bash`

**Failsafe host test harness**

`$ cat tests/connectivity_failsafe_unknown_openness_test.sh | head -60; sed -n 1,160p tests/connectivity_failsafe_unknown_openness_test.cpp`

<details>
<summary>191 lines</summary>

```
#!/bin/zsh
set -euo pipefail

# WO-2026-10-24-001 (recovery item 18): the failsafe acts when openness is
# Unknown and skips only when it is Closed.
#
# The REAL connectivityFailsafeSupervisor() body is extracted from
# src/power/ConnectivityFailsafe.cpp, compared with the source (COPY_MISMATCH on
# any difference) and compiled against fakes in
# connectivity_failsafe_unknown_openness_test.cpp. The three timings are parsed
# from the production branch of src/power/ConnectivityPolicy.h.
#
# UNKNOWN_OPENNESS_TAMPER_COPY=1 alters the compiled copy to show COPY_MISMATCH.

repo_root="${0:A:h:h}"
src="$repo_root/src/power/ConnectivityFailsafe.cpp"
policy="$repo_root/src/power/ConnectivityPolicy.h"
work="${TMPDIR:-/tmp}/failsafe_unknown_openness_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

extract() {
  python3 - "$src" "$1" <<'PY'
import re, sys

app = open(sys.argv[1]).read()
m = re.search(r"^void connectivityFailsafeSupervisor\(\) \{.*?^\}\n", app, re.S | re.M)
if not m:
    sys.exit("EXTRACT_FAILED: connectivityFailsafeSupervisor() not found")
open(sys.argv[2], "w").write(m.group(0))
PY
}

extract "$work/body.h"
if [[ "${UNKNOWN_OPENNESS_TAMPER_COPY:-0}" == 1 ]]; then
  print -r -- "// tampered" >> "$work/body.h"
fi
extract "$work/fresh.h"
cmp -s "$work/body.h" "$work/fresh.h" || {
  echo "COPY_MISMATCH: the supervisor body compiled by the harness differs from $src" >&2
  exit 1
}

read_policy() {
  python3 - "$policy" "$1" <<'PY'
import re, sys

text = open(sys.argv[1]).read()
block = re.search(r"#if CONNECTIVITY_FAILSAFE_TEST_MODE(.*?)#else(.*?)#endif", text, re.S)
if not block:
    sys.exit("EXTRACT_FAILED: CONNECTIVITY_FAILSAFE_TEST_MODE block not found")
m = re.search(r"constexpr\s+time_t\s+%s\s*=\s*([^;]+);" % re.escape(sys.argv[2]), block.group(2))
if not m:
    sys.exit("EXTRACT_FAILED: %s not found" % sys.argv[2])
expr = m.group(1).replace("L", "").strip()
if not re.fullmatch(r"[0-9*+\s]+", expr):
    sys.exit("EXTRACT_FAILED: %s is not a plain arithmetic literal" % sys.argv[2])
print(int(eval(expr)))
PY
}
// WO-2026-10-24-001 (recovery item 18): the connectivity failsafe acts when
// openness is Unknown (an untrusted clock after its own stage-2 reset) and
// still skips when it is Closed.
//
// Driven by tests/connectivity_failsafe_unknown_openness_test.sh, which
// extracts the REAL connectivityFailsafeSupervisor() body from
// src/power/ConnectivityFailsafe.cpp into SUPERVISOR_BODY_H (production build,
// CONNECTIVITY_FAILSAFE_TEST_MODE=0) and compiles it here against small fakes.
// REAL_STALE_SEC / REAL_COOLDOWN_SEC / REAL_JITTER_MAX_SEC come from
// src/power/ConnectivityPolicy.h.

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>

#ifndef SUPERVISOR_BODY_H
#error "SUPERVISOR_BODY_H must name the extracted supervisor body"
#endif
#ifndef REAL_STALE_SEC
#error "REAL_STALE_SEC must be injected from src/power/ConnectivityPolicy.h"
#endif

enum BatteryTier { TIER_HEALTHY, TIER_CONSERVING, TIER_CRITICAL, TIER_SURVIVAL };
enum { FIRMWARE_UPDATE_STATE = 99 };
enum { BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD = 1, RESET_CAUSE_CONNECTIVITY_FAILSAFE = 2 };

namespace {
enum Action { NONE, RESET, DEEP_POWER_DOWN };
Action lastActionTaken = NONE;
int actionCount = 0;
uint8_t persistedStage = 0;
time_t persistedAction = 0;
// The persisted stage AT THE MOMENT the action was called. A real System.reset() or deep
// power-down does not return, so anything the supervisor persists after it is lost: the
// stage must be on flash before the call (otherwise stage 2 could repeat on every boot).
uint8_t persistedStageAtAction = 0;
// WO-2026-11-03-005: flushAllBeforeReset() must run after the stage record and before the action.
bool flushed = false;
bool flushedAfterRecordBeforeAction = false;
struct ActionStop {};  // thrown by the fakes: the real calls never return
constexpr uint32_t FAKE_JITTER_SEC = 1000;
} // namespace

namespace Clock {
enum class Openness : uint8_t { Open, Closed, Unknown };
Openness fakeOpenness = Openness::Open;
inline bool isTimeValid() { return true; }
inline Openness openness() { return fakeOpenness; }
} // namespace Clock

namespace ConnectivityPolicy {
constexpr time_t CONNECTIVITY_FAILSAFE_STALE_SEC = REAL_STALE_SEC;
constexpr time_t CONNECTIVITY_FAILSAFE_COOLDOWN_SEC = REAL_COOLDOWN_SEC;
} // namespace ConnectivityPolicy

namespace RecoveryState {
uint8_t stage = 0;
time_t lastAction = 0;
inline uint8_t get_connectivityRecoveryStage() { return stage; }
inline time_t get_lastConnectivityRecoveryAction() { return lastAction; }
inline void set_connectivityRecoveryStage(uint8_t) {}
inline void set_powerOffRequested(uint8_t) {}
} // namespace RecoveryState

namespace SystemConfig {
enum { CONNECTED, DISCONNECTED };
time_t lastConnection = 0;
time_t openTimeToday = 0;
inline int get_openTime() { return 6; }
inline time_t get_lastConnection() { return lastConnection; }
inline void flushNow() {}
} // namespace SystemConfig

namespace DailyBoundary {
inline time_t todayAt(int) { return SystemConfig::openTimeToday; }
} // namespace DailyBoundary

struct ReportingRuntime { uint32_t effectiveIntervalSec; };
namespace ReportingPolicyResolver {
inline ReportingRuntime resolveRuntime(int, time_t) { return {900}; }
} // namespace ReportingPolicyResolver

struct PowerManagerFake {
  static PowerManagerFake &instance() { static PowerManagerFake p; return p; }
  int effectiveConnectionMode() { return SystemConfig::CONNECTED; }
  int soc() { return 0; }
  bool downgradeActive() { return false; }
};
using PowerManager = PowerManagerFake;

struct SystemFake {
  bool updatesPending() { return false; }
  void reset(int) { flushedAfterRecordBeforeAction = flushed; lastActionTaken = RESET; ++actionCount; persistedStageAtAction = persistedStage; throw ActionStop{}; }
} System;

struct Ab1805Fake {
  void deepPowerDown() { flushedAfterRecordBeforeAction = flushed; lastActionTaken = DEEP_POWER_DOWN; ++actionCount; persistedStageAtAction = persistedStage; throw ActionStop{}; }
} ab1805;

struct TimeFake {
  time_t current = 0;
  time_t now() { return current; }
} Time;

struct LogFake {
  void info(const char *, ...) {}
} Log;

int state = 0;
const char *batteryTierShortName(BatteryTier) { return "H"; }
bool activeConnectAttemptWithinBudget() { return false; }
void setAppBreadcrumb(int) {}
BatteryTier currentBatteryTierForFailsafe() { return TIER_HEALTHY; }
bool connectivityFailsafeHasExternalPower() { return true; }
uint32_t connectivityFailsafeJitterSec(uint8_t) { return FAKE_JITTER_SEC; }
void persistConnectivityFailsafeState(uint8_t stage, time_t actionTime, bool) {
  persistedStage = stage;
  persistedAction = actionTime;
}

void flushAllBeforeReset() { flushed = (persistedStage != 0 && persistedAction != 0); }

#include SUPERVISOR_BODY_H

namespace {

int failures = 0;

constexpr time_t OPENED_AT = 2000000;
constexpr time_t COOLDOWN_AND_JITTER = REAL_COOLDOWN_SEC + FAKE_JITTER_SEC;

struct Scenario {
  Clock::Openness openness;
  time_t now;
  time_t lastConnection;
  uint8_t stage;
  time_t lastAction;
};

Action run(const Scenario &s) {
  Clock::fakeOpenness = s.openness;
  Time.current = s.now;
  SystemConfig::lastConnection = s.lastConnection;
  SystemConfig::openTimeToday = OPENED_AT;
  RecoveryState::stage = s.stage;
  RecoveryState::lastAction = s.lastAction;
  lastActionTaken = NONE;
  actionCount = 0;
  persistedStage = 0;
  persistedAction = 0;
  persistedStageAtAction = 0;
  flushed = false;
  flushedAfterRecordBeforeAction = false;
  try {
    connectivityFailsafeSupervisor();
  } catch (const ActionStop &) {
  }
  return lastActionTaken;
}
<shellId: 11 completed with exit code 0>
```

</details>


---

<sub>1m 4s</sub>

### `bash`

**P7 off time; duration signature**

`$ grep -n "P7" docs/work-orders/WO-2026-11-01-001-ab1805-native-driver-spike.md | head -12; grep -rn "deepPowerDown(" src lib/AB1805_RK/src/AB1805_RK.h | head; D=~/.particle/toolchains/deviceOS/6.6.0; grep -n "duration\|POWER_OFF" $D/wiring/inc/spark_wiring_system_sleep_configuration.h 2>/dev/null | head -20; grep -rn "SystemSleepConfiguration& duration" $D/wiring/inc | head`

<details>
<summary>10 lines</summary>

```
14:2. **Run P7 first** (v48's vendored `deepPowerDown()` on a serial command): it answers whether today's fleet stage 3 powers down.
15:3. **No meter:** the P6/P7 pass is the modem LED dark, the device offline for the timed period, and a timer boot with a power-on reset reason. A USB meter's current reading is preferred if Chip has one.
62:**Order: P7 first, then P1-P6.** (Amendment 2.)
69:**P7. The old failsafe stage-3 question (item 18), for contrast.**
107:**P7 (vendored `deepPowerDown(120)`, chip unbound): PASS, it powers down** (architect verdict). Call at sys 1793708957; boot about 122 s later with `resetReason=20` (PIN_RESET), data 0, `bootSTATUS=0xff`, RTC at boot 0 (system time 946684809, invalid): a full power loss, and the stored session was lost. Record for wake classification: expect reason 20 after a deepPowerDown wake on this hardware (the AB1805 asserting nRST at power-up is a guess, not confirmed). LED observation (Chip, 4 Nov): the modem LED went dark and the serial port disconnected for 120 s, then the device came back up and connected to the cloud. No meter reading. An earlier call at sys 1793708749 is in the log; only the second is treated as the P7 run. `~/spike-p7.log` is a 138-byte stub; `screenlog.0` holds the data.
src/power/ConnectivityFailsafe.cpp:316:  ab1805.deepPowerDown();
src/state/State_Error.cpp:179:      ab1805.deepPowerDown();
lib/AB1805_RK/src/AB1805_RK.h:24:        DEEP_POWER_DOWN,    //!< The deepPowerDown() function was used (RTC was in sleep mode)
lib/AB1805_RK/src/AB1805_RK.h:325:    bool deepPowerDown(int seconds = 30);
<shellId: 12 completed with exit code 0>
```

</details>


---

<sub>1m 11s</sub>

### `bash`

**Duration types and boot classification refs**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; grep -rn "duration(" $D/wiring/inc/spark_wiring_system_sleep*.h $D/wiring/inc/*sleep* | head -5; grep -n "duration" $D/hal/inc/exrtc_hal.h | head; grep -n "RESET_REASON_PIN_RESET\|isInferredWatchdog\|powerOffRequested\|watchdogClassified" -r src | head -30`

<details>
<summary>22 lines</summary>

```
grep: /Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_system_sleep*.h: No such file or directory
grep: /Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/*sleep*: No such file or directory
195:    system_tick_t duration;
src/MyPersistentData.cpp:1068:uint8_t get_powerOffRequested() { return sysStatus.getValue<uint8_t>(offsetof(sysStatusData::SysData, reservedUint8_0)); }
src/MyPersistentData.cpp:1069:void set_powerOffRequested(uint8_t value) { sysStatus.setValue<uint8_t>(offsetof(sysStatusData::SysData, reservedUint8_0), value); }
src/power/ConnectivityFailsafe.cpp:317:  RecoveryState::set_powerOffRequested(0); RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow(); // not expected to return: no recent stage-3 evidence
src/state/State_Error.cpp:177:      RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog
src/state/State_Error.cpp:180:      RecoveryState::set_powerOffRequested(0); SystemConfig::flushNow(); // not expected to return: drop the evidence
src/time/HibernateWakeDiagnostics.h:169:inline bool isInferredWatchdog(uint8_t evidenceByte, uint8_t failsafeStage, int64_t failsafeActionAt, bool timeValid, int64_t now) {
src/time/HibernateCycle.h:82:/// result, e.g. `RESET_REASON_PIN_RESET`/`RESET_REASON_POWER_MANAGEMENT`).
src/Generalized-Core-Counter.cpp:588:  case RESET_REASON_PIN_RESET:
src/Generalized-Core-Counter.cpp:611:  case RESET_REASON_PIN_RESET:
src/Generalized-Core-Counter.cpp:752:  if (reason == RESET_REASON_PIN_RESET) {
src/Generalized-Core-Counter.cpp:755:    inferredWatchdog = HibernateWakeDiagnostics::isInferredWatchdog(
src/Generalized-Core-Counter.cpp:756:        RecoveryState::get_powerOffRequested(), RecoveryState::get_connectivityRecoveryStage(),
src/Generalized-Core-Counter.cpp:761:  RecoveryState::set_powerOffRequested(0);
src/Generalized-Core-Counter.cpp:778:  const bool watchdogClassified = watchdogResetDetected || inferredWatchdog;
src/Generalized-Core-Counter.cpp:779:  if (watchdogClassified) {
src/persist/RecoveryState.h:30: * AB1805_PIN: Device OS reported RESET_REASON_PIN_RESET, but the AB1805
src/persist/RecoveryState.h:79:uint8_t get_powerOffRequested();
src/persist/RecoveryState.h:80:void set_powerOffRequested(uint8_t value);
<shellId: 13 completed with exit code 0>
```

</details>


---

<sub>1m 13s</sub>

### `bash`

**Boot classification and persist helper**

`$ sed -n 735,785p src/Generalized-Core-Counter.cpp; sed -n 160,185p src/time/HibernateWakeDiagnostics.h; grep -n "persistConnectivityFailsafeState" -A18 src/power/ConnectivityFailsafe.cpp | head -40; grep -rn "SystemSleepConfiguration& *duration\|duration(" ~/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_system.h | head -4`

<details>
<summary>105 lines</summary>

```
// hook ran. Do not claim the skew took effect (A.4).
        Log.error("RtcSkewTest: ONE-SHOT bench hook FAILED - ExternalRtc::set() write did not "
                  "succeed; RTC left unchanged (before(raw=%ld readOk=%d) attemptedAfter=%s)",
                  (long)rtcSkewRawBefore, rtcSkewReadBeforeOk ? 1 : 0,
                  Time.format(rtcSkewAfter, TIME_FORMAT_DEFAULT).c_str());
      }
    }
  }
#endif

  // ===== WAKE CLASSIFICATION (HibernateCycle) =====
  // Reason 30 is judged by HibernateCycle from the OS reason and the retained
  // request. Reason 20 is judged from persisted evidence only (a power-down
  // clears `retained`); no evidence is counted as an inferred watchdog.
  const HibernateCycle::WakeVerdict wakeVerdict = HibernateCycle::classifyWake(reason);

  bool inferredWatchdog = false;
  if (reason == RESET_REASON_PIN_RESET) {
    startupPinResetChecked = true;
    const bool timeValid = Time.isValid();
    inferredWatchdog = HibernateWakeDiagnostics::isInferredWatchdog(
        RecoveryState::get_powerOffRequested(), RecoveryState::get_connectivityRecoveryStage(),
        (int64_t)RecoveryState::get_lastConnectivityRecoveryAction(), timeValid, (int64_t)Time.now());
    startupInferredWatchdog = inferredWatchdog;
    Log.info("PIN_RESET: %s", inferredWatchdog ? "no power-off evidence - inferred watchdog" : "power-off evidence - power-up wake");
  }
  RecoveryState::set_powerOffRequested(0);
  SystemConfig::flushNow(); // consume the evidence durably before anything can reload it

  // Device OS keeps the chip watchdog running across a reset; init() re-arms it.
  if (ExternalWatchdog.init(WatchdogConfiguration().timeout(124000)) != SYSTEM_ERROR_NONE || ExternalWatchdog.start() != SYSTEM_ERROR_NONE) {
    Log.warn("ExternalWatchdog: init/start failed");
  }

  // ===== WATCHDOG RESET PERSISTENCE / FORENSICS =====
  // Moved here (from immediately after SystemConfig::setup(), before AB1805 init)
  // so it can act on the classification above. The publish queue was
  // already initialized above, so this is a reordering, not a new
  // dependency. Covers both the Device-OS-detected case
  // (reason == RESET_REASON_WATCHDOG) and the inferred PIN_RESET
  // case classified above (the "inferred" source); both reuse the same lastWatchdog* fields, tagged
  // with lastWatchdogSource. An inconclusive or non-watchdog PIN_RESET must
  // not overwrite a previously persisted watchdog's forensic data.
  const bool watchdogClassified = watchdogResetDetected || inferredWatchdog;
  if (watchdogClassified) {
    const uint16_t priorWatchdogResetCount = RecoveryState::get_watchdogResetCount();
    if (priorWatchdogResetCount < 0xFFFF) {
      RecoveryState::set_watchdogResetCount((uint16_t)(priorWatchdogResetCount + 1));
    }
    RecoveryState::set_lastWatchdogBreadcrumb(previousBreadcrumb);
    RecoveryState::set_lastWatchdogUptimeMs(previousBreadcrumbMs);
      (unsigned long)f.hibernateCount);
}

/// Reason-20 boot: persisted power-off evidence (the Error-path byte, or a
/// failsafe stage 3 within 600 s on a valid clock) means a power-up wake; none means an inferred
/// watchdog. `retained` is never evidence (a power-down clears it).
constexpr int64_t kPowerOffEvidenceMaxAgeSec = 600;
/// The Error-path evidence byte counts only when it is exactly this value (0 and the legacy structuresVersion 2 are "no record").
constexpr uint8_t kPowerOffSentinel = 0xA5;
inline bool isInferredWatchdog(uint8_t evidenceByte, uint8_t failsafeStage, int64_t failsafeActionAt, bool timeValid, int64_t now) {
  const bool errorPathByte = evidenceByte == kPowerOffSentinel;
  const int64_t age = now - failsafeActionAt;
  const bool failsafeRecent = failsafeStage == 3 && (timeValid && age >= 0 && age <= kPowerOffEvidenceMaxAgeSec);
  return !errorPathByte && !failsafeRecent;
}

} // namespace HibernateWakeDiagnostics

#endif
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
--
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
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_system.h:597:        return system_button_pushed_duration(button, nullptr);
<shellId: 14 completed with exit code 0>
```

</details>


---

<sub>1m 21s</sub>

### `bash`

**File structure**

`$ grep -n "^}\|^namespace\|^} // " src/power/ConnectivityFailsafe.cpp | sed -n 1,20p; grep -n "connectivityFailsafeSupervisor" src/power/ConnectivityFailsafe.cpp; grep -rn "ConnectivityFailsafe.h" src | head`

<details>
<summary>17 lines</summary>

```
12:namespace {
26:}
40:}
57:}
75:}
86:}
105:}
122:}
124:} // namespace
128:}
132:}
156:}
318:}
173:void connectivityFailsafeSupervisor() {
src/power/ConnectivityFailsafe.cpp:1:#include "power/ConnectivityFailsafe.h"
src/Generalized-Core-Counter.cpp:26:#include "power/ConnectivityFailsafe.h"
<shellId: 15 completed with exit code 0>
```

</details>


---

<sub>1m 23s</sub>

### `bash`

**View area before supervisor**

`$ sed -n 120,175p src/power/ConnectivityFailsafe.cpp`

<details>
<summary>48 lines</summary>

```
session.failsafeDeferLogMask |= bit;
  return true;
}

} // namespace

const char *failsafeDeferReasonName(FailsafeDeferReason reason) {
  return failsafeDeferReasonNameLocal(reason);
}

bool claimFailsafeDeferLog(FailsafeDeferReason reason) {
  return claimFailsafeDeferLogLocal(reason);
}

void clearConnectivityFailsafeRecovery(const char *reason) {
  const bool hadRecoveryState =
      RecoveryState::get_connectivityRecoveryStage() != 0 ||
      RecoveryState::get_lastConnectivityRecoveryAction() != 0 ||
      RecoveryState::get_connectivityRecoveryCount() != 0;
  const bool hadAlert = (RecoveryState::get_alertCode() == ConnectivityPolicy::CONNECTIVITY_FAILSAFE_ALERT);

  if (!hadRecoveryState && !hadAlert) {
    return;
  }

  RecoveryState::set_connectivityRecoveryStage(0);
  RecoveryState::set_lastConnectivityRecoveryAction(0);
  RecoveryState::set_connectivityRecoveryCount(0);
  SystemConfig::flushNow();

  if (hadAlert) {
    RecoveryState::set_alertCode(0);
    RecoveryState::set_lastAlertTime(0);
  }

  Log.info("Failsafe: cleared reason=%s", reason ? reason : "unknown");
}

/**
 * @brief Monitors connection health and triggers escalating recovery when stale.
 *
 * Implements escalating recovery when connection attempts exceed policy thresholds:
 * Stage 2 (system reset), then Stage 3 (deep power-down, Boron only). Stage 1
 * (radio reset) is retired (WO-2026-10-02-001 item B); the first action is stage 2.
 * Prevents indefinite modem-on battery drain and recovers from radio firmware wedges.
 *
 * Acts unless Closed (Unknown counts as open), measuring the age from the later of lastConnection
 * and today's opening. Defers if disconnected, firmware updating, time invalid,
 * recently connected,
 * within cooldown period, or jitter delay not elapsed (prevents fleet-wide simultaneous resets).
 *
 * @note Stage 3 deep power-down will not wake for hours/days - last resort only.
 */
void connectivityFailsafeSupervisor() {
  if (PowerManager::instance().effectiveConnectionMode() == SystemConfig::DISCONNECTED) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
<shellId: 16 completed with exit code 0>
```

</details>


---

<sub>1m 36s</sub>

### Copilot

Now edits.

---

<sub>1m 36s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/power/ConnectivityFailsafe.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/power/ConnectivityFailsafe.cpp`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/power/ConnectivityFailsafe.cpp updated with changes.
```


---

<sub>1m 36s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/power/ConnectivityFailsafe.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/power/ConnectivityFailsafe.cpp`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/power/ConnectivityFailsafe.cpp updated with changes.
```


---

<sub>1m 36s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/power/ConnectivityFailsafe.h.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/power/ConnectivityFailsafe.h`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/power/ConnectivityFailsafe.h updated with changes.
```


---

<sub>1m 36s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/state/State\_Error.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/state/State_Error.cpp`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/state/State_Error.cpp updated with changes.
```


---

<sub>1m 36s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/state/State\_Error.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/state/State_Error.cpp`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/state/State_Error.cpp updated with changes.
```


---

<sub>1m 40s</sub>

### `bash`

**Budget check**

`$ git diff --numstat 62e3dbc -- src; git diff 62e3dbc -- src | grep '^[+-]' | grep -v '^+++\|^---'`

<details>
<summary>20 lines</summary>

```
9	1	src/power/ConnectivityFailsafe.cpp
2	0	src/power/ConnectivityFailsafe.h
3	1	src/state/State_Error.cpp
+void powerOffForRecovery() {
+  SystemSleepConfiguration cfg; // RTC wake only: no GPIO source, so noise cannot cut the off period short
+  cfg.mode(SystemSleepMode::POWER_OFF).duration(kPowerOffRecoverySec * 1000UL);
+  const SystemSleepResult result = System.sleep(cfg);
+  Log.error("PowerOff: System.sleep returned err=%d", (int)result.error()); // reached only if power was not cut
+}
+
-  ab1805.deepPowerDown();
+  powerOffForRecovery();
+  System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);
+constexpr uint32_t kPowerOffRecoverySec = 30; // the vendored deepPowerDown() default
+void powerOffForRecovery(); // timed POWER_OFF; returns only if Device OS did not cut power
+#include "power/ConnectivityFailsafe.h"
-      ab1805.deepPowerDown();
+      powerOffForRecovery();
+      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);
<shellId: 17 completed with exit code 0>
```

</details>


---

<sub>1m 45s</sub>

### `bash`

**View tests to update**

`$ sed -n 90,106p tests/connectivity_failsafe_stage1_retired_test.py; echo ----; sed -n 1,80p tests/deliberate_reset_flush_test.py; sed -n 160,215p tests/deliberate_reset_flush_test.py`

<details>
<summary>113 lines</summary>

```
check(mapping[0] == 2, "with no stage recorded (0) the next action is stage 2")
        check(mapping[1] == 2, "a stage 1 persisted by older firmware progresses to stage 2")
        check(mapping[2] == 3, "stage 2 still escalates to stage 3")
        check(1 not in mapping.values(),
              "failsafeStage never newly takes 1")

    check("if (nextStage == 2) {" in supervisor,
          "stage 2 (System.reset) is still the system-reset action")
    check("RESET_CAUSE_CONNECTIVITY_FAILSAFE" in supervisor,
          "stage 2 still resets with RESET_CAUSE_CONNECTIVITY_FAILSAFE (code 2)")
    check("ab1805.deepPowerDown()" in supervisor,
          "stage 3 is still the AB1805 deep power-down")

    print("")
    print("--- What became unused is gone ---")

    hits = []
----
#!/usr/bin/env python3
"""WO-2026-11-03-005: flushAllBeforeReset() at the deliberate-reset sites.

1. What it flushes: the REAL function body (src/state/State_Error.cpp) is compiled against a fake backend that
   records the flush events of the current store (C), sysStatus (S), sensor store (N) and the publish queue (Q).
   All four must fire, stores before the queue.
2. Where it is called: seven production sites (thrash tier 3, Error soft, Error hard, nightly heap guard,
   sleep exhausted, failsafe stage 2, failsafe stage 3), each immediately before its reset or power-down and after
   its own record writes. The OOM reset must not call it.
3. Mutations of private text copies: each must be detected.

Nothing under src/ is written; mutants live in memory or in a temp dir.
"""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
CALL = "flushAllBeforeReset();"
FILES = {
    "error": "state/State_Error.cpp",
    "thrash": "ThrashGuard.cpp",
    "sleep": "state/State_Sleep.cpp",
    "failsafe": "power/ConnectivityFailsafe.cpp",
    "main": "Generalized-Core-Counter.cpp",
}

# (name, file key, terminator that ends the site, text that must come before the flush)
SITES = [
    ("thrash tier 3", "thrash", "System.reset(RESET_CAUSE_THRASH_TIER3);", "RecoveryState::raiseAlert(18);  // Alert 18: Severe thrash"),
    ("error soft", "error", "System.reset(RESET_CAUSE_ERROR_STATE_SOFT);", "Executing soft reset from ERROR_STATE"),
    ("error hard", "error", "ab1805.deepPowerDown();", "Executing deep power down from ERROR_STATE"),
    ("heap guard", "sleep", "System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);", "PowerDiagnostics::flushDiagBatch();"),
    ("sleep exhausted", "sleep", "System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);", "PowerDiagnostics::flushDiagBatch();"),
    ("failsafe stage 2", "failsafe", "System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);", "persistConnectivityFailsafeState(2, now, true);"),
    ("failsafe stage 3", "failsafe", "ab1805.deepPowerDown();\n  RecoveryState::set_powerOffRequested(0);", "persistConnectivityFailsafeState(3, now, true);"),
]


def load():
    return {k: (SRC / v).read_text() for k, v in FILES.items()}


def site_problems(texts):
    """Returns a list of problems; empty when every structural rule holds."""
    out = []
    for name, key, term, before in SITES:
        text = texts[key]
        if text.count(term) != 1:
            out.append(f"{name}: terminator {term!r} is not unique")
            continue
        at = text.index(term)
        head = text[:at].rstrip()
        if not head.endswith(CALL):
            out.append(f"{name}: {CALL} is not immediately before the reset/power-down")
            continue
        flush_at = len(head) - len(CALL)
        pos = text.rfind(before, 0, flush_at)
        if pos == -1 or flush_at - pos > 600:
            out.append(f"{name}: {before!r} must be written shortly before the flush")
    fs = texts["failsafe"]
    persist = re.search(r"^void persistConnectivityFailsafeState\(.*?^\}\n", fs, re.S | re.M)
    if not persist or "flushNow" in persist.group(0) or CALL in persist.group(0):
        out.append("failsafe: persistConnectivityFailsafeState() must only record; the flush follows it at the reset sites")
    oom = re.search(r"if \(outOfMemory >= 0\) \{.*?\n  \}\n", texts["main"], re.S)
    if not oom or "System.reset(RESET_CAUSE_OUT_OF_MEMORY);" not in oom.group(0):
        out.append("OOM block not found")
    elif "flushAllBeforeReset" in oom.group(0):
        out.append("OOM must not flush (immediate reset)")
    if CALL in texts["main"]:
        out.append("Generalized-Core-Counter.cpp must not call flushAllBeforeReset()")
    total = sum(t.count(CALL) for k, t in texts.items() if k != "main")
    if total != len(SITES):
        out.append(f"expected {len(SITES)} call sites, found {total}")
    return out


    t = m[key]
    at = t.index(term)
    head = t[:at].rstrip()
    start = len(head) - len(CALL)
    m[key] = t[:start] + t[at:]
    check(bool(site_problems(m)), f"mutation caught: call removed at {name}")

# Add a call at the OOM site.
m = dict(texts)
m["main"] = m["main"].replace("    System.reset(RESET_CAUSE_OUT_OF_MEMORY);", "    flushAllBeforeReset();\n    System.reset(RESET_CAUSE_OUT_OF_MEMORY);", 1)
check(m["main"] != texts["main"] and bool(site_problems(m)), "mutation caught: flush added at the OOM site")

# Failsafe: the flush before the stage record (record moved after the flush).
for stage in (2, 3):
    rec = f"persistConnectivityFailsafeState({stage}, now, true);"
    m = dict(texts)
    fs = m["failsafe"]
    fs = fs.replace(rec + "\n", "", 1)
    fs = fs.replace(f"  {CALL}\n  ab1805.deepPowerDown();", f"  {CALL}\n  {rec}\n  ab1805.deepPowerDown();", 1) if stage == 3 else \
        fs.replace(f"    {CALL}\n    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);", f"    {CALL}\n    {rec}\n    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);", 1)
    check(m["failsafe"] != fs and bool(site_problems({**m, "failsafe": fs})), f"mutation caught: failsafe stage {stage} flushes before its record")

# The persist helper regaining its own flush (flush-then-record order for stage 2/3 would follow).
fs = texts["failsafe"].replace("    }\n  }\n}\n\nconst char *failsafeDeferReasonNameLocal",
                               "    }\n  }\n  SystemConfig::flushNow();\n}\n\nconst char *failsafeDeferReasonNameLocal", 1)
check(fs != texts["failsafe"] and bool(site_problems({**texts, "failsafe": fs})), "mutation caught: persistConnectivityFailsafeState() flushes on its own again")

if failures:
    print(f"\nFAILED: {failures} check(s)")
    sys.exit(1)
print("\ndeliberate_reset_flush_test: ok")
<shellId: 18 completed with exit code 0>
```

</details>


---

<sub>1m 50s</sub>

### `bash`

**Patch flush test; view modem teardown test**

```
$ python3 - <<'E'
import re
p='tests/deliberate_reset_flush_test.py'
s=open(p).read()
s=s.replace('"ab1805.deepPowerDown();", "Executing deep','"powerOffForRecovery();", "Executing deep')
s=s.replace('"ab1805.deepPowerDown();\\n  RecoveryState::set_powerOffRequested(0);"','"powerOffForRecovery();\\n  RecoveryState::set_powerOffRequested(0);"')
s=s.replace('{rec}\\n  ab1805.deepPowerDown();"','{rec}\\n  powerOffForRecovery();"').replace('f"  {CALL}\\n  ab1805.deepPowerDown();"','f"  {CALL}\\n  powerOffForRecovery();"')
open(p,'w').write(s)
E
grep -n "deepPowerDown\|ab1805" tests/deliberate_reset_flush_test.py; sed -n 250,280p tests/modem_teardown_confirmation_logging_test.py; sed -n 25,40p tests/modem_teardown_confirmation_logging_test.py
```

<details>
<summary>45 lines</summary>

```
for wl in error_window:
            if token in wl and "ModemTeardown" not in wl:
                fail(f"blocking-call token {token!r} found within "
                     f"{WINDOW} lines of State_Error.cpp ModemTeardown "
                     f"point={error_tag} (line {error_modem_line + 1}): {wl!r}")
    print("PASS: no new blocking call token found immediately adjacent to "
          "the State_Error.cpp ModemTeardown log line (checked before and after)")

    # ---- Check 6: State_Error.cpp Case 3 (ab1805.deepPowerDown()) block
    # was NOT touched - no ModemTeardown line added there. Case 3 is a
    # full hardware power-cycle, explicitly out of scope per the WO. ----
    deep_power_down_lines = [
        i for i, l in enumerate(error_lines) if "ab1805.deepPowerDown();" in l
    ]
    if len(deep_power_down_lines) != 1:
        fail(f"expected exactly 1 ab1805.deepPowerDown() call in "
             f"State_Error.cpp, found {len(deep_power_down_lines)}")
    dpd_line = deep_power_down_lines[0]
    CASE3_WINDOW = 4
    case3_window_lines = error_lines[max(0, dpd_line - CASE3_WINDOW): dpd_line + 1]
    for wl in case3_window_lines:
        if "ModemTeardown" in wl:
            fail(f"State_Error.cpp Case 3 (ab1805.deepPowerDown(), line "
                 f"{dpd_line + 1}) must remain untouched per WO scope, but "
                 f"a ModemTeardown log line was found nearby: {wl!r}")
    print("PASS: State_Error.cpp Case 3 (ab1805.deepPowerDown() block) "
          "left untouched - no ModemTeardown line added there")

    # ---- Check 7: src/power/Connectivity.h and .cpp are untouched. ----
    # Use git diff against HEAD to confirm no modifications were made to
    # files outside the WO's permitted-files scope.
     of the four new log lines.
  3. Item 1 (network-keep-alive logging, already satisfied) was left
     untouched: the "Sleep: ULP standby=%d ..." line and the
     Particle.connected()/isRadioPoweredOn() invariant-check block both
     still exist, unduplicated.
  4. src/power/Connectivity.h and .cpp are untouched (git diff scope
     check) - no new teardown function was added anywhere the WO
     forbids it.
  5. State_Error.cpp's Case 3 (`ab1805.deepPowerDown()` block) was NOT
     touched - no ModemTeardown line was added there, since a full
     hardware power-cycle is explicitly out of scope per the WO.

This is real evidence traced through the actual code path (not a
hand-wave): every assertion below reads the literal line order from the
shipped files, and each check is written so that removing/altering the
new logging (as verified via mutation testing before this test was
<shellId: 19 completed with exit code 0>
```

</details>


---

<sub>1m 55s</sub>

### `bash`

**Patch tests; view s2 test**

```
$ python3 - <<'E'
p='tests/modem_teardown_confirmation_logging_test.py'
s=open(p).read()
s=s.replace('"ab1805.deepPowerDown();" in l','"powerOffForRecovery();" in l').replace('ab1805.deepPowerDown()','powerOffForRecovery()')
open(p,'w').write(s)
p='tests/connectivity_failsafe_stage1_retired_test.py'
s=open(p).read()
s=s.replace('''check("ab1805.deepPowerDown()" in supervisor,
          "stage 3 is still the AB1805 deep power-down")''','''check("powerOffForRecovery()" in supervisor,
          "stage 3 is still the deep power-down (now the timed POWER_OFF helper)")''')
open(p,'w').write(s)
E
sed -n 35,170p tests/s2_alarm_watchdog_wake_structural_test.py
```

<details>
<summary>121 lines</summary>

```
def _problems(files):
    p = []
    gcc = strip(files["gcc"])
    sleep = strip(files["sleep"])
    err = strip(files["err"])
    fs = strip(files["fs"])
    hc = strip(files["hc"])

    for name in ("clearAb1805StaleAlarmInterrupts", "interruptAtTime", "setWDT", "stopWDT", "resumeWDT",
                 "ab1805WakeReasonName", "getWakeReason", "updateWakeReason"):
        for label, t in files.items():
            if label != "raw" and re.search(r"\b" + name + r"\b", strip(t)):
                p.append(f"{name} still referenced in {label}")

    # Only the two S3 callers may still touch the AB1805 object for power-down.
    for label in ("gcc", "sleep", "err", "hc", "fs"):
        t = strip(files[label])
        for m in re.finditer(r"\bab1805\.(\w+)", t):
            if m.group(1) != "deepPowerDown":
                p.append(f"ab1805.{m.group(1)} in {label}")

    # Reason 20 is judged from persisted evidence, not the retained struct, and a reason-30-only wake.
    if "in.wakeReasonIsHibernateWake = in.resetReasonIsPowerManagement;" not in hc:
        p.append("reason 30 alone must be the successful-HIBERNATE-wake arm")
    if re.search(r"retained\w*", hc) and "PIN_RESET" in hc:
        pass
    setup = gcc[gcc.index("classifyWake(reason)"):]
    block = setup[:setup.index("ExternalWatchdog.init")]
    if "isInferredWatchdog(" not in block or "get_powerOffRequested()" not in block:
        p.append("reason-20 classification must use isInferredWatchdog with the persisted byte")
    if re.search(r"\bretained\w*", block):
        p.append("retained must not be reason-20 evidence")
    if not 0 <= block.find("isInferredWatchdog(") < block.find("set_powerOffRequested(0)"):
        p.append("the byte must be read before it is cleared")
    if gcc.count("set_powerOffRequested(0)") != 1:
        p.append("the byte must be cleared exactly once, at boot")
    if not re.search(r"set_powerOffRequested\(0\);\s*SystemConfig::flushNow\(\);", gcc):
        p.append("the boot clear must be flushed immediately (forced consumption)")
    if 'ab1805WakeReason\\":\\"N/A\\"' not in files["gcc"]:
        p.append("the status payload must keep ab1805WakeReason with the constant N/A")
    if "if (reason == RESET_REASON_PIN_RESET)" not in block:
        p.append("reason 20 gate missing")

    # The byte is persisted before the deep power-down and the flush, never after.
    case3 = err[err.index("case 3:"):]
    case3 = case3[:case3.index("break;")]
    order = [case3.find(s) for s in ("set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel)", "flushAllBeforeReset()", "ab1805.deepPowerDown()")]
    if min(order) < 0 or order != sorted(order):
        p.append("State_Error case 3 must write the sentinel, then flush, then deepPowerDown")
    if not re.search(r"deepPowerDown\(\);\s*RecoveryState::set_powerOffRequested\(0\);\s*SystemConfig::flushNow\(\);", case3):
        p.append("State_Error case 3 must clear the byte and flush after a returning deepPowerDown()")
    if not re.search(r"deepPowerDown\(\);\s*RecoveryState::set_powerOffRequested\(0\);\s*RecoveryState::set_connectivityRecoveryStage\(2\);\s*SystemConfig::flushNow\(\);", fs):
        p.append("the failsafe caller must clear the byte, downgrade stage 3 and flush after a returning deepPowerDown()")

    # The chip watchdog: init+start in setup, refreshed in the main loop, stopped before and started after every sleep.
    if gcc.count("ExternalWatchdog.refresh()") != 1:
        p.append("ExternalWatchdog.refresh() must appear exactly once in the main loop")
    if gcc.count("ExternalWatchdog.init(") != 1 or gcc.count("ExternalWatchdog.start()") != 1:
        p.append("ExternalWatchdog init/start must appear once in setup")
    loop = gcc[gcc.index("void loop()"):]
    if loop.index("ExternalWatchdog.refresh()") > loop.index("checkClockResync()"):
        p.append("refresh must precede checkClockResync() (the old ab1805.loop() slot)")

    hib = fn_body(sleep, "bool hibernateAttempt(") if "bool hibernateAttempt(" in sleep else fn_body(sleep, "void hibernateAttempt(")
    ulp_i = sleep.index("sleepAndResume(")
    sites = [("hibernate", hib), ("ulp", sleep[ulp_i:])]
    for label, body in sites:
        stop = body.find("ExternalWatchdog.stop()")
        call = body.find("System.sleep(")
        start = body.find("ExternalWatchdog.start()", call)
        if stop < 0 or call < 0 or start < 0 or not (stop < call < start):
            p.append(f"{label}: ExternalWatchdog.stop() must precede System.sleep() and start() follow it")
    if sleep.count("ExternalWatchdog.stop()") != 2:
        p.append("expected exactly two ExternalWatchdog.stop() sites (hibernate, ULP/STOP)")
    if sleep.count("ExternalWatchdog.start()") != 3:
        p.append("expected exactly three ExternalWatchdog.start() sites (hibernate return, all-failed, post-ULP)")

    # One native alarm: duration() once on the hibernate config, both GPIO wakes kept, no second arming.
    cfg = hib[hib.index("hibernateConfig.mode("):]
    cfg = cfg[:cfg.index(";")]
    if cfg.count(".duration(") != 1 or "wakeInSeconds" not in cfg or cfg.count(".gpio(") != 2:
        p.append("hibernate config must arm exactly one duration(wakeInSeconds) plus both GPIO wakes")
    if "SystemSleepMode::HIBERNATE" not in cfg:
        p.append("hibernate mode missing")

    # A native error return is abandoned, the dog restarted, and the ULP fallback follows.
    res = hib[hib.index("System.sleep(hibernateConfig)"):]
    if not (res.index("HibernateCycle::abandon()") < res.index("ExternalWatchdog.start()")):
        p.append("a returned hibernate must abandon() then restart the watchdog")
    if "falling back to ULTRA_LOW_POWER" not in sleep:
        p.append("the ULP fallback log is missing")
    return p


raw = {
    "gcc": read("Generalized-Core-Counter.cpp"),
    "sleep": read("state/State_Sleep.cpp"),
    "err": read("state/State_Error.cpp"),
    "fs": read("power/ConnectivityFailsafe.cpp"),
    "hc": read("time/HibernateCycle.cpp"),
    "diag": read("time/ClockDiagnostics.cpp"),
    "elig": read("time/HibernateEligibility.cpp"),
}
raw["raw"] = ""


def mutated(label, old, new, count=1):
    f = dict(raw)
    if f[label].count(old) < 1:
        print(f"FAIL: probe did not apply: {old!r}")
        sys.exit(1)
    f[label] = f[label].replace(old, new, count)
    return f


probes = {
    "reason 20 always a wake": mutated("hc", "in.wakeReasonIsHibernateWake = in.resetReasonIsPowerManagement;", "in.wakeReasonIsHibernateWake = true;"),
    "evidence read from retained": mutated("gcc", "RecoveryState::get_powerOffRequested(),", "retainedHibernatePending,"),
    "byte not cleared": mutated("gcc", "RecoveryState::set_powerOffRequested(0);", ""),
    "byte cleared before read": mutated("gcc", "  bool inferredWatchdog = false;\n", "  RecoveryState::set_powerOffRequested(0);\n  bool inferredWatchdog = false;\n"),
    "byte written after the call": mutated("err", "RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog\n      flushAllBeforeReset();\n      ab1805.deepPowerDown();",
                                           "flushAllBeforeReset();\n      ab1805.deepPowerDown();\n      RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel);"),
    "boot clear not flushed": mutated("gcc", "  SystemConfig::flushNow(); // consume the evidence durably before anything can reload it\n", ""),
    "no clear after a returning error-path power-down": mutated("err", "      RecoveryState::set_powerOffRequested(0); SystemConfig::flushNow();", ""),
    "failsafe stage 3 not downgraded on return": mutated("fs", "RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow();", ""),
    "ab1805WakeReason dropped from the payload": mutated("gcc", '\\"ab1805WakeReason\\":\\"N/A\\",', ""),
    "alarm armed twice": mutated("sleep", ".duration((uint32_t)wakeInSeconds * 1000UL);\n\n        const bool watchdogStopped",
                                 ".duration((uint32_t)wakeInSeconds * 1000UL).duration(1);\n\n        const bool watchdogStopped"),
    "no watchdog stop before hibernate": mutated("sleep", "const bool watchdogStopped = (ExternalWatchdog.stop() == SYSTEM_ERROR_NONE);\n        pauseAwakeWatchdogForSleep",
                                                  "const bool watchdogStopped = true;\n        pauseAwakeWatchdogForSleep"),
    "refresh missing": mutated("gcc", "ExternalWatchdog.refresh();", ""),
    "hibernate fallback restart removed": mutated("sleep", "HibernateCycle::abandon();\n        ExternalWatchdog.start();", "HibernateCycle::abandon();"),
    "ab1805 stale clear back": dict(raw, elig="bool clearAb1805StaleAlarmInterrupts();\n" + raw["elig"]),
}
<shellId: 20 completed with exit code 0>
```

</details>


---

<sub>2m 2s</sub>

### `bash`

**Patch s2 test; view rest of failsafe harness**

```
$ python3 - <<'E'
p='tests/s2_alarm_watchdog_wake_structural_test.py'
s=open(p).read()
s=s.replace('''    # Only the two S3 callers may still touch the AB1805 object for power-down.''','''    # S3: no caller touches the AB1805 object any more.''')
s=s.replace('''            if m.group(1) != "deepPowerDown":
                p.append(f"ab1805.{m.group(1)} in {label}")''','''            p.append(f"ab1805.{m.group(1)} in {label}")''')
s=s.replace('"ab1805.deepPowerDown()")]','"powerOffForRecovery()")]').replace('then deepPowerDown")','then power off")')
s=s.replace(r'''r"deepPowerDown\(\);\s*RecoveryState::set_powerOffRequested\(0\);\s*SystemConfig::flushNow\(\);", case3''',r'''r"powerOffForRecovery\(\);\s*RecoveryState::set_powerOffRequested\(0\);\s*SystemConfig::flushNow\(\);\s*System\.reset\(RESET_CAUSE_ERROR_STATE_SOFT\);", case3''')
s=s.replace(r'''r"deepPowerDown\(\);\s*RecoveryState::set_powerOffRequested\(0\);\s*RecoveryState::set_connectivityRecoveryStage\(2\);\s*SystemConfig::flushNow\(\);", fs''',r'''r"powerOffForRecovery\(\);\s*RecoveryState::set_powerOffRequested\(0\);\s*RecoveryState::set_connectivityRecoveryStage\(2\);\s*SystemConfig::flushNow\(\);\s*System\.reset\(RESET_CAUSE_CONNECTIVITY_FAILSAFE\);", fs''')
s=s.replace("returning deepPowerDown()","returning power-off")
s=s.replace("ab1805.deepPowerDown();","powerOffForRecovery();")
open(p,'w').write(s)
E
grep -n "deepPowerDown" tests/s2_alarm_watchdog_wake_structural_test.py; sed -n 160,260p tests/connectivity_failsafe_unknown_openness_test.cpp; sed -n 60,140p tests/connectivity_failsafe_unknown_openness_test.sh
```

<details>
<summary>74 lines</summary>

```
}

const char *name(Action a) {
  return a == RESET ? "System.reset" : a == DEEP_POWER_DOWN ? "deepPowerDown" : "no action";
}

void check(const char *what, const Scenario &s, Action want, uint8_t wantStage = 0) {
  const Action got = run(s);
  const bool stageOk = (want == NONE) || (persistedStageAtAction == wantStage && flushedAfterRecordBeforeAction);
  if (got == want && stageOk && actionCount == (want == NONE ? 0 : 1)) {
    printf("  ok   %-64s -> %s\n", what, name(got));
  } else {
    printf("  FAIL %-64s got %s (stage %u), want %s\n", what, name(got),
           (unsigned)persistedStageAtAction, name(want));
    failures++;
  }
}

} // namespace

int main() {
  using Clock::Openness;
  printf("--- the real supervisor body (stale=%ld cooldown=%ld jitter<=%ld) ---\n",
         (long)REAL_STALE_SEC, (long)REAL_COOLDOWN_SEC, (long)REAL_JITTER_MAX_SEC);

  // The last connection is before today's opening, so the age starts at it.
  const time_t last = OPENED_AT - 3600;
  const time_t staleNow = OPENED_AT + REAL_STALE_SEC;
  const time_t stage2At = OPENED_AT + 100;
  const time_t stage3Now = stage2At + COOLDOWN_AND_JITTER;

  // 1. Unknown at stage 0: stage 2 acts.
  check("Unknown, stage 0, stale: stage 2 system reset",
        {Openness::Unknown, staleNow, last, 0, 0}, RESET, 2);

  // 2. Unknown at stage 2: the trap case.
  check("Unknown, stage 2, past cooldown + jitter: stage 3 deep power-down",
        {Openness::Unknown, stage3Now, last, 2, stage2At}, DEEP_POWER_DOWN, 3);
  check("Unknown, stage 2, one second inside the cooldown + jitter: nothing",
        {Openness::Unknown, stage3Now - 1, last, 2, stage2At}, NONE);
  check("Unknown, stage 2, one second past the cooldown, inside jitter: nothing",
        {Openness::Unknown, stage2At + REAL_COOLDOWN_SEC + 1, last, 2, stage2At}, NONE);
  check("Unknown, stage 3: nothing (no loop)",
        {Openness::Unknown, stage3Now + 86400, last, 3, stage3Now}, NONE);

  // 3. Closed: never acts.
  check("Closed, stage 0, stale: nothing",
        {Openness::Closed, staleNow, last, 0, 0}, NONE);
  check("Closed, stage 2, past cooldown + jitter: nothing",
        {Openness::Closed, stage3Now, last, 2, stage2At}, NONE);

  // 4. Open: unchanged.
  check("Open, stage 0, stale: stage 2 system reset",
        {Openness::Open, staleNow, last, 0, 0}, RESET, 2);
  check("Open, stage 2, past cooldown + jitter: stage 3 deep power-down",
        {Openness::Open, stage3Now, last, 2, stage2At}, DEEP_POWER_DOWN, 3);
  check("Open, stage 2, inside the cooldown + jitter: nothing",
        {Openness::Open, stage3Now - 1, last, 2, stage2At}, NONE);
  check("Open, stage 0, one second short of the stale threshold: nothing",
        {Openness::Open, staleNow - 1, last, 0, 0}, NONE);

  // 00:00-06:00 local: today's opening is in the future, so now <= ageBase.
  check("Unknown, before today's opening (now <= ageBase): nothing",
        {Openness::Unknown, OPENED_AT - 10, last, 0, 0}, NONE);
  check("Unknown, now == ageBase: nothing",
        {Openness::Unknown, OPENED_AT, last, 0, 0}, NONE);

  if (failures != 0) {
    printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  printf("\nconnectivity_failsafe_unknown_openness cases passed\n");
  return 0;
}
}

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -DSUPERVISOR_BODY_H="\"$work/body.h\"" \
  -DREAL_STALE_SEC="$(read_policy CONNECTIVITY_FAILSAFE_STALE_SEC)" \
  -DREAL_COOLDOWN_SEC="$(read_policy CONNECTIVITY_FAILSAFE_COOLDOWN_SEC)" \
  -DREAL_JITTER_MAX_SEC="$(read_policy CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC)" \
  "$repo_root/tests/connectivity_failsafe_unknown_openness_test.cpp" \
  -o "$work/unknown_openness"
"$work/unknown_openness"

echo "connectivity_failsafe_unknown_openness_test passed"
<shellId: 21 completed with exit code 0>
```

</details>


---

<sub>2m 24s</sub>

### `bash`

**Policy constants and supervisor head**

`$ grep -n "constexpr" src/power/ConnectivityPolicy.h | head -20; sed -n 173,262p src/power/ConnectivityFailsafe.cpp; sed -n 1,30p src/time/HibernateWakeDiagnostics.h | head -40; grep -n "resetWait\|resetTimer" src/state/State_Error.cpp | head`

<details>
<summary>134 lines</summary>

```
62:constexpr unsigned long CONNECT_BUDGET_DEFAULT_MS = 5UL * 60UL * 1000UL; // 5 minutes
63:constexpr unsigned long CONNECT_BUDGET_DEEP_MS = 11UL * 60UL * 1000UL;   // 11 minutes
64:constexpr uint16_t CONNECT_BUDGET_CONFIG_MIN_SEC = 120;                  // Particle docs minimum
65:constexpr uint16_t CONNECT_BUDGET_CONFIG_MAX_SEC = 900;
80:constexpr uint8_t DEEP_ATTEMPT_COUNTER_THRESHOLD = 3; // deep when attemptCounter >= 3
81:constexpr float DEEP_ATTEMPT_SOC_THRESHOLD = 50.0f;   // deep when SoC > 50%
98:constexpr unsigned long FIRMWARE_UPDATE_MAX_MS = 5UL * 60UL * 1000UL;
109:constexpr time_t CONNECTIVITY_FAILSAFE_STALE_SEC = 5L * 60L;
110:constexpr time_t CONNECTIVITY_FAILSAFE_COOLDOWN_SEC = 15L * 60L;
111:constexpr time_t CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC = 0L;
112:constexpr time_t CONNECTIVITY_FAILSAFE_TEST_MAX_CLOSED_SLEEP_SEC = 60L;
114:constexpr time_t CONNECTIVITY_FAILSAFE_STALE_SEC = 3L * 3600L;
115:constexpr time_t CONNECTIVITY_FAILSAFE_COOLDOWN_SEC = 6L * 3600L;
116:constexpr time_t CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC = 30L * 60L;
117:constexpr time_t CONNECTIVITY_FAILSAFE_TEST_MAX_CLOSED_SLEEP_SEC = 0L;
119:constexpr int8_t CONNECTIVITY_FAILSAFE_ALERT = 45;
136:constexpr unsigned long CLOUD_OPS_GATE_TIMEOUT_MS = 30000UL;
137:constexpr unsigned long CLOUD_OPS_STATUS_LOG_INTERVAL_MS = 5000UL;
152:constexpr unsigned long OUTPUT_LEDGER_SYNC_TIMEOUT_MS = 70000UL;
154:constexpr unsigned long OUTPUT_LEDGER_SYNC_TIMEOUT_MS = 30000UL;
 * Acts unless Closed (Unknown counts as open), measuring the age from the later of lastConnection
 * and today's opening. Defers if disconnected, firmware updating, time invalid,
 * recently connected,
 * within cooldown period, or jitter delay not elapsed (prevents fleet-wide simultaneous resets).
 *
 * @note Stage 3 deep power-down will not wake for hours/days - last resort only.
 */
void connectivityFailsafeSupervisor() {
  if (PowerManager::instance().effectiveConnectionMode() == SystemConfig::DISCONNECTED) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferDisconnectedMode();
#endif
    return;
  }

  if (state == FIRMWARE_UPDATE_STATE || System.updatesPending()) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferUpdatePending();
#endif
    return;
  }

  // WO-2026-09-19 Step 3b: deliberately NOT converted to Clock::isTrusted(),
  // even though this function's connectionAgeSec math below is exactly the
  // kind of untrusted-clock arithmetic Step 3b otherwise guards against.
  // This escalation ladder (system reset -> deep power-down)
  // exists to recover a wedged modem/radio - and a device with a wedged
  // radio has, by definition, never completed a sync this boot either, so
  // gating the recovery on Clock::isTrusted() would disable the recovery
  // mechanism in exactly the case it exists for (circular: the fault this
  // supervisor recovers from is often the same fault that prevents the
  // trust signal from ever going true). The narrower risk of staying on
  // isTimeValid() (a plausible-but-wrong RTC-seeded epoch skewing
  // connectionAgeSec) is judged acceptable against the alternative of
  // silently disabling hardware recovery. Revisit only with a case that
  // demonstrates the escalation firing on a genuinely wrong clock in the
  // field - do not "fix" this speculatively.
  if (!Clock::isTimeValid()) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferInvalidTime();
#endif
    return;
  }

  const time_t lastConnection = SystemConfig::get_lastConnection();
  if (lastConnection == 0) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferNoLastConnection();
#endif
    return;
  }

  // WO-2026-10-02-001 item A: act unless Closed (Unknown counts as open), and measure the age
  // from the later of lastConnection and today's opening. Wall-clock age would
  // make every device cross the 3 h threshold on its 06:00 wake, because the
  // overnight gap since the 22:00 close is 7-8 h and this supervisor runs at
  // the top of loop() before the report has started connecting.
  const time_t now = Time.now();
  const time_t openedAt = DailyBoundary::todayAt(SystemConfig::get_openTime());
  const time_t ageBase = (openedAt > lastConnection) ? openedAt : lastConnection;
  // Recovery item 18 (WO-2026-10-24-001): Unknown is treated as Open here. Its own
  // stage-2 reset leaves the clock untrusted, so requiring Open is the circular case above.
  if (Clock::openness() == Clock::Openness::Closed || now <= ageBase) {
    return;
  }

  const time_t connectionAgeSec = now - ageBase;
  if (connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC) {
    return;
  }

  uint8_t currentStage = RecoveryState::get_connectivityRecoveryStage();
  if (currentStage > 3) {
    currentStage = 0;
  }
  if (currentStage >= 3) {
    return;
  }

  time_t lastAction = RecoveryState::get_lastConnectivityRecoveryAction();
  if (lastAction > now) {
    lastAction = 0;
  }

  // WO-2026-10-02-001 item B: stage 1 (radio reset) is retired - it never
  // restored a wedged connection in the field. Stage numbers keep their
  // meaning, so the first action is stage 2 and a stage 1 persisted by older
  // firmware still progresses to 2.
  const uint8_t nextStage = (currentStage <= 1) ? 2 : (uint8_t)(currentStage + 1);
  time_t requiredDelay = ConnectivityPolicy::CONNECTIVITY_FAILSAFE_COOLDOWN_SEC;
#pragma once

/**
 * @file HibernateWakeDiagnostics.h
 * @brief Pure, dependency-free data types and forensic-event payload
 *        construction for the hibernate wake-validation gate
 *        (WO-2026-08-29-001; WO-2026-09-16 Step 2).
 *
 * @details The gate's six-condition classification itself is owned by
 *          `time/HibernateCycle.cpp`'s `classifyWake()` - this header does
 *          not classify anything anymore. What it still owns is the shared,
 *          Particle-free layer both `HibernateCycle` and
 *          `Generalized-Core-Counter.cpp` need: `GateInputs` (the six raw
 *          conditions), `GateArm` (which one failed, or none), and the
 *          `EventFields`/`buildEventFields()`/`buildEventPayload()` trio
 *          that renders a classification into the "hibernate_wake" cloud
 *          event. When the gate fails, nothing previously reached the
 *          cloud, and serial cannot be trusted to capture the `Log.info`
 *          line on this boot (USB CDC has not re-enumerated yet) - see the
 *          Work Order for the full incident history.
 *
 *          No Particle/AB1805 dependency, so this compiles and tests on the
 *          host, the same pattern used by `power/ChargeInhibitPolicy.h` and
 *          `time/ClockTrust.h`.
 */

#ifndef __HIBERNATE_WAKE_DIAGNOSTICS_H
#define __HIBERNATE_WAKE_DIAGNOSTICS_H

#include <stdint.h>
124:  static unsigned long resetTimer = 0;
150:    resetTimer = millis();
164:    if (millis() - resetTimer > resetWait) {
176:    if (millis() - resetTimer > resetWait) {
<shellId: 22 completed with exit code 0>
```

</details>


---

<sub>2m 37s</sub>

### `bash`

**Patch fake and run updated tests**

```
$ python3 - <<'E'
p='tests/connectivity_failsafe_unknown_openness_test.cpp'
s=open(p).read()
s=s.replace('''struct Ab1805Fake {
  void deepPowerDown() { flushedAfterRecordBeforeAction = flushed; lastActionTaken = DEEP_POWER_DOWN; ++actionCount; persistedStageAtAction = persistedStage; throw ActionStop{}; }
} ab1805;
''','''// The shared timed power-off helper: it cuts power and never returns on success.
void powerOffForRecovery() { flushedAfterRecordBeforeAction = flushed; lastActionTaken = DEEP_POWER_DOWN; ++actionCount; persistedStageAtAction = persistedStage; throw ActionStop{}; }
''')
open(p,'w').write(s)
E
bash -c 'zsh tests/connectivity_failsafe_unknown_openness_test.sh 2>&1 | tail -4; for t in deliberate_reset_flush connectivity_failsafe_stage1_retired modem_teardown_confirmation_logging s2_alarm_watchdog_wake_structural; do python3 tests/${t}_test.py 2>&1 | tail -3; done'
```

<details>
<summary>11 lines</summary>

```
ok   Unknown, now == ageBase: nothing                                 -> no action

connectivity_failsafe_unknown_openness cases passed
connectivity_failsafe_unknown_openness_test passed
PASS: mutation caught: persistConnectivityFailsafeState() flushes on its own again

FAILED: 1 check(s)
  ok   ConnectivityFailsafeTest.h includes cloud/BatteryBackoffPolicy.h (for BatteryTier)

connectivity_failsafe_stage1_retired_test passed
PASS: src/power/Connectivity.h and .cpp are untouched (git diff scope check)

All modem-teardown confirmation logging regression checks passed
s2_alarm_watchdog_wake_structural_test: ok (14 probes detected)
<shellId: 23 completed with exit code 0>
```

</details>


---

<sub>2m 44s</sub>

### `bash`

**See flush test failure**

`$ python3 tests/deliberate_reset_flush_test.py 2>&1 | grep -v "^PASS" | head`

<details>
<summary>5 lines</summary>

```
FAIL: all seven sites call flushAllBeforeReset() immediately before the reset, after their record writes
      error soft: terminator 'System.reset(RESET_CAUSE_ERROR_STATE_SOFT);' is not unique
      failsafe stage 2: terminator 'System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);' is not unique

FAILED: 1 check(s)
<shellId: 24 completed with exit code 0>
```

</details>


---

<sub>2m 53s</sub>

### `bash`

**Fix flush test uniqueness**

```
$ python3 - <<'E'
p='tests/deliberate_reset_flush_test.py'
s=open(p).read()
s=s.replace('''        if text.count(term) != 1:
            out.append(f"{name}: terminator {term!r} is not unique")''','''        if text.count(term) < 1:  # the S3 fallback resets repeat two terminators; the first occurrence is the site
            out.append(f"{name}: terminator {term!r} not found")''')
open(p,'w').write(s)
E
python3 tests/deliberate_reset_flush_test.py 2>&1 | tail -3
```

```
PASS: mutation caught: persistConnectivityFailsafeState() flushes on its own again

deliberate_reset_flush_test: ok
<shellId: 25 completed with exit code 0>
```


---

<sub>3m 29s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/tests/s3\_timed\_power\_off\_test.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/tests/s3_timed_power_off_test.cpp`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/tests/s3_timed_power_off_test.cpp with 8055 characters
```


---

<sub>3m 42s</sub>

### `bash`

**Remove stray define**

```
$ python3 - <<'E'
p='tests/s3_timed_power_off_test.cpp'
s=open(p).read()
s=s.replace("#define FLUSH_EVENTS_NAMESPACE_GUARD\n","")
open(p,'w').write(s)
E
printf '%s\n' '#!/bin/zsh' > /dev/null; echo ok
```

```
ok
<shellId: 26 completed with exit code 0>
```


---

<sub>3m 41s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/tests/s3\_timed\_power\_off\_test.sh.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/tests/s3_timed_power_off_test.sh`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/tests/s3_timed_power_off_test.sh with 2447 characters
```


---

<sub>3m 46s</sub>

### `bash`

**Run new S3 test**

`$ chmod +x tests/s3_timed_power_off_test.sh; zsh tests/s3_timed_power_off_test.sh 2>&1 | tail -40`

```
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/src/power/ConnectivityFailsafe.h:4:constexpr uint32_t kPowerOffRecoverySec = 30; // the vendored deepPowerDown() default
AB1805_REFERENCE_LEFT: deepPowerDown/ab1805. still referenced in src/
<shellId: 27 completed with exit code 0>
```


---

<sub>3m 54s</sub>

### `bash`

**Reword comment and rerun**

`$ sed -i '' 's|// the vendored deepPowerDown() default|// the off time the vendored AB1805 call used (its default)|' src/power/ConnectivityFailsafe.h && zsh tests/s3_timed_power_off_test.sh 2>&1 | tail -40`

<details>
<summary>19 lines</summary>

```
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/tests/s3_timed_power_off_test.cpp:43:15: warning: unused variable 'FALLING' [-Wunused-const-variable]
constexpr int FALLING = 2;
              ^
1 warning generated.
--- helper: POWER_OFF, the real off time, RTC wake only ---
  ok   success: one POWER_OFF sleep of kPowerOffRecoverySec, no GPIO wake, nothing after
  ok   error: logs and returns (never resets inside)
  ok   error: logs and returns (never resets inside)
--- failsafe stage 3 ---
  ok   success: record, log, flush, then exactly one sleep; nothing follows
  ok   return: clear evidence, downgrade, flush, then System.reset(failsafe cause)
  ok   return: clear evidence, downgrade, flush, then System.reset(failsafe cause)
--- ERROR_STATE hard recovery ---
  ok   success: log, evidence, flush, then exactly one sleep; nothing follows
  ok   return: clear evidence, flush, then System.reset(ERROR_STATE_SOFT)
  ok   return: clear evidence, flush, then System.reset(ERROR_STATE_SOFT)

s3_timed_power_off cases passed
s3_timed_power_off_test passed
<shellId: 28 completed with exit code 0>
```

</details>


---

<sub>4m 22s</sub>

### `bash`

**Run mutations on copies of src**

```
$ python3 - <<'E'
p='tests/s3_timed_power_off_test.cpp'
s=open(p).read()
s=s.replace("constexpr int FALLING = 2;","constexpr int FALLING = 2, WAKEUP_PIN = 1, BUTTON_PIN = 2; // so a GPIO-wake mutant compiles and fails on its behaviour")
s=s.replace("enum { SYSTEM_ERROR_NONE","enum { SYSTEM_ERROR_NONE")
open(p,'w').write(s)
E
mkdir -p build-tmp/wo-2026-11-05-005-stage6
cat > build-tmp/wo-2026-11-05-005-stage6/mut.py <<'E'
import shutil, subprocess, sys, os
root = os.getcwd()
scratch = f"{root}/build-tmp/wo-2026-11-05-005-stage6"
FS, ER, H = "power/ConnectivityFailsafe.cpp", "state/State_Error.cpp", "power/ConnectivityFailsafe.h"
M = {
 "stage3 flush after sleep": (FS, "  flushAllBeforeReset();\n  powerOffForRecovery();", "  powerOffForRecovery();\n  flushAllBeforeReset();"),
 "error flush after sleep": (ER, "flushAllBeforeReset();\n      powerOffForRecovery();", "powerOffForRecovery();\n      flushAllBeforeReset();"),
 "stage3 fallback removed": (FS, "\n  System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);\n}\n", "\n}\n"),
 "error fallback removed": (ER, "\n      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);\n    }\n    break;\n\n  default", "\n    }\n    break;\n\n  default"),
 "gpio wake added": (FS, "SystemSleepMode::POWER_OFF)", "SystemSleepMode::POWER_OFF).gpio(WAKEUP_PIN, FALLING)"),
 "duration zero": (FS, "duration(kPowerOffRecoverySec * 1000UL)", "duration(0)"),
 "duration const zero": (H, "kPowerOffRecoverySec = 30;", "kPowerOffRecoverySec = 0;"),
 "stage3 wrong cause": (FS, "\n  System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);\n}\n", "\n  System.reset(RESET_CAUSE_ERROR_STATE_SOFT);\n}\n"),
 "error wrong cause": (ER, "System.reset(RESET_CAUSE_ERROR_STATE_SOFT);\n    }\n    break;\n\n  default", "System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);\n    }\n    break;\n\n  default"),
 "stage3 reset before clear": (FS, "  powerOffForRecovery();\n  RecoveryState::set_powerOffRequested(0); RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow(); // not expected to return: no recent stage-3 evidence\n  System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);",
    "  powerOffForRecovery();\n  System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);\n  RecoveryState::set_powerOffRequested(0); RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow();"),
 "error reset before clear": (ER, "      RecoveryState::set_powerOffRequested(0); SystemConfig::flushNow(); // not expected to return: drop the evidence\n      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);",
    "      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);\n      RecoveryState::set_powerOffRequested(0); SystemConfig::flushNow();"),
 "helper resets inside": (FS, "  Log.error(\"PowerOff: System.sleep returned", "  System.reset(1);\n  Log.error(\"PowerOff: System.sleep returned"),
 "ab1805 call back": (FS, "  powerOffForRecovery();\n  RecoveryState", "  ab1805.deepPowerDown();\n  RecoveryState"),
}
bad = 0
for name, (f, old, new) in M.items():
    d = f"{scratch}/src_mut"
    shutil.rmtree(d, ignore_errors=True)
    shutil.copytree(f"{root}/src", d)
    t = open(f"{d}/{f}").read()
    if t.count(old) != 1:
        print(f"PROBE-DID-NOT-APPLY {name}"); bad += 1; continue
    open(f"{d}/{f}", "w").write(t.replace(old, new))
    r = subprocess.run(["zsh", f"{root}/tests/s3_timed_power_off_test.sh"], env={**os.environ, "S3_SRC": d}, capture_output=True, text=True)
    print(("CAUGHT " if r.returncode else "SURVIVED ") + name)
    bad += r.returncode == 0
shutil.rmtree(f"{scratch}/src_mut", ignore_errors=True)
sys.exit(1 if bad else 0)
E
python3 build-tmp/wo-2026-11-05-005-stage6/mut.py
```

<details>
<summary>14 lines</summary>

```
CAUGHT stage3 flush after sleep
CAUGHT error flush after sleep
CAUGHT stage3 fallback removed
CAUGHT error fallback removed
CAUGHT gpio wake added
CAUGHT duration zero
CAUGHT duration const zero
CAUGHT stage3 wrong cause
CAUGHT error wrong cause
CAUGHT stage3 reset before clear
CAUGHT error reset before clear
CAUGHT helper resets inside
CAUGHT ab1805 call back
<shellId: 29 completed with exit code 0>
```

</details>


---

<sub>4m 27s</sub>

### `bash` — Failed

**Verify gpio mutant fails behaviourally**

`$ cd . && rm -rf /tmp/sm && cp -R src /tmp/sm && sed -i '' 's/SystemSleepMode::POWER_OFF)/SystemSleepMode::POWER_OFF).gpio(WAKEUP_PIN, FALLING)/' /tmp/sm/power/ConnectivityFailsafe.cpp && S3_SRC=/tmp/sm zsh tests/s3_timed_power_off_test.sh 2>&1 | grep -E "FAIL|error" | head -3; rm -rf /tmp/sm`

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>4m 31s</sub>

### `bash`

**Gpio mutant check in scratch**

`$ S=build-tmp/wo-2026-11-05-005-stage6/sm; rm -rf $S; cp -R src $S && sed -i '' 's/SystemSleepMode::POWER_OFF)/SystemSleepMode::POWER_OFF).gpio(WAKEUP_PIN, FALLING)/' $S/power/ConnectivityFailsafe.cpp && S3_SRC=$PWD/$S zsh tests/s3_timed_power_off_test.sh 2>&1 | grep -E "FAIL|error:" | head -3; rm -rf $S`

```
FAIL success: one POWER_OFF sleep of kPowerOffRecoverySec, no GPIO wake, nothing after
  FAIL error: logs and returns (never resets inside)
  FAIL error: logs and returns (never resets inside)
<shellId: 31 completed with exit code 0>
```


---

<sub>4m 34s</sub>

### Copilot

Now the full suite per CI discovery and the build.

---

<sub>4m 34s</sub>

### `bash`

**CI discovery and build hints**

`$ grep -n "tests/\*\|zsh\|python3" .github/workflows/*.yml | head; ls ~/.particle/toolchains; grep -rn "BUILD_PATH_BASE" docs/*.md Makefile *.md 2>/dev/null | head -5; ls`

<details>
<summary>38 lines</summary>

```
36:      - name: Install zsh
37:        run: sudo apt-get install -y zsh
38:      - name: Run host tests (sh via zsh, py via python3)
45:          for t in tests/*.sh; do
47:            if zsh "$t"; then pass=$((pass+1)); else failed+=("$t"); fi
49:          for t in tests/*.py; do
51:            if python3 "$t"; then pass=$((pass+1)); else failed+=("$t"); fi
53:          echo "$pass/$total (sh via zsh, py via python3)" | tee -a "$GITHUB_STEP_SUMMARY"
buildscripts
buildtools
deviceOS
gcc-arm
openocd
AI_DEVELOPMENT_WORKFLOW.md:106:  - **Agents:** use a fresh `BUILD_PATH_BASE` for every build type, in a
AI_DEVELOPMENT_WORKFLOW.md:111:    `BUILD_PATH_BASE` arguments as the build.
AI_DEVELOPMENT_WORKFLOW.md:117:  §3 command and its own `BUILD_PATH_BASE`. (Corrected 2026-10-07: this rule
AI_DEVELOPMENT_WORKFLOW.md:434:         BUILD_PATH_BASE=<fresh scratch dir that doesn't exist yet>
AI_DEVELOPMENT_WORKFLOW.md:439:Use a new `BUILD_PATH_BASE` for each build type (see §2, "Verifying compile-time
AI_DEVELOPMENT_WORKFLOW.md
ARCHITECTURE_OVERVIEW.md
CHANGELOG.md
Doxyfile
LICENSE
PROJECT_STATUS.md
README.md
STYLE.md
build-tmp
bump_version.sh
default-settings-v3.23.json
docs
lib
project.properties
release
release.sh
src
tests
tools
<shellId: 32 completed with exit code 0>
```

</details>


---

<sub>4m 36s</sub>

### `bash`

**CI loop and build command**

`$ sed -n 36,60p .github/workflows/*.yml | head -30; sed -n 420,445p AI_DEVELOPMENT_WORKFLOW.md; ls ~/.particle/toolchains/buildscripts ~/.particle/toolchains/buildtools`

<details>
<summary>51 lines</summary>

```
- name: Install zsh
        run: sudo apt-get install -y zsh
      - name: Run host tests (sh via zsh, py via python3)
        id: tests
        env:
          TMPDIR: ${{ runner.temp }}
          DEVICE_OS_ROOT: ${{ github.workspace }}/device-os-6.4.1
        run: |
          pass=0; total=0; failed=()
          for t in tests/*.sh; do
            total=$((total+1))
            if zsh "$t"; then pass=$((pass+1)); else failed+=("$t"); fi
          done
          for t in tests/*.py; do
            total=$((total+1))
            if python3 "$t"; then pass=$((pass+1)); else failed+=("$t"); fi
          done
          echo "$pass/$total (sh via zsh, py via python3)" | tee -a "$GITHUB_STEP_SUMMARY"
          echo "result=$pass/$total" >> "$GITHUB_OUTPUT"
          if [ "${#failed[@]}" -gt 0 ]; then
            printf 'FAILED: %s\n' "${failed[@]}"
            exit 1
          fi

  firmware:
the release build is the CI build (see "Release gate: the CI build", below).

The two use different include-search orders. A source file can compile cleanly in
the cloud and fail locally — or the reverse — so a cloud build passing is **not**
evidence that the firmware builds. The most common form is a header name that
collides with one Device OS ships (`Config.h` is the known case; Device OS has
`services/inc/Config.h`). Within `src/`, subdirectory files include app headers
by relative path (`"../Config.h"`) precisely to avoid this; a bare include is
resolution-order dependent and may work only by accident.

    export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH
    cd ~/.particle/toolchains/deviceOS/6.6.0/main
    make -s PLATFORM=boron APPDIR=<copy-of-tree> TARGET_DIR=<copy>/localbuild \
         DEVICE_OS_PATH=~/.particle/toolchains/deviceOS/6.6.0 \
         BUILD_PATH_BASE=<fresh scratch dir that doesn't exist yet>

This is also the build that produces the symbolised ELF the linkage check above
requires, so the two checks share the work.

Use a new `BUILD_PATH_BASE` for each build type (see §2, "Verifying compile-time
flags"); `make clean-user` does not clean this path.

Added 2026-08-26 after WO-2026-08-25-001. An include rewritten from
`"../Config.h"` to `"Config.h"` broke the local build and survived **seven
implementation rounds and three independent Stage 7 reviews** — because every
acceptance test, on every side, used the cloud compiler. Codex had flagged the
/Users/chipmc/.particle/toolchains/buildscripts:
1.10.2
1.17.2

/Users/chipmc/.particle/toolchains/buildtools:
1.1.1
<shellId: 33 completed with exit code 0>
```

</details>


---

<sub>9m 13s</sub>

### `bash`

**Local Device OS 6.6.0 build**

`$ S=build-tmp/wo-2026-11-05-005-stage6; rm -rf $S/app $S/bpb; mkdir -p $S/app && cp -R src lib project.properties $S/app/ && export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && cd ~/.particle/toolchains/deviceOS/6.6.0/main && A=$OLDPWD && make -s PLATFORM=boron APPDIR=$A/$S/app TARGET_DIR=$A/$S/app/localbuild DEVICE_OS_PATH=$HOME/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE=$A/$S/bpb > $A/$S/build.log 2>&1; echo rc=$?; tail -8 $A/$S/build.log`

<details>
<summary>10 lines</summary>

```
rc=0
      |            ^~~~~~~~
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/build-tmp/wo-2026-11-05-005-stage6/app/lib/LocalTimeRK/src/LocalTimeRK.cpp: In member function 'void LocalTimeConvert::nextDay(LocalTimeHMS)':
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/build-tmp/wo-2026-11-05-005-stage6/app/lib/LocalTimeRK/src/LocalTimeRK.cpp:1255:12: warning: unused variable 'origTime' [-Wunused-variable]
 1255 |     time_t origTime = time;
      |            ^~~~~~~~
Creating /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/build-tmp/wo-2026-11-05-005-stage6/app/localbuild/platform_user_ram.ld ...
   text	   data	    bss	    dec	    hex	filename
 150908	   1114	   2240	 154262	  25a96	/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/build-tmp/wo-2026-11-05-005-stage6/app/localbuild/app.elf
<shellId: 34 completed with exit code 0>
```

</details>


---

<sub>13m 44s</sub>

### `bash`

**Run full host suite as CI does**

`$ pass=0; total=0; failed=(); for t in tests/*.sh; do total=$((total+1)); if zsh "$t" >/dev/null 2>&1; then pass=$((pass+1)); else failed+=("$t"); fi; done; for t in tests/*.py; do total=$((total+1)); if python3 "$t" >/dev/null 2>&1; then pass=$((pass+1)); else failed+=("$t"); fi; done; echo "$pass/$total (sh via zsh, py via python3)"; printf 'FAILED: %s\n' "${failed[@]}"; python3 --version`

<details>
<summary>7 lines</summary>

```
117/121 (sh via zsh, py via python3)
FAILED: tests/reset_cause_structural_test.py
FAILED: tests/sleep_config_ownership_structural_test.py
FAILED: tests/sleep_execution_structural_test.py
FAILED: tests/thermal_coupling_structural_test.py
Python 3.14.6
<shellId: 35 completed with exit code 0>
```

</details>


---

<sub>13m 51s</sub>

### `bash`

**Diagnose failures**

`$ for t in reset_cause_structural sleep_config_ownership_structural sleep_execution_structural thermal_coupling_structural; do echo "== $t"; python3 tests/${t}_test.py 2>&1 | grep -iE "fail|error|mismatch" | head -5; done`

<details>
<summary>9 lines</summary>

```
== reset_cause_structural
FAIL: expected 7 System.reset() call sites in src/, found 9: ['src/Generalized-Core-Counter.cpp:1130', 'src/ThrashGuard.cpp:136', 'src/observability/LoopForensics.cpp:398', 'src/power/ConnectivityFailsafe.cpp:300', 'src/power/ConnectivityFailsafe.cpp:311', 'src/state/State_Error.cpp:168', 'src/state/State_Error.cpp:182', 'src/state/State_Sleep.cpp:648', 'src/state/State_Sleep.cpp:899']. Add the new site's code to the ResetCause enum and update this test.
== sleep_config_ownership_structural
FAILED: power/ConnectivityFailsafe.cpp:134: file-scope SystemSleepConfiguration object - it is never destroyed, so every reset orphans a wake-source list
== sleep_execution_structural
FAIL: src/power/ConnectivityFailsafe.cpp: SystemSleepConfiguration must be a fresh local, found: SystemSleepConfiguration cfg;
== thermal_coupling_structural
FAIL: readTmp112TemperatureC() is referenced from outside SensorManager.h/.cpp: ['/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/build-tmp/wo-2026-11-05-005-stage6/app/src/sensors/SensorManager.cpp', '/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/build-tmp/wo-2026-11-05-005-stage6/app/src/sensors/SensorManager.h']
<shellId: 36 completed with exit code 0>
```

</details>


---

<sub>13m 55s</sub>

### `bash`

**View failing tests**

`$ cd . && grep -n "SystemSleepConfiguration" -B8 -A12 tests/sleep_config_ownership_structural_test.py | head -90; grep -n "must be a fresh local" -B25 tests/sleep_execution_structural_test.py | head -70; grep -n "7\b\|expected" tests/reset_cause_structural_test.py | head -20`

<details>
<summary>122 lines</summary>

```
1-#!/usr/bin/env python3
2-"""
3-WO-2026-10-02-003 (v34-SleepConfigLeak) - structural test for sleep
4-configuration ownership.
5-
6:Device OS's `SystemSleepConfiguration` move assignment
7-(`system/inc/system_sleep_configuration.h:205-210` in 6.4.1) `memcpy`s the
8-incoming configuration over the existing one *without* freeing the old
9-`wakeup_sources` list. Only the destructor frees that list. A long-lived
10-(global or static) configuration that is reset with
11:`config = SystemSleepConfiguration();` before each sleep is therefore never
12-destroyed, and orphans its whole wake-source list on every single wake
13-cycle. The fix is to build a *fresh local* configuration at each sleep site,
14-so the destructor runs and frees the list when the scope ends.
15-
16-This test is a source-invariant check on the REAL checked-in files.
17-Comments and string/char literals are blanked out before any check, so prose
18-(including this file's own history, if pasted into source) cannot produce a
19-false positive or a false negative.
20-
21-Checks (WO Acceptance 1 and 3):
22-
23-  1. No reused long-lived sleep configuration remains anywhere in `src/`:
24:     no file-scope or `static` `SystemSleepConfiguration` object, no
25:     `extern SystemSleepConfiguration` declaration, and no assignment to an
26:     existing `SystemSleepConfiguration` (`... = SystemSleepConfiguration(`,
27-     or any `name = ...` to a declared configuration variable, whatever
28-     follows the `=`, e.g. `ulpConfig = {};`).
29-
30-  2. Every `System.sleep(x)` call in `src/` passes a configuration that is
31:     declared as a block-scope local `SystemSleepConfiguration`, and no two
32-     sleep sites share one object.
33-
34-  3. The four `State_Sleep.cpp` sleep sites still configure exactly the same
35-     wake sources - the same pins and edges, the same RTC duration, and
36-     network standby under the same `useNetworkStandby` / cellular guards -
37-     as at the WO's baseline commit 43b8a69. The expectations below are
38-     transcribed from that commit; when the baseline commit is reachable via
39-     git, the same extractor is additionally run against it and the two are
40-     required to agree.
41-
42-Set SLEEP_CONFIG_SRC_ROOT to point the checks at a copy of `src/` (mutation
43-testing never touches the production tree); the self-mutation section at the
--
233-    for m in re.finditer(r"System\s*\.\s*sleep\s*\(\s*([A-Za-z_]\w*)\s*\)", scrubbed):
234-        var = m.group(1)
235-        seen_vars.append((var, line_of(text, m.start())))
236-        # A site's configuration starts at the nearest preceding point where
237-        # the object is created or reset: its declaration, or (in the old
238-        # shared-object pattern this WO removes) an assignment reset. Only
239-        # calls after that boundary belong to this site.
240-        boundaries = [b.start() for b in re.finditer(
241:            r"(?:SystemSleepConfiguration\s+%s\s*;)|(?:\b%s\s*=\s*SystemSleepConfiguration\s*\()"
242-            % (re.escape(var), re.escape(var)), scrubbed) if b.start() < m.start()]
243-        start_at = max(boundaries) if boundaries else -1
244-        calls = [c for c in calls_on(scrubbed, text, var)
245-                 if start_at < c[2] < m.start()]
246-        mode = None
247-        gpio = []
248-        gpio_guarded = []
249-        duration = None
250-        network = None
251-        network_guarded = False
252-        for method, args, off in calls:
253-            if method == "mode" and args:
--
258-                                                   scrubbed[max(0, off - 60):off].rsplit(var, 1)[0])))
259-            elif method == "duration" and args:
260-                duration = args[0]
261-            elif method == "network" and len(args) >= 2:
262-                network = (args[0], args[1])
263-                before = enclosing_text(text, scrubbed, off)
264-                network_guarded = ("if (useNetworkStandby)" in before
265-                                   and "#if HAL_PLATFORM_CELLULAR" in before)
266:        decl = re.search(r"(^|[;{}\n])([ \t]*)(static\s+)?SystemSleepConfiguration\s+%s\s*;"
267-                         % re.escape(var), scrubbed, re.M)
268-        decl_local = bool(decl) and decl.group(3) is None and decl.group(2) != ""
269-        if mode == "SystemSleepMode::STOP":
270-            tag = "stop-gpio" if gpio else "stop-timer-only"
271-        elif mode == "SystemSleepMode::HIBERNATE":
272-            tag = "hibernate"
273-        elif mode == "SystemSleepMode::ULTRA_LOW_POWER":
274-            tag = "ulp"
275-        else:
276-            tag = "unknown:%s" % mode
277-        sites[tag] = {
278-            "mode": mode, "gpio": gpio, "gpio_guarded": gpio_guarded, "duration": duration,
--
282-        }
283-    sites["__vars__"] = seen_vars
89-teardown_args = ["SleepTeardown::run(useNetworkStandbyRequested, useNetworkStandbyEffective, "
90-                 "standbyReturn.forceModemOff, standbyReturn.error)"]
91-mentions = [i for i, l in enumerate(code_lines) if "standbyReturn" in l]
92-for i in mentions:
93-    if i in allowed:
94-        continue
95-    if teardown_args[0] in code_lines[i]:
96-        continue
97-    fail(f"State_Sleep.cpp line {i + 1} mentions standbyReturn outside effectiveStandby(), sleepAndResume() "
98-         f"and the teardown call: {code_lines[i].strip()}")
99-if sum(teardown_args[0] in l for l in code_lines) != 1:
100-    fail("the SleepTeardown::run(...) call must pass standbyReturn.forceModemOff and standbyReturn.error, once")
101-for path in sorted(SRC.rglob("*")):
102-    if path != SLEEP and path.suffix in (".cpp", ".h") and "standbyReturn" in strip_comments(path.read_text()):
103-        fail(f"{path.relative_to(REPO_ROOT)} mentions standbyReturn")
104-print("PASS: only effectiveStandby(), sleepAndResume() and the teardown call arguments mention standbyReturn")
105-
106-# ---- 3. every SystemSleepConfiguration is a fresh local ----
107-fresh = 0
108-for path in sorted(SRC.rglob("*")):
109-    if path.suffix not in (".cpp", ".h"):
110-        continue
111-    text = strip_comments(path.read_text())
112-    for m in re.finditer(r"[^\n]*\bSystemSleepConfiguration\b[^\n]*", text):
113-        if not re.fullmatch(r"\s+SystemSleepConfiguration\s+\w+;", m.group(0)):
114:            fail(f"{path.relative_to(REPO_ROOT)}: SystemSleepConfiguration must be a fresh local, found: {m.group(0).strip()}")
23:     `RESET_CAUSE_OUT_OF_MEMORY` (7) - WO-2026-10-02-003 item B.
38:EXPECTED_SITE_COUNT = 7
42:OOM_CAUSE_VALUE = 7
109:            f"expected {EXPECTED_SITE_COUNT} System.reset() call sites in src/, found "
125:    # --- The out-of-memory site carries code 7 -------------------------------
<shellId: 37 completed with exit code 0>
```

</details>


---

<sub>14m 1s</sub>

### `bash`

**Move comment, retest**

```
$ python3 - <<'E'
p='src/power/ConnectivityFailsafe.cpp'
s=open(p).read()
s=s.replace("  SystemSleepConfiguration cfg; // RTC wake only: no GPIO source, so noise cannot cut the off period short\n","  // RTC wake only: no GPIO source, so noise cannot cut the off period short\n  SystemSleepConfiguration cfg;\n")
open(p,'w').write(s)
E
python3 tests/sleep_config_ownership_structural_test.py 2>&1 | grep -iE "fail" | head; python3 tests/sleep_execution_structural_test.py 2>&1 | grep -iE "fail"|head -3; sed -n 30,45p tests/reset_cause_structural_test.py; sed -n 95,125p tests/reset_cause_structural_test.py
```

<details>
<summary>43 lines</summary>

```
FAILED: power/ConnectivityFailsafe.cpp:134: file-scope SystemSleepConfiguration object - it is never destroyed, so every reset orphans a wake-source list
FAIL: src/power/ConnectivityFailsafe.cpp: SystemSleepConfiguration declared outside a function body
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC = REPO_ROOT / "src"
RESET_CAUSE_HEADER = SRC / "ResetCause.h"
MAIN = SRC / "Generalized-Core-Counter.cpp"

EXPECTED_SITE_COUNT = 7

# WO-2026-10-02-003 item B: the out-of-memory reset must carry its own code.
OOM_CAUSE_NAME = "RESET_CAUSE_OUT_OF_MEMORY"
OOM_CAUSE_VALUE = 7

# v50 S1: the binding-delete guard (cause 8) is retired; the code stays reserved.
BINDING_DELETE_CAUSE_NAME = "RESET_CAUSE_EXRTC_BINDING_DELETE"
        code = strip_comments(path.read_text())
        for match in re.finditer(r"System\.reset\s*\(([^;)]*)\)", code):
            argument = match.group(1).strip()
            line = code[: match.start()].count("\n") + 1
            where = f"{path.relative_to(REPO_ROOT)}:{line}"
            if not argument:
                fail(
                    f"{where} calls System.reset() with no cause code; every firmware-issued "
                    "reset must name itself (WO-2026-10-01-001 item C)"
                )
            sites.append((where, argument))

    if len(sites) != EXPECTED_SITE_COUNT:
        fail(
            f"expected {EXPECTED_SITE_COUNT} System.reset() call sites in src/, found "
            f"{len(sites)}: {[s[0] for s in sites]}. Add the new site's code to the "
            "ResetCause enum and update this test."
        )

    used = {}
    for where, argument in sites:
        if argument not in declared:
            fail(f"{where} passes '{argument}', which is not a ResetCause enum value")
        if argument in used:
            fail(
                f"{where} reuses the cause code '{argument}' already used by {used[argument]}; "
                "codes must be distinct so a reset can be attributed"
            )
        used[argument] = where

    # --- The out-of-memory site carries code 7 -------------------------------
<shellId: 38 completed with exit code 0>
```

</details>


---

<sub>14m 6s</sub>

### `bash`

**View scope detection logic**

`$ grep -n "file-scope" -B30 tests/sleep_config_ownership_structural_test.py | sed -n 1,80p; grep -n "declared outside a function body" -B25 tests/sleep_execution_structural_test.py`

<details>
<summary>89 lines</summary>

```
1-#!/usr/bin/env python3
2-"""
3-WO-2026-10-02-003 (v34-SleepConfigLeak) - structural test for sleep
4-configuration ownership.
5-
6-Device OS's `SystemSleepConfiguration` move assignment
7-(`system/inc/system_sleep_configuration.h:205-210` in 6.4.1) `memcpy`s the
8-incoming configuration over the existing one *without* freeing the old
9-`wakeup_sources` list. Only the destructor frees that list. A long-lived
10-(global or static) configuration that is reset with
11-`config = SystemSleepConfiguration();` before each sleep is therefore never
12-destroyed, and orphans its whole wake-source list on every single wake
13-cycle. The fix is to build a *fresh local* configuration at each sleep site,
14-so the destructor runs and frees the list when the scope ends.
15-
16-This test is a source-invariant check on the REAL checked-in files.
17-Comments and string/char literals are blanked out before any check, so prose
18-(including this file's own history, if pasted into source) cannot produce a
19-false positive or a false negative.
20-
21-Checks (WO Acceptance 1 and 3):
22-
23-  1. No reused long-lived sleep configuration remains anywhere in `src/`:
24:     no file-scope or `static` `SystemSleepConfiguration` object, no
--
298-            text = fh.read()
299-        if "SystemSleepConfiguration" not in text:
300-            continue
301-        scrubbed = scrub(text)
302-        for m in extern_re.finditer(scrubbed):
303-            fail("%s:%d: extern SystemSleepConfiguration - the shared sleep "
304-                 "configuration must not come back" % (rel, line_of(text, m.start())))
305-        for m in static_re.finditer(scrubbed):
306-            fail("%s:%d: static SystemSleepConfiguration - a long-lived "
307-                 "configuration is never destroyed and leaks its wake-source "
308-                 "list on every reset" % (rel, line_of(text, m.start())))
309-        for m in assign_re.finditer(scrubbed):
310-            fail("%s:%d: assignment to an existing SystemSleepConfiguration - "
311-                 "Device OS's move assignment memcpys over the old "
312-                 "wakeup_sources list without freeing it"
313-                 % (rel, line_of(text, m.start())))
314-        # Any assignment to a declared configuration variable, whatever the
315-        # right-hand side (`= {}`, `= std::move(...)`, ...), reaches the same
316-        # leaking move assignment (WO-2026-10-02-003 Stage 7 round 1, P2).
317-        names = set(re.findall(r"\bSystemSleepConfiguration\s+([A-Za-z_]\w*)", scrubbed))
318-        for name in names:
319-            for m in re.finditer(r"(?<![\w.>:])%s\s*=(?!=)" % re.escape(name), scrubbed):
320-                if re.search(r"SystemSleepConfiguration\s+$", scrubbed[:m.start()]):
321-                    continue  # a declaration with an initializer, not an assignment
322-                fail("%s:%d: assignment to SystemSleepConfiguration variable '%s' - "
323-                     "Device OS's move assignment memcpys over the old "
324-                     "wakeup_sources list without freeing it"
325-                     % (rel, line_of(text, m.start()), name))
326-        for m in decl_re.finditer(scrubbed):
327-            decl = m.group(0)
328:            # A file-scope definition has no leading indentation on its line.
329-            line_start = scrubbed.rfind("\n", 0, m.end()) + 1
330-            indent = scrubbed[line_start:m.end()]
331-            indent = indent[:len(indent) - len(indent.lstrip())]
332-            at_file_scope = scrubbed.count("{", 0, m.start()) == scrubbed.count("}", 0, m.start())
333-            if at_file_scope and "extern" not in decl:
334:                fail("%s:%d: file-scope SystemSleepConfiguration object - it is "
92-for i in mentions:
93-    if i in allowed:
94-        continue
95-    if teardown_args[0] in code_lines[i]:
96-        continue
97-    fail(f"State_Sleep.cpp line {i + 1} mentions standbyReturn outside effectiveStandby(), sleepAndResume() "
98-         f"and the teardown call: {code_lines[i].strip()}")
99-if sum(teardown_args[0] in l for l in code_lines) != 1:
100-    fail("the SleepTeardown::run(...) call must pass standbyReturn.forceModemOff and standbyReturn.error, once")
101-for path in sorted(SRC.rglob("*")):
102-    if path != SLEEP and path.suffix in (".cpp", ".h") and "standbyReturn" in strip_comments(path.read_text()):
103-        fail(f"{path.relative_to(REPO_ROOT)} mentions standbyReturn")
104-print("PASS: only effectiveStandby(), sleepAndResume() and the teardown call arguments mention standbyReturn")
105-
106-# ---- 3. every SystemSleepConfiguration is a fresh local ----
107-fresh = 0
108-for path in sorted(SRC.rglob("*")):
109-    if path.suffix not in (".cpp", ".h"):
110-        continue
111-    text = strip_comments(path.read_text())
112-    for m in re.finditer(r"[^\n]*\bSystemSleepConfiguration\b[^\n]*", text):
113-        if not re.fullmatch(r"\s+SystemSleepConfiguration\s+\w+;", m.group(0)):
114-            fail(f"{path.relative_to(REPO_ROOT)}: SystemSleepConfiguration must be a fresh local, found: {m.group(0).strip()}")
115-        depth = text[:m.start()].count("{") - text[:m.start()].count("}")
116-        if depth < 2:
117:            fail(f"{path.relative_to(REPO_ROOT)}: SystemSleepConfiguration declared outside a function body")
<shellId: 39 completed with exit code 0>
```

</details>


---

<sub>14m 12s</sub>

### `bash`

**Compute brace depth**

```
$ python3 - <<'E'
t=open('src/power/ConnectivityFailsafe.cpp').read()
i=t.index("SystemSleepConfiguration cfg")
print(t[:i].count("{")-t[:i].count("}"))
E
grep -n "^void\|^namespace\|^}" src/state/State_Sleep.cpp | head -20
```

<details>
<summary>22 lines</summary>

```
1
31:namespace {
33:void logWakeSummary(const char *reason) {
40:}
43:void drainSerialBeforeSleep() {
48:}
70:}
118:}
131:} cloudGate;
352:}
355:void endOfCycleSummary(bool useNetworkStandby) {
424:}
432:} wifiGuard;
486:}
627:}
659:}
687:}
689:void hibernateAttempt(bool overnightFallbackSleep, int wakeInSeconds) {
746:}
986:}
988:} // namespace
<shellId: 40 completed with exit code 0>
```

</details>


---

<sub>14m 16s</sub>

### `bash`

**Inspect decl\_re and scrub**

`$ grep -n "decl_re\s*=\|def scrub" -A22 tests/sleep_config_ownership_structural_test.py | head -70`

<details>
<summary>48 lines</summary>

```
98:def scrub(text):
99-    """Blank out comments and string/char literals, preserving offsets."""
100-    out = list(text)
101-    i, n = 0, len(text)
102-    while i < n:
103-        c = text[i]
104-        if c == '/' and i + 1 < n and text[i + 1] == '/':
105-            while i < n and text[i] != '\n':
106-                out[i] = ' '
107-                i += 1
108-        elif c == '/' and i + 1 < n and text[i + 1] == '*':
109-            while i < n and not (text[i] == '*' and i + 1 < n and text[i + 1] == '/'):
110-                if text[i] != '\n':
111-                    out[i] = ' '
112-                i += 1
113-            for j in range(i, min(i + 2, n)):
114-                out[j] = ' '
115-            i += 2
116-        elif c in '"\'':
117-            quote = c
118-            out[i] = ' '
119-            i += 1
120-            while i < n and text[i] != quote:
--
289:    decl_re = re.compile(
290-        r"(?:^|[;{}])\s*(?:static\s+|extern\s+)?SystemSleepConfiguration\s+[A-Za-z_]\w*\s*[;=]")
291-    assign_re = re.compile(r"=\s*SystemSleepConfiguration\s*\(")
292-    extern_re = re.compile(r"\bextern\s+SystemSleepConfiguration\b")
293-    static_re = re.compile(r"\bstatic\s+SystemSleepConfiguration\b")
294-
295-    for path in sources(src_root):
296-        rel = os.path.relpath(path, src_root)
297-        with open(path, "r", errors="replace") as fh:
298-            text = fh.read()
299-        if "SystemSleepConfiguration" not in text:
300-            continue
301-        scrubbed = scrub(text)
302-        for m in extern_re.finditer(scrubbed):
303-            fail("%s:%d: extern SystemSleepConfiguration - the shared sleep "
304-                 "configuration must not come back" % (rel, line_of(text, m.start())))
305-        for m in static_re.finditer(scrubbed):
306-            fail("%s:%d: static SystemSleepConfiguration - a long-lived "
307-                 "configuration is never destroyed and leaks its wake-source "
308-                 "list on every reset" % (rel, line_of(text, m.start())))
309-        for m in assign_re.finditer(scrubbed):
310-            fail("%s:%d: assignment to an existing SystemSleepConfiguration - "
311-                 "Device OS's move assignment memcpys over the old "
<shellId: 41 completed with exit code 0>
```

</details>


---

<sub>14m 31s</sub>

### `bash`

**Adjust forced tests and rerun**

```
$ python3 - <<'E'
p='tests/sleep_config_ownership_structural_test.py'
s=open(p).read()
a='at_file_scope = scrubbed.count("{", 0, m.start()) == scrubbed.count("}", 0, m.start())'
assert a in s
s=s.replace(a,'at_file_scope = scrubbed.count("{", 0, m.end()) == scrubbed.count("}", 0, m.end())  # m.start() may be the function\'s own "{"')
open(p,'w').write(s)
p='tests/sleep_execution_structural_test.py'
s=open(p).read()
a="        if depth < 2:\n"
assert a in s
s=s.replace(a,"        if depth < 1:  # a function body, in a namespace or (the S3 power-off helper) at file level\n")
open(p,'w').write(s)
p='tests/reset_cause_structural_test.py'
s=open(p).read()
s=s.replace("EXPECTED_SITE_COUNT = 7","EXPECTED_SITE_COUNT = 9  # 7 primary sites + the two S3 fallback resets that follow a returned POWER_OFF")
a='''        if argument in used:
            fail('''
assert a in s
s=s.replace(a,'''        if argument in used and used[argument].split(":")[0] == where.split(":")[0] and argument in S3_FALLBACK_CAUSES:
            continue  # S3: a returned POWER_OFF falls back to the same caller's own reset cause
        if argument in used:
            fail(''')
s=s.replace("OOM_CAUSE_VALUE = 7\n","OOM_CAUSE_VALUE = 7\n\nS3_FALLBACK_CAUSES = {\"RESET_CAUSE_CONNECTIVITY_FAILSAFE\", \"RESET_CAUSE_ERROR_STATE_SOFT\"}\n",1)
open(p,'w').write(s)
E
for t in reset_cause_structural sleep_config_ownership_structural sleep_execution_structural; do python3 tests/${t}_test.py 2>&1 | grep -iE "fail|passed|ok$" | head -3; done
```

```
Reset cause structural test passed (9 call sites, distinct non-zero codes, resetReasonData still published)
sleep_config_ownership_structural_test passed
FAIL: expected 5 fresh SystemSleepConfiguration locals (hibernate, ULP, STOP, STOP timer-only, boot storm), found 6
<shellId: 42 completed with exit code 0>
```


---

<sub>14m 35s</sub>

### `bash`

**View count check**

`$ grep -n "expected 5 fresh" -B3 tests/sleep_execution_structural_test.py`

```
117-            fail(f"{path.relative_to(REPO_ROOT)}: SystemSleepConfiguration declared outside a function body")
118-        fresh += 1
119-if fresh != 5:
120:    fail(f"expected 5 fresh SystemSleepConfiguration locals (hibernate, ULP, STOP, STOP timer-only, boot storm), found {fresh}")
<shellId: 43 completed with exit code 0>
```


---

<sub>14m 39s</sub>

### `bash`

**Update count and rerun**

```
$ python3 - <<'E'
p='tests/sleep_execution_structural_test.py'
s=open(p).read()
s=s.replace("if fresh != 5:","if fresh != 6:").replace("expected 5 fresh SystemSleepConfiguration locals (hibernate, ULP, STOP, STOP timer-only, boot storm)","expected 6 fresh SystemSleepConfiguration locals (hibernate, ULP, STOP, STOP timer-only, boot storm, S3 power-off helper)")
open(p,'w').write(s)
E
python3 tests/sleep_execution_structural_test.py 2>&1 | tail -3
```

```
PASS: SleepTeardown.{h,cpp} unchanged, no new file under src/, nothing retained added (base 1e6d34f)
PASS: effectiveStandby(), hibernateAttempt() and sleepAndResume() are byte-identical to the base blocks apart from the field renames, `return true;` and the added `wake = {...};` / `return false;`
sleep_execution_structural_test: all invariants hold
<shellId: 44 completed with exit code 0>
```


---

<sub>18m 28s</sub>

### `bash`

**Rebuild, nm, clean scratch**

`$ S=build-tmp/wo-2026-11-05-005-stage6; rm -rf $S/app $S/bpb; mkdir -p $S/app && cp -R src lib project.properties $S/app/ && export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && cd ~/.particle/toolchains/deviceOS/6.6.0/main && A=$OLDPWD && make -s PLATFORM=boron APPDIR=$A/$S/app TARGET_DIR=$A/$S/app/localbuild DEVICE_OS_PATH=$HOME/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE=$A/$S/bpb > $A/$S/build.log 2>&1; echo rc=$?; tail -2 $A/$S/build.log | cut -c1-80; arm-none-eabi-nm -C $A/$S/app/localbuild/app.elf | grep -i "powerOffForRecovery\|deepPowerDown"; arm-none-eabi-size $A/$S/app/localbuild/app.elf | tail -1 | cut -c1-50; cd $A; rm -rf $S/app $S/bpb; git status --short`

<details>
<summary>19 lines</summary>

```
rc=0
   text	   data	    bss	    dec	    hex	filename
 150908	   1114	   2240	 154262	  25a96	/private/tmp/claude-501/-Users-chipmc-Do
000bdf1c T powerOffForRecovery()
 150908	   1114	   2240	 154262	  25a96	/private/t
 M src/power/ConnectivityFailsafe.cpp
 M src/power/ConnectivityFailsafe.h
 M src/state/State_Error.cpp
 M tests/connectivity_failsafe_stage1_retired_test.py
 M tests/connectivity_failsafe_unknown_openness_test.cpp
 M tests/deliberate_reset_flush_test.py
 M tests/modem_teardown_confirmation_logging_test.py
 M tests/reset_cause_structural_test.py
 M tests/s2_alarm_watchdog_wake_structural_test.py
 M tests/sleep_config_ownership_structural_test.py
 M tests/sleep_execution_structural_test.py
?? tests/s3_timed_power_off_test.cpp
?? tests/s3_timed_power_off_test.sh
<shellId: 45 completed with exit code 0>
```

</details>


---

<sub>26m 56s</sub>

### `bash`

**Full suite, numstat, clean-export thermal**

```
$ pass=0; total=0; failed=(); for t in tests/*.sh; do total=$((total+1)); if zsh "$t" >/dev/null 2>&1; then pass=$((pass+1)); else failed+=("$t"); fi; done; for t in tests/*.py; do total=$((total+1)); if python3 "$t" >/dev/null 2>&1; then pass=$((pass+1)); else failed+=("$t"); fi; done; echo "$pass/$total (sh via zsh, py via python3)"; printf 'FAILED: %s\n' "${failed[@]}"; git diff --numstat 62e3dbc -- src; 
E=build-tmp/wo-2026-11-05-005-stage6/export; rm -rf $E; mkdir -p $E && git ls-files -z | xargs -0 -I{} cp --parents {} $E 2>/dev/null || (git ls-files | tar -cf - -T - | tar -xf - -C $E); cp src/power/ConnectivityFailsafe.* $E/src/power/; python3 $E/tests/thermal_coupling_structural_test.py 2>&1 | tail -2
```

<details>
<summary>8 lines</summary>

```
121/121 (sh via zsh, py via python3)
FAILED: 
10	1	src/power/ConnectivityFailsafe.cpp
2	0	src/power/ConnectivityFailsafe.h
3	1	src/state/State_Error.cpp
      with no `static` cross-call accumulator (AC-C9) and all
      samples are collected within a single bounded loop (AC-C10).
<shellId: 46 completed with exit code 0>
```

</details>


---

<sub>27m 3s</sub>

### `bash`

**Thermal on clean tracked export**

`$ E=build-tmp/wo-2026-11-05-005-stage6/export; rm -rf $E; mkdir -p $E; git ls-files | tar -cf - -T - | tar -xf - -C $E; python3 $E/tests/thermal_coupling_structural_test.py >/dev/null 2>&1; echo thermal-clean-export rc=$?; rm -rf $E; ls build-tmp/wo-2026-11-05-005-stage6; git ls-files | grep -c "^tests/s3_" `

```
thermal-clean-export rc=0
build.log
mut.py
0
<shellId: 47 completed with exit code 1>
```


---

<sub>27m 17s</sub>

### `bash`

**Line numbers for report**

`$ grep -n "powerOffForRecovery\|System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE)\|deep-powerdown" src/power/ConnectivityFailsafe.cpp; grep -n "powerOffForRecovery\|Executing deep power\|kPowerOffSentinel); \|System.reset(RESET_CAUSE_ERROR_STATE_SOFT)" src/state/State_Error.cpp; D=~/.particle/toolchains/deviceOS/6.6.0/hal/shared/am18x5.cpp; sed -n 1058,1075p $D; ls docs/work-orders | grep "005-stage6"`

<details>
<summary>31 lines</summary>

```
134:void powerOffForRecovery() {
315:    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);
321:  Log.info("Failsafe: stage=3 action=deep-powerdown age=%lds",
324:  powerOffForRecovery();
326:  System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);
168:      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);
177:      Log.info("Executing deep power down from ERROR_STATE (alert=%d)", RecoveryState::get_alertCode());
178:      RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog
180:      powerOffForRecovery();
182:      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);
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
WO-2026-11-03-005-stage6-copilot-dispatch.md
WO-2026-11-03-005-stage6-copilot-report.md
WO-2026-11-03-005-stage6-copilot-transcript.md
WO-2026-11-05-005-stage6-copilot-dispatch.md
<shellId: 48 completed with exit code 0>
```

</details>


---

<sub>27m 51s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/docs/work-orders/WO-2026-11-05-005-stage6-copilot-report.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/docs/work-orders/WO-2026-11-05-005-stage6-copilot-report.md`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s3/docs/work-orders/WO-2026-11-05-005-stage6-copilot-report.md with 9990 characters
```


---

<sub>27m 59s</sub>

### Copilot

Stage 3 and the ERROR_STATE hard recovery now call one shared timed power-off helper instead of `ab1805.deepPowerDown()`. Nothing is committed, pushed or flashed, and I make no hardware claims.

- **Helper:** `powerOffForRecovery()` in `ConnectivityFailsafe.cpp` builds `POWER_OFF` with a 30 s off time (`kPowerOffRecoverySec`) and no GPIO wake. It logs and returns on any error.
- **Callers:** both keep their log lines, flush and S2 after-return blocks. They then call `System.reset(<cause>)` as the fallback: `RESET_CAUSE_CONNECTIVITY_FAILSAFE` for stage 3, `RESET_CAUSE_ERROR_STATE_SOFT` for ERROR_STATE.
- **Budget:** `src/` nets +13 against 62e3dbc (limit +20).
- **Tests:** the host suite passes 121/121 (sh via zsh, py via python3), including the new `s3_timed_power_off_test` and `thermal_coupling_structural_test.py` on a clean export. All 13 mutations I ran were caught. They ran on copies of `src/`, so the real tree was never touched.
- **Build:** Device OS 6.6.0 gives 150908 / 1114 / 2240 text/data/bss, and `nm` shows `powerOffForRecovery()` linked.
- **Existing tests changed:** the new call forced edits to eight existing tests. The forced edits include allowing the two fallback resets in the reset-cause test and relaxing two sleep-config checks. The failsafe ladder expectations are untouched, and the report lists each edit.
- **Step 0:** nothing broke the plan. The vendored call used 30 s by default, not the 120 s P7 observed. The native driver disables the chip watchdog itself, so I added no stop.
- **Deviation:** `State_Error.cpp` has no direct `AB1805_RK.h` include, so there was nothing to remove there.
- **Open question:** S2's stage-3 evidence needs a valid clock at the reason-20 boot check. If the clock is invalid after the power-off, that recovery would be counted as an inferred watchdog. That is S2 logic I was not allowed to change.
- **Reasoning level:** dispatched as medium; I couldn't observe the level actually run.

The report is at `docs/work-orders/WO-2026-11-05-005-stage6-copilot-report.md`.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>
