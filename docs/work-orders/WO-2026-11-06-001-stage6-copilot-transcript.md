# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `11640dac-41b5-429b-a2d6-fc39737e1856`  
> - **Started:** 11/8/2026, 8:33:06 AM  
> - **Duration:** 24m 21s  
> - **Exported:** 11/8/2026, 8:57:27 AM  

---

<sub>6s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-06-001-stage6-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>8s</sub>

### `bash`

**Read dispatch**

`$ cat docs/work-orders/WO-2026-11-06-001-stage6-copilot-dispatch.md; ls docs/work-orders`

<details>
<summary>6 lines</summary>

```
Output too large to read at once (29.1 KB). Saved to: /var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794106994918-copilot-tool-output-79769-6dac68f0-8f66-45ef-8037-aa426fa918d4.txt
Consider using tools like grep (for searching), head/tail (for viewing start/end), view with view_range (for specific sections), or jq (for JSON) to examine portions of the output.

Preview (first 500 chars):
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/` (the boolean's home, the registration after the existing clock-source read in `setup()`, and nothing else), tests under `tests/` (new S4 tests; existing tests and `tests/stubs/` only as the change forces), and write the report; run host tests, the local Device OS toolchain build and nm/objdump; scratch `build-tmp/wo-2026-11-06-001-stage6/` (create it; use only that) / NOT authorized: committing, pushin
<shellId: 0 completed with exit code 0>
```

</details>


---

<sub>11s</sub>

### `view`

**view the file at /var/folders/\_r/m12xn8s15wb\_15djjdgfp5gr0000gn/T/1794106994918-copilot-tool-output-79769-6dac68f0-8f66-45ef-8037-aa426fa918d4.txt.**

`/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794106994918-copilot-tool-output-79769-6dac68f0-8f66-45ef-8037-aa426fa918d4.txt`

```
File too large to read at once (29.1 KB). Consider using the grep tool to search within the file, or view with view_range to read specific sections.
```


---

<sub>13s</sub>

### `view`

**view the file at /var/folders/\_r/m12xn8s15wb\_15djjdgfp5gr0000gn/T/1794106994918-copilot-tool-output-79769-6dac68f0-8f66-45ef-8037-aa426fa918d4.txt.**

`/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794106994918-copilot-tool-output-79769-6dac68f0-8f66-45ef-8037-aa426fa918d4.txt (lines 1-120)`

<details>
<summary>119 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/` (the boolean's home, the registration after the existing clock-source read in `setup()`, and nothing else), tests under `tests/` (new S4 tests; existing tests and `tests/stubs/` only as the change forces), and write the report; run host tests, the local Device OS toolchain build and nm/objdump; scratch `build-tmp/wo-2026-11-06-001-stage6/` (create it; use only that) / NOT authorized: committing, pushing, merging, flashing, creating PRs or tags, touching `lib/`, S1/S1b/S2/S3 behaviour, any trust/revoke logic (I21), the status payload or persisted/`retained` layouts, deleting anything you did not create (including `build-tmp/` itself).

# Stage 6 dispatch: WO-2026-11-06-001 (v50 piece S4, oscillator-fault latch)
Binding spec: `docs/work-orders/WO-2026-11-06-001-v50-s4-oscillator-fault-latch.md` **including Amendment 1**, and the parent `docs/work-orders/WO-2026-11-05-002-v50-ab1805-switch.md`. Base: this branch (`integration/v50-ab1805` at e21a4a4: S1, S1b, S2, S3 merged; re-cite every line on this head). **Budget: `src/` at most +10 net** (removals count); stop and report the cut rather than trim style. Leave changes uncommitted. **Run at the dispatched reasoning level (medium); the report states the level actually run, or that it could not be observed.** Boot/time logic: be conservative.
## Do
1. **Step 0 first** (WO items 1-4): read-only, answered in the report with file:line against Device OS 6.6.0 (`~/.particle/toolchains/deviceOS/6.6.0`). Stop and report if any answer breaks the plan.
2. Implement: the process-lifetime boolean (RAM only, never persisted, never cleared) with an inline accessor; set from the existing `startupOscUsingRC` read (registration-time rule); register `ExternalTime.onEvent` for `RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE` **after** that read, with a callback that only sets the flag (one log line only if the budget allows). No trust logic, no flush, no payload change.
3. **Tests and mutations** per the WO: false after a normal bind; true after an RC-fallback bind; true after a delivered event; stays true after a second event and a later healthy read; registered once and after the clock-source read (structural order test); no persistence write. Mutations (each must fail; back up, mutate, restore from the backup; never `git checkout`/`git restore` while your work is uncommitted): registration-time check removed; event ignored; flag cleared by a later read; callback registered before the bind; handler registered before the `clockSource()` read; flag persisted; wrong event enum.
4. **Verify (workflow §10, binding):** CI's discovery inside this git worktree: `tests/*.sh` via zsh and `tests/*.py` via python3 (count `p=$((p+1))`; report `N/N (sh via zsh, py via python3)`; the S3 head ran 121/121 and S4 adds its tests: state the measured baseline you saw before editing); any new helper under `tests/` exits 0 bare; `thermal_coupling_structural_test.py` also on a clean export of tracked files (remove your scratch copies from `build-tmp/` first); local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE` as a make argument): text/data/bss (head: S3 merged, 150924 / 1114 / 2240) and `nm` showing the callback linked; `git diff --numstat e21a4a4 -- src` net <= +10 (state the number).
## Stop and report if
the budget cannot hold; Step 0 breaks the plan; S1-S3 behaviour or a layout would have to change.
## Report
`docs/work-orders/WO-2026-11-06-001-stage6-copilot-report.md`: model and reasoning level actually used, interpreter line, Step 0 answers, net `src/` lines per file, tests and mutation results, build and `nm` evidence, deviations, open questions. Claim nothing about hardware.
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
2026-10-07-step6-extensibility-evidence.md
2026-10-09-connected-idle-sleep-inventory-dispatch.md
2026-10-09-connected-idle-sleep-inventory.md
2026-10-10-v40-fleet-health-check-report.md
2026-10-10-v40-fleet-health-check.md
2026-10-12-v41-release-and-soak.md
2026-10-15-v42-fleet-health-check-report.md
2026-10-15-v42-fleet-health-check.md
2026-10-15-v43-release-and-soak.md
2026-10-16-v44-deviceos-6.6.0-release-and-soak.md
2026-10-17-v43-fleet-health-check-report.md
2026-10-17-v43-fleet-health-check.md
2026-10-19-sleep-watchdog-investigation-report.md
2026-10-19-v44-bench-soak-dev09-report.md
2026-10-19-v44-bench-soak-dev14-report.md
2026-10-20-v45-release-and-soak.md
2026-10-21-v44-fleet-canaries-report.md
2026-10-22-v44-fleet-health-check-report.md
2026-10-22-v44-fleet-health-check.md
2026-10-22-v45-bench-soak-result.md
2026-10-24-v45-baseline-day1-result.md
2026-10-25-item-20-hour-straddler-step0-codex-dispatch.md
2026-10-25-item-20-hour-straddler-step0-report.md
2026-10-25-v45-baseline-day2-result.md
2026-10-25-v46-release-and-soak.md
2026-10-26-v46-bench-soak-result.md
2026-10-26-v46-day1-result.md
2026-10-27-v46-day2-result.md
2026-10-27-v47-release-and-soak.md
2026-10-28-v46-fleet-health-check-report.md
2026-10-28-v47-day1-and-24h-result.md
2026-10-29-item-10-post-close-credit-step0-codex-dispatch.md
2026-10-29-item-10-post-close-credit-step0-report.md
2026-10-31-v47-fleet-health-check-report.md
2026-10-31-v48-release-and-soak.md
2026-11-05-v49-release-and-soak.md
WO-2026-08-05-002-battery-authority-delta-guard.md
WO-2026-08-05-003-ledger-write-ordering.md
WO-2026-08-06-001-powermanager-phase1.md
WO-2026-08-07-001-state-report-merge-reconciliation.md
WO-2026-08-10-001-watchdog-instrumentation.md
WO-2026-08-11-001-sleep-prep-threshold-exclusion.md
WO-2026-08-12-001-modem-teardown-confirmation-logging.md
WO-2026-08-24-001-unconditional-usb-source-override.md
WO-2026-08-24-002-soc-high-for-vcell-detection.md
WO-2026-08-25-001-power-management-consolidation.md
WO-2026-08-29-001-hibernate-wake-cloud-observability.md
WO-2026-08-29-002-time-sync-guarantee-and-clock-trust.md
WO-2026-08-31-001-p2-build-break-pmic-incomplete-type.md
WO-2026-08-31-002-hibernate-wake-sync-sequencing.md
WO-2026-08-31-003-rtc-skew-bench-hook.md
WO-2026-08-31-004-rtc-loses-time-during-hibernate.md
WO-2026-09-03-001-occupancy-anomaly-clock-trust.md
WO-2026-09-03-002-publish-queue-corrupted-discards.md
WO-2026-09-03-003-ledger-write-fresh-pairing-failures.md
WO-2026-09-03-004-mafc1-sleep-path-watchdog-stalls.md
WO-2026-09-14-001-reporting-policy-adapter-test-stub-bypass.md
WO-2026-09-14-002-hibernate-wake-gate-single-owner.md
WO-2026-09-15-001-excessive-repeat-logging-powerdiag-ledger.md
WO-2026-09-15-002-cellular-acquire-registration-stall-no-escalation.md
WO-2026-09-16-001-appbreadcrumb-enum-no-cross-tu-home.md
WO-2026-09-16-002-hibernate-cycle-owner.md
WO-2026-09-18-001-clock-owner.md
WO-2026-09-19-001-clock-trust-standard.md
WO-2026-09-20-001-rtc-skew-test-silent-noop.md
WO-2026-09-21-001-battery-authority.md
WO-2026-09-21-002-battery-authority-lowbatterymode-flap.md
WO-2026-09-21-003-pdiag-rapid-wake-cycle.md
WO-2026-09-22-001-occupancy-start-time-clamp-unreachable.md
WO-2026-09-22-002-reporting-policy-clock-routing.md
WO-2026-09-23-001-step5-persistent-data-facade-split.md
WO-2026-09-23-002-serial-settle-test-regression.md
WO-2026-09-24-001-daily-cleanup-boundary-fix.md
WO-2026-09-24-001-stage6-copilot-dispatch.md
WO-2026-09-24-001-stage6-round2-copilot-dispatch.md
WO-2026-09-24-001-stage7-round2-codex-dispatch.md
WO-2026-09-24-001-stage7-verdict.md
WO-2026-09-24-003-device-status-timing-visibility.md
WO-2026-09-24-004-retire-open-equals-close-convention.md
WO-2026-09-24-004-stage6-copilot-dispatch.md
WO-2026-09-24-004-stage6-copilot-report.md
WO-2026-09-24-004-stage7-codex-dispatch.md
WO-2026-09-24-004-stage7-verdict.md
WO-2026-09-25-001-bench-diagnostic-D.diff
WO-2026-09-25-001-phase1-codex-answers.md
WO-2026-09-25-001-phase1-codex-dispatch.md
WO-2026-09-25-001-phase1-provenance-compile.log
WO-2026-09-25-001-report-webhook-loss.md
WO-2026-09-25-001-stage4-codex
WO-2026-09-25-001-stage4-codex-investigation-dispatch.md
WO-2026-09-25-001-stage6-copilot-dispatch.md
WO-2026-09-25-001-stage6-copilot-report.md
WO-2026-09-25-001-stage6-decision10-copilot-dispatch.md
WO-2026-09-25-001-stage6-decision10-copilot-report.md
WO-2026-09-25-001-stage6-decision11-copilot-dispatch.md
WO-2026-09-25-001-stage6-decision11-copilot-report.md
```

</details>


---

<sub>14s</sub>

### `bash`

**Read WO**

`$ cat docs/work-orders/WO-2026-11-06-001-v50-s4-oscillator-fault-latch.md; git log --oneline | head -3; git status --short | head`

<details>
<summary>44 lines</summary>

```
# WO-2026-11-06-001: v50 piece S4 — the oscillator-fault latch ("fault seen since bind")

**Author:** Claude Code, 6 Nov 2026, DRAFT for the architect's review; not dispatched · **Status:** DISPATCHED 7 Nov 2026 (after S1, S1b, S2, S3 merged; integration head e21a4a4); pre-ruled: S1 Amendment 2 and Amendment 3 ruling 4 send the latch here, at most +10 `src/`
**Parent:** `WO-2026-11-05-002-v50-ab1805-switch.md` (including the 6 Nov rulings: the S-set stays on `integration/v50-ab1805` until all five bench items and both 8 h nights pass on its CI binaries) · **Depends on:** S1 (`WO-2026-11-05-003`, the bind and `Am18x5.status()`) · **Consumer:** I21 (`WO-2026-11-05-007`), which reads only the boolean
**Branch (when dispatched):** `wo/…-v50-s4-oscillator-fault-latch`, from `integration/v50-ab1805` after S1 has passed Stage 7 and its PR has been merged to the integration branch; S4's PR goes to the integration branch, **never to `main` alone**.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (time and trust inputs: Codex xhigh); §12; §13 (c), (j) · **Agents:** Stage 6 Copilot standard/medium (re-run at the dispatched level if a run comes back lower on anything larger than a few lines); Stage 7 Codex `gpt-5.6-sol` **xhigh**, at most two rounds · **Budget:** `src/` at most **+10 net** (removals count); stop and report rather than trim style.
**Dispatch gate:** S1 reviewed (Stage 7 passed) and merged to the integration branch. S4 does not merge to `main` before the set.

## Plain goal
I21 will let clock trust carry across a restart, and must withdraw it on evidence. One piece of evidence is "the oscillator faulted since the chip was bound". Device OS reports it as an event; the application needs a single boolean it can read. S4 adds that boolean and nothing else: **a process-lifetime flag in RAM, never persisted** (Chip's ruling: no persisted latch and no persisted clock-source byte). It is S1's budget overflow: S1 implemented the callback and compiled it, but it was about +9 lines over S1's +20, so it was removed and moved here.

## Rulings that bind this piece (not reopened)
1. **The latch is a boolean, RAM only,** named for what it means (for example `oscillatorFaultSeenSinceBind`), declared where I21 can read it without a new dependency (Step 0 chooses the home; `time/ClockTrust.h` is the pure, host-testable neighbour; the setter lives with the registration).
2. **It is set** (a) by an `ExternalTime.onEvent` callback for `RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE`, and (b) **at registration** if `Am18x5.status().clockSource()` reports the RC fallback (a persistent fault is caught at boot by the clock source, not by an event). It is **never cleared** in the process; a reboot starts clean and re-derives (b).
3. **I21 reads only the boolean.** S4 contains no trust logic, no revoke, no flush: those are I21's.
4. **Order (Chip, 6 Nov): `clockSource()` is read BEFORE the handler is registered.** Registering a handler clears the chip's oscillator-fail flag (`am18x5.cpp:646`), so a fault already flagged at boot is seen only through the clock source; read it first, set the flag from it, then register the callback. Step 0 confirms the order from source and records it; it does not reopen it.
5. **Persisted continuity evidence is not added** (Chip, 6 Nov; see I21's "Why persisted continuity evidence is not needed" and the residual window).

## Facts from S1's Stage 6 Step 0 (Device OS 6.6.0; **Step 0 here re-verifies each with file:line**)
- `RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE = 0x02` (`exrtc_hal.h:86-88`); the callback is `void(RtcEvents, void*)`; the event is mapped from the chip's XT oscillator-failure condition at `am18x5.cpp:695-715`.
- The oscillator-failure interrupt (OFIE) is enabled **only with the external-crystal oscillator** (`am18x5.cpp:595`, `:635`), and **registering a handler clears the OF flag** (`:646`). So the clock source must be read **before** registration (ruling 4), or a fault already flagged is lost.
- S1 already publishes `usingRC` from `Am18x5.status().clockSource()` at boot; S4 reuses that read.

## The change (re-cite on the then-current integration head)
1. The boolean and its accessor (`inline`, no new `.cpp` if it fits).
2. After S1's bind in `setup()`: read `clockSource()`; if RC fallback, set the flag; register the `ExternalTime.onEvent` callback whose body only sets the flag (and may log one line, counted in the budget).
3. Nothing else. No change to `Clock`, the write-back, the payload, `retained` or persisted layouts.

## Step 0 (read-only, inside Stage 6, before editing; stop and report if any answer breaks the plan)
1. Re-verify the three Device OS facts above with file:line, including the flag clear at `:646`, which is why the ruled order (read `clockSource()`, then register) is the one that cannot lose a fault; and whether the callback can run before the flag is declared (interrupt versus loop context: is the setter safe to call from the event path, and does the event path run on the application thread?).
2. **Where the flag lives** so I21 can read it at the first `isClockTrusted()` call after boot (that call must see the registration-time value): name the header, the lifecycle, and whether `setup()` order guarantees it is set before the first trust decision.
3. What a **mid-boot fault** does today to anything that already read the flag (nothing, by design: I21 owns the revoke); confirm no S1/S2/S3 code path consumes the flag.
4. **Count** the net `src/` lines; if over +10, stop and report the cut (S1 measured about +9 for the callback and registration; the boolean and the registration-time read are the remainder).

## Tests (host; the real functions)
Fakes for `Am18x5`/`ExternalTime`: the flag is false after a normal bind (crystal source); true after a bind that reports the RC fallback (registration-time rule); true after a delivered `CLOCK_SOURCE_EXTERNAL_FAILURE` event; stays true after a second event and after the source later reads healthy (never cleared); the callback is registered once and after the bind (structural, in order); no persistence write is made (structural: no `set_`/`flushNow` in S4's code). Mutations (each must fail): registration-time check removed; event ignored; flag cleared by a later read; callback registered before the bind; the handler registered before the `clockSource()` read (ruling 4, a structural order test); flag persisted; the wrong event enum.

## Bench (Chip; part of the integrated head's bench, not alone)
Host injection is the required proof. Physical induction of a crystal fault is **optional** (the spike marked it unsafe). On the integrated head, confirm on a bound device that the flag reads false on a healthy crystal and (if a device on the RC fallback is available) true at boot, via the existing boot log line S4 adds or the `usingRC` status field.

## Protected / not in scope
I21 (every trust decision), S1-S3, L; the clock-trust rule, the failsafe, item 19; the persisted and `retained` layouts; M-SoM.

## Rollback
Revert the S4 commit on the integration branch before the set merges. After the set ships, rollback is to v49.1 (the guard deletes the stored binding).

## Amendment 1 (Claude Code, 7 Nov 2026, from the merged integration head e21a4a4)
1. **State of the head.** S1 already reads the clock source once at boot: `Generalized-Core-Counter.cpp:657-662` (`startupOscUsingRC = Am18x5.status().clockSource() == particle::RtcClockSource::INTERNAL;`, the `OscState` log, and `DeviceStatusPublisher.cpp:312` publishes `usingRC`). That read is the "read `clockSource()` first" half of ruling 4; **reuse it for the registration-time rule** (set the flag from `startupOscUsingRC`) rather than adding a second read, and register the `ExternalTime.onEvent` callback **after** it. Step 0 re-cites the lines on the head and confirms no `ExternalTime.onEvent` registration exists yet.
2. **Budget unchanged: `src/` at most +10 net.** Stop and report the cut if it does not fit.
3. **Test-suite rule (workflow §10):** the local suite runs CI's discovery (`tests/*.sh` via zsh, `tests/*.py` via python3), inside a real git worktree (several tests read git history, so an export fails them); any helper under `tests/` exits 0 when run bare; confirm CI green on the PR before "ready". Baseline at the head: the suite count is measured, not inferred (state it).
4. **Where the boolean is read:** I21 is not yet implemented, so S4's consumer today is the boot log line and the status payload only if the budget allows without a payload change; do **not** add a payload field (the contract bump is I21's).
76557c9 WO-2026-11-06-001 (S4): dispatch with Amendment 1 (merged head e21a4a4); Stage 6 Copilot dispatch
e21a4a4 Merge pull request #204 from See-Insights/wo/2026-11-05-005-v50-s3-timed-power-off
6fe6181 WO-2026-11-05-005 (S3): Stage 7 round 2 verdict (PASS on dd856f8); watchdog-window answer recorded in the residual
<shellId: 1 completed with exit code 0>
```

</details>


---

<sub>17s</sub>

### `bash`

**Explore source**

`$ grep -n "startupOscUsingRC\|ExternalTime\|Am18x5\|OscState" -r src | head -30; ls tests | head -80; ls tests | wc -l; ls build-tmp 2>&1 | head; ls ~/.particle/toolchains/deviceOS/`

<details>
<summary>95 lines</summary>

```
src/Generalized-Core-Counter.cpp:252:bool startupOscUsingRC = false;
src/Generalized-Core-Counter.cpp:294:  if (!Am18x5.status().bound() && !PreBindRtc::run(ab1805, Wire)) Log.warn("PreBindRtc: chip access failed");
src/Generalized-Core-Counter.cpp:295:  ExternalRtc::ensureBound(Am18x5, particle::Am18x5Configuration().i2c(Wire).interruptPin(WKP).watchdogPin(WATCHDOG_DONE_PIN)
src/Generalized-Core-Counter.cpp:658:  startupOscUsingRC = Am18x5.status().clockSource() == particle::RtcClockSource::INTERNAL;
src/Generalized-Core-Counter.cpp:659:  const auto oscCaps = Am18x5.getConfig().capabilities();
src/Generalized-Core-Counter.cpp:662:  Log.info("OscState: usingRC=%d aos=%d fos=%d", startupOscUsingRC ? 1 : 0, startupOscAos ? 1 : 0, startupOscFos ? 1 : 0);
src/cloud/DeviceStatusPublisher.cpp:45:extern bool startupOscUsingRC;
src/cloud/DeviceStatusPublisher.cpp:312:        writerBase.name("usingRC").value(startupOscUsingRC);
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
     185
ls: build-tmp: No such file or directory
2.1.0
6.3.4
6.4.1
6.6.0
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>20s</sub>

### `bash`

**View source and tests**

`$ sed -n 245,256p src/Generalized-Core-Counter.cpp; sed -n 640,670p src/Generalized-Core-Counter.cpp; cat tests/external_rtc_structural_test.py | head -60; cat tests/external_rtc_test.cpp | head -40; ls tests/stubs; cat .gitignore | grep -n build`

<details>
<summary>149 lines</summary>

```
// Reason-20 (PIN_RESET) boot classification (see setup()). The watchdogSource
// field is published only when startupPinResetChecked is true.
bool startupPinResetChecked = false;
bool startupInferredWatchdog = false;

// Oscillator state captured once in setup() and published by DeviceStatusPublisher.cpp.
bool startupOscUsingRC = false;
bool startupOscAos = false;
bool startupOscFos = false;

// Short-term webhook monitoring starts only after a successful cloud
    digitalWrite(ledPower, HIGH);
  } else {
    digitalWrite(ledPower, LOW);
  }

  // Configure publish queue to retain ~30+ days of hourly reports
  // across all supported platforms (P2, Boron, Argon). With an
  // hourly reporting interval, 800 file-backed events provide
  // headroom over the 720 events needed for a full 30 days.
  PublishQueuePosix::instance()
      .withFileQueueSize(800)
      .setup(); // Initialize the publish queue

  // ===== TIME AND RTC =====
  // Device OS seeds Time from the chip before setup() when it is already bound; on an unbound boot S1b seeds it only on a successful plausible pre-bind chip read.
  const bool timeValidBeforeRtc = Time.isValid();

  // Oscillator snapshot: usingRC from the live clock source, AOS/FOS from the configuration. The raw OSC registers are not readable through the public API (published as null).
  startupOscUsingRC = Am18x5.status().clockSource() == particle::RtcClockSource::INTERNAL;
  const auto oscCaps = Am18x5.getConfig().capabilities();
  startupOscAos = !!(oscCaps & particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY);
  startupOscFos = !!(oscCaps & particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL);
  Log.info("OscState: usingRC=%d aos=%d fos=%d", startupOscUsingRC ? 1 : 0, startupOscAos ? 1 : 0, startupOscFos ? 1 : 0);

    // Re-run the occupancyStartTime future-date clamp once here: Time is valid here only if Device OS or a successful plausible S1b pre-bind read seeded it.
  CurrentReadings::revalidateOccupancyStartTimeIfTimeAvailable();

#if PLATFORM_ID == PLATFORM_BORON && ENABLE_RTC_SKEW_TEST
  // ===== BENCH-ONLY: DELIBERATE RTC SKEW (WO-2026-08-31-003, Amendment A) =====
  // Compiled in ONLY when ENABLE_RTC_SKEW_TEST=1 (see BuildProfile.h) on
  // Boron. Fires ONCE - not once per boot, but once ever, across resets,
#!/usr/bin/env python3
"""v50 S1: structural checks on src/ plus the ExternalRtc helper/bind host test and its mutations.

Each mutation is applied to a copy of the real source in a temp dir; the check must then FAIL.
Run from anywhere; zsh not required.
"""
import os, re, subprocess, sys, tempfile, shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
MAIN = SRC / "Generalized-Core-Counter.cpp"
HDR = SRC / "time" / "ExternalRtc.h"
TEST = ROOT / "tests" / "external_rtc_test.cpp"
STUB = ROOT / "tests" / "stubs" / "external_rtc"


def strip(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    return re.sub(r"//[^\n]*", "", code)


def structural(main_text, hdr_text, resetcause_text, publisher_text):
    errs = []
    m = strip(main_text)
    if "ab1805.withFOUT" in m or "ab1805.setup()" in m:
        errs.append("ab1805.setup() is still called")
    if "RESET_CAUSE_EXRTC_BINDING_DELETE" in m or "disable(" in m and "Am18x5" in m:
        errs.append("the v49 binding-delete guard is still present")
    sm = re.search(r"inline bool set\(time_t t\) \{(.*?)\n\}", strip(hdr_text), re.S)
    body = sm.group(1) if sm else ""
    if "hal_rtc_set_time" not in body or "get(back)" not in body or body.index("hal_rtc_set_time") > body.index("get(back)"):
        errs.append("set() does not read the chip back after hal_rtc_set_time")
    if "ExternalRtc::ensureBound(Am18x5" not in m:
        errs.append("setup() does not call ExternalRtc::ensureBound(Am18x5, ...)")
    if not re.search(r"\.i2c\(Wire\)\.interruptPin\(WKP\)\.watchdogPin\(WATCHDOG_DONE_PIN\)", m):
        errs.append("fleet config is not Wire/WKP/D5")
    if ".capabilities(particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL)" not in m:
        errs.append("fallback capability missing from the fleet config")
    if ".clockSource(particle::RtcClockSource::EXTERNAL)" not in m:
        errs.append("clock source EXTERNAL missing from the fleet config")
    if m.index("ensureBound") > m.index("ensureRetainedLoopForensicsInitialized();"):
        errs.append("bind must come before the first Time-dependent setup work")
    if "RESET_CAUSE_EXRTC_BINDING_DELETE" in strip(resetcause_text):
        errs.append("reset cause 8 is still declared")
    if "kLedgerSchemaVersion = 3" not in publisher_text:
        errs.append("kLedgerSchemaVersion is not 3")
    if not re.search(r'name\("oscStatus"\)\.nullValue\(\)', publisher_text) or not re.search(r'name\("oscCtrl"\)\.nullValue\(\)', publisher_text):
        errs.append("oscStatus/oscCtrl are not null")
    if "startupOscStatusReg" in publisher_text or "startupOscStatusReg" in m:
        errs.append("raw osc register global still present")
    for f in ("time/Clock.cpp", "time/HibernateEligibility.cpp"):
        t = strip((SRC / f).read_text())
        if re.search(r"ab1805\.(getRtcAsTime|setRtcFromSystem|isRTCSet|setRtcFromTime)", t):
            errs.append(f"{f} still reads/writes the RTC through ab1805")
    return errs


def build_and_run(hdr_text):
    with tempfile.TemporaryDirectory() as d:
// v50 S1: ExternalRtc helpers and the bind-once logic, compiled from the real src/time/ExternalRtc.h
// against tests/stubs/external_rtc/Particle.h. Not a bench result: the chip itself is untested here.
#include "time/ExternalRtc.h"
#include <cstdlib>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

struct FakeRtc {
  bool bound_ = false;
  int enableResult = 0, enables = 0;
  particle::Am18x5Configuration stored, lastEnabled;
  particle::Status status() const { return particle::Status{bound_}; }
  int enable(const particle::Am18x5Configuration &c) {
    enables++;
    lastEnabled = c;
    if (enableResult) return enableResult;
    bound_ = true;
    stored = c;
    return 0;
  }
  particle::Am18x5Configuration getConfig() const { return stored; }
};

static particle::Am18x5Configuration fleet() {
  return particle::Am18x5Configuration().i2c(0).interruptPin(8).watchdogPin(5).capabilities(0x3).clockSource(particle::RtcClockSource::EXTERNAL);
}

int main() {
  FakeHal &h = fakeHal();

  time_t t = 0;
  h.chipTime = 1700000000;
  CHECK(ExternalRtc::get(t) && t == 1700000000 && h.lastSource == HAL_RTC_SOURCE_EXTERNAL);
  h.getResult = -1;
  t = 5;
  CHECK(!ExternalRtc::get(t) && t == 5);
  h.getResult = 0;

  CHECK(ExternalRtc::set(1750000000) && h.lastSource == HAL_RTC_SOURCE_EXTERNAL && h.lastUsec == 0);
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
7:# Particle / build outputs
14:# Local build artifacts
17:# Visible scratch dir for temporary host-test build artifacts (must stay
19:# build and test artifacts" rule)
20:/build-tmp/
35:/build_output.txt
39:/failsafe_test_build.log
<shellId: 3 completed with exit code 0>
```

</details>


---

<sub>26s</sub>

### `bash`

**Locate Device OS files**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; ls $D | head; grep -rn "CLOCK_SOURCE_EXTERNAL_FAILURE" $D --include=*.h --include=*.cpp | head; find $D -name am18x5.cpp | head`

<details>
<summary>16 lines</summary>

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
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/inc/exrtc_hal.h:88:    HAL_EXRTC_EVENT_CLOCK_SOURCE_EXTERNAL_FAILURE = 0x02
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/am18x5.cpp:702:        exrtcEvents |= HAL_EXRTC_EVENT_CLOCK_SOURCE_EXTERNAL_FAILURE;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/user/tests/wiring/api/system.cpp:554:    API_COMPILE({ auto event = particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE; (void)event; });
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_exrtc.h:88:    CLOCK_SOURCE_EXTERNAL_FAILURE = HAL_EXRTC_EVENT_CLOCK_SOURCE_EXTERNAL_FAILURE,
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/am18x5.cpp
<shellId: 4 completed with exit code 0>
```

</details>


---

<sub>31s</sub>

### `bash`

**Step 0 Device OS facts**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; sed -n 585,660p $D/hal/shared/am18x5.cpp; sed -n 680,760p $D/hal/shared/am18x5.cpp; sed -n 80,92p $D/hal/inc/exrtc_hal.h; grep -n "onEvent\|RtcEventHandler\|std::function\|typedef.*Event" $D/wiring/inc/spark_wiring_exrtc.h; grep -n "ExternalTime" $D/wiring/inc/*.h | head`

<details>
<summary>167 lines</summary>

```
if (config_.rc_on_battery) {
        oscControl |= OSC_CONTROL_AOS_MASK;
    } else {
        oscControl &= ~OSC_CONTROL_AOS_MASK;
    }
    if (config_.rc_fallback) {
        oscControl |= OSC_CONTROL_FOS_MASK;
    } else {
        oscControl &= ~OSC_CONTROL_FOS_MASK;
    }
    if (oscEventHandler_ && (config_.osc_src == Am18x5Oscillator::EXTERNAL_CRYSTAL) && (subscribedOscEvents_ & Am18x5OscEvent::XT_OSC_FAILURE)) {
        oscControl |= OSC_CONTROL_OFIE_MASK;
        // Clear the oscillator failure flag
        CHECK(writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK, OSC_STATUS_OF_SHIFT));
    } else {
        oscControl &= ~OSC_CONTROL_OFIE_MASK;
    }
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
    CHECK(writeRegister(Am18x5Register::OSC_CONTROL, oscControl));

    // Digital calibration to improve accuracy.
    CHECK(xtOscillatorDigitalCalibration(configuredXtalCalibration()));

    // Enable square wave output on the CLKOUT pin
    if (config_.clk_out_en) {
        CHECK(enableClkOut(config_.clk_out_freq));
    } else {
        CHECK(disableClkOut());
    }

    // Automatically clear interrupt flags after reading the the status register.
    CHECK(writeRegister(Am18x5Register::CONTROL1, 1, false, true, CONTROL1_ARST_MASK, CONTROL1_ARST_SHIFT));
    return SYSTEM_ERROR_NONE;
}

int Am18x5::getOscillatorSource(Am18x5Oscillator* source) {
    Am18x5Lock lock;
    CHECK_TRUE(source, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t oscStatus = 0;
    CHECK(readRegister(Am18x5Register::OSC_STATUS, &oscStatus));
    *source = (oscStatus & OSC_STATUS_OMODE_MASK) ? Am18x5Oscillator::INTERNAL_RC : Am18x5Oscillator::EXTERNAL_CRYSTAL;
    return SYSTEM_ERROR_NONE;
}

int Am18x5::onOscillatorEvent(uint8_t events, Am18x5OscEventHandler handler, void* context) {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t oscControl = 0x00;
    CHECK(readRegister(Am18x5Register::OSC_CONTROL, &oscControl));
    if (handler && (config_.osc_src == Am18x5Oscillator::EXTERNAL_CRYSTAL) && (events & Am18x5OscEvent::XT_OSC_FAILURE)) {
        oscControl |= OSC_CONTROL_OFIE_MASK;
    } else {
        oscControl &= ~OSC_CONTROL_OFIE_MASK;
    }
    if (handler && (events & Am18x5OscEvent::AUTO_CAL_FAILURE)) {
        oscControl |= OSC_CONTROL_ACIE_MASK;
    } else {
        oscControl &= ~OSC_CONTROL_ACIE_MASK;
    }
    // Clear the oscillator failure flag
    CHECK(writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK, 0));
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
    CHECK(writeRegister(Am18x5Register::OSC_CONTROL, oscControl));
    subscribedOscEvents_ = events;
    oscEventHandler_ = handler;
    oscEventHandlerContext_ = context;
    return SYSTEM_ERROR_NONE;
}

int Am18x5::lock() {
    return hal_i2c_lock(config_.i2c_if, nullptr);
}

int Am18x5::unlock() {
    return hal_i2c_unlock(config_.i2c_if, nullptr);
    bytes2hexbuf(&mfgLotLo, 1, id);
    id += 2;
    uint8_t mfgWafer = ((ids_[6] >> 2) & 0x1F); // Wafer[4:0]
    bytes2hexbuf(&mfgWafer, 1, id);
    id += 2;
    uint8_t idHi = (ids_[4] & 0x7F); // ID[14:8]
    bytes2hexbuf(&idHi, 1, id);
    id += 2;
    uint8_t idLo = ids_[5]; // ID[7:0]
    bytes2hexbuf(&idLo, 1, id);
    id += 2;
    *id = '\0';
    return SYSTEM_ERROR_NONE;
}

void Am18x5::exRtcOscEventHandler(uint8_t events, void* context) {
    auto self = static_cast<Am18x5*>(context);
    if (!self) {
        return;
    }
    uint32_t exrtcEvents = HAL_EXRTC_EVENT_NONE;
    if (events & Am18x5OscEvent::XT_OSC_FAILURE) {
        exrtcEvents |= HAL_EXRTC_EVENT_CLOCK_SOURCE_EXTERNAL_FAILURE;
    }
    if (events & Am18x5OscEvent::AUTO_CAL_FAILURE) {
        exrtcEvents |= HAL_EXRTC_EVENT_CALIBRATION_FAILURE;
    }
    if (!exrtcEvents) {
        return;
    }
    for (auto h = self->eventHandlers_.front(); h; h = h->next) {
        if (h->handler) {
            h->handler(exrtcEvents, nullptr, h->context);
        }
    }
}

void* Am18x5::addEventHandler(hal_exrtc_event_handler_t handler, void* context, hal_exrtc_event_cleanup_handler_t cleanup) {
    CHECK_TRUE(handler, nullptr);
    auto h = new(std::nothrow) ExRtcEventHandler();
    CHECK_TRUE(h, nullptr);
    h->handler = handler;
    h->context = context;
    h->cleanup = cleanup;
    eventHandlers_.pushFront(h);
    if (updateEventHandlers()) {
        eventHandlers_.popFront();
        if (h->cleanup) {
            h->cleanup(h->context);
        }
        delete h;
        return nullptr;
    }
    return h;
}

int Am18x5::removeEventHandler(void* cookie) {
    CHECK_TRUE(cookie, SYSTEM_ERROR_INVALID_ARGUMENT);
    for (auto h = eventHandlers_.front(), prev = static_cast<ExRtcEventHandler*>(nullptr); h; prev = h, h = h->next) {
        if (h == cookie) {
            eventHandlers_.pop(h, prev);
            if (h->cleanup) {
                h->cleanup(h->context);
            }
            delete h;
            return updateEventHandlers();
        }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int Am18x5::clearEventHandlers() {
    while (auto h = eventHandlers_.popFront()) {
        if (h->cleanup) {
            h->cleanup(h->context);
        }
        delete h;
    }
    return updateEventHandlers();
}

    HAL_EXRTC_CAPS_EXTI = 0x80,
    HAL_EXRTC_CAPS_EXTI_LEVEL_TRIGGER = 0x100,
    HAL_EXRTC_CAPS_WATCHDOG = 0x200,
} hal_exrtc_capability_t;

typedef enum hal_exrtc_event_t {
    HAL_EXRTC_EVENT_NONE = 0,
    HAL_EXRTC_EVENT_CALIBRATION_FAILURE = 0x01,
    HAL_EXRTC_EVENT_CLOCK_SOURCE_EXTERNAL_FAILURE = 0x02
} hal_exrtc_event_t;

typedef enum hal_exrtc_clock_source_t {
    HAL_EXRTC_CLOCK_SOURCE_NONE = 0,
94:using ExRtcOnEventStdFunction = std::function<void(RtcEvents events)>;
510:    RtcEventSubscription onEvent(ExRtcOnEventCallback callback, void* context = nullptr) {
520:        auto cookie = hal_exrtc_event_handler_add(instance_, onEventCallback, handler, destroyEventHandler, nullptr);
524:    RtcEventSubscription onEvent(const ExRtcOnEventStdFunction& callback) {
533:        auto cookie = hal_exrtc_event_handler_add(instance_, onEventStdFunctionCallback, handler, destroyEventHandler, nullptr);
538:    RtcEventSubscription onEvent(void(T::*callback)(RtcEvents), T* instance) {
539:        return onEvent((callback && instance) ? std::bind(callback, instance, std::placeholders::_1) : ExRtcOnEventStdFunction(nullptr));
559:    static void onEventCallback(uint32_t events, void* extra, void* context) {
566:    static void onEventStdFunctionCallback(uint32_t events, void* extra, void* context) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_exrtc.h:653:#define ExternalTime ExRtc::instance()
<shellId: 5 completed with exit code 0>
```

</details>


---

<sub>34s</sub>

### `bash`

**Line cites**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; grep -n "OFIE_MASK\|OSC_STATUS_OF_MASK\|exRtcOscEventHandler\|oscEventHandler_(\|updateEventHandlers\|onOscillatorEvent\|XT_OSC_FAILURE" $D/hal/shared/am18x5.cpp; sed -n 500,575p $D/wiring/inc/spark_wiring_exrtc.h; sed -n 76,96p $D/wiring/inc/spark_wiring_exrtc.h`

<details>
<summary>111 lines</summary>

```
157:          oscEventHandler_(nullptr),
525:    CHECK(updateEventHandlers());
595:    if (oscEventHandler_ && (config_.osc_src == Am18x5Oscillator::EXTERNAL_CRYSTAL) && (subscribedOscEvents_ & Am18x5OscEvent::XT_OSC_FAILURE)) {
596:        oscControl |= OSC_CONTROL_OFIE_MASK;
598:        CHECK(writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK, OSC_STATUS_OF_SHIFT));
600:        oscControl &= ~OSC_CONTROL_OFIE_MASK;
630:int Am18x5::onOscillatorEvent(uint8_t events, Am18x5OscEventHandler handler, void* context) {
635:    if (handler && (config_.osc_src == Am18x5Oscillator::EXTERNAL_CRYSTAL) && (events & Am18x5OscEvent::XT_OSC_FAILURE)) {
636:        oscControl |= OSC_CONTROL_OFIE_MASK;
638:        oscControl &= ~OSC_CONTROL_OFIE_MASK;
646:    CHECK(writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK, 0));
695:void Am18x5::exRtcOscEventHandler(uint8_t events, void* context) {
701:    if (events & Am18x5OscEvent::XT_OSC_FAILURE) {
725:    if (updateEventHandlers()) {
745:            return updateEventHandlers();
758:    return updateEventHandlers();
761:int Am18x5::updateEventHandlers() {
763:            (Am18x5OscEvent::XT_OSC_FAILURE | Am18x5OscEvent::AUTO_CAL_FAILURE) : 0;
767:    return onOscillatorEvent(events, events ? exRtcOscEventHandler : nullptr, this);
1440:            if (!(oscControl & OSC_CONTROL_OFIE_MASK) && !(oscControl & OSC_CONTROL_ACIE_MASK)) {
1449:            if (instance->subscribedOscEvents_ & Am18x5OscEvent::XT_OSC_FAILURE) {
1457:                    if (oscStatus & OSC_STATUS_OF_MASK) {
1458:                        events |= Am18x5OscEvent::XT_OSC_FAILURE;
1467:            if (oscStatus & (OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK)) {
1469:                instance->writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK, 0);
1472:                instance->oscEventHandler_(events, instance->oscEventHandlerContext_);
    void off() const {
        hal_exrtc_event_handler_del(instance_, nullptr, nullptr);
    }

    void off(const RtcEventSubscription& subscription) const {
        if (subscription) {
            hal_exrtc_event_handler_del(instance_, subscription.getCookie(), nullptr);
        }
    }

    RtcEventSubscription onEvent(ExRtcOnEventCallback callback, void* context = nullptr) {
        if (!callback) {
            return RtcEventSubscription();
        }
        auto handler = new(std::nothrow) EventHandler();
        if (!handler) {
            return RtcEventSubscription();
        }
        handler->rawCallback = callback;
        handler->rawContext = context;
        auto cookie = hal_exrtc_event_handler_add(instance_, onEventCallback, handler, destroyEventHandler, nullptr);
        return RtcEventSubscription(cookie);
    }

    RtcEventSubscription onEvent(const ExRtcOnEventStdFunction& callback) {
        if (!callback) {
            return RtcEventSubscription();
        }
        auto handler = new(std::nothrow) EventHandler();
        if (!handler) {
            return RtcEventSubscription();
        }
        handler->callback = callback;
        auto cookie = hal_exrtc_event_handler_add(instance_, onEventStdFunctionCallback, handler, destroyEventHandler, nullptr);
        return RtcEventSubscription(cookie);
    }

    template<typename T>
    RtcEventSubscription onEvent(void(T::*callback)(RtcEvents), T* instance) {
        return onEvent((callback && instance) ? std::bind(callback, instance, std::placeholders::_1) : ExRtcOnEventStdFunction(nullptr));
    }

protected:
    int bind(const hal_exrtc_binding_t* binding) {
        return hal_exrtc_bind(instance_, binding, nullptr);
    }
    int setConfigCommon(const hal_exrtc_config_t* config, const hal_exrtc_vendor_config_t* vendor) {
        return hal_exrtc_set_config(instance_, config, vendor, nullptr);
    }
    int getConfigCommon(hal_exrtc_config_t* config, hal_exrtc_vendor_config_t* vendor) const {
        return hal_exrtc_get_config(instance_, config, vendor, nullptr);
    }

    struct EventHandler {
        ExRtcOnEventCallback rawCallback = nullptr;
        void* rawContext = nullptr;
        ExRtcOnEventStdFunction callback;
    };

    static void onEventCallback(uint32_t events, void* extra, void* context) {
        auto handler = static_cast<EventHandler*>(context);
        if (handler && handler->rawCallback) {
            handler->rawCallback(RtcEvents::fromUnderlying(events), handler->rawContext);
        }
    }

    static void onEventStdFunctionCallback(uint32_t events, void* extra, void* context) {
        auto handler = static_cast<EventHandler*>(context);
        if (handler && handler->callback) {
            handler->callback(RtcEvents::fromUnderlying(events));
        }
    }

    static void destroyEventHandler(void* context) {
        delete static_cast<EventHandler*>(context);
    }
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
<shellId: 6 completed with exit code 0>
```

</details>


---

<sub>37s</sub>

### `bash`

**Context of handler, stubs, tests**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; sed -n 1415,1485p $D/hal/shared/am18x5.cpp; sed -n 760,768p $D/hal/shared/am18x5.cpp; grep -n "Am18x5\b\|Am18x5 *=\|define Am18x5" $D/wiring/inc/*.h | head -5; cat tests/stubs/external_rtc/Particle.h | head -80; sed -n 60,200p tests/external_rtc_structural_test.py; sed -n 40,200p tests/external_rtc_test.cpp`

<details>
<summary>287 lines</summary>

```
Am18x5Lock lock;

            uint8_t alm = 0;
            if (instance->readRegister(Am18x5Register::STATUS, &alm, false, STATUS_ALM_MASK) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (alm) {
                int currYear = instance->getYears();
                if (currYear < 0) {
                    continue;
                }
                if (instance->alarmYear_ == currYear && instance->alarmHandler_) {
                    // One-shot alarm
                    instance->setAlarm(false);
                    instance->alarmHandler_(instance->alarmHandlerContext_);
                }
            }

            if (!(instance->subscribedOscEvents_) || !(instance->oscEventHandler_)) {
                continue;
            }
            uint8_t oscControl = 0x00;
            if (instance->readRegister(Am18x5Register::OSC_CONTROL, &oscControl) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (!(oscControl & OSC_CONTROL_OFIE_MASK) && !(oscControl & OSC_CONTROL_ACIE_MASK)) {
                // No oscillator failure interrupts enabled
                continue;
            }
            uint8_t oscStatus = 0x00;
            if (instance->readRegister(Am18x5Register::OSC_STATUS, &oscStatus) != SYSTEM_ERROR_NONE) {
                continue;
            }
            uint8_t events = 0x00;
            if (instance->subscribedOscEvents_ & Am18x5OscEvent::XT_OSC_FAILURE) {
                uint8_t stopped;
                if (instance->readRegister(Am18x5Register::CONTROL1, &stopped, false, CONTROL1_STOP_MASK, CONTROL1_STOP_SHIFT) != SYSTEM_ERROR_NONE) {
                    continue;
                }
                // When the STOP bit is set or the OSEL bit is set to 1 to select the RC Oscillator,
                // OF will always be set
                if (!stopped && !(oscControl & OSC_CONTROL_OSEL_MASK)) {
                    if (oscStatus & OSC_STATUS_OF_MASK) {
                        events |= Am18x5OscEvent::XT_OSC_FAILURE;
                    }
                }
            }
            if (instance->subscribedOscEvents_ & Am18x5OscEvent::AUTO_CAL_FAILURE) {
                if (oscStatus & OSC_STATUS_ACF_MASK) {
                    events |= Am18x5OscEvent::AUTO_CAL_FAILURE;
                }
            }
            if (oscStatus & (OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK)) {
                // Clear the oscillator failure flag
                instance->writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK, 0);
            }
            if (events) {
                instance->oscEventHandler_(events, instance->oscEventHandlerContext_);
            }
        }
    }
    os_thread_exit(instance->exRtcWorkerThread_);
}

#endif // HAL_PLATFORM_EXTERNAL_RTC

int Am18x5::updateEventHandlers() {
    const uint8_t events = eventHandlers_.front() ?
            (Am18x5OscEvent::XT_OSC_FAILURE | Am18x5OscEvent::AUTO_CAL_FAILURE) : 0;
    if (!initialized_) {
        return SYSTEM_ERROR_NONE;
    }
    return onOscillatorEvent(events, events ? exRtcOscEventHandler : nullptr, this);
}
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_exrtc.h:671:#define Am18x5 Am18x5Rtc::instance()
#pragma once
// Host fake of the Device OS surface src/time/ExternalRtc.h uses.
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <sys/time.h>

enum hal_rtc_source_t { HAL_RTC_SOURCE_INTERNAL = 0, HAL_RTC_SOURCE_EXTERNAL = 1 };
struct hal_rtc_option_t {
  uint16_t size;
  int source;
};

struct FakeHal {
  int getResult = 0, setResult = 0;
  time_t chipTime = 0;
  int lastSource = -1;
  int lastUsec = -1;
  int sets = 0;
  bool valid = false;
  bool noStick = false;      // the write returns success but the chip keeps its time
  time_t writeSkew = 0;      // the chip stores written time + this
  int readbackResult = 0;    // get error once a set has happened (the read-back)
  bool afterSet = false;
};
inline FakeHal &fakeHal() { static FakeHal h; return h; }

inline int hal_rtc_get_time(struct timeval *tv, const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  if (fakeHal().getResult) return fakeHal().getResult;
  if (fakeHal().afterSet && fakeHal().readbackResult) return fakeHal().readbackResult;
  tv->tv_sec = fakeHal().chipTime;
  tv->tv_usec = 0;
  return 0;
}
inline int hal_rtc_set_time(const struct timeval *tv, const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  fakeHal().lastUsec = (int)tv->tv_usec;
  fakeHal().sets++;
  if (fakeHal().setResult) return fakeHal().setResult;
  fakeHal().afterSet = true;
  if (!fakeHal().noStick) fakeHal().chipTime = tv->tv_sec + fakeHal().writeSkew;
  return 0;
}
inline bool hal_rtc_time_is_valid(const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  return fakeHal().valid;
}

struct FakeTime {
  bool valid = true;
  time_t now_ = 1800000000;
  bool isValid() { return valid; }
  time_t now() { return now_; }
};
inline FakeTime &fakeTime() { static FakeTime t; return t; }
#define Time fakeTime()

struct FakeLog {
  int infos = 0, warns = 0;
  template <typename... A> void info(const char *, A...) { infos++; }
  template <typename... A> void warn(const char *, A...) { warns++; }
};
inline FakeLog &fakeLog() { static FakeLog l; return l; }
#define Log fakeLog()

namespace particle {
enum class RtcClockSource : uint8_t { NONE = 0, INTERNAL = 1, EXTERNAL = 2 };
struct HalDevice { struct { int interface; } i2c; };
struct Caps { uint32_t v = 0; uint32_t value() const { return v; } };
struct Am18x5Configuration {
  bool valid_ = false;
  uint8_t addr = 0x69;
  int intPin = -1, wdtPin = -1, wire = 0, tsrc = 0;
  uint32_t caps = 0;
  RtcClockSource src = RtcClockSource::INTERNAL;
  HalDevice dev{};
  Am18x5Configuration &i2c(int w) { wire = w; dev.i2c.interface = w; valid_ = true; return *this; }
  Am18x5Configuration &interruptPin(int p) { intPin = p; return *this; }
  Am18x5Configuration &watchdogPin(int p) { wdtPin = p; return *this; }
    with tempfile.TemporaryDirectory() as d:
        (Path(d) / "time").mkdir()
        (Path(d) / "time" / "ExternalRtc.h").write_text(hdr_text)
        exe = Path(d) / "t"
        r = subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-I", str(STUB), "-I", d, str(TEST), "-o", str(exe)],
                           capture_output=True, text=True)
        if r.returncode:
            return False, r.stderr
        r = subprocess.run([str(exe)], capture_output=True, text=True)
        return r.returncode == 0, r.stdout


def main():
    hdr = HDR.read_text()
    errs = structural(MAIN.read_text(), hdr, (SRC / "ResetCause.h").read_text(), (SRC / "cloud" / "DeviceStatusPublisher.cpp").read_text())
    if errs:
        for e in errs:
            print("FAIL:", e)
        sys.exit(1)
    print("OK: structural checks")
    ok, out = build_and_run(hdr)
    if not ok:
        print("FAIL: host test\n" + out)
        sys.exit(1)
    print("OK: host helper/bind test")

    def rep(a, b):
        assert a in hdr, a
        return hdr.replace(a, b, 1)

    hmut = {
        "enable-unconditionally": rep("if (!rtc.status().bound()) {", "if (true) {"),
        "skip-bound-check": rep("if (!sameFleetConfig(stored, want)) {", "if (false) {"),
        "reset-on-enable-failure": rep("r = rtc.enable(fleet);\n    Log.info", "r = rtc.enable(fleet); if (r) std::exit(1);\n    Log.info"),
        "write-via-internal-source": hdr.replace("HAL_RTC_SOURCE_EXTERNAL", "HAL_RTC_SOURCE_INTERNAL"),
        "no-readback": rep("return get(back) && (back > t ? back - t : t - back) <= kWriteVerifyToleranceSec;", "return true;"),
        "threshold-removed": rep("(back > t ? back - t : t - back) <= kWriteVerifyToleranceSec", "true"),
        "readback-error-ignored": rep("time_t back = 0;\n  return get(back) &&", "time_t back = t;\n  get(back); return"),
        "wrong-sign": rep("(back > t ? back - t : t - back) <= kWriteVerifyToleranceSec", "back - t <= kWriteVerifyToleranceSec"),
        "ignore-write-error": rep("if (hal_rtc_set_time(&tv, &opt) != 0) return false;", "hal_rtc_set_time(&tv, &opt);"),
        "ignore-fallback-capability": rep("a.capabilities().value() == b.capabilities().value()", "true"),
        "ignore-clock-source": rep("a.clockSource() == b.clockSource()", "true"),
        "wrong-clock-source-value": rep("a.clockSource() == b.clockSource()", "a.clockSource() == particle::RtcClockSource::INTERNAL"),
        "ignore-pin": rep("a.watchdogPin() == b.watchdogPin()", "true"),
        "nonzero-usec": rep("tv.tv_sec = t;", "tv.tv_sec = t; tv.tv_usec = 5;"),
    }
    bad = 0
    for name, text in hmut.items():
        ok, _ = build_and_run(text)
        print(("OK" if not ok else "FAIL"), f"mutation {name} is caught")
        bad += ok
    main_text, rc, pub = MAIN.read_text(), (SRC / "ResetCause.h").read_text(), (SRC / "cloud" / "DeviceStatusPublisher.cpp").read_text()
    smut = {
        "readback-before-set": (main_text, rc, pub, hdr.replace("hal_rtc_set_time(&tv, &opt) != 0) return false;\n  time_t back = 0;\n  return get(back)", "time_t back = 0; get(back);\n  if (hal_rtc_set_time(&tv, &opt) != 0) return false;\n  return true && get(back)", 1)),
        "setup-left-in": (main_text.replace("ExternalRtc::ensureBound(Am18x5", "ab1805.withFOUT(WKP).setup(); ExternalRtc::ensureBound(Am18x5", 1), rc, pub),
        "guard-kept": (main_text + "\nRESET_CAUSE_EXRTC_BINDING_DELETE", rc, pub),
        "wrong-pin": (main_text.replace("interruptPin(WKP)", "interruptPin(D4)", 1), rc, pub),
        "no-fallback-cap": (main_text.replace(".capabilities(particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL)", ".capabilities(particle::RtcCap::NONE)", 1), rc, pub),
        "no-clock-source": (main_text.replace(".clockSource(particle::RtcClockSource::EXTERNAL)", "", 1), rc, pub),
        "cause8-kept": (main_text, rc + "\nRESET_CAUSE_EXRTC_BINDING_DELETE = 8,", pub),
        "schema-2": (main_text, rc, pub.replace("kLedgerSchemaVersion = 3", "kLedgerSchemaVersion = 2")),
    }
    for name, tup in smut.items():
        a, b, c = tup[:3]
        caught = bool(structural(a, tup[3] if len(tup) > 3 else hdr, b, c))
        print(("OK" if caught else "FAIL"), f"mutation {name} is caught")
        bad += not caught
    if bad:
        sys.exit(1)
    print("external_rtc_structural_test: all checks passed")


main()
  CHECK(ExternalRtc::set(1750000000) && h.lastSource == HAL_RTC_SOURCE_EXTERNAL && h.lastUsec == 0);
  time_t back = 0;
  CHECK(ExternalRtc::get(back) && back == 1750000000);
  h.setResult = -3;
  CHECK(!ExternalRtc::set(1760000000) && h.chipTime == 1750000000);
  h.setResult = 0;

  // Read-back verification: the write only counts if the chip holds the time within the tolerance.
  CHECK(ExternalRtc::kWriteVerifyToleranceSec == 2);
  h.writeSkew = 2;
  CHECK(ExternalRtc::set(1750000000));
  h.writeSkew = -2;
  CHECK(ExternalRtc::set(1750000000));
  h.writeSkew = 3;
  CHECK(!ExternalRtc::set(1750000000) && h.sets > 0);
  h.writeSkew = -3;
  CHECK(!ExternalRtc::set(1750000000));
  h.writeSkew = 0;
  h.noStick = true;
  h.chipTime = 1700000000;
  CHECK(!ExternalRtc::set(1750000000));
  h.noStick = false;
  h.readbackResult = -5;
  CHECK(!ExternalRtc::set(1750000000));
  h.readbackResult = 0;
  h.setResult = -3;
  h.writeSkew = 0;
  CHECK(!ExternalRtc::set(1750000000));
  h.setResult = 0;
  CHECK(ExternalRtc::set(1750000000));

  h.sets = 0;
  fakeTime().valid = false;
  CHECK(!ExternalRtc::setFromSystem() && h.sets == 0);
  fakeTime().valid = true;
  CHECK(ExternalRtc::setFromSystem() && h.chipTime == fakeTime().now_);
  h.noStick = true;
  h.chipTime = 1;
  CHECK(!ExternalRtc::setFromSystem());
  h.noStick = false;

  h.valid = true;
  CHECK(ExternalRtc::isSet() && h.lastSource == HAL_RTC_SOURCE_EXTERNAL);
  h.valid = false;
  CHECK(!ExternalRtc::isSet());

  FakeRtc rtc;
  CHECK(ExternalRtc::ensureBound(rtc, fleet()) == 0 && rtc.enables == 1);
  CHECK(rtc.lastEnabled.wire == 0 && rtc.lastEnabled.intPin == 8 && rtc.lastEnabled.wdtPin == 5 && rtc.lastEnabled.caps == 0x3);
  CHECK(ExternalRtc::ensureBound(rtc, fleet()) == 0 && rtc.enables == 1);  // bound and matching: no re-bind

  FakeLog &log = fakeLog();
  int warns = log.warns;
  rtc.stored.intPin = 9;  // stored config differs
  CHECK(ExternalRtc::ensureBound(rtc, fleet()) == 0 && rtc.enables == 2 && log.warns == warns + 1);
  CHECK(rtc.stored.intPin == 8);

  rtc.stored.wdtPin = 4;  // watchdog pin differs
  CHECK(ExternalRtc::ensureBound(rtc, fleet()) == 0 && rtc.enables == 3);

  FakeRtc bad;
  bad.enableResult = -7;
  CHECK(ExternalRtc::ensureBound(bad, fleet()) == -7 && bad.enables == 1);  // failure is returned, never fatal

  FakeRtc missingCap;
  missingCap.bound_ = true;
  missingCap.stored = fleet().capabilities(0);
  CHECK(ExternalRtc::ensureBound(missingCap, fleet()) == 0 && missingCap.enables == 1);

  FakeRtc internalRc;  // stored config identical except it runs from the internal RC
  internalRc.bound_ = true;
  internalRc.stored = fleet().clockSource(particle::RtcClockSource::INTERNAL);
  warns = log.warns;
  CHECK(ExternalRtc::ensureBound(internalRc, fleet()) == 0 && internalRc.enables == 1 && log.warns == warns + 1);
  CHECK(internalRc.stored.clockSource() == particle::RtcClockSource::EXTERNAL);

  particle::Am18x5Configuration c1 = fleet(), c2 = fleet();
  CHECK(ExternalRtc::sameFleetConfig(c1, c2));
  c2.clockSource(particle::RtcClockSource::INTERNAL);
  CHECK(!ExternalRtc::sameFleetConfig(c1, c2) && !ExternalRtc::sameFleetConfig(c2, c1));

  if (failures) return 1;
  std::printf("external_rtc_test: all checks passed\n");
  return 0;
}
<shellId: 7 completed with exit code 0>
```

</details>


---

<sub>48s</sub>

### `bash`

**Check subscription, EnumFlags, header**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; grep -n "class RtcEventSubscription" -A25 $D/wiring/inc/spark_wiring_exrtc.h; grep -n "operator&\|operator bool\|explicit" $D/wiring/inc/spark_wiring_enumflags.h | head; cat src/time/ExternalRtc.h | head -40; sed -n 80,140p tests/stubs/external_rtc/Particle.h; grep -n "ExternalRtc.h" src/*.cpp`

<details>
<summary>82 lines</summary>

```
98:class RtcEventSubscription {
99-public:
100-    explicit RtcEventSubscription(void* cookie = nullptr)
101-            : cookie_(cookie) {
102-    }
103-
104-    explicit operator bool() const {
105-        return cookie_ != nullptr;
106-    }
107-
108-    void* getCookie() const {
109-        return cookie_;
110-    }
111-
112-private:
113-    void* cookie_;
114-};
115-
116-class RtcConfigurationTag {
117-protected:
118-    ~RtcConfigurationTag() = default;
119-};
120-
121-template <typename Concrete>
122-class RtcConfigurationBase: public RtcConfigurationTag {
123-public:
grep: /Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_enumflags.h: No such file or directory
#pragma once
// v50 S1: Device OS owns the carrier AB1805. Every app read/write of the chip goes through hal_rtc_* (external source).
#include "Particle.h"

namespace ExternalRtc {
constexpr time_t kWriteVerifyToleranceSec = 2;  // read-back must land within this of the written time
inline bool get(time_t &t) {
  struct timeval tv = {};
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  if (hal_rtc_get_time(&tv, &opt) != 0) return false;
  t = tv.tv_sec;
  return true;
}
inline bool set(time_t t) {
  struct timeval tv = {};
  tv.tv_sec = t;
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  if (hal_rtc_set_time(&tv, &opt) != 0) return false;
  time_t back = 0;
  return get(back) && (back > t ? back - t : t - back) <= kWriteVerifyToleranceSec;  // a silent no-op write fails
}
inline bool setFromSystem() { return Time.isValid() && set(Time.now()); }
inline bool isSet() {
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  return hal_rtc_time_is_valid(&opt);
}
// The fields the fleet configuration fixes (including the clock source: Device OS reports the actual one).
template <typename C> bool sameFleetConfig(C &a, C &b) {
  return a.valid() && a.address() == b.address() && a.interruptPin() == b.interruptPin() &&
         a.watchdogPin() == b.watchdogPin() && a.capabilities().value() == b.capabilities().value() &&
         a.defaultTimeSource() == b.defaultTimeSource() && a.clockSource() == b.clockSource() && a.halDevice()->i2c.interface == b.halDevice()->i2c.interface;
}
// Bind once with the fleet configuration; a failure is logged and the boot continues on the internal clock.
template <typename R, typename C> int ensureBound(R &rtc, const C &fleet) {
  Am18x5Configuration &watchdogPin(int p) { wdtPin = p; return *this; }
  Am18x5Configuration &capabilities(uint32_t c) { caps = c; return *this; }
  Am18x5Configuration &clockSource(RtcClockSource c) { src = c; return *this; }
  RtcClockSource clockSource() const { return src; }
  bool valid() const { return valid_; }
  uint8_t address() const { return addr; }
  int interruptPin() const { return intPin; }
  int watchdogPin() const { return wdtPin; }
  Caps capabilities() const { Caps c; c.v = caps; return c; }
  int defaultTimeSource() const { return tsrc; }
  const HalDevice *halDevice() const { return &dev; }
};
struct Status { bool b; bool bound() const { return b; } };
}  // namespace particle
src/Generalized-Core-Counter.cpp:48:#include "time/ExternalRtc.h"
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>50s</sub>

### `bash`

**EnumFlags ops, rest of header**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; grep -rn "operator&\|operator bool\|operator|" $D/services/inc/enumflags.h | head; sed -n 40,70p src/time/ExternalRtc.h`

<details>
<summary>27 lines</summary>

```
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/services/inc/enumflags.h:57:    EnumFlags<T> operator&(const EnumFlags<T>& flags) const;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/services/inc/enumflags.h:58:    EnumFlags<T>& operator&=(const EnumFlags<T>& flags);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/services/inc/enumflags.h:60:    EnumFlags<T> operator|(const EnumFlags<T>& flags) const;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/services/inc/enumflags.h:61:    EnumFlags<T>& operator|=(const EnumFlags<T>& flags);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/services/inc/enumflags.h:80:    operator bool() const;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/services/inc/enumflags.h:92:inline EnumFlags<T> operator&(const T& lhs, const T& rhs) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/services/inc/enumflags.h:97:inline EnumFlags<T> operator&(const T& lhs, const EnumFlags<T>& rhs) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/services/inc/enumflags.h:102:inline EnumFlags<T> operator|(const T& lhs, const T& rhs) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/services/inc/enumflags.h:107:inline EnumFlags<T> operator|(const T& lhs, const EnumFlags<T>& rhs) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/services/inc/enumflags.h:173:inline particle::EnumFlags<T> particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator&(const EnumFlags<T>& flags) const {
template <typename R, typename C> int ensureBound(R &rtc, const C &fleet) {
  int r = 0;
  if (!rtc.status().bound()) {
    r = rtc.enable(fleet);
    Log.info("AB1805: enable r=%d", r);
  } else {
    C stored = rtc.getConfig();
    C want = fleet;
    if (!sameFleetConfig(stored, want)) {
      r = rtc.enable(fleet);
      Log.warn("AB1805: stored config differs, re-bound r=%d", r);
    }
  }
  return r;
}
}  // namespace ExternalRtc
<shellId: 9 completed with exit code 0>
```

</details>


---

<sub>54s</sub>

### `bash`

**EnumFlags details and counts**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; sed -n 50,100p $D/services/inc/enumflags.h; git diff --stat; (cd tests; for f in *.sh; do :; done; ls *.sh | wc -l; ls *.py | wc -l)`

<details>
<summary>39 lines</summary>

```
EnumFlags<T>& clear(const EnumFlags<T>& flags);

    ValueType value() const;

    bool isSet() const;
    bool isSet(const EnumFlags<T>& flags) const;

    EnumFlags<T> operator&(const EnumFlags<T>& flags) const;
    EnumFlags<T>& operator&=(const EnumFlags<T>& flags);

    EnumFlags<T> operator|(const EnumFlags<T>& flags) const;
    EnumFlags<T>& operator|=(const EnumFlags<T>& flags);

    EnumFlags<T> operator^(const EnumFlags<T>& flags) const;
    EnumFlags<T>& operator^=(const EnumFlags<T>& flags);

    EnumFlags<T> operator~() const;

    EnumFlags<T> operator<<(size_t n) const;
    EnumFlags<T>& operator<<=(size_t n);
    EnumFlags<T> operator>>(size_t n) const;
    EnumFlags<T>& operator>>=(size_t n);

    bool operator==(const EnumFlags<T>& flags) const;
    bool operator!=(const EnumFlags<T>& flags) const;
    bool operator>(const EnumFlags<T>& flags) const;
    bool operator>=(const EnumFlags<T>& flags) const;
    bool operator<(const EnumFlags<T>& flags) const;
    bool operator<=(const EnumFlags<T>& flags) const;

    operator bool() const;
    bool operator!() const;

    static EnumFlags<T> fromUnderlying(ValueType value);

private:
    ValueType value_;

    explicit EnumFlags(ValueType value);
};

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value && EnableBitwise<T>::enable>>
inline EnumFlags<T> operator&(const T& lhs, const T& rhs) {
    return EnumFlags<T>(lhs) & EnumFlags<T>(rhs);
}

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value && EnableBitwise<T>::enable>>
inline EnumFlags<T> operator&(const T& lhs, const EnumFlags<T>& rhs) {
    return EnumFlags<T>(lhs) & rhs;
}

      67
      54
<shellId: 10 completed with exit code 0>
```

</details>


---

<sub>10m 23s</sub>

### `bash`

**Baseline test suite**

```
$ mkdir -p build-tmp/wo-2026-11-06-001-stage6 && cat > build-tmp/wo-2026-11-06-001-stage6/run.sh <<'EOF'
p=0; f=0
for t in tests/*.sh; do zsh $t >/dev/null 2>&1 && p=$((p+1)) || { f=$((f+1)); echo FAIL $t; }; done
for t in tests/*.py; do python3 $t >/dev/null 2>&1 && p=$((p+1)) || { f=$((f+1)); echo FAIL $t; }; done
echo "pass=$p fail=$f"
EOF
python3 --version; zsh --version; zsh build-tmp/wo-2026-11-06-001-stage6/run.sh 2>&1 | tail
```

```
Python 3.14.6
zsh 5.9 (arm64-apple-darwin26.0)
pass=121 fail=0
<shellId: 11 completed with exit code 0>
```


---

<sub>10m 29s</sub>

### `bash`

**noop**

```
$ python3 - <<'EOF'
EOF
echo
```

```

<shellId: 12 completed with exit code 0>
```


---

<sub>10m 29s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/src/time/ExternalRtc.h.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/src/time/ExternalRtc.h`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/src/time/ExternalRtc.h updated with changes.
```


---

<sub>10m 29s</sub>

### `edit` — Failed

**edit the file at /private/tmp/claude-501/-Users-chipmc/Documents-Maker-Particle-Projects-Generalized-Core-Counter/x.**

`/private/tmp/claude-501/-Users-chipmc/Documents-Maker-Particle-Projects-Generalized-Core-Counter/x`

```
Unhandled error: Error: ENOENT: no such file or directory, open '/private/tmp/claude-501/-Users-chipmc/Documents-Maker-Particle-Projects-Generalized-Core-Counter/x'
```


---

<sub>10m 34s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/src/Generalized-Core-Counter.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/src/Generalized-Core-Counter.cpp`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/src/Generalized-Core-Counter.cpp updated with changes.
```


---

<sub>10m 34s</sub>

### `bash`

**Count**

`$ git diff --numstat e21a4a4 -- src; grep -n "Log.info(\"OscState" -B1 src/Generalized-Core-Counter.cpp | head -3`

```
2	0	src/Generalized-Core-Counter.cpp
5	0	src/time/ExternalRtc.h
661-  startupOscFos = !!(oscCaps & particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL);
662:  Log.info("OscState: usingRC=%d aos=%d fos=%d", startupOscUsingRC ? 1 : 0, startupOscAos ? 1 : 0, startupOscFos ? 1 : 0);
<shellId: 13 completed with exit code 0>
```


---

<sub>10m 38s</sub>

### `bash`

**Extend stub (stub file edit)**

```
$ python3 - <<'EOF'
p='tests/stubs/external_rtc/Particle.h'
s=open(p).read()
s=s.replace("struct Status { bool b;","enum class RtcEvent : uint32_t { NONE = 0, CALIBRATION_FAILURE = 1, CLOCK_SOURCE_EXTERNAL_FAILURE = 2 };\nstruct RtcEvents {\n  uint32_t v;\n  RtcEvents(RtcEvent e) : v((uint32_t)e) {}\n  bool isSet(RtcEvents f) const { return (v & f.v) == f.v; }\n};\nstruct Status { bool b;",1)
open(p,'w').write(s)
EOF
grep -n "RtcEvent" tests/stubs/external_rtc/Particle.h | head -3
```

```
92:enum class RtcEvent : uint32_t { NONE = 0, CALIBRATION_FAILURE = 1, CLOCK_SOURCE_EXTERNAL_FAILURE = 2 };
93:struct RtcEvents {
95:  RtcEvents(RtcEvent e) : v((uint32_t)e) {}
<shellId: 14 completed with exit code 0>
```


---

<sub>10m 45s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/tests/external\_rtc\_test.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/tests/external_rtc_test.cpp`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/tests/external_rtc_test.cpp updated with changes.
```


---

<sub>10m 54s</sub>

### `bash`

**Remove tautological check**

```
$ python3 - <<'EOF'
import re
p='tests/external_rtc_test.cpp'
s=open(p).read()
s=s.replace("  CHECK(fakeHal().sets == h.sets);  // no chip write from the latch\n","")
open(p,'w').write(s)
EOF
grep -n "no chip write" tests/external_rtc_test.cpp
```

```

<shellId: 15 completed with exit code 1>
```


---

<sub>11m 1s</sub>

### Copilot

Now structural checks and mutations.

---

<sub>11m 1s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/tests/external\_rtc\_structural\_test.py.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/tests/external_rtc_structural_test.py`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/tests/external_rtc_structural_test.py updated with changes.
```


---

<sub>11m 10s</sub>

### `bash`

**Add mutations and run**

```
$ python3 - <<'EOF'
p='tests/external_rtc_structural_test.py'
s=open(p).read()
s=s.replace('''        "nonzero-usec": rep("tv.tv_sec = t;", "tv.tv_sec = t; tv.tv_usec = 5;"),
''','''        "nonzero-usec": rep("tv.tv_sec = t;", "tv.tv_sec = t; tv.tv_usec = 5;"),
        "latch-event-ignored": rep("if (e.isSet(", "if (false && e.isSet("),
        "latch-wrong-event-enum": rep("e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE)", "e.isSet(particle::RtcEvent::CALIBRATION_FAILURE)"),
        "latch-cleared-by-later-event": rep("oscillatorFaultSeenSinceBind() = true;\\n}", "oscillatorFaultSeenSinceBind() = true; else oscillatorFaultSeenSinceBind() = false;\\n}"),
''',1)
setline="  if (startupOscUsingRC) ExternalRtc::oscillatorFaultSeenSinceBind() = true;"
s=s.replace('''        "schema-2":''','''        "latch-registration-time-check-removed": (main_text.replace("if (startupOscUsingRC) ExternalRtc::oscillatorFaultSeenSinceBind() = true;", "", 1), rc, pub),
        "latch-cleared-by-later-read": (main_text.replace("if (startupOscUsingRC) ExternalRtc::oscillatorFaultSeenSinceBind() = true;", "ExternalRtc::oscillatorFaultSeenSinceBind() = startupOscUsingRC;", 1), rc, pub),
        "latch-callback-before-bind": (main_text.replace("ExternalTime.onEvent(ExternalRtc::onOscillatorEvent);", "", 1).replace("ExternalRtc::ensureBound(Am18x5", "ExternalTime.onEvent(ExternalRtc::onOscillatorEvent); ExternalRtc::ensureBound(Am18x5", 1), rc, pub),
        "latch-callback-before-clocksource-read": (main_text.replace("ExternalTime.onEvent(ExternalRtc::onOscillatorEvent);", "", 1).replace("startupOscUsingRC = Am18x5.status()", "ExternalTime.onEvent(ExternalRtc::onOscillatorEvent); startupOscUsingRC = Am18x5.status()", 1), rc, pub),
        "latch-registered-twice": (main_text.replace("ExternalTime.onEvent(ExternalRtc::onOscillatorEvent);", "ExternalTime.onEvent(ExternalRtc::onOscillatorEvent); ExternalTime.onEvent(ExternalRtc::onOscillatorEvent);", 1), rc, pub),
        "latch-persisted": (main_text + "\\ncurrent.set_oscFault(ExternalRtc::oscillatorFaultSeenSinceBind());", rc, pub),
        "schema-2":''',1)
open(p,'w').write(s)
EOF
python3 tests/external_rtc_structural_test.py | grep -v "^OK: *mutation [a-z-]* is caught" ; python3 tests/external_rtc_structural_test.py | grep -c latch
```

```
FAIL: header: the latch is cleared or persisted: inline bool &oscillatorFaultSeenSinceBind() { static bool seen = false; return seen; }
1
<shellId: 16 completed with exit code 0>
```


---

<sub>11m 30s</sub>

### `bash`

**Rerun structural test**

```
$ python3 - <<'EOF'
p='tests/external_rtc_structural_test.py'
s=open(p).read()
s=s.replace('r"= false|set_|','r"oscillatorFaultSeenSinceBind\\(\\) = false|set_|')
open(p,'w').write(s)
EOF
python3 tests/external_rtc_structural_test.py | grep -i "latch\|FAIL\|passed"
```

<details>
<summary>12 lines</summary>

```
OK mutation reset-on-enable-failure is caught
OK mutation latch-event-ignored is caught
OK mutation latch-wrong-event-enum is caught
OK mutation latch-cleared-by-later-event is caught
OK mutation latch-registration-time-check-removed is caught
OK mutation latch-cleared-by-later-read is caught
OK mutation latch-callback-before-bind is caught
OK mutation latch-callback-before-clocksource-read is caught
OK mutation latch-registered-twice is caught
OK mutation latch-persisted is caught
external_rtc_structural_test: all checks passed
<shellId: 17 completed with exit code 0>
```

</details>


---

<sub>19m 20s</sub>

### `bash`

**Run full suite; find build recipe**

`$ zsh build-tmp/wo-2026-11-06-001-stage6/run.sh 2>&1 | tail -5; ls ~/.particle/toolchains; ls ~/.particle/toolchains/deviceOS/6.6.0/build | head; grep -rn "make\b.*PLATFORM\|BUILD_PATH_BASE" docs/*.md README.md 2>/dev/null | head -5`

<details>
<summary>17 lines</summary>

```
pass=121 fail=0
buildscripts
buildtools
deviceOS
gcc-arm
openocd
arm
arm-tlm.mk
arm-tools.mk
bin
build.md
checks.mk
common-tools.mk
create_module.py
gcc-tools.mk
lang-std.mk
<shellId: 18 completed with exit code 0>
```

</details>


---

<sub>19m 24s</sub>

### `bash`

**Find prior build commands**

`$ grep -rhn "BUILD_PATH_BASE" docs/work-orders/*.md | head -5; ls ~/.particle/toolchains/buildscripts ~/.particle/toolchains/buildtools`

<details>
<summary>11 lines</summary>

```
15:- No other source changes. Fresh `BUILD_PATH_BASE` build; `strings` shows the version; tests `N/N (sh via zsh, py via python3)`.
17:- Fresh `BUILD_PATH_BASE` build; `strings` shows the version; record the `.bin` size (versus v42's 152558 bytes) and sha256; tests `N/N (sh via zsh, py via python3)`.
18:- Fresh `BUILD_PATH_BASE` build on the 6.6.0 toolchain; `strings` shows the version; record the `.bin` size (Step 0 predicted 152726 B) and sha256; suite `N/N`.
20:- Fresh `BUILD_PATH_BASE` build on 6.6.0; `strings` shows the version; record the `.bin` size and sha256 against v44's; suite `N/N`, serial. Open the PR; **don't merge until the soak passes.**
16:- Fresh `BUILD_PATH_BASE` build on 6.6.0 (Boron); `strings` shows the version; record the `.bin` size and sha256 against v45's (153,054 B; a small text change is expected from the S-6 move, about −8 B, and the S-5 move); suite `N/N`, serial. Open the PR; **don't merge until the soak passes.**
/Users/chipmc/.particle/toolchains/buildscripts:
1.10.2
1.17.2

/Users/chipmc/.particle/toolchains/buildtools:
1.1.1
<shellId: 19 completed with exit code 0>
```

</details>


---

<sub>19m 27s</sub>

### `bash`

**Find build invocation**

`$ grep -rhn "make .*APPDIR\|make -f\|PLATFORM=boron\|TARGET_FILE" docs/work-orders/WO-2026-11-05-00*.md docs/work-orders/*s1*report* 2>/dev/null | head -8; ls docs/work-orders | grep -i "11-0"; ls project.properties`

<details>
<summary>179 lines</summary>

```
990:make -f '/Users/chipmc/.particle/toolchains/buildscripts/1.17.2/Makefile' compile-user -s
1058:DEVICE_OS_PATH="$T/deviceOS/6.6.0" APPDIR="$PWD" PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH="$T/gcc-arm/10.2.1/bin/" PATH="$PATH:$T/gcc-arm/10.2.1/bin" BUILD_PATH_BASE="$PWD/$B/bpb" make -f $T/buildscripts/1.17.2/Makefile compile-user -s > $B/build.log 2>&1; echo rc=$?; tail -5 $B/build.log; find $B -name "*.elf"
1278:DEVICE_OS_PATH="$T/deviceOS/6.6.0" APPDIR="$PWD" PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH="$T/gcc-arm/10.2.1/bin/" PATH="$PATH:$T/gcc-arm/10.2.1/bin" make -f $T/buildscripts/1.17.2/Makefile compile-user -s BUILD_PATH_BASE="$PWD/$B/bpb" > $B/build.log 2>&1; echo rc=$?; tail -5 $B/build.log|cut -c1-300; find $B target -name "*.elf" -newer src/time/ExternalRtc.h
22:Invocation: Device OS 6.6.0, Boron, gcc-arm 10.2.1, buildscripts 1.17.2, `DEVICE_OS_PATH=$T/deviceOS/6.6.0 APPDIR=$PWD PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH=... make -f $T/buildscripts/1.17.2/Makefile compile-user -s BUILD_PATH_BASE=$PWD/build-tmp/wo-2026-11-05-003-r1fix/bpb` (fresh base passed as a make argument). Head = `3261210` plus this uncommitted change.
1067:`$ grep -n "make -f\|buildtools\|PLATFORM_ID\|APPDIR" docs/work-orders/WO-2026-11-05-003-amend3-stage6-copilot-transcript.md | head -8; rm -rf /var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/tmp.3o65sRpCbr; ls ~/.particle/toolchains; ls ~/.particle/toolchains/buildtools 2>/dev/null; ls build-tmp`
1077:990:make -f '/Users/chipmc/.particle/toolchains/buildscripts/1.17.2/Makefile' compile-user -s
1079:1058:DEVICE_OS_PATH="$T/deviceOS/6.6.0" APPDIR="$PWD" PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH="$T/gcc-arm/10.2.1/bin/" PATH="$PATH:$T/gcc-arm/10.2.1/bin" BUILD_PATH_BASE="$PWD/$B/bpb" make -f $T/buildscripts/1.17.2/Makefile compile-user -s > $B/build.log 2>&1; echo rc=$?; tail -5 $B/build.log; find $B -name "*.elf"
1080:1278:DEVICE_OS_PATH="$T/deviceOS/6.6.0" APPDIR="$PWD" PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH="$T/gcc-arm/10.2.1/bin/" PATH="$PATH:$T/gcc-arm/10.2.1/bin" make -f $T/buildscripts/1.17.2/Makefile compile-user -s BUILD_PATH_BASE="$PWD/$B/bpb" > $B/build.log 2>&1; echo rc=$?; tail -5 $B/build.log|cut -c1-300; find $B target -name "*.elf" -newer src/time/ExternalRtc.h
2026-11-05-v49-release-and-soak.md
WO-2026-08-11-001-sleep-prep-threshold-exclusion.md
WO-2026-11-01-001-ab1805-native-driver-spike.md
WO-2026-11-01-002-pmic-charger-reapply-step0.md
WO-2026-11-01-002-step0-report.md
WO-2026-11-02-001-closing-record.md
WO-2026-11-02-001-stage6-copilot-dispatch.md
WO-2026-11-02-001-stage6-round2-copilot-dispatch.md
WO-2026-11-02-001-stage6-round2-transcript.md
WO-2026-11-02-001-stage6-round3-copilot-dispatch.md
WO-2026-11-02-001-stage6-round3-transcript.md
WO-2026-11-02-001-stage6-transcript.md
WO-2026-11-02-001-stage7-round1-codex-dispatch.md
WO-2026-11-02-001-stage7-round1-verdict.md
WO-2026-11-02-001-stage7-round2-codex-dispatch.md
WO-2026-11-02-001-stage7-round2-verdict.md
WO-2026-11-02-001-stage7-round3-codex-dispatch.md
WO-2026-11-02-001-stage7-round3-verdict.md
WO-2026-11-02-001-usb-charge-latch-recovery.md
WO-2026-11-02-002-untrusted-soc-tier-fix.md
WO-2026-11-02-003-ci-gate-soak-report.md
WO-2026-11-02-003-github-actions-build.md
WO-2026-11-02-003-predispatch-checks.md
WO-2026-11-02-003-stage6-copilot-dispatch.md
WO-2026-11-02-003-stage6-copilot-report.md
WO-2026-11-02-003-stage6-copilot-transcript.md
WO-2026-11-02-003-stage6-round2-copilot-dispatch.md
WO-2026-11-02-003-stage6-round2-copilot-transcript.md
WO-2026-11-02-003-stage7-codex-dispatch.md
WO-2026-11-02-003-stage7-round2-codex-dispatch.md
WO-2026-11-02-003-stage7-round2-verdict.md
WO-2026-11-02-003-stage7-verdict.md
WO-2026-11-02-004-stage6-copilot-dispatch.md
WO-2026-11-02-004-stage6-copilot-report.md
WO-2026-11-02-004-stage6-copilot-transcript.md
WO-2026-11-02-004-stage6-round2-copilot-dispatch.md
WO-2026-11-02-004-stage6-round2-copilot-report.md
WO-2026-11-02-004-stage6-round2-copilot-transcript.md
WO-2026-11-02-004-stage7-codex-dispatch.md
WO-2026-11-02-004-stage7-round2-codex-dispatch.md
WO-2026-11-02-004-stage7-round2-verdict.md
WO-2026-11-02-004-stage7-verdict.md
WO-2026-11-02-004-superseded-planned-reset.patch
WO-2026-11-02-004-timezone-applies-to-running-clock.md
WO-2026-11-03-001-step0-report.md
WO-2026-11-03-003-checks-report.md
WO-2026-11-03-003-release-pipeline.md
WO-2026-11-03-003-stage6-copilot-dispatch.md
WO-2026-11-03-003-stage6-copilot-report.md
WO-2026-11-03-003-stage6-copilot-transcript.md
WO-2026-11-03-003-stage6-partb-copilot-dispatch.md
WO-2026-11-03-003-stage6-partb-copilot-report.md
WO-2026-11-03-003-stage6-partb-copilot-transcript.md
WO-2026-11-03-003-stage6-partb-fmt-copilot-dispatch.md
WO-2026-11-03-003-stage6-partb-fmt-copilot-report.md
WO-2026-11-03-003-stage6-partb-fmt-copilot-transcript.md
WO-2026-11-03-003-stage6-r2-copilot-dispatch.md
WO-2026-11-03-003-stage6-r2-copilot-report.md
WO-2026-11-03-003-stage6-r2-copilot-transcript.md
WO-2026-11-03-003-stage6-r3-copilot-dispatch.md
WO-2026-11-03-003-stage6-r3-copilot-report.md
WO-2026-11-03-003-stage6-r3-copilot-transcript.md
WO-2026-11-03-003-stage7-partb-fmt-r1-codex-dispatch.md
WO-2026-11-03-003-stage7-partb-fmt-r1-verdict.md
WO-2026-11-03-003-stage7-partb-r1-codex-dispatch.md
WO-2026-11-03-003-stage7-partb-r1-verdict.md
WO-2026-11-03-003-stage7-r1-codex-dispatch.md
WO-2026-11-03-003-stage7-r1-verdict.md
WO-2026-11-03-003-stage7-r2-codex-dispatch.md
WO-2026-11-03-003-stage7-r2-verdict.md
WO-2026-11-03-003-stage7-r3-codex-dispatch.md
WO-2026-11-03-003-stage7-r3-verdict.md
WO-2026-11-03-004-item19-c1-millis-expiry.md
WO-2026-11-03-004-stage6-copilot-dispatch.md
WO-2026-11-03-004-stage6-copilot-report.md
WO-2026-11-03-004-stage6-copilot-transcript.md
WO-2026-11-03-004-stage7-r1-codex-dispatch.md
WO-2026-11-03-004-stage7-r1-verdict.md
WO-2026-11-03-005-item26-durability-flush.md
WO-2026-11-03-005-stage6-copilot-dispatch.md
WO-2026-11-03-005-stage6-copilot-report.md
WO-2026-11-03-005-stage6-copilot-transcript.md
WO-2026-11-03-005-stage7-r1-codex-dispatch.md
WO-2026-11-03-005-stage7-r1-verdict.md
WO-2026-11-03-006-item19-c2-untrusted-split.md
WO-2026-11-03-006-stage6-copilot-dispatch.md
WO-2026-11-03-006-stage6-copilot-report.md
WO-2026-11-03-006-stage6-copilot-transcript.md
WO-2026-11-03-006-stage6-r2-copilot-dispatch.md
WO-2026-11-03-006-stage6-r2-copilot-report.md
WO-2026-11-03-006-stage6-r2-copilot-transcript.md
WO-2026-11-03-006-stage7-r1-codex-dispatch.md
WO-2026-11-03-006-stage7-r1-verdict.md
WO-2026-11-03-006-stage7-r2-codex-dispatch.md
WO-2026-11-03-006-stage7-r2-verdict.md
WO-2026-11-04-002-stage6-copilot-dispatch.md
WO-2026-11-04-002-stage6-copilot-report.md
WO-2026-11-04-002-stage6-copilot-transcript.md
WO-2026-11-04-002-stage6-r2-copilot-dispatch.md
WO-2026-11-04-002-stage6-r2-copilot-report.md
WO-2026-11-04-002-stage6-r2-copilot-transcript.md
WO-2026-11-04-002-stage7-r1-codex-dispatch.md
WO-2026-11-04-002-stage7-r1-verdict.md
WO-2026-11-04-002-stage7-r2-codex-dispatch.md
WO-2026-11-04-002-stage7-r2-verdict.md
WO-2026-11-04-002-v49-binding-delete-guard.md
WO-2026-11-05-001-stage7-r1-codex-dispatch.md
WO-2026-11-05-001-stage7-r1-verdict.md
WO-2026-11-05-001-upload-file-name-copilot-dispatch.md
WO-2026-11-05-001-upload-file-name-copilot-report.md
WO-2026-11-05-001-upload-file-name-copilot-transcript.md
WO-2026-11-05-002-v50-ab1805-switch.md
WO-2026-11-05-003-amend3-stage6-copilot-dispatch.md
WO-2026-11-05-003-amend3-stage6-copilot-report.md
WO-2026-11-05-003-amend3-stage6-copilot-transcript.md
WO-2026-11-05-003-r1fix-stage6-copilot-dispatch.md
WO-2026-11-05-003-r1fix-stage6-copilot-report.md
WO-2026-11-05-003-r1fix-stage6-copilot-transcript.md
WO-2026-11-05-003-stage6-copilot-dispatch.md
WO-2026-11-05-003-stage6-copilot-report.md
WO-2026-11-05-003-stage6-copilot-transcript.md
WO-2026-11-05-003-stage7-r1-codex-dispatch.md
WO-2026-11-05-003-stage7-r1-verdict.md
WO-2026-11-05-003-stage7-r2-codex-dispatch.md
WO-2026-11-05-003-stage7-r2-verdict.md
WO-2026-11-05-003-v50-s1-boot-config-time.md
WO-2026-11-05-004-architect-ruling-closing-stage7.md
WO-2026-11-05-004-stage6-copilot-dispatch.md
WO-2026-11-05-004-stage6-copilot-report.md
WO-2026-11-05-004-stage6-copilot-transcript.md
WO-2026-11-05-004-stage6-r2-copilot-dispatch.md
WO-2026-11-05-004-stage6-r2-copilot-report.md
WO-2026-11-05-004-stage6-r2-copilot-transcript.md
WO-2026-11-05-004-stage6-r3-copilot-dispatch.md
WO-2026-11-05-004-stage6-r3-copilot-report.md
WO-2026-11-05-004-stage6-r3-copilot-transcript.md
WO-2026-11-05-004-stage7-r1-codex-dispatch.md
WO-2026-11-05-004-stage7-r1-verdict.md
WO-2026-11-05-004-stage7-r2-codex-dispatch.md
WO-2026-11-05-004-stage7-r2-verdict.md
WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md
WO-2026-11-05-005-stage6-copilot-dispatch.md
WO-2026-11-05-005-stage6-copilot-report.md
WO-2026-11-05-005-stage6-copilot-transcript.md
WO-2026-11-05-005-stage6-r2-copilot-dispatch.md
WO-2026-11-05-005-stage6-r2-copilot-report.md
WO-2026-11-05-005-stage6-r2-copilot-transcript.md
WO-2026-11-05-005-stage7-r1-codex-dispatch.md
WO-2026-11-05-005-stage7-r1-verdict.md
WO-2026-11-05-005-stage7-r2-codex-dispatch.md
WO-2026-11-05-005-stage7-r2-verdict.md
WO-2026-11-05-005-v50-s3-timed-power-off.md
WO-2026-11-06-001-stage6-copilot-dispatch.md
WO-2026-11-06-001-v50-s4-oscillator-fault-latch.md
WO-2026-11-06-002-architect-ruling-closing-stage7.md
WO-2026-11-06-002-stage6-copilot-dispatch.md
WO-2026-11-06-002-stage6-copilot-report.md
WO-2026-11-06-002-stage6-copilot-transcript.md
WO-2026-11-06-002-stage6-r2-copilot-dispatch.md
WO-2026-11-06-002-stage6-r2-copilot-report.md
WO-2026-11-06-002-stage6-r2-copilot-transcript.md
WO-2026-11-06-002-stage6-r3-copilot-dispatch.md
WO-2026-11-06-002-stage6-r3-copilot-report.md
WO-2026-11-06-002-stage6-r3-copilot-transcript.md
WO-2026-11-06-002-stage7-r1-codex-dispatch.md
WO-2026-11-06-002-stage7-r1-verdict.md
WO-2026-11-06-002-stage7-r2-codex-dispatch.md
WO-2026-11-06-002-stage7-r2-verdict.md
WO-2026-11-06-002-v50-s1b-pre-bind-rtc-access.md
project.properties
<shellId: 20 completed with exit code 0>
```

</details>


---

<sub>23m 14s</sub>

### `bash`

**Toolchain build**

`$ T=$HOME/.particle/toolchains; B=build-tmp/wo-2026-11-06-001-stage6; DEVICE_OS_PATH="$T/deviceOS/6.6.0" APPDIR="$PWD" PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH="$T/gcc-arm/10.2.1/bin/" PATH="$PATH:$T/gcc-arm/10.2.1/bin" make -f $T/buildscripts/1.17.2/Makefile compile-user -s BUILD_PATH_BASE="$PWD/$B/bpb" > $B/build.log 2>&1; echo rc=$?; tail -5 $B/build.log | cut -c1-300; find $B target -name "*.elf" 2>/dev/null`

<details>
<summary>6 lines</summary>

```
rc=0
   text	   data	    bss	    dec	    hex	filename
 151084	   1114	   2256	 154454	  25b56	/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/target/6.6.0/boron/wt-s4.elf

*** COMPILED SUCCESSFULLY ***

target/6.6.0/boron/wt-s4.elf
<shellId: 21 completed with exit code 0>
```

</details>


---

<sub>23m 18s</sub>

### `bash`

**nm evidence and status**

`$ T=$HOME/.particle/toolchains; NM=$T/gcc-arm/10.2.1/bin/arm-none-eabi-nm; $NM -C target/6.6.0/boron/wt-s4.elf | grep -i "onOscillatorEvent\|oscillatorFaultSeen"; git status --short; git diff --numstat e21a4a4 -- src`

<details>
<summary>10 lines</summary>

```
000b5358 W ExternalRtc::onOscillatorEvent(particle::EnumFlags<particle::RtcEvent, void>, void*)
2003d728 V ExternalRtc::oscillatorFaultSeenSinceBind()::seen
 M src/Generalized-Core-Counter.cpp
 M src/time/ExternalRtc.h
 M tests/external_rtc_structural_test.py
 M tests/external_rtc_test.cpp
 M tests/stubs/external_rtc/Particle.h
2	0	src/Generalized-Core-Counter.cpp
5	0	src/time/ExternalRtc.h
<shellId: 22 completed with exit code 0>
```

</details>


---

<sub>23m 53s</sub>

### `bash`

**Clean export check, cleanup**

`$ rm -rf target; B=build-tmp/wo-2026-11-06-001-stage6; rm -rf $B/bpb; E=$B/export; mkdir -p $E && git ls-files -z | xargs -0 -I{} true 2>/dev/null; git archive HEAD | tar -x -C $E; cp src/Generalized-Core-Counter.cpp $E/src/; cp src/time/ExternalRtc.h $E/src/time/; (cd $E && python3 tests/thermal_coupling_structural_test.py >/dev/null 2>&1; echo export rc=$?); rm -rf $E; for t in tests/*.py tests/*.sh; do :; done; ls tests | grep -v "\.sh$\|\.py$\|\.cpp$\|stubs\|fixtures\|README"; ls build-tmp build-tmp/wo-2026-11-06-001-stage6`

<details>
<summary>192 lines</summary>

```
export rc=0
SensorManagerBatteryAuthorityTests.cpp
__pycache__
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
s2_alarm_watchdog_wake_structural_test.py
s2_hibernate_normalise.py
s2_transcript_normalise.py
s3_timed_power_off_test.cpp
s3_timed_power_off_test.sh
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
sleep_plan_equivalence_test.cpp
sleep_plan_equivalence_test.sh
sleep_plan_structural_test.py
sleep_preflight_equivalence_test.cpp
sleep_preflight_equivalence_test.sh
sleep_preflight_structural_test.py
sleep_prep_span_timing_test.cpp
sleep_teardown_equivalence_test.cpp
sleep_teardown_equivalence_test.sh
sleep_teardown_structural_test.py
sleep_wake_equivalence_test.cpp
sleep_wake_equivalence_test.sh
sleep_wake_structural_test.py
standby_latch_daily_clear_test.cpp
standby_latch_daily_clear_test.sh
startup_snapshot_test.cpp
thermal_coupling_structural_test.py
thermal_threshold_migration_test.cpp
thermal_threshold_migration_test.sh
untrusted_piece_c2_test.sh
version_guard_preprocessor_test.sh
wake_gate_single_owner_structural_test.py
wake_summary_led_test.cpp
wake_summary_led_test.sh
watchdog_alert_code_test.cpp
watchdog_alert_code_test.sh
watchdog_inferred_classification_test.cpp
watchdog_inferred_classification_test.sh
webhook_timeout_range_test.py
build-tmp:
wo-2026-11-03-004-stage6
wo-2026-11-03-006-stage6-r2
wo-2026-11-06-001-stage6

build-tmp/wo-2026-11-06-001-stage6:
build.log
run.sh
<shellId: 23 completed with exit code 0>
```

</details>


---

<sub>24m 15s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/docs/work-orders/WO-2026-11-06-001-stage6-copilot-report.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/docs/work-orders/WO-2026-11-06-001-stage6-copilot-report.md`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/docs/work-orders/WO-2026-11-06-001-stage6-copilot-report.md with 5914 characters
```


---

<sub>24m 21s</sub>

### Copilot

S4 is implemented as `src/` +7 net, uncommitted, and the report is written. It has not run on hardware, so the bench confirmation is still open.

- **Change:** the process-lifetime flag `ExternalRtc::oscillatorFaultSeenSinceBind()` is in `src/time/ExternalRtc.h`, along with the callback that sets it. In `setup()`, the existing `startupOscUsingRC` read now sets the flag, and the `ExternalTime.onEvent` callback is registered after that read.
- **Step 0:** nothing breaks the plan. The registration-time clear of the chip's fail flag is at `am18x5.cpp:646`, so reading the clock source first is the safe order. The callback runs on the Am18x5 worker thread, not in an interrupt.
- **Budget:** net +7 in `src/` (`Generalized-Core-Counter.cpp` +2, `ExternalRtc.h` +5). I did not add a log line to the callback.
- **Tests:** the suite ran 121/121 before editing and 121/121 after (sh via zsh, py via python3). The new cases are in the existing `external_rtc` tests, not new files. All seven WO mutations, plus two extra (registered twice, cleared by a later event), are caught. The "true after an RC-fallback bind" and "false after a normal bind" cases are checked structurally on the `setup()` text, because `setup()` cannot run on the host.
- **Build:** Device OS 6.6.0 Boron build passes at text/data/bss 151084 / 1114 / 2256, which is +160 / 0 / +16 over the S3 head. `nm` shows `onOscillatorEvent` and the flag linked.
- **Reasoning level:** dispatched at `medium`; I could not observe the level actually run, and the report says so.
- **Deviation:** I edited the stub `tests/stubs/external_rtc/Particle.h` with a one-off python rewrite instead of the edit tool.

The report is `docs/work-orders/WO-2026-11-06-001-stage6-copilot-report.md`.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>
