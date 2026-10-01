# WO-2026-10-01-001: v31, connectivity and diagnosability fixes

**Goal, in plain language:** a device doesn't go to sleep in the middle of a firmware download that's still making progress; its long connection attempt comes round when it should, even if the previous attempts failed; after any reset the firmware issues itself, the next status names the code that issued it; and the signal reads "not available" when Device OS has no reading, instead of 0/0.

**Status:** Opened 2026-10-01. Stage 5 decided by Chip and the architect; open points decided by Chip 2026-10-01. Round 1: Stage 7 NOT VERIFIED (A). **Round 2 of 2 (item A, Particle's reference pattern): Stage 7 VERIFIED WITH NOTES.** B, C, D VERIFIED in round 1 and unchanged. **At USER GATE 2** (Stage 8: Chip).

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`: Stage 5 → Stage 6 (Copilot) → Stage 7 (Codex), §5 model routing, and §12 (plain goal, size budget, two-round rule, check facts against their source).

**Spec source:** `docs/work-orders/2026-10-01-connectivity-investigation-codex-report.md` (report proposals #1, #2, #3, #5). Dispatch: `docs/work-orders/2026-10-01-connectivity-investigation-codex-dispatch.md`.

**Branch:** `wo/2026-10-01-001-connectivity-fixes`, from `9e10778` (the PR #53 head, v30), because v30's merge is held for MAFC-1 and Court3 (Chip, 2026-10-01). Rebase onto `main` once PR #53 merges; if the merge changes any file below, re-check the cited lines. Every file:line below was checked on 2026-10-01 against `9e10778`.

**Version:** `v31-ConnectivityFixes`, product version 31, set with `bump_version.sh`.

## Problem (evidence)

From the investigation (Codex, gpt-6-astra high, 2026-10-01). Claude Code checked the key claims against the source:

1. **A device can sleep in the middle of a download.** Device OS clears the "update pending" flag when a download starts (`system/src/firmware_update.cpp:163`, Device OS 6.4.1). So `System.updatesPending()`, which every existing check relies on, is false during the transfer. On 30 Sep from 21:43 SGT, Dev-09 slept partway through its v30 download about five times. Each time, `Low-power idle: no updates pending` was logged just after `Starting firmware update`, followed by a 70 s queue gate and sleep. Its closing report was delivered 8 h late. Dev-14 finished the same image in one session.
2. **The long attempt can be starved.** The connection-attempt counter only goes up after a successful connection (`State_Connect.cpp:554–556`). A device at or below 50% charge whose connections keep failing never reaches `DEEP_ATTEMPT_COUNTER_THRESHOLD` (3). So it never gets the 660 s attempt that leaves room for Device OS's 10-minute modem power-cycle (Particle cellular connect guidance: wait at least 11 minutes).
3. **A firmware-issued reset can't be attributed.** PCKL1 (e00fce6865fe7770b80a014e) was silent for about an hour after its 22:00 EDT closing report on 30 Sep, then sent status 03:01:59Z: `resetReason=140`, `resetReasonData=0`, `appBreadcrumb=18`, `failsafeStage=0`, `alert=0`. Reason 140 means `System.reset()`, and six call sites can issue it. Nothing records which one did.
4. **0/0 is a conversion artefact.** Before the network is ready, `Cellular.RSSI()` returns an invalid signal with −1 percentages. `sampleConnectionSignal()` rounds that to 0 and sets `valid = true` unconditionally (`State_Connect.cpp:63–71`). So the `sig=na` branches that already exist in the logs never run.

## Change

**Total size budget: at most about 65 lines of `src/` (tests separate).** Going over any item's budget means stop and report (guardrail 3). One Copilot round (two-round rule: a second round needs Chip).

### A — round 2 (supersedes the round-1 design below; Chip, 2026-10-01). Budget about 30 lines.

**Reference design:** Particle, "Wake publish sleep (cellular)", https://docs.particle.io/firmware/low-power/wake-publish-sleep-cellular/ ("Finite state machines", and the `STATE_SLEEP`, `STATE_FIRMWARE_UPDATE` and `firmwareUpdateHandler` code). The saved copy is `build-tmp/connectivity-archive/particle-docs/firmware_low-power_wake-publish-sleep-cellular.txt`, lines 216–413. In the reference, the handler sets `firmwareUpdateInProgress` on `firmware_update_begin` and clears it on `complete`/`failed`. `STATE_SLEEP` first checks the flag and goes to `STATE_FIRMWARE_UPDATE`. That state returns to `STATE_SLEEP` when the flag clears or after `firmwareUpdateMaxTime` (5 min). On completion Device OS resets the device itself.

1. **Handler (record only):** `firmware_update` begin sets `firmwareUpdateInProgress`; complete and failed clear it. **Deliberate difference 1:** begin and progress also record `millis()` as the last activity.
2. **Check before sleeping:** in `handleSleepingState()`, if `firmwareUpdateInProgress`, `transitionTo(FIRMWARE_UPDATE_STATE, …)` and return, instead of continuing toward sleep.
   - **Placement (Claude Code, checked against the source 2026-10-01):** the check sits **before the first teardown request** (cloud disconnect / radio off at `State_Sleep.cpp:755`, `:773`, `:903`), on every pass while no disconnect has been requested. It must not be at the night-sleep commitment (`:973`) or the sleep calls (`:1134`, `:1386`), which run only after teardown, when a download would already be lost. That was Stage 7 round 1's P1.
   - This is the reference's placement, at the top of `STATE_SLEEP`.
   - It covers both night sleep and regular sleep, because both commitments are reached only through this handler.
3. **`handleFirmwareUpdateState()`** exits to `SLEEPING_STATE` when:
   - the flag clears, or
   - 5 minutes pass with no `firmware_update_progress` event. **Deliberate difference 2:** the timer starts at entry and restarts on each progress event; the reference uses a fixed 5 minutes.

   The button override stays. There is no special handling for `complete`: it clears the flag, and Device OS resets the device.
4. **Kept from round 1:**
   - ThrashGuard `markProgress` on each new recorded activity (**deliberate difference 3**);
   - ThrashGuard's update-state timeout of 330 s;
   - the webhook-ack hold (3 lines, verified);
   - the configuration-load block.
5. **Removed from round 1:**
   - the loop-level begin transition and its flag;
   - the last-event record;
   - the complete→Idle and failed→Idle exits.

   Round 2 replaces Stage 7's three separate fixes: begin consumed too late, the begin flag left set, and the stale terminal event. Each is removed by construction.
6. **Unchanged:** entry on `System.updatesPending()` (`State_Connect.cpp:651–652`). With the flag clear, such an entry now leaves straight for `SLEEPING_STATE`, where the check in step 2 catches a download that starts there. (Before, it waited up to 5 minutes, and after round 1 until progress stopped.)

#### A — round 1 (superseded; kept as the record)

**A (round 1). Don't sleep while a firmware download is making progress (report #1, Chip's design). Budget ≤ 20 lines.**

1. Subscribe to the `firmware_update` system event (`System.on(firmware_update, …)`, next to the existing `System.on(out_of_memory, …)` at `Generalized-Core-Counter.cpp:887`). The handler **only records** the event (begin / progress / complete / failed, and the `millis()` of the latest begin or progress). It makes no transitions. Parameters, per Device OS 6.4.1 `system/inc/system_event.h:52, 77–80` and the Particle docs (System events, `firmware_update`): `firmware_update_begin` = 0, `firmware_update_complete` = 1, `firmware_update_progress` = 2, `firmware_update_failed` = −1.
2. **The main loop makes the transition.** On a recorded `begin`, from any state other than `FIRMWARE_UPDATE_STATE`, `transitionTo(FIRMWARE_UPDATE_STATE, …)`. Put this with the other loop-level transitions (the user-switch block at `Generalized-Core-Counter.cpp:1684–1689`).
3. **`handleFirmwareUpdateState()` (`State_Connect.cpp:722–777`) exits only on:**
   - `complete` → `IDLE_STATE` (the existing "update complete" target);
   - `failed` → `IDLE_STATE`;
   - **5 minutes with no `progress` event** → `SLEEPING_STATE` (the existing timeout target). The timer starts at entry and restarts on each `progress` event;
   - the existing button override (`:760–765`).
4. **Remove the other exits from the state:** the `!System.updatesPending()` exit (`:752–757`) and the fixed cap (`:767–774`, `FIRMWARE_UPDATE_MAX_MS`, `ConnectivityPolicy.h:94`). The sleep gate (`State_Sleep.cpp:490`) and Idle's `updatesPending` checks (`State_Idle.cpp:225`, `:269`) stay as they are. They can't run while the state machine is in `FIRMWARE_UPDATE_STATE`, and step 2 moves the device into that state on `begin`.
5. **Webhook-ack timeout:** pause it while in the state **only if that takes ≤ 3 lines**. Otherwise leave it, and say so in the report.
6. Entry on `System.updatesPending()` at `State_Connect.cpp:651–652` is unchanged. A device that enters that way leaves after 5 minutes without a `begin`/`progress` event, which matches today's cap.

### B. The "deep budget after 3 attempts" rule works as intended (report #2, bug fix only). Budget ≤ 5 lines.

Count a failed attempt toward `connectionAttemptCounter` at the connection-timeout path (`State_Connect.cpp:704–718`, before `transitionTo(SLEEPING_STATE, "connect-timeout")`), with the same `< DEEP_ATTEMPT_COUNTER_THRESHOLD` guard as the success-path increment (`:554–556`). Unchanged:
- the success increment;
- the reset to 0 when a deep attempt starts (`:276–281`);
- the thresholds (`ConnectivityPolicy.h:80–81`);
- the "above 50%" rule (`State_Connect.cpp:118–121`).

### C. After any firmware-issued reset, the next startup status names the code that issued it (report #3, narrowed). Budget ≤ 25 lines.

**Decided design (Stage 5 decision 2):** one reset-cause enum with distinct non-zero values, and each `System.reset()` call in `src/` becomes `System.reset(CAUSE_…)`. Device OS 6.4.1 `System.reset(uint32_t data)` (`wiring/inc/spark_wiring_system.h:437`) resets with `RESET_REASON_USER` (140) and hands `data` to the next boot as `System.resetReasonData()` (`wiring/src/spark_wiring_system.cpp:52–55`). The startup status already publishes it (`Generalized-Core-Counter.cpp:2129`, `:2189`, `:2218`), so nothing new is retained, published or cleared. (This replaces the original retained-variable design.)

The `System.reset()` call sites in `src/` (checked 2026-10-01). There are **six**, not the three the investigation dispatch assumed:

| Site | Path |
|---|---|
| `Generalized-Core-Counter.cpp:1813` | `appWatchdogHandler()`, `#else` of `Wiring_Watchdog` (not compiled on Boron, but in `src/`) |
| `Generalized-Core-Counter.cpp:2632` | connectivity failsafe stage 2 |
| `ThrashGuard.cpp:151` | ThrashGuard tier 3 |
| `State_Sleep.cpp:1171` | sleep path (heap guard after the gate) |
| `State_Sleep.cpp:1469` | sleep path (exhausted sleep after the call) |
| `State_Error.cpp:161` | error state soft reset |

**Outside `System.reset()`:** `ab1805.deepPowerDown()` at `Generalized-Core-Counter.cpp:2640` and `State_Error.cpp:171` falls back to a library `System.reset()` after 30 s if the power-down fails (`lib/AB1805_RK/src/AB1805_RK.cpp:583`). That also reports 140, with `resetReasonData=0`. Under the decided design, 140 with data 0 means the reset came from outside our `src/` reset sites. Tagging the library fallback is out of scope.

### D. Signal reads "not available" when Device OS has no reading (report #5). Budget ≤ 12 lines (expected about 3).

In `sampleConnectionSignal()` (`State_Connect.cpp:63–71`), set `valid` only when Device OS's reading is valid (strength and quality ≥ 0), and leave −1 otherwise, instead of rounding and setting `valid = true` unconditionally. Make the same fix in the WiFi branch (`:72–76`) for consistency. **No log line needs to change:** the `sig=na` branches already exist for ConnDiag (`:469`), ConnSummary ok (`:519`) and ConnSummary fail (`:693`). The `Connect: ok … sig=%.0f/%.0f` lines (`:612`, `:622`) read `Cellular.RSSI()` directly after the cloud connects, when the reading is valid. Leave them unchanged.

## Stage 5 decisions (Chip, 2026-10-01, after the opening dispatch)

1. **A: ThrashGuard is another exit from the update state.** `ThrashGuard::timeoutForStateSec(FIRMWARE_UPDATE_STATE)` is 180 s (`ThrashGuard.cpp:66–67`). Without `markProgress`, a trip happens 180 s into any download. Tier 2 (a second trip within its window) runs `requestFullDisconnectAndRadioOff()` and moves to `SLEEPING_STATE` (`:139–145`), and tier 3 resets (`:148–151`). So the state can be ended by something other than the four exits Chip specified. **Decided (approved, within A's 20 lines):** call `thrashGuard.markProgress(...)` on each recorded `progress` event, and raise the update-state timeout to above 300 s (e.g. 330 s) so ThrashGuard doesn't pre-empt the 5-minute no-progress exit. Other loop-level exits stay: the out-of-memory transition to `ERROR_STATE` (`Generalized-Core-Counter.cpp:1680`), a safety exit, and the user switch to `REPORTING_STATE` (`:1688`), which is a button override.
2. **C: a smaller design using `System.reset(data)`.** Device OS 6.4.1 has `System.reset(uint32_t data)` (`wiring/inc/spark_wiring_system.h:437`). It resets with `RESET_REASON_USER` (140) and hands `data` to the next boot as `System.resetReasonData()` (`wiring/src/spark_wiring_system.cpp:52–55`). The startup status **already publishes** `resetReasonData` (`Generalized-Core-Counter.cpp:2129`, `:2189`, `:2218`). So: one cause-code enum, and `System.reset()` becomes `System.reset(CAUSE_…)` at the six sites.
   - **Advantages:** about 10 lines, no new retained state, no linker-map check, nothing to clear (it's per boot by construction), and no status payload growth.
   - **Side benefit:** `140` with `resetReasonData=0` then means "not our code" (the AB1805 library fallback, a cloud reset, or Device OS), which narrows PCKL1-type cases.
   - **Trade-off:** it can't tag the AB1805 library's fallback reset. The retained-variable design could, by setting the code before `deepPowerDown()`. But that needs a careful clear, because a successful power-down wakes with a different reset reason.
   - **Decided:** `System.reset(data)`. Stage 7's linker-map check is replaced by a host check that the status payload carries `resetReasonData` (already true).
3. **Branch (decided):** v30 hasn't merged (PR #53 open, release held for MAFC-1 and Court3). Branch from the PR #53 head (`9e10778`) now, and rebase onto `main` after the merge.

## Permitted files

`src/Generalized-Core-Counter.cpp`, `src/state/State_Connect.cpp`, `src/ThrashGuard.cpp` (item A: update-state timeout and progress refresh only), `src/state/State_Sleep.cpp` and `src/state/State_Error.cpp` (item C reset sites only), `src/power/ConnectivityPolicy.h` (only to retire `FIRMWARE_UPDATE_MAX_MS` if it becomes unused, or to add the no-progress constant), a new or existing small header for the reset-cause enum (item C), `src/FirmwareVersion.h`, `CHANGELOG.md` and the version files via `bump_version.sh`, and `tests/`.

## Protected (must not change)

- The connection budgets and thresholds (`ConnectivityPolicy.h:62–65`, `:80–81`) and the "above 50%" rule.
- The sleep gate's logic other than item A's stated scope.
- Device OS recovery behavior (no new modem resets).
- The status payload's existing fields and order.
- `lib/`.
- The `WITH_ACK` publish path (`tests/publish_with_ack_structural_test.py` must pass unchanged).

## Non-goals (deferred, recorded)

- **Report #2's redesign:** every 4th retry long, a deep attempt at boot, and dropping the "above 50%" rule. This trades battery against connection latency and needs Chip's decision.
- **Report #4:** a cap on total awake time across repeated retries in one wake. Wait until item C (and further telemetry) shows how the existing bounds were bypassed.
- **Report #6:** the modem-shutdown standby suppression. Measure first and change nothing yet.
- Anything about breadcrumb-18 watchdog stalls (WO-2026-09-25-005) or diagnostic backlog resets (WO-2026-09-21-003).

## Tests (Stage 6 adds; Stage 7 verifies)

- **A:**
  - `begin` enters `FIRMWARE_UPDATE_STATE` from Sleep, Idle and Connecting.
  - `progress` events keep it there past 5 minutes in total.
  - 5 minutes without `progress` exits to Sleep.
  - `complete` and `failed` exit to Idle.
  - No sleep transition happens while in the state.
  - The event handler makes no transitions (structural).
  - **Mutations:** restoring the fixed cap, or restoring the `updatesPending()` exit, fails a test. Removing the progress `markProgress`, or lowering the update-state ThrashGuard timeout below 300 s, fails a test.
- **B:**
  - A failed attempt increments the counter.
  - After 3 failures, at charge ≤ 50%, the next attempt uses the deep budget.
  - **Mutation:** removing the failure increment fails a test.
- **C:**
  - A structural test: every `System.reset(` in `src/` carries a distinct cause code. The test fails if a new reset is added without one, or if two sites share a code.
  - The code appears in the next startup status.
- **D:**
  - With no reading (`Cellular.RSSI()` invalid / −1), `valid` is false and the log prints `sig=na`, not 0/0.
  - **Mutation:** restoring the unconditional `valid = true` fails a test.
- **Everywhere:**
  - The suite passes (sh via zsh, py via python3).
  - `tests/publish_with_ack_structural_test.py` passes unchanged.
  - Local toolchain build plus linkage check (`nm`) for the new handler, per Stage 7's mandatory checks.

## Acceptance (Stage 7, narrow: each item against its own goal)

1. A, B, C, D each meet their goal, with the tests above passing and each listed mutation failing a test.
2. Linkage: the `firmware_update` handler and the loop-level transition have production call sites and are present in the linked ELF.
3. Size: each item within its budget, and the total `src/` diff at most about 65 lines.
4. Version `v31-ConnectivityFixes`, product 31.

## Expected post-change telemetry

- **OTA:** a download completes in one session. No `Low-power idle: no updates pending` line follows `Starting firmware update`. Reports held during the transfer drain after the update reboot.
- **B:** at charge ≤ 50% with repeated failures, `Connect: start budget=660s` appears every 4th attempt.
- **C:** the startup status after any firmware-issued reset carries a non-zero cause code. `140` with no code means the reset came from outside our `src/` reset sites.
- **D:** `sig=na` during cellular acquisition, and 0/0 only when Device OS actually reports 0.

## Compatibility, security, rollback

- **Compatibility:** no payload field changes. `resetReasonData` is already published.
- **Security:** no new external inputs. The `firmware_update` event comes from Device OS.
- **Rollback:** reflash v30 (product 30). No persistent format changes.

## Approval record

- [x] Stage 6 round 1: Copilot `claude-opus-5` medium. A over budget (29/20) and continued instead of stopping; deleted `build-tmp/` (restored from S3; rule added to AI_DEVELOPMENT_WORKFLOW.md §2 Copilot restrictions, the temporary-artifacts rule and §4). Report: `WO-2026-10-01-001-stage6-copilot-report.md`.
- [x] Stage 7 round 1: Codex `gpt-6-astra` high, NOT VERIFIED (A: P1 begin consumed after the state dispatch, P2 begin flag left latched, P2 stale terminal record; budget). B, C, D VERIFIED. Verdict: `WO-2026-10-01-001-stage7-verdict.md`.
- [x] Round 2 of 2: Chip, 2026-10-01. Item A only, reworked to Particle's reference pattern (URL above); budget about 30 lines; B, C, D frozen; narrow Stage 7 on A (round-1 reproductions plus the existing A checks); if NOT VERIFIED, stop and restate the goal.

- [x] Stage 5: Chip and the architect, 2026-10-01, in the opening dispatch: items A–D, budgets, Stage 7 checks, version, deferrals, routing (one Copilot round `claude-opus-5` medium; one narrow Stage 7 Codex `gpt-6-astra` high; not authorized: commits, flashing). 
- [x] Stage 5 decisions 1–3: Chip, 2026-10-01: (1) ThrashGuard progress refresh plus an update-state timeout above 300 s, approved; (2) `System.reset(code)` instead of a retained variable; (3) branch from the PR #53 head.
- [x] Stage 6 round 2: Copilot `claude-opus-5` medium, item A only. A +27 net `src/` code lines against `9e10778` (budget about 30). Suite 51/51 (sh via zsh, py via python3). 5/5 mutations caught. Build 150420 / 1090 / 2204. Deviations: `loop_stage_sleep_prep_exclusion_test.py` `EXPECTED_TRANSITION_CALLS` 15 → 16 (the new `transitionTo`); the redundant `configLoadedInUpdateMode = false` dropped; `cloudSyncStartMs` reset when leaving the sleep gate for the update state. Deleted only its own scratch directory. Claude Code checked: B, C, D `src/` lines byte-identical to round 1; archive file count unchanged (120,584). Report: `WO-2026-10-01-001-stage6-r2-copilot-report.md`.
- [x] Stage 7 round 2: Codex `gpt-6-astra` high, **VERIFIED WITH NOTES**. Round 1's three reproductions pass. Reference pattern followed apart from the three deliberate differences. A 20-minute progressing download stays with zero ThrashGuard trips; the inactivity exit fires at 300001 ms. The mid-gate return starts a fresh gate. 5/5 mutations caught. Suite 51/51. Linkage in the ELF. Build 150420 / 1090 / 2204. A +27; B/C/D 4 / 14 / 4. Notes:
  - (1) The timer uses `>`, where the reference uses `>=`.
  - (2) After the inactivity exit, the flag is still set, so the next sleep pass re-enters the update state. The reference behaves the same. Claude Code checked Device OS 6.4.1: every transfer end emits `complete` or `failed` (`system/src/firmware_update.cpp:446`), including its own 300 s no-chunk timeout (`communication/src/firmware_update.cpp:178–181` → `cancelUpdate()`) and a protocol/session reset (`communication/src/protocol.cpp:568` → `FirmwareUpdate::reset()` → `cancelUpdate()`). Progress is emitted per chunk (`system/src/firmware_update.cpp:268`). So the re-entry lasts only until Device OS's own 300 s timeout sends `failed`.
  - (3) One ThrashGuard tier-1 trip (backoff only) in that transition, under Sleep's 60 s timeout.
  Verdict: `WO-2026-10-01-001-stage7-r2-verdict.md`.
- [x] Stage 8 (Chip, 2026-10-01): accepted; commit v31 and the workflow deletion rule as separate commits (by Claude Code on Chip's instruction), push the branch and open the PR. **Don't merge** until the Dev-09 bench (OTA to a version-only v31.1-BenchOTA, product 32, never released; a reset-cause check) passes and v30 (PR #53) has merged.

## Budget versus actual (closing record)

Per `AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3. Actuals are the figures recorded in the Stage 7 verdicts (`WO-2026-10-01-001-stage7-verdict.md` for B, C, D; `WO-2026-10-01-001-stage7-r2-verdict.md` for A); "not recorded" means the verdict has no figure.

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| A | ≤ 20 | about 30 (Chip, round 2 of 2: rework to Particle's reference pattern; round 1 was 29 against 20 and had three defects) | +27 (round 2; round 1 was +29) | not recorded |
| B | ≤ 5 | — | +4 | not recorded |
| C | ≤ 25 | — | +14 | not recorded |
| D | ≤ 12 | — | +4 | not recorded |
| Total `src/` | about 65 | — | not recorded as a total for round 2 (round 1 total: 51) | not recorded |
