# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `7c2f4df6-ec5a-4487-91cd-e4441d889147`  
> - **Started:** 11/3/2026, 3:44:03 PM  
> - **Duration:** 31m 34s  
> - **Exported:** 11/3/2026, 4:15:38 PM  

---

<sub>5s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-03-005-stage6-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>7s</sub>

### `view`

**view the file at /private/tmp/gcc-step0/dur/docs/work-orders/WO-2026-11-03-005-stage6-copilot-dispatch.md.**

/private/tmp/gcc-step0/dur/docs/work-orders/WO-2026-11-03-005-stage6-copilot-dispatch.md

<details>
<summary>5 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/` files named in the WO, add tests under `tests/`, write the report; scratch `build-tmp/wo-2026-11-03-005-stage6/` / NOT authorized: committing, pushing, merging, flashing, creating PRs, touching the OOM reset path, the watchdog callbacks, HIBERNATE, `lib/` (no library change), the persisted or `retained` layout, deleting anything you didn't create (including `build-tmp/`).

# Stage 6 dispatch — WO-2026-11-03-005 (item 26 v49 durability: flushAllBeforeReset)
Binding spec: `docs/work-orders/WO-2026-11-03-005-item26-durability-flush.md` **including Amendment 1** (the function lives in `State_Error`; alert codes are in the `current` store and are written before the flush; the failsafe's `SystemConfig::flushNow()` is replaced by the new call), and `docs/work-orders/WO-2026-11-03-002-step0-report.md` §1-§5. Branch `wo/2026-11-03-005-item26-durability-flush` at the head you were given. Leave changes uncommitted. **Budget: `src/` ≤ +20 net. Stop and report instead of trimming style.**
Do Step 0 first (re-read every cited site on this base; confirm which store holds `RecoveryState` alert codes; confirm `PublishQueuePosix` exposes a synchronous `writeQueueToFiles()` through the instance the app owns; report any mismatch and stop if the design cannot be done in +20). Then implement the two store facades, `flushAllBeforeReset()` and the call sites (thrash tier 3, Error soft and hard, nightly heap guard, sleep-exhausted, failsafe stages 2 and 3; **not OOM**), tests per the WO (order, what it flushes, OOM structural exclusion, failsafe record-then-flush, mutations), host suite (`N/N (sh via zsh, py via python3)`), local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE`) and `nm` linkage of `flushAllBeforeReset` and its call sites. Write `docs/work-orders/WO-2026-11-03-005-stage6-copilot-report.md` (model and reasoning actually used, net `src/` per file, test names, mutation results, nm evidence, Step 0 findings, open questions). The Dev-14 bench is Chip's later step: do not claim it.
```

</details>


---

<sub>9s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && git status --short | head; git branch --show-current; ls docs/work-orders tests | head -80; wc -c docs/work-orders/WO-2026-11-03-005-item26-durability-flush.md docs/work-orders/WO-2026-11-03-002-step0-report.md**

$ cd /private/tmp/gcc-step0/dur && git status --short | head; git branch --show-current; ls docs/work-orders tests | head -80; wc -c docs/work-orders/WO-2026-11-03-005-item26-durability-flush.md docs/work-orders/WO-2026-11-03-002-step0-report.md

<details>
<summary>85 lines</summary>

```
wo/2026-11-03-005-item26-durability-flush
docs/work-orders:
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
wc: docs/work-orders/WO-2026-11-03-002-step0-report.md: open: No such file or directory
    8096 docs/work-orders/WO-2026-11-03-005-item26-durability-flush.md
    8096 total
<shellId: 0 completed with exit code 1>
```

</details>


---

<sub>11s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && cat docs/work-orders/WO-2026-11-03-005-item26-durability-flush.md; ls docs/work-orders | grep 2026-11; git log --oneline | head -3**

$ cd /private/tmp/gcc-step0/dur && cat docs/work-orders/WO-2026-11-03-005-item26-durability-flush.md; ls docs/work-orders | grep 2026-11; git log --oneline | head -3

<details>
<summary>68 lines</summary>

```
# WO-2026-11-03-005: item 26 v49 durability — one flushAllBeforeReset() at the deliberate-reset sites

**Author:** Claude Code, 3 Nov 2026, DRAFT for the architect's review · **Status:** DRAFT; not dispatched
**Branch:** `wo/2026-11-03-005-item26-durability-flush`, from `main`
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (persistence across reset: Codex xhigh); §12; §13 (c), (j)
**Target:** **v49** (RC1 scope: #172 + item 25 + this + the binding-delete guard).
**Budget:** `src/` at most **+20 net**. Stop and report rather than trim style.
**Agents:** Stage 6 Copilot standard, medium. Stage 7 Codex `gpt-5.6-sol`, **xhigh**, an `UNDER REVIEW` header, `-stage7-r<N>` files, at most two rounds.

## Plain goal
A deliberate reset can lose data that was "saved": `/usr/current.dat` (occupancy, `dailyoccupancy`, open-session state; 250 ms delayed write, no forced flush), `/usr/sensor.dat` (250 ms delayed write, no forced flush) and the **RAM publish queue** (`PublishQueuePosixRK` RAM size 2; the library says a non-software-update reset loses it). Only `/usr/sysStatus.dat` has a synchronous flush (`SystemConfig::flushNow()`), and only failsafe stage 2/3 uses it (`ConnectivityFailsafe.cpp:77-87,301-314`). Found by Codex in Stage 7 round 1 of WO-2026-11-02-004 (B1, B2) and mapped in the item-26 Step 0 (`WO-2026-11-03-002-step0-report.md` §1-§5). A reset right after a day-boundary close can persist `lastDailyCleanup` while the current record lags, so the old open session or total can return, or the close credit be lost.
This WO adds **one** function that forces all of it to flash before the reset, and calls it from the sites that can run it safely. It is the durability half of the item-26 design; the single reset owner (phase state, timer) comes after the AB1805 switch.

## Architect ruling (3 Nov)
- v49 = durability: one `flushAllBeforeReset()` (force-flush **C, S, N and Q**), called at **thrash tier 3, Error soft and hard, the nightly heap guard, sleep-exhausted, and failsafe stages 2 and 3**.
- **OOM is excluded** (no flush, no warning; the immediate reset in `Generalized-Core-Counter.cpp:1208-1213` is unchanged; `tests/oom_immediate_reset_test.sh`).
- **Documented exceptions (no change):** the two hardware watchdogs (internal 60 s and AB1805), the software ApplicationWatchdog callback (`LoopForensics.cpp:395-399`), and HIBERNATE.
- **"Queue an alert" = write the alert code to the persistent store before the reset** (no publish, no queue event): the code is written into the record that `flushAllBeforeReset()` flushes.
- Phase state, timer, deadline and the single owner: later, after the AB1805 switch (likely post-RC1).

## The change (re-check at Step 0; lines are from main `21804e3`)
1. **Facades** for the two stores that lack one: a synchronous flush for `CurrentReadings` (`persist/CurrentReadings.h:20-28`, definition beside `CurrentReadings::loop()`, `MyPersistentData.cpp:1103-1108`) and for `SensorSettings` (`persist/SystemConfig.h:223-253`, `MyPersistentData.cpp:1010-1033`), each forwarding to the store's `flush(true)` exactly as `SystemConfig::flushNow()` does (`MyPersistentData.cpp:914-919`). About +4 lines.
2. **`flushAllBeforeReset()`** (one place, e.g. `SystemConfig.h/.cpp` or a small header the reset sites already include): `CurrentReadings` flush, `SystemConfig::flushNow()`, `SensorSettings` flush, then the publish queue's synchronous `writeQueueToFiles()` (`PublishQueuePosixRK.h:231-234`). Order: stores first, queue last (the queue file write is the longest). No allocation. About +6-8 lines.
3. **Seven call sites**, one line each, immediately before the reset or power-down: `ThrashGuard.cpp:149-152`, `State_Error.cpp:153-170` (soft and hard), `State_Sleep.cpp:630-649` (heap guard), `State_Sleep.cpp:892-909` (sleep exhausted), `ConnectivityFailsafe.cpp:301-314` (stage 2 and stage 3; the existing `SystemConfig::flushNow()` there is replaced by the new call, and the stage/action/count record must still be written **before** the flush). About +7 lines.
4. Alerts that a site raises just before its reset (thrash tier 3's alert 18, the Error alert) are written to the store **before** `flushAllBeforeReset()` so they are in the flushed record.

## Protected / not in scope
OOM, the watchdog callbacks and HIBERNATE; the Error state's behavior (suppression policy, the 30 s dwell); the failsafe ladder (stage values, cooldowns, TEST_MODE); the persisted layout and `retained` RAM (zero change); `deepPowerDown()`'s internal reset fallback (stays; native `POWER_OFF` replaces it at the AB1805 switch); `lib/PublishQueuePosixRK` (no library change); hibernate ordering (`HibernateCycle::armForSleep()`).

## Time budget (provisional; no latency is measured in the repo)
Each flush is synchronous with no cancel. The total worst case must stay well below the 60 s internal watchdog; the bench measures the real figure (below) and the WO records it. Proposed ceilings from the Step 0: each store 250 ms, queue 2 s. If a measured flush exceeds them, report; do not add a timeout in this WO.

## Tests (host; the real functions)
- **Order and presence:** each of the seven sites calls `flushAllBeforeReset()` before the reset/power-down, after its own record writes; OOM does not call it (structural test next to `oom_immediate_reset_test.sh`).
- **What it flushes:** a fake backend records C, S, N, Q flush events: all four, stores before queue.
- **Failsafe:** stage/action/count written, then flush, then reset; `ConnectivityFailsafe` harnesses stay green.
- **Mutations (each must fail):** drop each of the four flushes; reverse queue/store order; remove the call at each of the seven sites in turn; add a call at the OOM site; call it before the stage record is written.
- `nm` on the local-toolchain ELF: `flushAllBeforeReset` and its seven production call sites.

## Acceptance and bench (Codex xhigh; Dev-14)
Dirty all three stores and hold two RAM events (the Step 0 plan): trigger each reachable site connected and offline (failsafe TEST_MODE stage 2 then 3; Error soft; the nightly heap guard if reachable); after each reboot verify `dailyoccupancy`, the open-session state and the sensor settings are the pre-reset values and the two queued events are published or in the file queue; verify each `resetReasonData`; record the measured flush durations; then two nightly hibernates and one OTA-in-progress to confirm no change there.

## Rollback
Revert the PR; v48 behavior returns; no data to migrate.

## Open questions for the architect
1. Where does the function live (`persist/` header vs a new `src/` file)? Recommendation: beside `SystemConfig::flushNow()`.
2. Thrash tier 3 and the Error state raise an alert code and reset in the same pass: confirm the alert code's store (`RecoveryState`, in `sysStatus` or `current`?) is one of the three stores flushed (Step 0 must confirm).
3. Does the failsafe keep its own `SystemConfig::flushNow()` call as well (belt and braces) or only the new one? Recommendation: only the new one, called after the stage record.

## Amendment 1 (architect rulings, 3 Nov) — status: APPROVED to dispatch
1. **`flushAllBeforeReset()` lives in `State_Error`** (declared in the state header the other reset sites already include; defined in `src/state/State_Error.cpp`). Sites that cannot see it today include that header; no new file.
2. **Alert codes are in the `current` store, which is flushed;** the alert is **written before** the flush (thrash tier 3's alert 18, the Error alert). Step 0 of this WO confirms the store by reading `RecoveryState::set_alertCode`.
3. **The failsafe's `SystemConfig::flushNow()` is replaced by the new call** (after the stage/action/count record is written), not kept as well.
4. **Provisional time limits accepted** (each store 250 ms, queue 2 s); the bench measures them and the WO records the figures.
5. **PRs:** no PR for the draft; this WO goes with its implementation PR. Agents and budget unchanged: Copilot standard/medium; Codex `gpt-5.6-sol` xhigh, §4 SHA rule, at most two rounds; `src/` ≤ +20.
WO-2026-11-01-001-ab1805-native-driver-spike.md
WO-2026-11-01-002-pmic-charger-reapply-step0.md
WO-2026-11-01-002-step0-report.md
WO-2026-11-02-001-usb-charge-latch-recovery.md
WO-2026-11-02-002-untrusted-soc-tier-fix.md
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
WO-2026-11-02-004-timezone-applies-to-running-clock.md
WO-2026-11-03-005-item26-durability-flush.md
WO-2026-11-03-005-stage6-copilot-dispatch.md
8fb62f4 WO amendments (architect rulings, 3 Nov) and Stage 6 dispatch
c75d427 Merge remote-tracking branch 'origin/main' into wo/2026-11-03-005-item26-durability-flush
06ee2fc WO-2026-11-03-005: item 26 v49 durability flush draft (flushAllBeforeReset), for the architect's review
<shellId: 1 completed with exit code 0>
```

</details>


---

<sub>13s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && ls; ls src src/state tests | head -150; sed -n 905,925p src/MyPersistentData.cpp; sed -n 1005,1035p src/MyPersistentData.cpp; sed -n 1095,1112p src/MyPersistentData.cpp**

$ cd /private/tmp/gcc-step0/dur && ls; ls src src/state tests | head -150; sed -n 905,925p src/MyPersistentData.cpp; sed -n 1005,1035p src/MyPersistentData.cpp; sed -n 1095,1112p src/MyPersistentData.cpp

<details>
<summary>215 lines</summary>

```
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
src:
BuildProfile.h
Config.cpp
Config.h
FirmwareVersion.h
Generalized-Core-Counter.cpp
MyPersistentData.cpp
MyPersistentData.h
ResetCause.h
ThrashGuard.cpp
ThrashGuard.h
cloud
device_pinout.cpp
device_pinout.h
diagnostics
observability
persist
power
reporting
sensors
state
time

src/state:
SleepPrepSpanTiming.h
SleepTeardown.cpp
SleepTeardown.h
StateHandlers.h
StateMachine.h
StateManager.h
State_Common.h
State_Connect.cpp
State_Error.cpp
State_Idle.cpp
State_Modes.cpp
State_Report.cpp
State_Sleep.cpp

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
exerciser_baseline_test.py
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
publish_with_ack_structural_test.py
report_payload_fields_test.py
report_payload_golden_test.cpp
report_payload_golden_test.sh
report_publisher_move_structural_test.py
reporting_interval_cap_test.cpp
reporting_interval_cap_test.sh
reporting_interval_store_seam_structural_test.py
reporting_policy_adapter_test.cpp
reporting_policy_adapter_test.sh

// The facades publish these capacities so ConfigApply.cpp can size its staging
// buffers without seeing SysData. Binding them to the real fields here means a
// future field-width change breaks the build instead of silently truncating.
static_assert(SystemConfig::kTimeZoneCapacity == sizeof(sysStatusData::SysData::timeZoneStr),
              "SystemConfig::kTimeZoneCapacity must match SysData::timeZoneStr");
static_assert(SystemConfig::kWebhookNameCapacity == sizeof(sysStatusData::SysData::webhookName),
              "SystemConfig::kWebhookNameCapacity must match SysData::webhookName");

namespace SystemConfig {

void setup() { sysStatus.setup(); }
void loop() { sysStatus.loop(); }
void flushNow() { sysStatus.flush(true); }

// sizeof(sysStatusData), matching the pre-split sysStatus.validate(sizeof(sysStatus)).
bool validateStoredData() { return sysStatus.validate(sizeof(sysStatusData)); }

bool get_verboseMode() { return sysStatus.get_verboseMode(); }
void set_verboseMode(bool value) { sysStatus.set_verboseMode(value); }

void set_hasValidLedgerConfig(bool value) { sysStatus.set_hasValidLedgerConfig(value); }

uint8_t get_configSource() { return sysStatus.get_configSource(); }
void set_configSource(uint8_t value) { sysStatus.set_configSource(value); }

namespace SensorSettings {

void setup() { sensorConfig.setup(); }
void loop() { sensorConfig.loop(); }

// sizeof(sensorConfigData), matching the pre-split sensorConfig.validate(sizeof(sensorConfig)).
bool validateStoredData() { return sensorConfig.validate(sizeof(sensorConfigData)); }

uint8_t get_sensorType() { return sensorConfig.get_sensorType(); }
void set_sensorType(uint8_t value) { sensorConfig.set_sensorType(value); }

uint32_t get_sensorSetting1() { return sensorConfig.get_sensorSetting1(); }
void set_sensorSetting1(uint32_t value) { sensorConfig.set_sensorSetting1(value); }

uint32_t get_sensorSetting2() { return sensorConfig.get_sensorSetting2(); }
void set_sensorSetting2(uint32_t value) { sensorConfig.set_sensorSetting2(value); }

uint32_t get_sensorSetting3() { return sensorConfig.get_sensorSetting3(); }
void set_sensorSetting3(uint32_t value) { sensorConfig.set_sensorSetting3(value); }

uint32_t get_sensorSetting4() { return sensorConfig.get_sensorSetting4(); }
void set_sensorSetting4(uint32_t value) { sensorConfig.set_sensorSetting4(value); }

}  // namespace SensorSettings

}  // namespace SystemConfig
float get_thermalChargeReleaseHighC() { return sysStatus.get_thermalChargeReleaseHighC(); }
void set_thermalChargeReleaseHighC(float value) { sysStatus.set_thermalChargeReleaseHighC(value); }

float get_thermalChargeReleaseLowC() { return sysStatus.get_thermalChargeReleaseLowC(); }
void set_thermalChargeReleaseLowC(float value) { sysStatus.set_thermalChargeReleaseLowC(value); }

}  // namespace PowerConfig

namespace CurrentReadings {

void setup() { current.setup(); }
void loop() { current.loop(); }
void resetEverything(bool includeSensorTotals) { current.resetEverything(includeSensorTotals); }
void revalidateOccupancyStartTimeIfTimeAvailable() { current.revalidateOccupancyStartTimeIfTimeAvailable(); }

uint16_t get_hourlyCount() { return current.get_hourlyCount(); }
void set_hourlyCount(uint16_t value) { current.set_hourlyCount(value); }
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>16s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && sed -n 120,185p src/ThrashGuard.cpp; echo ====; sed -n 120,200p src/state/State_Error.cpp; echo ===; sed -n 615,660p src/state/State_Sleep.cpp; echo ===; sed -n 880,915p src/state/State_Sleep.cpp; echo ===; sed -n 285,325p src/state/ConnectivityFailsafe.cpp 2>/dev/null || find src -name 'ConnectivityFailsafe*'**

$ cd /private/tmp/gcc-step0/dur && sed -n 120,185p src/ThrashGuard.cpp; echo ====; sed -n 120,200p src/state/State_Error.cpp; echo ===; sed -n 615,660p src/state/State_Sleep.cpp; echo ===; sed -n 880,915p src/state/State_Sleep.cpp; echo ===; sed -n 285,325p src/state/ConnectivityFailsafe.cpp 2>/dev/null || find src -name 'ConnectivityFailsafe*'

<details>
<summary>164 lines</summary>

```
int tier = 1;
  if (lastTripMs_ != 0 && (nowMs - lastTripMs_) <= TIER2_WINDOW_MS) {
    tier = 2;
  }
  if ((nowMs - windowStartMs_) <= TIER3_WINDOW_MS && recentTripCount_ >= TIER3_TRIP_COUNT) {
    tier = 3;
  }

  lastTripMs_ = nowMs;
  backoffUntilMs_ = nowMs + BACKOFF_MS;

  const uint32_t noprogSec = elapsedMs / 1000UL;

  if (tier == 1) {
    logTrip(currentState, noprogSec, lastTag_, 1, "backoff");
    return;
  }

  if (tier == 2) {
    logTrip(currentState, noprogSec, lastTag_, 2, "disconnect+sleep");
    RecoveryState::raiseAlert(18);  // Alert 18: Thrash detected (tier 2)
    Connectivity::requestFullDisconnectAndRadioOff();
    if (state != SLEEPING_STATE) {
      transitionTo(SLEEPING_STATE, "thrash tier 2");
    }
    return;
  }

  logTrip(currentState, noprogSec, lastTag_, 3, "reset");
  RecoveryState::raiseAlert(18);  // Alert 18: Severe thrash detected (tier 3 - reset)
  thrashResetCount++;
  System.reset(RESET_CAUSE_THRASH_TIER3);
}
====
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
      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);
    }
    break;

  case 3:
    // Hard recovery using AB1805 deep power down after delay. This fully
    // power-cycles the device and modem but is limited by resolveErrorAction
    // avoid thrashing.
    if (millis() - resetTimer > resetWait) {
      Log.info("Executing deep power down from ERROR_STATE (alert=%d)", RecoveryState::get_alertCode());
      ab1805.deepPowerDown();
    }
    break;

  default:
    // Should not happen, but don't get stuck here.
    transitionTo(IDLE_STATE, "error-unknown-resolution");
    break;
  }
}
===
        const int untilDeadline = (int)remaining + 1;  // wake just after the deadline
        if (untilDeadline < wakeInSeconds) wakeInSeconds = untilDeadline;
      } else if (debounceSeconds < (uint32_t)wakeInSeconds) {
        wakeInSeconds = (int)debounceSeconds;  // no deadline known: the full debounce
      }
      sleepReason = "debounce";
    } else {
      // Unoccupied - use scheduled wake time
      sleepReason = "scheduled";
    }
  }
  plan = {wakeInSeconds, sleepReason, overnightFallbackSleep};
  return false;
}

bool nightlyHeapGuard(bool overnightFallbackSleep) {
  // Before a long overnight
  // ULTRA_LOW_POWER fallback sleep, optionally reset first when heap has
  // drifted low. This gives the next day a clean start without affecting
  // open-hours behavior.
  if (overnightFallbackSleep) {
    const unsigned long freeHeap = (unsigned long)System.freeMemory();

    if (freeHeap <= ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES) {
      Log.info("ModemTeardown: radioOn=%d point=heap-guard-reset", (int)Connectivity::isRadioPoweredOn());
      Log.warn("Nightly heap guard: freeHeap=%lu <= %lu before overnight ULTRA_LOW_POWER fallback - resetting for clean next-day start",
               freeHeap,
               ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES);
      // Reset drops RAM - flush the diagnostics batch now, since this is
      // the only chance for this cycle's accumulated entries.
#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
      PowerDiagnostics::flushDiagBatch();
#endif
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

===
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
        System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);
        
        // Should never reach here, but set ERROR_STATE as fallback
        transitionTo(ERROR_STATE, "sleep-all-attempts-failed");
        return true;
      }
    }
===
src/power/ConnectivityFailsafe.cpp
src/power/ConnectivityFailsafe.h
src/diagnostics/ConnectivityFailsafeTest.h
src/diagnostics/ConnectivityFailsafeTest.cpp
<shellId: 3 completed with exit code 0>
```

</details>


---

<sub>16s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && cat tests/nightly_heap_guard_flush_test.sh | head -60; cat tests/oom_immediate_reset_test.sh; ls tests | tail -40; sed -n 15,32p src/persist/CurrentReadings.h; sed -n 215,255p src/persist/SystemConfig.h**

$ cd /private/tmp/gcc-step0/dur && cat tests/nightly_heap_guard_flush_test.sh | head -60; cat tests/oom_immediate_reset_test.sh; ls tests | tail -40; sed -n 15,32p src/persist/CurrentReadings.h; sed -n 215,255p src/persist/SystemConfig.h

<details>
<summary>346 lines</summary>

```
#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
generated_flag_on="$TMPDIR/nightly_heap_guard_flush_flagon.cpp"
generated_flag_off="$TMPDIR/nightly_heap_guard_flush_flagoff.cpp"
binary_flag_on="$TMPDIR/nightly_heap_guard_flush_flagon"
binary_flag_off="$TMPDIR/nightly_heap_guard_flush_flagoff"

extract_braced_block() {
  local file="$1"
  local marker="$2"
  awk -v marker="$marker" '
    !active && index($0, marker) { active = 1 }
    active {
      print
      opens = gsub(/\{/, "{")
      closes = gsub(/\}/, "}")
      depth += opens - closes
      if (seen_open && depth == 0) exit
      if (opens > 0) seen_open = 1
    }
  ' "$file"
}

generate() {
  local out_file="$1"
  {
    cat <<'CPP'
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "power/ConnectivityPolicy.h"
// WO-2026-10-01-001 item C: the extracted block now names its reset cause.
#include "ResetCause.h"

// Records the exact order System.reset()/System.freeMemory() and
// PowerDiagnostics::flushDiagBatch() were invoked in, so the test can assert
// that a flush happens *before* any reset - not merely that both happened
// somewhere during the call.
std::vector<std::string> callOrder;

struct FakeSystem {
  unsigned long freeMemoryValue = 0;

  unsigned long freeMemory() const { return freeMemoryValue; }

  void reset(uint32_t cause = 0) {
    // Item C: the guard must pass its own non-zero cause code.
    assert(cause == RESET_CAUSE_SLEEP_HEAP_GUARD);
    callOrder.push_back("System.reset");
  }
};

FakeSystem System;

namespace PowerDiagnostics {
void flushDiagBatch() { callOrder.push_back("PowerDiagnostics::flushDiagBatch"); }
#!/bin/zsh
# WO-2026-10-02-003 item B - an out-of-memory event must reset the device at
# once, with a cause code that says so.
#
# Behavioral test, following the firmware_update_dwell_test.sh pattern: the REAL
# out-of-memory block is extracted verbatim from `loop()` in
# src/Generalized-Core-Counter.cpp and compiled against a fake Particle /
# RecoveryState / state-machine surface, so the decision is executed rather than
# pattern-matched. The real src/ResetCause.h supplies the cause code.
#
# What it pins:
#   1. with outOfMemory >= 0 the block calls System.reset(RESET_CAUSE_OUT_OF_MEMORY);
#   2. it makes no transition to ERROR_STATE and raises no alert;
#   3. it does not depend on the reset count - it resets at 0, 3 and 200 alike
#      (3 was the threshold at which the old ERROR_STATE route suppressed the
#      reset and bounced back to Idle, leaving the heap exhausted);
#   4. with outOfMemory < 0 nothing happens.
#
# Mutation: a copy of the source with the old ERROR_STATE route restored
# (raiseAlert(14) + transitionTo(ERROR_STATE, "out of memory")) must fail the
# test. The mutation is applied to a copy in TMPDIR; the checked-in source is
# never written to.
set -euo pipefail

repo_root="${0:A:h:h}"
work="${TMPDIR:-/tmp}/oom_immediate_reset_test"
rm -rf "$work"
mkdir -p "$work"

extract_braced_block() {
  local file="$1"
  local marker="$2"
  awk -v marker="$marker" '
    !active && index($0, marker) { active = 1 }
    active {
      print
      opens = gsub(/\{/, "{")
      closes = gsub(/\}/, "}")
      depth += opens - closes
      if (seen_open && depth == 0) exit
      if (opens > 0) seen_open = 1
    }
  ' "$file"
}

# Builds and runs the test against the given copy of the application source.
# Returns the compiled binary's exit status (or 1 if the block is missing or
# the generated program does not compile).
build_and_run() {
  local app_src="$1"
  local tag="$2"
  local generated="$work/${tag}.cpp"
  local binary="$work/${tag}"

  local block
  block=$(extract_braced_block "$app_src" "if (outOfMemory >= 0) {")
  if [[ -z "$block" ]]; then
    echo "EXTRACTION FAILED: no \`if (outOfMemory >= 0) {\` block in $app_src" >&2
    return 1
  fi

  {
    cat <<'CPP'
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "ResetCause.h"   // the REAL cause codes

// ----- Fake Particle / application surface -----------------------------------

enum State { INITIALIZATION_STATE, ERROR_STATE, IDLE_STATE, SLEEPING_STATE };

struct Transition { State target; std::string reason; };
std::vector<Transition> transitions;
void transitionTo(State target, const char *reason) {
  transitions.push_back({target, reason ? reason : ""});
}

std::vector<uint32_t> resets;
unsigned long delaysMs = 0;
void delay(unsigned long ms) { delaysMs += ms; }

struct FakeSystem {
  uint32_t freeHeap = 128;
  void reset(uint32_t data) { resets.push_back(data); }
  uint32_t freeMemory() const { return freeHeap; }
};
FakeSystem System;

struct FakeLog {
  void info(const char *, ...) {}
  void error(const char *, ...) {}
  void warn(const char *, ...) {}
};
FakeLog Log;

// Present so that restoring the removed ERROR_STATE route still compiles - and
// then fails the assertions below.
namespace RecoveryState {
std::vector<int> alerts;
uint8_t resetCount = 0;
inline void raiseAlert(int code) { alerts.push_back(code); }
inline uint8_t get_resetCount() { return resetCount; }
inline int8_t get_alertCode() { return alerts.empty() ? 0 : (int8_t)alerts.back(); }
} // namespace RecoveryState

int outOfMemory = -1;

void oomBlock() {
CPP
    echo "// ===== Extracted verbatim from src/Generalized-Core-Counter.cpp ====="
    echo "$block"
    cat <<'CPP'
}

// ----- Test driver ------------------------------------------------------------

static void resetWorld() {
  transitions.clear();
  resets.clear();
  RecoveryState::alerts.clear();
  delaysMs = 0;
}

static void expectImmediateReset(uint8_t resetCount, int param) {
  resetWorld();
  RecoveryState::resetCount = resetCount;
  outOfMemory = param;
  oomBlock();

  if (resets.size() != 1) {
    std::cerr << "FAIL: out-of-memory with resetCount=" << (int)resetCount
              << " issued " << resets.size() << " reset(s), expected exactly 1"
              << std::endl;
    std::exit(1);
  }
  if (resets[0] != (uint32_t)RESET_CAUSE_OUT_OF_MEMORY) {
    std::cerr << "FAIL: out-of-memory reset carried code " << resets[0]
              << ", expected RESET_CAUSE_OUT_OF_MEMORY ("
              << (uint32_t)RESET_CAUSE_OUT_OF_MEMORY << ")" << std::endl;
    std::exit(1);
  }
  if (!transitions.empty()) {
    std::cerr << "FAIL: out-of-memory transitioned to state "
              << (int)transitions[0].target << " (\"" << transitions[0].reason
              << "\") instead of resetting directly" << std::endl;
    std::exit(1);
  }
  if (!RecoveryState::alerts.empty()) {
    std::cerr << "FAIL: out-of-memory raised alert "
              << RecoveryState::alerts[0]
              << "; the ERROR_STATE route was removed in WO-2026-10-02-003 item B"
              << std::endl;
    std::exit(1);
  }
}

int main() {
  // 1. Resets at once, with code 7.
  expectImmediateReset(0, 0);

  // 2. No dependence on the reset count. 3 was the old suppression threshold:
  //    from there on the ERROR_STATE route returned to Idle and never reset,
  //    and outOfMemory is never cleared, so the device bounced forever.
  expectImmediateReset(3, 1024);
  expectImmediateReset(4, 1024);
  expectImmediateReset(200, 65535);

  // 3. No out-of-memory event: the block does nothing.
  resetWorld();
  RecoveryState::resetCount = 0;
  outOfMemory = -1;
  oomBlock();
  if (!resets.empty() || !transitions.empty() || !RecoveryState::alerts.empty()) {
    std::cerr << "FAIL: the out-of-memory block acted with outOfMemory < 0"
              << std::endl;
    std::exit(1);
  }

  std::cout << "oom_immediate_reset_test: out-of-memory resets immediately with "
               "RESET_CAUSE_OUT_OF_MEMORY, no ERROR_STATE route, no reset-count "
               "dependence" << std::endl;
  return 0;
}
CPP
  } > "$generated"

  if ! clang++ -std=c++17 -Wall -Wextra -I "$repo_root/src" \
      "$generated" -o "$binary" 2>"$work/${tag}.build.log"; then
    return 1
  fi
  "$binary"
}

# --- Part 1: the shipped source must pass ------------------------------------
if ! build_and_run "$repo_root/src/Generalized-Core-Counter.cpp" "shipped"; then
  echo "FAILED: the shipped out-of-memory block does not reset immediately with RESET_CAUSE_OUT_OF_MEMORY" >&2
  [[ -s "$work/shipped.build.log" ]] && cat "$work/shipped.build.log" >&2
  exit 1
fi

# --- Part 2: mutation - restoring the ERROR_STATE route must fail ------------
mutant="$work/Generalized-Core-Counter.mutant.cpp"
cp "$repo_root/src/Generalized-Core-Counter.cpp" "$mutant"
python3 - "$mutant" <<'PY'
import sys

path = sys.argv[1]
text = open(path).read()
original = """  if (outOfMemory >= 0) {
    Log.info("out of memory occurred size=%d", outOfMemory);
    delay(100);
    System.reset(RESET_CAUSE_OUT_OF_MEMORY);
  }
"""
restored = """  if (outOfMemory >= 0) {
    Log.error("Out-of-memory event detected (param=%d freeHeap=%lu) - resetting",
              outOfMemory,
              (unsigned long)System.freeMemory());
    RecoveryState::raiseAlert(14);
    transitionTo(ERROR_STATE, "out of memory");
  }
"""
if original not in text:
    sys.exit("MUTATION SETUP FAILED: the shipped out-of-memory block does not "
             "match the expected text; update tests/oom_immediate_reset_test.sh")
open(path, "w").write(text.replace(original, restored, 1))
PY

if build_and_run "$mutant" "mutant" >/dev/null 2>&1; then
  echo "MUTATION NOT CAUGHT: restoring the ERROR_STATE route (raiseAlert(14) + transitionTo(ERROR_STATE, \"out of memory\")) still passes the test" >&2
  exit 1
fi
echo "Mutation caught: restoring the ERROR_STATE route fails the test"

# The checked-in source was never written to; only copies under $work were.
rm -rf "$work"
echo "oom_immediate_reset_test: full suite passed"
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
stubs
thermal_coupling_structural_test.py
thermal_threshold_migration_test.cpp
thermal_threshold_migration_test.sh
wake_gate_single_owner_structural_test.py
wake_summary_led_test.cpp
wake_summary_led_test.sh
watchdog_ab1805_classification_test.cpp
watchdog_ab1805_classification_test.sh
watchdog_alert_code_test.cpp
watchdog_alert_code_test.sh
webhook_timeout_range_test.py
 */

#include <stdint.h>
#include <time.h>

namespace CurrentReadings {

// *************** Store lifecycle (/usr/current.dat) ***************

/// Forwards to currentStatusData::setup().
void setup();

/// Forwards to currentStatusData::loop() - deferred flush servicing.
void loop();

/**
 * @brief Zero the counts for a new day.
 *
 * @brief The separate /usr/sensor.dat store (sensorConfigData / SensorData).
 *
 * Finding 3: this is a third PersistentDataFile-backed record, not a view of
 * sysStatus. It is nested here - rather than given a fifth facade - because
 * decision B folds sensor settings into SystemConfig's remit, and nesting
 * keeps its own get_sensorType() unambiguously distinct from the
 * sysStatus-level one above.
 */
namespace SensorSettings {

/// Forwards to sensorConfigData::setup().
void setup();

/// Forwards to sensorConfigData::loop().
void loop();

/**
 * @brief Forwards to sensorConfigData::validate() with the same size
 *        argument the pre-split call site passed (sizeof(sensorConfigData)).
 */
bool validateStoredData();

/// /usr/sensor.dat sensor type (1 = PIR, 2 = Analog, 3 = Ultrasonic, ...).
uint8_t get_sensorType();
void set_sensorType(uint8_t value);

uint32_t get_sensorSetting1();
void set_sensorSetting1(uint32_t value);

uint32_t get_sensorSetting2();
void set_sensorSetting2(uint32_t value);

uint32_t get_sensorSetting3();
void set_sensorSetting3(uint32_t value);

uint32_t get_sensorSetting4();
void set_sensorSetting4(uint32_t value);

}  // namespace SensorSettings

}  // namespace SystemConfig
<shellId: 4 completed with exit code 0>
```

</details>


---

<sub>19s</sub>

### Copilot

Now the failsafe, RecoveryState alert store, PublishQueue, and headers.

---

<sub>19s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && sed -n 60,100p src/power/ConnectivityFailsafe.cpp; sed -n 285,330p src/power/ConnectivityFailsafe.cpp; grep -n "set_alertCode\|raiseAlert" -A6 src/MyPersistentData.cpp | head -50; grep -rn "writeQueueToFiles" src lib --include=*.h --include=*.cpp | head; grep -rn "PublishQueuePosix" src | head -20; cat src/state/StateHandlers.h | head -60**

$ cd /private/tmp/gcc-step0/dur && sed -n 60,100p src/power/ConnectivityFailsafe.cpp; sed -n 285,330p src/power/ConnectivityFailsafe.cpp; grep -n "set_alertCode\|raiseAlert" -A6 src/MyPersistentData.cpp | head -50; grep -rn "writeQueueToFiles" src lib --include=*.h --include=*.cpp | head; grep -rn "PublishQueuePosix" src | head -20; cat src/state/StateHandlers.h | head -60

<details>
<summary>187 lines</summary>

```
#if CONNECTIVITY_FAILSAFE_TEST_MODE
  return ConnectivityFailsafeTest::jitterSec(stage);
#else
  if (ConnectivityPolicy::CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC <= 0) {
    return 0;
  }

  String deviceId = System.deviceID();
  uint32_t hash = 5381u;
  for (size_t index = 0; index < deviceId.length(); ++index) {
    hash = ((hash << 5) + hash) ^ (uint8_t)deviceId.charAt(index);
  }
  hash ^= (uint32_t)stage * 2654435761u;
  return hash % (uint32_t)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC;
#endif
}

void persistConnectivityFailsafeState(uint8_t stage, time_t actionTime, bool incrementCount) {
  RecoveryState::set_connectivityRecoveryStage(stage);
  RecoveryState::set_lastConnectivityRecoveryAction(actionTime);
  if (incrementCount) {
    uint8_t count = RecoveryState::get_connectivityRecoveryCount();
    if (count < 0xFF) {
      RecoveryState::set_connectivityRecoveryCount(count + 1);
    }
  }
  SystemConfig::flushNow();
}

const char *failsafeDeferReasonNameLocal(FailsafeDeferReason reason) {
  switch (reason) {
  case FAILSAFE_DEFER_INVALID_TIME:
    return "invalid-time";
  case FAILSAFE_DEFER_NO_LAST_CONNECTION:
    return "no-last-connection";
  case FAILSAFE_DEFER_DISCONNECTED_MODE:
    return "disconnected-mode";
  case FAILSAFE_DEFER_UPDATE_PENDING:
    return "update-pending";
  case FAILSAFE_DEFER_LOW_BATTERY_HARD_STAGE_SUPPRESSED:
    return "low-battery-hard-stage-suppressed";
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
    return;
  }

  if (nextStage == 2) {
    setAppBreadcrumb(BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD);
    persistConnectivityFailsafeState(2, now, true);
    Log.info("Failsafe: stage=2 action=system-reset age=%lds",
             (long)connectionAgeSec);
    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);
    return;
  }

  setAppBreadcrumb(BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD);
  persistConnectivityFailsafeState(3, now, true);
  Log.info("Failsafe: stage=3 action=deep-powerdown age=%lds",
           (long)connectionAgeSec);
  ab1805.deepPowerDown();
}
752:void currentStatusData::set_alertCode(int8_t value) {
753-    setValue<int8_t>(offsetof(CurrentData, alertCode), value);
754-}
755-
756-time_t currentStatusData::get_lastAlertTime() const {
757-    // lastAlertTime is stored as time_t in CurrentData; retrieve with correct type
758-    return getValue<time_t>(offsetof(CurrentData,lastAlertTime));
--
789:        // It needs its own tier, strictly greater than 3, so raiseAlert(19)
790-        // always supersedes whatever is currently active from that set.
791-        case 19: // watchdog reset (Device-OS-detected or AB1805-confirmed PIN_RESET)
792-            return 4; // watchdog / lost-execution - most urgent
793-
794-        case 23: // PMIC battery fault (general)
795-        case 30: // connectivity timeout with radio up
--
810:void currentStatusData::raiseAlert(int8_t value) {
811-    if (value <= 0) {
812-        return; // ignore attempts to "raise" a non-alert here
813-    }
814-
815-    int8_t existing = get_alertCode();
816-    if (getAlertSeverity(value) > getAlertSeverity(existing)) {
817:        set_alertCode(value);
818-        // WO-2026-08-29-002 item 8: deliberately NOT gated on
819-        // isClockTrusted(). State_Report.cpp's alert-40 escalation cooldown
820-        // reads get_lastAlertTime() as `lastEscalation` and treats
821-        // `lastEscalation == 0` as "no prior escalation - cooldown already
822-        // passed" (a control-flow branch, not just a recorded value). If an
823-        // alert were raised while the clock is untrusted and this wrote 0,
--
903:// cross-store reset in resetEverything(), raiseAlert()'s severity arbitration -
904-// that behavior stays in the class method and is not reimplemented here.
905-
906-// The facades publish these capacities so ConfigApply.cpp can size its staging
907-// buffers without seeing SysData. Binding them to the real fields here means a
908-// future field-width change breaks the build instead of silently truncating.
909-static_assert(SystemConfig::kTimeZoneCapacity == sizeof(sysStatusData::SysData::timeZoneStr),
--
1069:void set_alertCode(int8_t value) { current.set_alertCode(value); }
1070:void raiseAlert(int8_t value) { current.raiseAlert(value); }
1071-
1072-time_t get_lastAlertTime() { return current.get_lastAlertTime(); }
1073-void set_lastAlertTime(time_t value) { current.set_lastAlertTime(value); }
1074-
1075-}  // namespace RecoveryState
1076-
lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:89:            writeQueueToFiles();
lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:121:void PublishQueuePosix::writeQueueToFiles() {
lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:143:                _log.trace("writeQueueToFiles fileNum=%d", fileNum);
lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:230:            writeQueueToFiles();
lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:385:            writeQueueToFiles();
lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:405:        PublishQueuePosix::instance().writeQueueToFiles();
lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:234:    void writeQueueToFiles();
src/power/PowerDiagnostics.cpp:11:#include "PublishQueuePosixRK.h"
src/power/PowerDiagnostics.cpp:21:// leave byte headroom under PublishQueuePosixRK's ~622-byte event ceiling.
src/power/PowerDiagnostics.cpp:73:  // PublishQueuePosixRK's fixed 1000ms drain pacing, which otherwise makes
src/power/PowerDiagnostics.cpp:394:  PublishQueuePosix::instance().publish("pdiag", payload, PRIVATE | WITH_ACK);
src/sensors/SensorManager.cpp:26:#include "PublishQueuePosixRK.h"
src/BuildProfile.h:232: * compact "pdiag" JSON event and queued via PublishQueuePosix, which persists
src/observability/LoopForensics.cpp:2:#include "PublishQueuePosixRK.h"
src/observability/LoopForensics.cpp:190:                        (uint16_t)PublishQueuePosix::instance().getNumEvents());
src/observability/LoopForensics.cpp:245:      ? (uint16_t)PublishQueuePosix::instance().getNumEvents()
src/state/State_Connect.cpp:7:#include "PublishQueuePosixRK.h"
src/state/State_Connect.cpp:296:      const uint16_t pending = (uint16_t)PublishQueuePosix::instance().getNumEvents();
src/state/State_Connect.cpp:419:      const uint16_t queueDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
src/state/State_Connect.cpp:462:      const uint16_t summaryQueueDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
src/state/State_Connect.cpp:498:          (uint16_t)PublishQueuePosix::instance().getNumEvents(),
src/state/State_Connect.cpp:566:      size_t pending = PublishQueuePosix::instance().getNumEvents();
src/state/State_Connect.cpp:621:    const uint16_t summaryQueueDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
src/state/State_Sleep.cpp:17:#include "PublishQueuePosixRK.h"
src/state/State_Sleep.cpp:142:    bool queueEmpty = PublishQueuePosix::instance().getCanSleep();
src/state/State_Sleep.cpp:152:    uint16_t queueDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
src/state/State_Sleep.cpp:296:                 gateWaitMs, (unsigned)PublishQueuePosix::instance().getNumEvents());
#pragma once

#include "state/StateMachine.h"

/**
 * @file StateHandlers.h
 * @brief Public entry points for the main-loop state dispatch.
 */

/**
 * @brief Handles the IDLE_STATE wake-time decision and report scheduling logic.
 */
void handleIdleState();

/**
 * @brief Handles transition into low-power sleep and associated teardown work.
 */
void handleSleepingState();

/**
 * @brief Handles report preparation, queueing, and report-trigger decisions.
 */
void handleReportingState();

/**
 * @brief Handles network and cloud connection attempts plus the service window.
 */
void handleConnectingState();

/**
 * @brief Handles the bounded stay-awake window used for firmware updates.
 */
void handleFirmwareUpdateState();

/**
 * @brief Handles error-state containment and recovery backoff behavior.
 */
void handleErrorState();

/**
 * @brief Handles sensor processing when the device is configured for counting mode.
 */
void handleCountingMode();

/**
 * @brief Handles sensor processing when the device is configured for occupancy mode.
 */
void handleOccupancyMode();

/**
 * @brief Processes a sensor event that woke the device from sleep (LED-timeout session close, PIR-wake count or occupancy start); returns true when it has transitioned.
 */
bool processSleepWakeEvent(bool pirWake);

/**
 * @brief Advances occupancy debounce and occupied or unoccupied transitions.
 */
void updateOccupancyState();
<shellId: 5 completed with exit code 0>
```

</details>


---

<sub>26s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && grep -n "persistConnectivityFailsafeState\|^#include\|flushNow" src/power/ConnectivityFailsafe.cpp; grep -n "^#include" src/ThrashGuard.cpp src/state/State_Error.cpp src/state/State_Sleep.cpp src/state/StateMachine.h; sed -n 225,236p lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h; sed -n 115,125p lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp; grep -rn "persistConnectivityFailsafeState\|flushNow" tests | head -20; grep -n "ThrashGuard\|State_Error\|State_Sleep\|ConnectivityFailsafe" tests/*.sh tests/README.md | grep -i "include\|-I\|\.cpp" | head -20**

$ cd /private/tmp/gcc-step0/dur && grep -n "persistConnectivityFailsafeState\|^#include\|flushNow" src/power/ConnectivityFailsafe.cpp; grep -n "^#include" src/ThrashGuard.cpp src/state/State_Error.cpp src/state/State_Sleep.cpp src/state/StateMachine.h; sed -n 225,236p lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h; sed -n 115,125p lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp; grep -rn "persistConnectivityFailsafeState\|flushNow" tests | head -20; grep -n "ThrashGuard\|State_Error\|State_Sleep\|ConnectivityFailsafe" tests/*.sh tests/README.md | grep -i "include\|-I\|\.cpp" | head -20

<details>
<summary>116 lines</summary>

```
1:#include "power/ConnectivityFailsafe.h"
2:#include "state/StateMachine.h"
3:#include "power/ConnectivityPolicy.h"
4:#include "power/PowerPlatform.h"
5:#include "power/BatteryAuthority.h"
6:#include "diagnostics/ConnectivityFailsafeTest.h"
7:#include "observability/LoopForensics.h"
8:#include "reporting/ReportingPolicy.h"
9:#include "time/DailyBoundary.h"
10:#include "persist/PowerConfig.h"
11:#include "ResetCause.h"
77:void persistConnectivityFailsafeState(uint8_t stage, time_t actionTime, bool incrementCount) {
86:  SystemConfig::flushNow();
149:  SystemConfig::flushNow();
297:    persistConnectivityFailsafeState(currentStage, now, false);
303:    persistConnectivityFailsafeState(2, now, true);
311:  persistConnectivityFailsafeState(3, now, true);
src/ThrashGuard.cpp:1:#include "ThrashGuard.h"
src/ThrashGuard.cpp:2:#include "ResetCause.h"
src/ThrashGuard.cpp:3:#include "state/StateMachine.h"
src/ThrashGuard.cpp:4:#include "power/Connectivity.h"
src/ThrashGuard.cpp:5:#include "persist/RecoveryState.h"
src/state/State_Error.cpp:1:#include "state/State_Common.h"
src/state/State_Error.cpp:2:#include "ResetCause.h"
src/state/State_Error.cpp:3:#include "../Config.h"
src/state/State_Error.cpp:4:#include "cloud/Cloud.h"
src/state/State_Error.cpp:5:#include "LocalTimeRK.h"
src/state/State_Error.cpp:6:#include "persist/RecoveryState.h"
src/state/State_Error.cpp:7:#include "persist/SystemConfig.h"
src/state/State_Error.cpp:8:#include "power/PowerManager.h"
src/state/State_Error.cpp:9:#include "PublishQueuePosixRK.h"
src/state/State_Error.cpp:10:#include "sensors/SensorManager.h"
src/state/State_Error.cpp:11:#include "device_pinout.h"
src/state/State_Error.cpp:12:#include "sensors/SensorDefinitions.h"
src/state/State_Error.cpp:13:#include "AB1805_RK.h"
src/state/State_Error.cpp:14:#include "power/Connectivity.h"
src/state/State_Error.cpp:15:#include "time/Clock.h"
src/state/State_Sleep.cpp:1:#include "BuildProfile.h"
src/state/State_Sleep.cpp:2:#include "ResetCause.h"
src/state/State_Sleep.cpp:3:#include "state/State_Common.h"
src/state/State_Sleep.cpp:4:#include "../Config.h"
src/state/State_Sleep.cpp:5:#include "cloud/Cloud.h"
src/state/State_Sleep.cpp:6:#include "power/Connectivity.h"
src/state/State_Sleep.cpp:7:#include "power/ConnectivityPolicy.h"
src/state/State_Sleep.cpp:8:#include "power/ModemHealth.h"
src/state/State_Sleep.cpp:9:#include "power/PowerDiagnostics.h"
src/state/State_Sleep.cpp:10:#include "power/PowerManager.h"
src/state/State_Sleep.cpp:11:#include "power/BatteryAuthority.h"
src/state/State_Sleep.cpp:12:#include "LocalTimeRK.h"
src/state/State_Sleep.cpp:13:#include "persist/CurrentReadings.h"
src/state/State_Sleep.cpp:14:#include "persist/PowerConfig.h"
src/state/State_Sleep.cpp:15:#include "persist/RecoveryState.h"
src/state/State_Sleep.cpp:16:#include "persist/SystemConfig.h"
src/state/State_Sleep.cpp:17:#include "PublishQueuePosixRK.h"
src/state/State_Sleep.cpp:18:#include "sensors/SensorManager.h"
src/state/State_Sleep.cpp:19:#include "device_pinout.h"
src/state/State_Sleep.cpp:20:#include "sensors/SensorDefinitions.h"
src/state/State_Sleep.cpp:21:#include "AB1805_RK.h"
src/state/State_Sleep.cpp:22:#include "observability/AwakeCycleCounters.h"
src/state/State_Sleep.cpp:23:#include "observability/WakeCycleStats.h"
src/state/State_Sleep.cpp:24:#include "ThrashGuard.h"
src/state/State_Sleep.cpp:25:#include "state/SleepPrepSpanTiming.h"
src/state/State_Sleep.cpp:26:#include "state/SleepTeardown.h"
src/state/State_Sleep.cpp:27:#include "time/HibernateCycle.h"
src/state/State_Sleep.cpp:28:#include "time/HibernateEligibility.h"
src/state/State_Sleep.cpp:29:#include "time/Clock.h"
src/state/State_Sleep.cpp:30:#include "time/DailyBoundary.h"
src/state/StateMachine.h:3:#include "Particle.h"
src/state/StateMachine.h:4:#include "AB1805_RK.h"
	 *
	 * This function almost always returns true. If you queue more events than fit in the buffer the
	 * oldest (sometimes second oldest) is discarded.
	 */
	virtual bool publishCommon(const char *eventName, const char *data, int ttl, PublishFlags flags1, PublishFlags flags2 = PublishFlags());

    /**
     * @brief If there are events in the RAM queue, write them to files in the flash file system
     */
    void writeQueueToFiles();

    /**
        strcpy(event->eventName, eventName);
        strcpy(event->eventData, eventData);
    }
    return event;
}

void PublishQueuePosix::writeQueueToFiles() {

    WITH_LOCK(*this) {
        while(!ramQueue.empty()) {
            PublishQueueEvent *event = ramQueue.front();
tests/connectivity_failsafe_unknown_openness_test.cpp:111:void persistConnectivityFailsafeState(uint8_t stage, time_t actionTime, bool) {
tests/persistence_facade_behavior_test.cpp:197:  // flushNow() is the forced path, and it too must touch only sysStatus.
tests/persistence_facade_behavior_test.cpp:201:  SystemConfig::flushNow();
tests/persistence_facade_behavior_test.cpp:203:        "SystemConfig::flushNow() FORCE-flushes sysStatus.dat and nothing else");
tests/persistence_facade_behavior_test.cpp:301:  std::cout << "OK: each facade's loop() services its OWN store; flushNow() stays forced\n";
tests/connectivity_failsafe_stage1_retired_test.py:69:    check("persistConnectivityFailsafeState(1," not in supervisor,
tests/connectivity_failsafe_move_structural_test.py:17:    "persistConnectivityFailsafeState", "failsafeDeferReasonNameLocal",
tests/alert19_clears_after_report_test.sh:138:// ThrashGuard raises 18 and resets at once (ThrashGuard.cpp:150-152), before
tests/awake_cycle_counters_test.sh:11:# Part 2 is structural: every System.sleep() call in State_Sleep.cpp must be
tests/awake_cycle_counters_test.sh:17:sleep_src="$repo_root/src/state/State_Sleep.cpp"
tests/awake_cycle_counters_test.sh:155:  fail "State_Sleep.cpp must include observability/AwakeCycleCounters.h"
tests/config_window_hold_test.sh:11:# Part 2 checks the structure of the CONNECTED+open abort in State_Sleep.cpp
tests/config_window_hold_test.sh:16:# Usage: config_window_hold_test.sh [LedgerClient.cpp [State_Sleep.cpp]]
tests/config_window_hold_test.sh:22:sleep_src="${2:-$repo_root/src/state/State_Sleep.cpp}"
tests/config_window_hold_test.sh:107:  cp "$repo_root/src/state/State_Sleep.cpp" "$dir/State_Sleep.cpp"
tests/config_window_hold_test.sh:109:  [[ "$which" == sleep ]] && target="$dir/State_Sleep.cpp"
tests/config_window_hold_test.sh:120:  if "$script" "$dir/LedgerClient.cpp" "$dir/State_Sleep.cpp" >"$dir/out" 2>"$dir/err"; then
tests/connection_mode_downgrade_test.sh:173:structural_mutation "failsafe reads the raw flag" "power/ConnectivityFailsafe.cpp" \
tests/connection_mode_downgrade_test.sh:176:structural_mutation "diagnostic failsafe mirror reads the raw flag" "diagnostics/ConnectivityFailsafeTest.cpp" \
tests/connection_mode_downgrade_test.sh:182:structural_mutation "post-wake raw-flag gate dropped (State_Sleep.cpp)" "state/State_Sleep.cpp" \
tests/connectivity_failsafe_open_hours_test.sh:18:app_src="$repo_root/src/power/ConnectivityFailsafe.cpp"
tests/connectivity_failsafe_open_hours_test.sh:155:if ! grep -q 'clearConnectivityFailsafeRecovery("cloud-ok")' "$repo_root/src/state/State_Connect.cpp"; then
tests/connectivity_failsafe_open_hours_test.sh:172:failsafe_test_src="$repo_root/src/diagnostics/ConnectivityFailsafeTest.cpp"
tests/connectivity_failsafe_open_hours_test.sh:177:    fail "the test-mode prediction in ConnectivityFailsafeTest.cpp must also use: $needle"
tests/connectivity_failsafe_unknown_openness_test.sh:8:# src/power/ConnectivityFailsafe.cpp, compared with the source (COPY_MISMATCH on
tests/connectivity_failsafe_unknown_openness_test.sh:16:src="$repo_root/src/power/ConnectivityFailsafe.cpp"
tests/failsafe_cadence_rule_test.sh:36:path = src_root + "/power/ConnectivityFailsafe.cpp"
<shellId: 6 completed with exit code 0>
```

</details>


---

<sub>29s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && sed -n 138,152p src/power/ConnectivityFailsafe.cpp; grep -n "persistConnectivityFailsafeState\|flushNow" tests/connectivity_failsafe_unknown_openness_test.cpp tests/connectivity_failsafe_unknown_openness_test.sh | head; sed -n 1,50p tests/README.md; ls tools; grep -n "State_Error\|state/State" src/state/State_Common.h | head -3; grep -n "handleErrorState" src/state/*.h src/*.cpp | head**

$ cd /private/tmp/gcc-step0/dur && sed -n 138,152p src/power/ConnectivityFailsafe.cpp; grep -n "persistConnectivityFailsafeState\|flushNow" tests/connectivity_failsafe_unknown_openness_test.cpp tests/connectivity_failsafe_unknown_openness_test.sh | head; sed -n 1,50p tests/README.md; ls tools; grep -n "State_Error\|state/State" src/state/State_Common.h | head -3; grep -n "handleErrorState" src/state/*.h src/*.cpp | head

<details>
<summary>61 lines</summary>

````
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
tests/connectivity_failsafe_unknown_openness_test.cpp:111:void persistConnectivityFailsafeState(uint8_t stage, time_t actionTime, bool) {
# Host regression tests

The reporting-policy calculation is isolated from Particle Device OS so its
battery tiers, effective intervals, boundary alignment, and open-window filter
can be checked with the host compiler:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/reporting_policy_test.cpp src/reporting/ReportingPolicy.cpp \
  -o /tmp/reporting_policy_test
/tmp/reporting_policy_test
```

The runtime adapter itself (`ReportingPolicyResolver::resolveRuntime()` in
`src/reporting/RuntimeReportingPolicy.cpp`) is compiled and exercised directly
against a stub Particle/SensorManager surface - see
`tests/reporting_policy_adapter_test.sh` (WO-2026-08-25-001 Amendment C,
Decision C2 / AC-C6). This closes the gap where the guard's unit tests passed
in isolation while the production adapter still let an Invalid or Unavailable
vcell silently bypass the trust substitution and the 3.5V floor. The normal
Particle firmware compile remains the final integration check.

The boot-scoped startup snapshot has a separate host test:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/startup_snapshot_test.cpp src/observability/StartupSnapshot.cpp \
  -o /tmp/startup_snapshot_test
/tmp/startup_snapshot_test
```

## WO-2026-08-10-001 (watchdog reset instrumentation) host tests

The AB1805 wake-reason classification, watchdog alert code, sleep-entry
breadcrumb sequence, and item-4 serial-settle changes added by
`docs/work-orders/WO-2026-08-10-001-watchdog-instrumentation.md` cannot be
compiled standalone (they sit inside `Generalized-Core-Counter.cpp` and
`State_Sleep.cpp`, both of which pull in heavy Particle/AB1805/PublishQueue
dependencies). Each of the following tests either mirrors the relevant
decision logic in a dependency-free host function (cross-checked against the
real source via `grep`/`awk` so the mirror can't silently drift) or parses
the actual shipped source directly:

```sh
zsh tests/watchdog_ab1805_classification_test.sh   # item 1: AB1805 wake-reason classification
zsh tests/watchdog_alert_code_test.sh              # item 3: new alert code severity/auto-clear (payload bytes: report_payload_golden_test.sh)
zsh tests/report_payload_golden_test.sh             # WO-2026-10-14-001: golden webhook payload bytes, event name and flags (175 vectors)
python3 tests/sleep_breadcrumb_sequence_test.py    # item 2: sleep-entry breadcrumb ordering (traces real source)
python3 tests/serial_settle_test.py                # item 4: always-on serial-settle placement/gating (traces real source)
```
exerciser_baseline.py
11:#include "state/StateHandlers.h"
12:#include "state/StateMachine.h"
src/state/StateHandlers.h:38:void handleErrorState();
src/Generalized-Core-Counter.cpp:1157:    handleErrorState();
<shellId: 7 completed with exit code 0>
````

</details>


---

<sub>42s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && grep -n "include\|^void\|^int " src/state/StateMachine.h | head -20; grep -n "include" src/state/State_Common.h; grep -n "flushNow" -B3 -A3 src/persist/SystemConfig.h | head -20; grep -n "thrashResetCount" src/*.cpp src/*.h | head -4**

$ cd /private/tmp/gcc-step0/dur && grep -n "include\|^void\|^int " src/state/StateMachine.h | head -20; grep -n "include" src/state/State_Common.h; grep -n "flushNow" -B3 -A3 src/persist/SystemConfig.h | head -20; grep -n "thrashResetCount" src/*.cpp src/*.h | head -4

<details>
<summary>33 lines</summary>

```
3:#include "Particle.h"
4:#include "AB1805_RK.h"
40:void userSwitchISR();
83:void serviceAwakeWatchdog();
90:void pauseAwakeWatchdogForSleep(const char *context);
97:void restoreAwakeWatchdogAfterWake(const char *context);
111:int secondsUntilNextOpen();
116:void publishStateTransition();
124:void transitionTo(State newState, const char *reason);
129:void dailyCleanup();
134:void publishData(time_t stampOverride);
3:#include <algorithm>
5:#include "../Config.h"
6:#include "Particle.h"
7:#include "persist/CurrentReadings.h"
8:#include "persist/RecoveryState.h"
9:#include "persist/SystemConfig.h"
10:#include "power/PowerManager.h"
11:#include "state/StateHandlers.h"
12:#include "state/StateMachine.h"
13:#include "time/Clock.h"
14:#include "time/DailyBoundary.h"
89-void loop();
90-
91-/// Forwards to sysStatus.flush(true) - synchronous forced write.
92:void flushNow();
93-
94-/**
95- * @brief Forwards to sysStatusData::validate() with the same size argument
src/ThrashGuard.cpp:29:retained uint16_t thrashResetCount = 0;
src/ThrashGuard.cpp:151:  thrashResetCount++;
src/ThrashGuard.h:77:extern retained uint16_t thrashResetCount;
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>44s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && sed -n 118,126p src/state/StateMachine.h; grep -n "^void loop" -B2 -A2 src/persist/CurrentReadings.h; grep -n "^void loop();" -B2 -A2 src/persist/SystemConfig.h | sed -n 8,20p; grep -n "^void handleErrorState" src/state/State_Error.cpp**

$ cd /private/tmp/gcc-step0/dur && sed -n 118,126p src/state/StateMachine.h; grep -n "^void loop" -B2 -A2 src/persist/CurrentReadings.h; grep -n "^void loop();" -B2 -A2 src/persist/SystemConfig.h | sed -n 8,20p; grep -n "^void handleErrorState" src/state/State_Error.cpp

<details>
<summary>19 lines</summary>

```
/**
 * @brief Requests a top-level state transition with a concise log breadcrumb.
 *
 * @param newState Target state for the next main-loop dispatch
 * @param reason Short reason label for field logs
 */
void transitionTo(State newState, const char *reason);

/**
26-
27-/// Forwards to currentStatusData::loop() - deferred flush servicing.
28:void loop();
29-
30-/**
228-/// Forwards to sensorConfigData::loop().
229:void loop();
230-
231-/**
115:void handleErrorState() {
<shellId: 9 completed with exit code 0>
```

</details>


---

<sub>53s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 - <<'E'
import re
def sub(p,a,b):
    t=open(p).read(); assert t.count(a)==1,(p,a); open(p,'w').write(t.replace(a,b))
sub('src/persist/CurrentReadings.h',"void loop();\n","void loop();\nvoid flushNow();  // forwards to flush(true): synchronous forced write\n")
sub('src/persist/SystemConfig.h',"/// Forwards to sensorConfigData::loop().\nvoid loop();\n","/// Forwards to sensorConfigData::loop().\nvoid loop();\nvoid flushNow();  // forwards to flush(true): synchronous forced write\n")
sub('src/MyPersistentData.cpp',"void loop() { sensorConfig.loop(); }\n","void loop() { sensorConfig.loop(); }\nvoid flushNow() { sensorConfig.flush(true); }\n")
sub('src/MyPersistentData.cpp',"void loop() { current.loop(); }\n","void loop() { current.loop(); }\nvoid flushNow() { current.flush(true); }\n")
sub('src/state/StateMachine.h',"void transitionTo(State newState, const char *reason);\n","void transitionTo(State newState, const char *reason);\n\n/// Forces current, sysStatus, sensor stores and the publish queue to flash; call just before a deliberate reset.\nvoid flushAllBeforeReset();\n")
sub('src/state/State_Error.cpp',"void handleErrorState() {","""void flushAllBeforeReset() {
  CurrentReadings::flushNow();
  SystemConfig::flushNow();
  SystemConfig::SensorSettings::flushNow();
  PublishQueuePosix::instance().writeQueueToFiles();
}

void handleErrorState() {""")
sub('src/state/State_Error.cpp',"      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);","      flushAllBeforeReset();\n      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);")
sub('src/state/State_Error.cpp',"      ab1805.deepPowerDown();","      flushAllBeforeReset();\n      ab1805.deepPowerDown();")
sub('src/ThrashGuard.cpp',"  System.reset(RESET_CAUSE_THRASH_TIER3);","  flushAllBeforeReset();\n  System.reset(RESET_CAUSE_THRASH_TIER3);")
sub('src/state/State_Sleep.cpp',"      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);","      flushAllBeforeReset();\n      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);")
sub('src/state/State_Sleep.cpp',"        System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);","        flushAllBeforeReset();\n        System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);")
sub('src/power/ConnectivityFailsafe.cpp',"""    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);""","""    flushAllBeforeReset();
    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);""")
sub('src/power/ConnectivityFailsafe.cpp',"  ab1805.deepPowerDown();\n}","  flushAllBeforeReset();\n  ab1805.deepPowerDown();\n}")
sub('src/power/ConnectivityFailsafe.cpp',"  SystemConfig::flushNow();\n}\n\nconst char *failsafeDefer","}\n\nconst char *failsafeDefer")
sub('src/power/ConnectivityFailsafe.cpp',"    persistConnectivityFailsafeState(currentStage, now, false);\n    return;","    persistConnectivityFailsafeState(currentStage, now, false);\n    SystemConfig::flushNow();\n    return;")
E
git diff --stat; git diff --numstat -- src**

$ cd /private/tmp/gcc-step0/dur && python3 - <<'E'
import re
def sub(p,a,b):
    t=open(p).read(); assert t.count(a)==1,(p,a); open(p,'w').write(t.replace(a,b))
sub('src/persist/CurrentReadings.h',"void loop();\n","void loop();\nvoid flushNow();  // forwards to flush(true): synchronous forced write\n")
sub('src/persist/SystemConfig.h',"/// Forwards to sensorConfigData::loop().\nvoid loop();\n","/// Forwards to sensorConfigData::loop().\nvoid loop();\nvoid flushNow();  // forwards to flush(true): synchronous forced write\n")
sub('src/MyPersistentData.cpp',"void loop() { sensorConfig.loop(); }\n","void loop() { sensorConfig.loop(); }\nvoid flushNow() { sensorConfig.flush(true); }\n")
sub('src/MyPersistentData.cpp',"void loop() { current.loop(); }\n","void loop() { current.loop(); }\nvoid flushNow() { current.flush(true); }\n")
sub('src/state/StateMachine.h',"void transitionTo(State newState, const char *reason);\n","void transitionTo(State newState, const char *reason);\n\n/// Forces current, sysStatus, sensor stores and the publish queue to flash; call just before a deliberate reset.\nvoid flushAllBeforeReset();\n")
sub('src/state/State_Error.cpp',"void handleErrorState() {","""void flushAllBeforeReset() {
  CurrentReadings::flushNow();
  SystemConfig::flushNow();
  SystemConfig::SensorSettings::flushNow();
  PublishQueuePosix::instance().writeQueueToFiles();
}

void handleErrorState() {""")
sub('src/state/State_Error.cpp',"      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);","      flushAllBeforeReset();\n      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);")
sub('src/state/State_Error.cpp',"      ab1805.deepPowerDown();","      flushAllBeforeReset();\n      ab1805.deepPowerDown();")
sub('src/ThrashGuard.cpp',"  System.reset(RESET_CAUSE_THRASH_TIER3);","  flushAllBeforeReset();\n  System.reset(RESET_CAUSE_THRASH_TIER3);")
sub('src/state/State_Sleep.cpp',"      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);","      flushAllBeforeReset();\n      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);")
sub('src/state/State_Sleep.cpp',"        System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);","        flushAllBeforeReset();\n        System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);")
sub('src/power/ConnectivityFailsafe.cpp',"""    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);""","""    flushAllBeforeReset();
    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);""")
sub('src/power/ConnectivityFailsafe.cpp',"  ab1805.deepPowerDown();\n}","  flushAllBeforeReset();\n  ab1805.deepPowerDown();\n}")
sub('src/power/ConnectivityFailsafe.cpp',"  SystemConfig::flushNow();\n}\n\nconst char *failsafeDefer","}\n\nconst char *failsafeDefer")
sub('src/power/ConnectivityFailsafe.cpp',"    persistConnectivityFailsafeState(currentStage, now, false);\n    return;","    persistConnectivityFailsafeState(currentStage, now, false);\n    SystemConfig::flushNow();\n    return;")
E
git diff --stat; git diff --numstat -- src

<details>
<summary>18 lines</summary>

```
src/MyPersistentData.cpp           | 2 ++
 src/ThrashGuard.cpp                | 1 +
 src/persist/CurrentReadings.h      | 1 +
 src/persist/SystemConfig.h         | 1 +
 src/power/ConnectivityFailsafe.cpp | 4 +++-
 src/state/StateMachine.h           | 3 +++
 src/state/State_Error.cpp          | 9 +++++++++
 src/state/State_Sleep.cpp          | 2 ++
 8 files changed, 22 insertions(+), 1 deletion(-)
2	0	src/MyPersistentData.cpp
1	0	src/ThrashGuard.cpp
1	0	src/persist/CurrentReadings.h
1	0	src/persist/SystemConfig.h
3	1	src/power/ConnectivityFailsafe.cpp
3	0	src/state/StateMachine.h
9	0	src/state/State_Error.cpp
2	0	src/state/State_Sleep.cpp
<shellId: 10 completed with exit code 0>
```

</details>


---

<sub>1m 5s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && grep -rn "N/N\|run_all\|for t in tests" tools tests/README.md AI_DEVELOPMENT_WORKFLOW.md 2>/dev/null | head; ls build-tmp | head; ls tests/stubs | head -30**

$ cd /private/tmp/gcc-step0/dur && grep -rn "N/N\|run_all\|for t in tests" tools tests/README.md AI_DEVELOPMENT_WORKFLOW.md 2>/dev/null | head; ls build-tmp | head; ls tests/stubs | head -30

<details>
<summary>21 lines</summary>

```
copilot.out
Config.h
MyPersistentData.h
Particle.h
clock_status_republish_overrides
connection_mode_overrides
diag_overrides
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
<shellId: 11 completed with exit code 0>
```

</details>


---

<sub>7m 4s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && mkdir -p build-tmp/wo-2026-11-03-005-stage6 && cat > build-tmp/wo-2026-11-03-005-stage6/runall.sh <<'E'
#!/bin/zsh
cd /private/tmp/gcc-step0/dur
pass=0; fail=0; failed=()
for t in tests/*_test.sh tests/*_test.py; do
  if [[ $t == *.sh ]]; then zsh $t >/dev/null 2>&1; else python3 $t >/dev/null 2>&1; fi
  if [[ $? == 0 ]]; then pass=$((pass+1)); else fail=$((fail+1)); failed+=($t); fi
done
echo "pass=$pass fail=$fail"; printf '%s\n' $failed
E
zsh build-tmp/wo-2026-11-03-005-stage6/runall.sh**

$ cd /private/tmp/gcc-step0/dur && mkdir -p build-tmp/wo-2026-11-03-005-stage6 && cat > build-tmp/wo-2026-11-03-005-stage6/runall.sh <<'E'
#!/bin/zsh
cd /private/tmp/gcc-step0/dur
pass=0; fail=0; failed=()
for t in tests/*_test.sh tests/*_test.py; do
  if [[ $t == *.sh ]]; then zsh $t >/dev/null 2>&1; else python3 $t >/dev/null 2>&1; fi
  if [[ $? == 0 ]]; then pass=$((pass+1)); else fail=$((fail+1)); failed+=($t); fi
done
echo "pass=$pass fail=$fail"; printf '%s\n' $failed
E
zsh build-tmp/wo-2026-11-03-005-stage6/runall.sh

<details>
<summary>11 lines</summary>

```
pass=98 fail=9
tests/connectivity_failsafe_unknown_openness_test.sh
tests/nightly_heap_guard_flush_test.sh
tests/sleep_execution_equivalence_test.sh
tests/sleep_plan_equivalence_test.sh
tests/sleep_cloud_gate_structural_test.py
tests/sleep_execution_structural_test.py
tests/sleep_plan_structural_test.py
tests/sleep_preflight_structural_test.py
tests/sleep_wake_structural_test.py
<shellId: 12 completed with exit code 0>
```

</details>


---

<sub>7m 9s</sub>

### `bash` — Failed

**cd /private/tmp/gcc-step0/dur && git worktree add -q --detach build-tmp/wo-2026-11-03-005-stage6/base HEAD && cd build-tmp/wo-2026-11-03-005-stage6/base && for t in tests/connectivity_failsafe_unknown_openness_test.sh tests/nightly_heap_guard_flush_test.sh tests/sleep_execution_equivalence_test.sh tests/sleep_plan_equivalence_test.sh tests/sleep_cloud_gate_structural_test.py tests/sleep_execution_structural_test.py tests/sleep_plan_structural_test.py tests/sleep_preflight_structural_test.py tests/sleep_wake_structural_test.py; do if [[ $t == *.sh ]]; then zsh $t >/dev/null 2>&1; else python3 $t >/dev/null 2>&1; fi; echo "$? $t"; done**

$ cd /private/tmp/gcc-step0/dur && git worktree add -q --detach build-tmp/wo-2026-11-03-005-stage6/base HEAD && cd build-tmp/wo-2026-11-03-005-stage6/base && for t in tests/connectivity_failsafe_unknown_openness_test.sh tests/nightly_heap_guard_flush_test.sh tests/sleep_execution_equivalence_test.sh tests/sleep_plan_equivalence_test.sh tests/sleep_cloud_gate_structural_test.py tests/sleep_execution_structural_test.py tests/sleep_plan_structural_test.py tests/sleep_preflight_structural_test.py tests/sleep_wake_structural_test.py; do if [[ $t == *.sh ]]; then zsh $t >/dev/null 2>&1; else python3 $t >/dev/null 2>&1; fi; echo "$? $t"; done

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>8m 3s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && B=build-tmp/wo-2026-11-03-005-stage6/base && mkdir -p $B && git archive HEAD src tests lib | tar -x -C $B && cd $B && for t in tests/connectivity_failsafe_unknown_openness_test.sh tests/nightly_heap_guard_flush_test.sh tests/sleep_execution_equivalence_test.sh tests/sleep_plan_equivalence_test.sh tests/sleep_cloud_gate_structural_test.py tests/sleep_execution_structural_test.py tests/sleep_plan_structural_test.py tests/sleep_preflight_structural_test.py tests/sleep_wake_structural_test.py; do if [[ $t == *.sh ]]; then zsh $t >/dev/null 2>&1; else python3 $t >/dev/null 2>&1; fi; echo "$? $t"; done**

$ cd /private/tmp/gcc-step0/dur && B=build-tmp/wo-2026-11-03-005-stage6/base && mkdir -p $B && git archive HEAD src tests lib | tar -x -C $B && cd $B && for t in tests/connectivity_failsafe_unknown_openness_test.sh tests/nightly_heap_guard_flush_test.sh tests/sleep_execution_equivalence_test.sh tests/sleep_plan_equivalence_test.sh tests/sleep_cloud_gate_structural_test.py tests/sleep_execution_structural_test.py tests/sleep_plan_structural_test.py tests/sleep_preflight_structural_test.py tests/sleep_wake_structural_test.py; do if [[ $t == *.sh ]]; then zsh $t >/dev/null 2>&1; else python3 $t >/dev/null 2>&1; fi; echo "$? $t"; done

<details>
<summary>10 lines</summary>

```
0 tests/connectivity_failsafe_unknown_openness_test.sh
0 tests/nightly_heap_guard_flush_test.sh
0 tests/sleep_execution_equivalence_test.sh
0 tests/sleep_plan_equivalence_test.sh
1 tests/sleep_cloud_gate_structural_test.py
0 tests/sleep_execution_structural_test.py
1 tests/sleep_plan_structural_test.py
1 tests/sleep_preflight_structural_test.py
1 tests/sleep_wake_structural_test.py
<shellId: 14 completed with exit code 0>
```

</details>


---

<sub>8m 22s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && for t in tests/sleep_cloud_gate_structural_test.py tests/sleep_execution_structural_test.py tests/sleep_plan_structural_test.py tests/sleep_preflight_structural_test.py tests/sleep_wake_structural_test.py; do echo "== $t"; python3 $t 2>&1 | tail -4; done; zsh tests/connectivity_failsafe_unknown_openness_test.sh 2>&1 | tail -5; zsh tests/nightly_heap_guard_flush_test.sh 2>&1 | tail -5; zsh tests/sleep_plan_equivalence_test.sh 2>&1 | tail -5;zsh tests/sleep_execution_equivalence_test.sh 2>&1 | tail -5**

$ cd /private/tmp/gcc-step0/dur && for t in tests/sleep_cloud_gate_structural_test.py tests/sleep_execution_structural_test.py tests/sleep_plan_structural_test.py tests/sleep_preflight_structural_test.py tests/sleep_wake_structural_test.py; do echo "== $t"; python3 $t 2>&1 | tail -4; done; zsh tests/connectivity_failsafe_unknown_openness_test.sh 2>&1 | tail -5; zsh tests/nightly_heap_guard_flush_test.sh 2>&1 | tail -5; zsh tests/sleep_plan_equivalence_test.sh 2>&1 | tail -5;zsh tests/sleep_execution_equivalence_test.sh 2>&1 | tail -5

<details>
<summary>33 lines</summary>

```
== tests/sleep_cloud_gate_structural_test.py
FAIL: the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base
== tests/sleep_execution_structural_test.py
PASS: the three standby-return fields are declared once, in one anonymous-namespace struct
PASS: only effectiveStandby(), sleepAndResume() and the teardown call arguments mention standbyReturn
PASS: all 5 SystemSleepConfiguration objects are fresh locals
PASS: SleepTeardown.{h,cpp} unchanged, no new file under src/, nothing retained added (base 1e6d34f)
== tests/sleep_plan_structural_test.py
FAIL: the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base
== tests/sleep_preflight_structural_test.py
FAIL: the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base
== tests/sleep_wake_structural_test.py
FAIL: the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base
    ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//failsafe_unknown_openness_56446/body.h:143:3: error: use of undeclared identifier 'flushAllBeforeReset'
  flushAllBeforeReset();
  ^
3 errors generated.
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//nightly_heap_guard_flush_flagon.cpp:62:7: error: use of undeclared identifier 'flushAllBeforeReset'
      flushAllBeforeReset();
      ^
1 error generated.
In file included from /private/tmp/gcc-step0/dur/tests/sleep_plan_equivalence_test.cpp:86:
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//sleep_plan_equivalence.gZMmM4/cur_funcs.inc:160:7: error: use of undeclared identifier 'flushAllBeforeReset'
      flushAllBeforeReset();
      ^
1 error generated.
In file included from /private/tmp/gcc-step0/dur/tests/sleep_execution_equivalence_test.cpp:49:
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//sleep_execution_equivalence.fQMEgn/cur_funcs.inc:249:9: error: use of undeclared identifier 'flushAllBeforeReset'
        flushAllBeforeReset();
        ^
1 error generated.
<shellId: 15 completed with exit code 0>
```

</details>


---

<sub>8m 26s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && grep -n "BASE\|git " tests/sleep_plan_structural_test.py | head -20; grep -n "differ from the base" -B25 tests/sleep_plan_structural_test.py | head -60**

$ cd /private/tmp/gcc-step0/dur && grep -n "BASE\|git " tests/sleep_plan_structural_test.py | head -20; grep -n "differ from the base" -B25 tests/sleep_plan_structural_test.py | head -60

<details>
<summary>37 lines</summary>

```
30:ROUTE_BASE = "  bool pirWake = wake.pirWake;\n"
53:OCC_BASE = ("      if (debounceSeconds < (uint32_t)wakeInSeconds) {\n"
149:    plan = plan.replace(OCC_NEW, OCC_BASE)
174:    if rest.count(ROUTE_BASE) != 1:
176:    rest = rest[:rest.index(ROUTE_BASE)] + "@@ROUTE@@\n"
204:    print(f"BASE_CHECK_SKIPPED: commit {base} is not in this tree; only the structure checks ran")
207-
208-# No new src/ file; SleepTeardown.* unchanged.
209-ls = subprocess.run(["git", "-C", str(ROOT), "ls-tree", "-r", "--name-only", base, "src"], capture_output=True, text=True).stdout.split()
210-if SRC == ROOT / "src":
211-    cur_files = sorted(str(p.relative_to(ROOT)) for p in SRC.rglob("*") if p.is_file())
212-    if sorted(ls) != cur_files:
213-        fail(f"src/ file list differs from {base}: {sorted(set(cur_files) ^ set(ls))}")
214-for rel in ("src/state/SleepTeardown.cpp", "src/state/SleepTeardown.h"):
215-    want = git_show(base, rel)
216-    if want is None or (ROOT / rel).read_text() != want:
217-        fail(f"{rel} changed")
218-
219-# `retained` is not added anywhere in src/ (the base count equals the current count, comment-stripped).
220-base_retained = sum(len(re.findall(r"\bretained\b", strip(git_show(base, rel) or "")))
221-                    for rel in ls if rel.endswith((".cpp", ".h", ".ino")))
222-cur_retained = sum(len(re.findall(r"\bretained\b", strip(p.read_text())))
223-                   for p in sorted(SRC.rglob("*")) if p.suffix in (".cpp", ".h", ".ino"))
224-if cur_retained != base_retained:
225-    fail("a `retained` keyword or identifier was added in src/")
226-
227-# The S-2 functions, cloudGatePass() and the includes are unchanged.
228-tail_of = lambda t: t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))]
229-prefix_of = lambda t: t[:t.index("namespace {")]
230-cloud_of = lambda t: t[t.index("struct {\n  bool operationsCompleteLogged"):t.index("void endOfCycleSummary(") if "void endOfCycleSummary(" in t else t.index("struct SleepPlan {") if "struct SleepPlan {" in t else t.index("struct WakeContext {")]
231-if tail_of(sleep) != tail_of(base_sleep):
232:    fail("the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base")
233-if prefix_of(sleep) != prefix_of(base_sleep):
234:    fail("the includes differ from the base")
235-if cloud_of(sleep).rstrip("\n") != cloud_of(base_sleep).rstrip("\n"):
236:    fail("the cloudGate struct or cloudGatePass() differ from the base")
<shellId: 16 completed with exit code 0>
```

</details>


---

<sub>8m 30s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && sed -n 185,207p tests/sleep_plan_structural_test.py; grep -n "git_show\|^def strip\|base_sleep =" tests/sleep_plan_structural_test.py | head; git log --oneline -5 -- tests/sleep_plan_structural_test.py; for f in sleep_cloud_gate sleep_preflight sleep_wake sleep_execution; do grep -n "base = \|base=" tests/${f}_structural_test.py; done**

$ cd /private/tmp/gcc-step0/dur && sed -n 185,207p tests/sleep_plan_structural_test.py; grep -n "git_show\|^def strip\|base_sleep =" tests/sleep_plan_structural_test.py | head; git log --oneline -5 -- tests/sleep_plan_structural_test.py; for f in sleep_cloud_gate sleep_preflight sleep_wake sleep_execution; do grep -n "base = \|base=" tests/${f}_structural_test.py; done

<details>
<summary>38 lines</summary>

```
"a function static in the plan": sleep.replace("  int nightSleepSec = -1;\n  const Clock::Openness parkOpenness", "  static int nightSleepSec = -1;\n  const Clock::Openness parkOpenness", 1),
    "the plan moved behind WakeContext": sleep.replace("struct WakeContext {", "struct WakeContext0 {", 1).replace(STRUCT, "", 1) + "struct WakeContext {",
    "the guard call removed": sleep.replace(GUARD_CALL, "", 1),
    "the plan call without its early return": sleep.replace("if (sleepPlan(plan)) return;", "sleepPlan(plan);", 1),
}
for name, text in probes.items():
    if text == sleep:
        fail(f"probe did not apply: {name}")
    if not problems_in(text):
        fail(f"probe not detected: {name}")
problems = problems_in(sleep)
if problems:
    fail(problems[0])

head = FIXTURE.read_text()
m = re.search(r"^# Base commit: (\w+)$", head, re.M)
base = m.group(1) if m else "3efd9d8"
base_sleep = git_show(base, "src/state/State_Sleep.cpp")
if base_sleep is None:
    print(f"BASE_CHECK_SKIPPED: commit {base} is not in this tree; only the structure checks ran")
    print("sleep_plan_structural_test: ok")
    sys.exit(0)

66:def strip(code):
92:def git_show(commit, path):
202:base_sleep = git_show(base, "src/state/State_Sleep.cpp")
215:    want = git_show(base, rel)
220:base_retained = sum(len(re.findall(r"\bretained\b", strip(git_show(base, rel) or "")))
7322f77 The occupied sleep wakes at the time left in the debounce, not the full debounce (WO-2026-10-25-001, recovery item 20)
febcf0b WIP: Sleep S-6 Stage 6 (Copilot): wake-event processing to State_Modes.cpp, post-wake routing to one function
a06b09a WIP (Stage 6 output, for Stage 7 review): Sleep S-5 option D, the end-of-cycle summary and the Wi-Fi-off guard as two functions, WO-2026-10-21-004
d9ba803 WIP (Stage 6 output, for Stage 7 review): Sleep S-4 option D, the sleep plan and the nightly heap guard as two functions, WO-2026-10-21-003
195:base = m.group(1) if m else "9578709"
246:block_base, head_base = base_pieces(base_sleep)
237:base = m.group(1) if m else "e9475d7"
295:summary_base, guard_base, handler_base = base_pieces(base_sleep)
168:base = m.group(1) if m else "e6d3e69"
122:base = None
125:    base = m.group(1) if m else None
<shellId: 17 completed with exit code 0>
```

</details>


---

<sub>8m 34s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && git show 7322f77 --stat | head -30; ls tests/fixtures | head -30; git log --oneline | head -8; git log --format=%h -n1 -- tests/fixtures**

$ cd /private/tmp/gcc-step0/dur && git show 7322f77 --stat | head -30; ls tests/fixtures | head -30; git log --oneline | head -8; git log --format=%h -n1 -- tests/fixtures

<details>
<summary>44 lines</summary>

```
commit 7322f7701d718d4aa0a6bd5594b6cfdd37b4f9c5
Author: Charles McClelland <chip@mcclellands.org>
Date:   Sun Oct 25 12:27:07 2026 +0900

    The occupied sleep wakes at the time left in the debounce, not the full debounce (WO-2026-10-25-001, recovery item 20)
    
    After a wake that is not a PIR pulse (the hourly report), an occupied device
    slept a full debounce again, so the session end was noticed late and the late
    close time was credited (12-13 min instead of 9.5 on Dev-09, v45). The plan now
    sleeps remaining + 1 s to the LED deadline, falling back to the full debounce
    when no deadline is known. Also fixes the Wake: led= field (divided by 1000
    twice, always 0).
    
    src/ +4 net (State_Sleep.cpp). Stage 7 (Codex gpt-5.6-sol, high): VERIFIED
    WITH CONCERNS, all Low. Particle cloud compile pending (service failing, the
    unmodified base fails identically). Ships in v47.
    
    Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>

 ...10-25-001-debounce-sleep-uses-remaining-time.md |  16 ++-
 .../WO-2026-10-25-001-stage7-verdict.md            | 157 +++++++++++++++++++++
 src/state/State_Sleep.cpp                          |  10 +-
 tests/fixtures/sleep_plan_transcript.txt           | 138 +++++++++++++++++-
 tests/sleep_plan_equivalence_test.cpp              |  28 +++-
 tests/sleep_plan_equivalence_test.sh               |  47 +++++-
 tests/sleep_plan_structural_test.py                |  16 +++
 tests/sleep_preflight_structural_test.py           |  17 ++-
 tests/sleep_wake_structural_test.py                |   7 +-
 tests/wake_summary_led_test.cpp                    |  44 ++++++
 tests/wake_summary_led_test.sh                     |  56 ++++++++
battery-soc-vcell-samples-2026-08.csv
modem_health_transcript.txt
report_payload_golden.txt
sleep_cloud_gate_transcript.txt
sleep_execution_transcript.txt
sleep_plan_transcript.txt
sleep_preflight_transcript.txt
sleep_teardown_transcript.txt
sleep_wake_transcript.txt
8fb62f4 WO amendments (architect rulings, 3 Nov) and Stage 6 dispatch
c75d427 Merge remote-tracking branch 'origin/main' into wo/2026-11-03-005-item26-durability-flush
06ee2fc WO-2026-11-03-005: item 26 v49 durability flush draft (flushAllBeforeReset), for the architect's review
c0dc23f Merge pull request #178 from See-Insights/docs/2026-11-03-items-25-26
4e2a997 Merge pull request #179 from See-Insights/docs/2026-11-03-migration-pin-rule
01eef25 docs: recovery plan — item 25 is the one-constant fix (Amendment 2: Eastern default, behavior sentence, planned reset dropped); new item 26 (one safe-reset owner)
ffbb5a6 Merge pull request #177 from See-Insights/wo/2026-11-03-ci-summary-product-version
9b47665 docs: migration procedure — pin every wave device to the release under test and confirm the pin in the wave checklist (item 25 finding)
7322f77
<shellId: 18 completed with exit code 0>
```

</details>


---

<sub>8m 36s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && git show 7322f77 -- tests/sleep_wake_structural_test.py tests/sleep_preflight_structural_test.py tests/sleep_plan_structural_test.py | head -150**

$ cd /private/tmp/gcc-step0/dur && git show 7322f77 -- tests/sleep_wake_structural_test.py tests/sleep_preflight_structural_test.py tests/sleep_plan_structural_test.py | head -150

<details>
<summary>98 lines</summary>

```diff
commit 7322f7701d718d4aa0a6bd5594b6cfdd37b4f9c5
Author: Charles McClelland <chip@mcclellands.org>
Date:   Sun Oct 25 12:27:07 2026 +0900

    The occupied sleep wakes at the time left in the debounce, not the full debounce (WO-2026-10-25-001, recovery item 20)
    
    After a wake that is not a PIR pulse (the hourly report), an occupied device
    slept a full debounce again, so the session end was noticed late and the late
    close time was credited (12-13 min instead of 9.5 on Dev-09, v45). The plan now
    sleeps remaining + 1 s to the LED deadline, falling back to the full debounce
    when no deadline is known. Also fixes the Wake: led= field (divided by 1000
    twice, always 0).
    
    src/ +4 net (State_Sleep.cpp). Stage 7 (Codex gpt-5.6-sol, high): VERIFIED
    WITH CONCERNS, all Low. Particle cloud compile pending (service failing, the
    unmodified base fails identically). Ships in v47.
    
    Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>

diff --git a/tests/sleep_plan_structural_test.py b/tests/sleep_plan_structural_test.py
index eab99a4..6818282 100644
--- a/tests/sleep_plan_structural_test.py
+++ b/tests/sleep_plan_structural_test.py
@@ -40,6 +40,19 @@ SUMMARY_END = "  // On WiFi platforms"
 WIFI_START = "#if Wiring_WiFi\n  if (!useNetworkStandby) {"
 WIFI_END = "  }\n#endif\n  \n  WakeContext wake;"
 PLAN_TAIL = "  plan = {wakeInSeconds, sleepReason, overnightFallbackSleep};\n  return false;\n"
+# WO-2026-10-25-001: the occupied branch's wake is the only intended difference from the base plan block; it is
+# mapped back to the base text, so every other byte of the plan is still compared.
+OCC_NEW = (
+    "      const uint32_t remaining = signalLEDTimeRemaining();  // 0 when expired or no valid time\n"
+    "      if (remaining > 0) {\n"
+    "        const int untilDeadline = (int)remaining + 1;  // wake just after the deadline\n"
+    "        if (untilDeadline < wakeInSeconds) wakeInSeconds = untilDeadline;\n"
+    "      } else if (debounceSeconds < (uint32_t)wakeInSeconds) {\n"
+    "        wakeInSeconds = (int)debounceSeconds;  // no deadline known: the full debounce\n"
+    "      }\n")
+OCC_BASE = ("      if (debounceSeconds < (uint32_t)wakeInSeconds) {\n"
+            "        wakeInSeconds = (int)debounceSeconds;\n"
+            "      }\n")
 PLAN_START = "  // WO-2026-09-19 Step 3b: the behavioral core of this step."
 GUARD_START = "  // Before a long overnight\n"
 END = "  // The sleep configuration is built fresh"
@@ -131,6 +144,9 @@ def normalized(code):
     if not plan.endswith(PLAN_TAIL) or not guard.endswith("  return false;\n"):
         return None, None, None
     plan = plan[:-len(PLAN_TAIL)].replace("return true;", "return;")
+    if plan.count(OCC_NEW) != 1:
+        return None, None, None
+    plan = plan.replace(OCC_NEW, OCC_BASE)
     guard = guard[:-len("  return false;\n")].replace("return true;", "return;")
     handler = body_of(code, "void handleSleepingState()")
     if handler.count(PLAN_CALL) != 1 or handler.count(GUARD_CALL) != 1 or handler.count(ROUTE_CALL) != 1:
diff --git a/tests/sleep_preflight_structural_test.py b/tests/sleep_preflight_structural_test.py
index 6adf2cf..aacede3 100644
--- a/tests/sleep_preflight_structural_test.py
+++ b/tests/sleep_preflight_structural_test.py
@@ -272,7 +272,22 @@ if prefix_of(sleep) != prefix_of(base_sleep):
     fail("the includes differ from the base")
 if gate_of(sleep).rstrip("\n") != gate_of(base_sleep).rstrip("\n"):
     fail("the cloudGate struct or cloudGatePass() (S-3) differ from the base")
-if plan_of(sleep) != plan_of(base_sleep):
+# WO-2026-10-25-001: the occupied branch's wake at the LED time left is the one intended change to the S-4 plan; it is
+# mapped back to the base text so every other byte is still compared (sleep_plan_structural_test.py owns the new text).
+OCC_NEW = (
+    "      const uint32_t remaining = signalLEDTimeRemaining();  // 0 when expired or no valid time\n"
+    "      if (remaining > 0) {\n"
+    "        const int untilDeadline = (int)remaining + 1;  // wake just after the deadline\n"
+    "        if (untilDeadline < wakeInSeconds) wakeInSeconds = untilDeadline;\n"
+    "      } else if (debounceSeconds < (uint32_t)wakeInSeconds) {\n"
+    "        wakeInSeconds = (int)debounceSeconds;  // no deadline known: the full debounce\n"
+    "      }\n")
+OCC_BASE = ("      if (debounceSeconds < (uint32_t)wakeInSeconds) {\n"
+            "        wakeInSeconds = (int)debounceSeconds;\n"
+            "      }\n")
+if plan_of(sleep).count(OCC_NEW) != 1:
+    fail("the occupied branch of sleepPlan() (WO-2026-10-25-001) is not the expected text")
+if plan_of(sleep).replace(OCC_NEW, OCC_BASE) != plan_of(base_sleep):
     fail("struct SleepPlan, sleepPlan(), hibernateAttempt() or nightlyHeapGuard() (S-4) differ from the base")
 
 # The moved blocks and the handler are byte-identical to the base apart from the listed edits.
diff --git a/tests/sleep_wake_structural_test.py b/tests/sleep_wake_structural_test.py
index b9f8cec..cfed5f3 100644
--- a/tests/sleep_wake_structural_test.py
+++ b/tests/sleep_wake_structural_test.py
@@ -184,11 +184,16 @@ if cur_retained != base_retained:
 tail_of = lambda t: t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))]
 prefix_of = lambda t: t[:t.index("namespace {")]
 log_of = lambda t: t[t.index("void logWakeSummary("):t.index("\n}\n", t.index("void logWakeSummary(")) + 3]
+# WO-2026-10-25-001: the one intended change, the led= value is already in seconds; mapped back for the compare.
+LED_NEW = "(unsigned long)signalLEDTimeRemaining());"
+LED_BASE = "(unsigned long)(signalLEDTimeRemaining() / 1000UL));"
+if log_of(sleep).count(LED_NEW) != 1:
+    fail("logWakeSummary() must print signalLEDTimeRemaining() (seconds) without a further division")
 if tail_of(sleep) != tail_of(base_sleep):
     fail("the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base")
 if prefix_of(sleep) != prefix_of(base_sleep):
     fail("the State_Sleep.cpp includes differ from the base")
-if log_of(sleep) != log_of(base_sleep):
+if log_of(sleep).replace(LED_NEW, LED_BASE) != log_of(base_sleep):
     fail("logWakeSummary() differs from the base")
 base_modes = git_show(base, "src/state/State_Modes.cpp")
 cur_modes_head = modes[:modes.index("\nbool processSleepWakeEvent(bool pirWake) {")]
<shellId: 19 completed with exit code 0>
```

</details>


---

<sub>8m 40s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && grep -n "^sleep = \|^sleep=\|sleep = .*read_text\|SLEEP.*read_text" tests/sleep_*structural_test.py; grep -n "State_Sleep.cpp" tests/sleep_plan_equivalence_test.sh tests/sleep_execution_equivalence_test.sh tests/nightly_heap_guard_flush_test.sh | head -20**

$ cd /private/tmp/gcc-step0/dur && grep -n "^sleep = \|^sleep=\|sleep = .*read_text\|SLEEP.*read_text" tests/sleep_*structural_test.py; grep -n "State_Sleep.cpp" tests/sleep_plan_equivalence_test.sh tests/sleep_execution_equivalence_test.sh tests/nightly_heap_guard_flush_test.sh | head -20

<details>
<summary>28 lines</summary>

```
tests/sleep_cloud_gate_structural_test.py:164:sleep = SLEEP.read_text()
tests/sleep_execution_structural_test.py:60:raw = SLEEP.read_text()
tests/sleep_execution_structural_test.py:100:    if path != SLEEP and path.suffix in (".cpp", ".h") and "standbyReturn" in strip_comments(path.read_text()):
tests/sleep_plan_structural_test.py:181:sleep = SLEEP.read_text()
tests/sleep_preflight_structural_test.py:207:sleep = SLEEP.read_text()
tests/sleep_teardown_structural_test.py:98:sleep = SLEEP_CPP.read_text()
tests/sleep_wake_structural_test.py:120:sleep, modes, header = SLEEP.read_text(), MODES.read_text(), HEADER.read_text()
tests/sleep_plan_equivalence_test.sh:7:# plan block, hibernateAttempt call and guard block extracted by marker text from the base State_Sleep.cpp
tests/sleep_plan_equivalence_test.sh:14:# needs the base commit named in the fixture header. Mutants of a private copy of State_Sleep.cpp must each
tests/sleep_plan_equivalence_test.sh:33:base_src="$work/base_State_Sleep.cpp"
tests/sleep_plan_equivalence_test.sh:35:if git -C "$repo_root" show "${base}:src/state/State_Sleep.cpp" > "$base_src" 2>/dev/null; then have_base=1; fi
tests/sleep_plan_equivalence_test.sh:92:extract cur "$src_root/state/State_Sleep.cpp"
tests/sleep_plan_equivalence_test.sh:132:# Generated by extracting sleepPlan() and nightlyHeapGuard() by marker text from the current src/state/State_Sleep.cpp,
tests/sleep_plan_equivalence_test.sh:164:extract_fresh cur "$src_root/state/State_Sleep.cpp"
tests/sleep_plan_equivalence_test.sh:170:diff -u "$work/expected.txt" "$work/new.txt" > "$work/new.diff" || { head -60 "$work/new.diff"; echo "EQUIVALENCE_FAILED: State_Sleep.cpp differs from the frozen transcript"; exit 1; }
tests/sleep_plan_equivalence_test.sh:199:  # $1 = label, then old/new pairs applied to a private copy of State_Sleep.cpp
tests/sleep_plan_equivalence_test.sh:203:  cp "$src_root/state/State_Sleep.cpp" "$m/State_Sleep.cpp"
tests/sleep_plan_equivalence_test.sh:204:  python3 - "$m/State_Sleep.cpp" "$@" <<'PY'
tests/sleep_plan_equivalence_test.sh:216:  python3 "$work/extract.py" cur "$m/State_Sleep.cpp" "$m"
tests/sleep_execution_equivalence_test.sh:7:# scenario table through (a) the blocks extracted by marker text from the base State_Sleep.cpp
tests/sleep_execution_equivalence_test.sh:14:# Mutants of a private copy of State_Sleep.cpp must each change the transcript.
tests/sleep_execution_equivalence_test.sh:28:base_src="$work/base_State_Sleep.cpp"
tests/sleep_execution_equivalence_test.sh:30:if git -C "$repo_root" show "${base}:src/state/State_Sleep.cpp" > "$base_src" 2>/dev/null; then have_base=1; fi
tests/sleep_execution_equivalence_test.sh:76:extract cur "$src_root/state/State_Sleep.cpp"
tests/sleep_execution_equivalence_test.sh:118:# Generated once by extracting the blocks by marker text from {base}:src/state/State_Sleep.cpp (git show),
tests/sleep_execution_equivalence_test.sh:149:extract_fresh cur "$src_root/state/State_Sleep.cpp"
tests/sleep_execution_equivalence_test.sh:155:diff -u "$work/expected.txt" "$work/new.txt" > "$work/new.diff" || { head -60 "$work/new.diff"; echo "EQUIVALENCE_FAILED: State_Sleep.cpp differs from the frozen transcript"; exit 1; }
<shellId: 20 completed with exit code 0>
```

</details>


---

<sub>8m 58s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 - <<'E'
note='# WO-2026-11-03-005: the deliberate-reset flush call is the one intended addition; mapped back so every other byte is still compared.\nsleep = re.sub(r"^ *flushAllBeforeReset\\(\\);\\n", "", sleep, flags=re.M)\n'
for f,a in [('cloud_gate','sleep = SLEEP.read_text()\n'),('plan','sleep = SLEEP.read_text()\n'),('preflight','sleep = SLEEP.read_text()\n')]:
    p=f'tests/sleep_{f}_structural_test.py'; t=open(p).read(); assert t.count(a)==1; assert 'import re' in t or 'import ' in t
    open(p,'w').write(t.replace(a,a+note))
p='tests/sleep_wake_structural_test.py'; t=open(p).read()
a='sleep, modes, header = SLEEP.read_text(), MODES.read_text(), HEADER.read_text()\n'; assert t.count(a)==1
open(p,'w').write(t.replace(a,a+note))
E
grep -n "^import\|^from" tests/sleep_wake_structural_test.py tests/sleep_cloud_gate_structural_test.py tests/sleep_plan_structural_test.py tests/sleep_preflight_structural_test.py | grep -n "re\b"; for t in cloud_gate plan preflight wake; do python3 tests/sleep_${t}_structural_test.py 2>&1 | tail -2; done**

$ cd /private/tmp/gcc-step0/dur && python3 - <<'E'
note='# WO-2026-11-03-005: the deliberate-reset flush call is the one intended addition; mapped back so every other byte is still compared.\nsleep = re.sub(r"^ *flushAllBeforeReset\\(\\);\\n", "", sleep, flags=re.M)\n'
for f,a in [('cloud_gate','sleep = SLEEP.read_text()\n'),('plan','sleep = SLEEP.read_text()\n'),('preflight','sleep = SLEEP.read_text()\n')]:
    p=f'tests/sleep_{f}_structural_test.py'; t=open(p).read(); assert t.count(a)==1; assert 'import re' in t or 'import ' in t
    open(p,'w').write(t.replace(a,a+note))
p='tests/sleep_wake_structural_test.py'; t=open(p).read()
a='sleep, modes, header = SLEEP.read_text(), MODES.read_text(), HEADER.read_text()\n'; assert t.count(a)==1
open(p,'w').write(t.replace(a,a+note))
E
grep -n "^import\|^from" tests/sleep_wake_structural_test.py tests/sleep_cloud_gate_structural_test.py tests/sleep_plan_structural_test.py tests/sleep_preflight_structural_test.py | grep -n "re\b"; for t in cloud_gate plan preflight wake; do python3 tests/sleep_${t}_structural_test.py 2>&1 | tail -2; done

<details>
<summary>9 lines</summary>

```
1:tests/sleep_wake_structural_test.py:12:import re
5:tests/sleep_cloud_gate_structural_test.py:9:import re
9:tests/sleep_plan_structural_test.py:10:import re
13:tests/sleep_preflight_structural_test.py:12:import re
sleep_cloud_gate_structural_test: ok
sleep_plan_structural_test: ok
sleep_preflight_structural_test: ok
sleep_wake_structural_test: ok
<shellId: 21 completed with exit code 0>
```

</details>


---

<sub>9m 2s</sub>

### Copilot

Now the harness stubs.

---

<sub>9m 2s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && grep -n "flushDiagBatch\|System.reset\|void reset" tests/sleep_plan_equivalence_test.cpp tests/sleep_execution_equivalence_test.cpp tests/connectivity_failsafe_unknown_openness_test.cpp tests/nightly_heap_guard_flush_test.sh tests/connectivity_failsafe_open_hours_test.cpp tests/failsafe_cadence_rule_test.cpp | head -30; sed -n 90,130p tests/connectivity_failsafe_unknown_openness_test.cpp**

$ cd /private/tmp/gcc-step0/dur && grep -n "flushDiagBatch\|System.reset\|void reset" tests/sleep_plan_equivalence_test.cpp tests/sleep_execution_equivalence_test.cpp tests/connectivity_failsafe_unknown_openness_test.cpp tests/nightly_heap_guard_flush_test.sh tests/connectivity_failsafe_open_hours_test.cpp tests/failsafe_cadence_rule_test.cpp | head -30; sed -n 90,130p tests/connectivity_failsafe_unknown_openness_test.cpp

<details>
<summary>46 lines</summary>

```
tests/connectivity_failsafe_unknown_openness_test.cpp:34:// The persisted stage AT THE MOMENT the action was called. A real System.reset() or deep
tests/connectivity_failsafe_unknown_openness_test.cpp:88:  void reset(int) { lastActionTaken = RESET; ++actionCount; persistedStageAtAction = persistedStage; throw ActionStop{}; }
tests/connectivity_failsafe_unknown_openness_test.cpp:153:  return a == RESET ? "System.reset" : a == DEEP_POWER_DOWN ? "deepPowerDown" : "no action";
tests/nightly_heap_guard_flush_test.sh:39:// Records the exact order System.reset()/System.freeMemory() and
tests/nightly_heap_guard_flush_test.sh:40:// PowerDiagnostics::flushDiagBatch() were invoked in, so the test can assert
tests/nightly_heap_guard_flush_test.sh:50:  void reset(uint32_t cause = 0) {
tests/nightly_heap_guard_flush_test.sh:53:    callOrder.push_back("System.reset");
tests/nightly_heap_guard_flush_test.sh:60:void flushDiagBatch() { callOrder.push_back("PowerDiagnostics::flushDiagBatch"); }
tests/nightly_heap_guard_flush_test.sh:65:// same way flushDiagBatch() is stubbed above - a fixed value is fine here
tests/nightly_heap_guard_flush_test.sh:82:  // flushDiagBatch() to run before System.reset() so accumulated
tests/nightly_heap_guard_flush_test.sh:98:  assert(callOrder[0] == "PowerDiagnostics::flushDiagBatch");
tests/nightly_heap_guard_flush_test.sh:99:  assert(callOrder[1] == "System.reset");
tests/nightly_heap_guard_flush_test.sh:107:  assert(callOrder[0] == "System.reset");

struct Ab1805Fake {
  void deepPowerDown() { lastActionTaken = DEEP_POWER_DOWN; ++actionCount; persistedStageAtAction = persistedStage; throw ActionStop{}; }
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
<shellId: 22 completed with exit code 0>
```

</details>


---

<sub>9m 7s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && sed -n 20,90p tests/connectivity_failsafe_unknown_openness_test.cpp; sed -n 130,260p tests/connectivity_failsafe_unknown_openness_test.cpp**

$ cd /private/tmp/gcc-step0/dur && sed -n 20,90p tests/connectivity_failsafe_unknown_openness_test.cpp; sed -n 130,260p tests/connectivity_failsafe_unknown_openness_test.cpp

<details>
<summary>143 lines</summary>

```
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
} // namespace RecoveryState

namespace SystemConfig {
enum { CONNECTED, DISCONNECTED };
time_t lastConnection = 0;
time_t openTimeToday = 0;
inline int get_openTime() { return 6; }
inline time_t get_lastConnection() { return lastConnection; }
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
  void reset(int) { lastActionTaken = RESET; ++actionCount; persistedStageAtAction = persistedStage; throw ActionStop{}; }
} System;

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
  try {
    connectivityFailsafeSupervisor();
  } catch (const ActionStop &) {
  }
  return lastActionTaken;
}

const char *name(Action a) {
  return a == RESET ? "System.reset" : a == DEEP_POWER_DOWN ? "deepPowerDown" : "no action";
}

void check(const char *what, const Scenario &s, Action want, uint8_t wantStage = 0) {
  const Action got = run(s);
  const bool stageOk = (want == NONE) || persistedStageAtAction == wantStage;
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
<shellId: 23 completed with exit code 0>
```

</details>


---

<sub>9m 7s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && cat tests/connectivity_failsafe_unknown_openness_test.sh**

$ cd /private/tmp/gcc-step0/dur && cat tests/connectivity_failsafe_unknown_openness_test.sh

<details>
<summary>63 lines</summary>

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

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -DSUPERVISOR_BODY_H="\"$work/body.h\"" \
  -DREAL_STALE_SEC="$(read_policy CONNECTIVITY_FAILSAFE_STALE_SEC)" \
  -DREAL_COOLDOWN_SEC="$(read_policy CONNECTIVITY_FAILSAFE_COOLDOWN_SEC)" \
  -DREAL_JITTER_MAX_SEC="$(read_policy CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC)" \
  "$repo_root/tests/connectivity_failsafe_unknown_openness_test.cpp" \
  -o "$work/unknown_openness"
"$work/unknown_openness"

echo "connectivity_failsafe_unknown_openness_test passed"
<shellId: 24 completed with exit code 0>
```

</details>


---

<sub>9m 15s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/connectivity_failsafe_unknown_openness_test.cpp'
t=open(p).read()
def sub(a,b,c=1):
    global t; assert t.count(a)==c,a; t=t.replace(a,b)
sub("uint8_t persistedStageAtAction = 0;\n","uint8_t persistedStageAtAction = 0;\n// WO-2026-11-03-005: flushAllBeforeReset() must run after the stage record and before the action.\nbool flushed = false;\nbool flushedAfterRecordBeforeAction = false;\n")
sub("inline time_t get_lastConnection() { return lastConnection; }\n","inline time_t get_lastConnection() { return lastConnection; }\ninline void flushNow() {}\n")
sub("void reset(int) { lastActionTaken = RESET;","void reset(int) { flushedAfterRecordBeforeAction = flushed; lastActionTaken = RESET;")
sub("void deepPowerDown() { lastActionTaken","void deepPowerDown() { flushedAfterRecordBeforeAction = flushed; lastActionTaken")
sub("#include SUPERVISOR_BODY_H","void flushAllBeforeReset() { flushed = (persistedStage != 0 && persistedAction != 0); }\n\n#include SUPERVISOR_BODY_H")
sub("  persistedStageAtAction = 0;\n  try","  persistedStageAtAction = 0;\n  flushed = false;\n  flushedAfterRecordBeforeAction = false;\n  try")
sub("const bool stageOk = (want == NONE) || persistedStageAtAction == wantStage;","const bool stageOk = (want == NONE) || (persistedStageAtAction == wantStage && flushedAfterRecordBeforeAction);")
open(p,'w').write(t)
E
zsh tests/connectivity_failsafe_unknown_openness_test.sh 2>&1 | tail -8**

$ cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/connectivity_failsafe_unknown_openness_test.cpp'
t=open(p).read()
def sub(a,b,c=1):
    global t; assert t.count(a)==c,a; t=t.replace(a,b)
sub("uint8_t persistedStageAtAction = 0;\n","uint8_t persistedStageAtAction = 0;\n// WO-2026-11-03-005: flushAllBeforeReset() must run after the stage record and before the action.\nbool flushed = false;\nbool flushedAfterRecordBeforeAction = false;\n")
sub("inline time_t get_lastConnection() { return lastConnection; }\n","inline time_t get_lastConnection() { return lastConnection; }\ninline void flushNow() {}\n")
sub("void reset(int) { lastActionTaken = RESET;","void reset(int) { flushedAfterRecordBeforeAction = flushed; lastActionTaken = RESET;")
sub("void deepPowerDown() { lastActionTaken","void deepPowerDown() { flushedAfterRecordBeforeAction = flushed; lastActionTaken")
sub("#include SUPERVISOR_BODY_H","void flushAllBeforeReset() { flushed = (persistedStage != 0 && persistedAction != 0); }\n\n#include SUPERVISOR_BODY_H")
sub("  persistedStageAtAction = 0;\n  try","  persistedStageAtAction = 0;\n  flushed = false;\n  flushedAfterRecordBeforeAction = false;\n  try")
sub("const bool stageOk = (want == NONE) || persistedStageAtAction == wantStage;","const bool stageOk = (want == NONE) || (persistedStageAtAction == wantStage && flushedAfterRecordBeforeAction);")
open(p,'w').write(t)
E
zsh tests/connectivity_failsafe_unknown_openness_test.sh 2>&1 | tail -8

<details>
<summary>8 lines</summary>

```
ok   Open, stage 2, past cooldown + jitter: stage 3 deep power-down   -> deepPowerDown
  ok   Open, stage 2, inside the cooldown + jitter: nothing             -> no action
  ok   Open, stage 0, one second short of the stale threshold: nothing  -> no action
  ok   Unknown, before today's opening (now <= ageBase): nothing        -> no action
  ok   Unknown, now == ageBase: nothing                                 -> no action

connectivity_failsafe_unknown_openness cases passed
connectivity_failsafe_unknown_openness_test passed
<shellId: 25 completed with exit code 0>
```

</details>


---

<sub>9m 17s</sub>

### Copilot

Now heap-guard, plan-equiv, exec-equiv harnesses.

---

<sub>9m 17s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && sed -n 56,125p tests/nightly_heap_guard_flush_test.sh; grep -n "resets\|reset(" tests/sleep_plan_equivalence_test.cpp tests/sleep_execution_equivalence_test.cpp | head -20**

$ cd /private/tmp/gcc-step0/dur && sed -n 56,125p tests/nightly_heap_guard_flush_test.sh; grep -n "resets\|reset(" tests/sleep_plan_equivalence_test.cpp tests/sleep_execution_equivalence_test.cpp | head -20

<details>
<summary>58 lines</summary>

```
FakeSystem System;

namespace PowerDiagnostics {
void flushDiagBatch() { callOrder.push_back("PowerDiagnostics::flushDiagBatch"); }
} // namespace PowerDiagnostics

// WO-2026-08-12-001 item 2 added a pure-read confirmation log call to
// Connectivity::isRadioPoweredOn() inside this extracted block. Stub it the
// same way flushDiagBatch() is stubbed above - a fixed value is fine here
// since this test only cares about flush-before-reset ordering, not the
// radio-state value itself (that is covered by
// tests/modem_teardown_confirmation_logging_test.py).
namespace Connectivity {
bool isRadioPoweredOn() { return false; }
} // namespace Connectivity

CPP

    extract_braced_block "$repo_root/src/state/State_Sleep.cpp" "bool nightlyHeapGuard(bool overnightFallbackSleep) {"

    cat <<'CPP'

int main() {
  // Non-empty-accumulator scenario: heap has drifted below the reset
  // threshold during an overnight fallback-sleep cycle. Fix #1 requires
  // flushDiagBatch() to run before System.reset() so accumulated
  // PowerDiag/ChargeDiag/stale-SOC-resync entries from this cycle are not
  // silently dropped by the reset.
  // WO-2026-10-21-003: the guard is nightlyHeapGuard(), extracted whole; it
  // returns true exactly on the reset path and false otherwise.
  callOrder.clear();
  System.freeMemoryValue = ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES - 1;
  assert(nightlyHeapGuard(/*overnightFallbackSleep=*/true));

CPP

    if [[ "$out_file" == "$generated_flag_on" ]]; then
      cat <<'CPP'
  // ENABLE_DIAGNOSTICS_PUBLISH_MODE=1 build: flush must be called, and must
  // precede the reset.
  assert(callOrder.size() == 2);
  assert(callOrder[0] == "PowerDiagnostics::flushDiagBatch");
  assert(callOrder[1] == "System.reset");
  std::cout << "Nightly heap-guard flush-before-reset test passed (flag=1)\n";
CPP
    else
      cat <<'CPP'
  // ENABLE_DIAGNOSTICS_PUBLISH_MODE=0 (default/shipped) build: the flush
  // call site is compiled out entirely (fix #3), so only the reset happens.
  assert(callOrder.size() == 1);
  assert(callOrder[0] == "System.reset");
  std::cout << "Nightly heap-guard reset-only test passed (flag=0)\n";
CPP
    fi

    cat <<'CPP'

  // Heap above both thresholds: neither flush nor reset should fire.
  callOrder.clear();
  System.freeMemoryValue = ConnectivityPolicy::NIGHTLY_HEAP_WARN_THRESHOLD_BYTES + 1;
  assert(!nightlyHeapGuard(/*overnightFallbackSleep=*/true));
  assert(callOrder.empty());

  // At the warn threshold (above reset): monitoring only, no reset, returns false.
  callOrder.clear();
  System.freeMemoryValue = ConnectivityPolicy::NIGHTLY_HEAP_WARN_THRESHOLD_BYTES;
  assert(!nightlyHeapGuard(/*overnightFallbackSleep=*/true));
  assert(callOrder.empty());
<shellId: 26 completed with exit code 0>
```

</details>


---

<sub>9m 24s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/nightly_heap_guard_flush_test.sh'
t=open(p).read()
def sub(a,b):
    global t; assert t.count(a)==1,a; t=t.replace(a,b)
sub("""FakeSystem System;

namespace PowerDiagnostics {""","""FakeSystem System;

// WO-2026-11-03-005: the stores and queue are flushed after the diagnostics batch, just before the reset.
void flushAllBeforeReset() { callOrder.push_back("flushAllBeforeReset"); }

namespace PowerDiagnostics {""")
sub("""  assert(callOrder.size() == 2);
  assert(callOrder[0] == "PowerDiagnostics::flushDiagBatch");
  assert(callOrder[1] == "System.reset");""","""  assert(callOrder.size() == 3);
  assert(callOrder[0] == "PowerDiagnostics::flushDiagBatch");
  assert(callOrder[1] == "flushAllBeforeReset");
  assert(callOrder[2] == "System.reset");""")
sub("""  assert(callOrder.size() == 1);
  assert(callOrder[0] == "System.reset");""","""  assert(callOrder.size() == 2);
  assert(callOrder[0] == "flushAllBeforeReset");
  assert(callOrder[1] == "System.reset");""")
open(p,'w').write(t)
E
zsh tests/nightly_heap_guard_flush_test.sh 2>&1 | tail -4; sed -n 70,110p tests/sleep_plan_equivalence_test.sh; grep -n "FakeSystem\|struct.*System\|flushDiagBatch" tests/sleep_plan_equivalence_test.cpp tests/sleep_execution_equivalence_test.cpp | head**

$ cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/nightly_heap_guard_flush_test.sh'
t=open(p).read()
def sub(a,b):
    global t; assert t.count(a)==1,a; t=t.replace(a,b)
sub("""FakeSystem System;

namespace PowerDiagnostics {""","""FakeSystem System;

// WO-2026-11-03-005: the stores and queue are flushed after the diagnostics batch, just before the reset.
void flushAllBeforeReset() { callOrder.push_back("flushAllBeforeReset"); }

namespace PowerDiagnostics {""")
sub("""  assert(callOrder.size() == 2);
  assert(callOrder[0] == "PowerDiagnostics::flushDiagBatch");
  assert(callOrder[1] == "System.reset");""","""  assert(callOrder.size() == 3);
  assert(callOrder[0] == "PowerDiagnostics::flushDiagBatch");
  assert(callOrder[1] == "flushAllBeforeReset");
  assert(callOrder[2] == "System.reset");""")
sub("""  assert(callOrder.size() == 1);
  assert(callOrder[0] == "System.reset");""","""  assert(callOrder.size() == 2);
  assert(callOrder[0] == "flushAllBeforeReset");
  assert(callOrder[1] == "System.reset");""")
open(p,'w').write(t)
E
zsh tests/nightly_heap_guard_flush_test.sh 2>&1 | tail -4; sed -n 70,110p tests/sleep_plan_equivalence_test.sh; grep -n "FakeSystem\|struct.*System\|flushDiagBatch" tests/sleep_plan_equivalence_test.cpp tests/sleep_execution_equivalence_test.cpp | head

<details>
<summary>40 lines</summary>

```
Nightly heap-guard flush-before-reset test passed (flag=1)
Nightly heap-guard reset-only test passed (flag=0)
for line in sys.stdin.read().splitlines():
    if line.startswith("BUILD "):
        skip = in_occ = False
    elif re.match(r"S\d+ ", line):
        skip, in_occ = " debounce-left " in line, False
    elif line.startswith(("SCENARIOS ", "EVENT ")):
        skip = True
    if skip:
        continue
    if line.startswith("  in "):
        in_occ = re.search(r" mode=1 .* occ=1 ", line) is not None
    elif in_occ and line == "    CALL signalLEDTimeRemaining":
        continue
    print(line)
PY

extract() {
  # $1 = base|cur, $2 = source file
  python3 "$work/extract.py" "$1" "$2" "$work"
}

if (( have_base )); then extract base "$base_src"; fi
extract cur "$src_root/state/State_Sleep.cpp"
if [[ "${SLEEP_PLAN_TAMPER_COPY:-}" == 1 ]]; then echo "// drift" >> "$work/cur_funcs.inc"; fi

build_run() {
  # $1 = test-mode flag (0|1), $2 = mode (a|b), $3 = output, $4 = work dir holding the .inc copies
  local tm="$1" mode="$2" out="$3" inc="$4"
  c++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1 \
    -DCONNECTIVITY_FAILSAFE_TEST_MODE="$tm" -I"$stubs" -I"$src_root" \
    -DBASE_REGION_INC="\"$inc/base_region.inc\"" -DCUR_FUNCS_INC="\"$inc/cur_funcs.inc\"" \
    -DCUR_REGION_INC="\"$inc/cur_region.inc\"" "$tests_dir/sleep_plan_equivalence_test.cpp" -o "$out.bin"
  "$out.bin" "$mode" > "$out"
}

run_all() {
  # $1 = mode, $2 = output prefix, $3 = work dir holding the .inc copies
  build_run 0 "$1" "$2.release" "$3"
  build_run 1 "$1" "$2.testmode" "$3"
  cat "$2.release" "$2.testmode" > "$2"
}
<shellId: 27 completed with exit code 0>
```

</details>


---

<sub>9m 28s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && grep -n "reset\|^#include\|ThrashGuard\|transitionTo" tests/sleep_plan_equivalence_test.cpp | head -30; grep -n "reset\|^#include" tests/sleep_execution_equivalence_test.cpp | head -30**

$ cd /private/tmp/gcc-step0/dur && grep -n "reset\|^#include\|ThrashGuard\|transitionTo" tests/sleep_plan_equivalence_test.cpp | head -30; grep -n "reset\|^#include" tests/sleep_execution_equivalence_test.cpp | head -30

<details>
<summary>30 lines</summary>

```
9:#include <algorithm>
10:#include <cstdio>
11:#include <cstring>
12:#include <functional>
13:#include <set>
14:#include <string>
15:#include "SleepExecutionFakes.h"
16:#include "Config.h"
78:#include BASE_REGION_INC
86:#include CUR_FUNCS_INC
89:#include CUR_REGION_INC
197:  // The nightly heap guard: at and around the reset and warn thresholds, overnight or not.
9:#include <algorithm>
10:#include <cstdio>
11:#include <cstring>
12:#include <set>
13:#include <string>
14:#include "SleepExecutionFakes.h"
24:#include BASE_DRAIN_INC
32:#include BASE_STANDBY_INC
36:#include BASE_HIB_INC
40:#include BASE_ULP_INC
48:#include CUR_DRAIN_INC
49:#include CUR_FUNCS_INC
80:#include CUR_CALL_EFFECTIVE_INC
82:#include CUR_CALL_TEARDOWN_INC
83:#include CUR_CALL_HIBERNATE_INC
84:#include CUR_CALL_SLEEP_INC
85:#include CUR_CALL_ROUTE_INC
<shellId: 28 completed with exit code 0>
```

</details>


---

<sub>9m 29s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && F=$(find tests -name SleepExecutionFakes.h); echo $F; grep -n "reset\|flushDiagBatch" $F | head**

$ cd /private/tmp/gcc-step0/dur && F=$(find tests -name SleepExecutionFakes.h); echo $F; grep -n "reset\|flushDiagBatch" $F | head

```
tests/stubs/sleep_teardown/SleepExecutionFakes.h
109:  void reset(uint32_t cause) { sim::emit("RESET cause=%u", (unsigned)cause); }
171:inline void flushDiagBatch() { sim::emit("POWER flushDiagBatch"); }
<shellId: 29 completed with exit code 0>
```


---

<sub>9m 32s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && sed -n 165,173p tests/stubs/sleep_teardown/SleepExecutionFakes.h**

$ cd /private/tmp/gcc-step0/dur && sed -n 165,173p tests/stubs/sleep_teardown/SleepExecutionFakes.h

<details>
<summary>10 lines</summary>

```
inline bool isRadioPoweredOn() { return sim::world.radioOn; }
inline void requestCloudDisconnectOnly() { sim::emit("CALL Connectivity::requestCloudDisconnectOnly"); }
inline void requestRadioPowerOff() { sim::emit("CALL Connectivity::requestRadioPowerOff"); }
}
namespace PowerDiagnostics {
inline void logPowerState(const char *tag) { sim::emit("POWER %s", tag); }
inline void flushDiagBatch() { sim::emit("POWER flushDiagBatch"); }
}  // namespace PowerDiagnostics
namespace AwakeCycles {
<shellId: 30 completed with exit code 0>
```

</details>


---

<sub>10m 27s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/stubs/sleep_teardown/SleepExecutionFakes.h'
t=open(p).read()
a="}  // namespace PowerDiagnostics\nnamespace AwakeCycles {"
assert t.count(a)==1
t=t.replace(a,"}  // namespace PowerDiagnostics\n// WO-2026-11-03-005: unrecorded, so the frozen base-equivalence transcripts are unchanged (the flush order is tested in deliberate_reset_flush_test).\ninline void flushAllBeforeReset() {}\nnamespace AwakeCycles {")
open(p,'w').write(t)
E
zsh tests/sleep_plan_equivalence_test.sh 2>&1|tail -3; zsh tests/sleep_execution_equivalence_test.sh 2>&1|tail -3; zsh tests/alert19_clears_after_report_test.sh 2>&1 | tail -2**

$ cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/stubs/sleep_teardown/SleepExecutionFakes.h'
t=open(p).read()
a="}  // namespace PowerDiagnostics\nnamespace AwakeCycles {"
assert t.count(a)==1
t=t.replace(a,"}  // namespace PowerDiagnostics\n// WO-2026-11-03-005: unrecorded, so the frozen base-equivalence transcripts are unchanged (the flush order is tested in deliberate_reset_flush_test).\ninline void flushAllBeforeReset() {}\nnamespace AwakeCycles {")
open(p,'w').write(t)
E
zsh tests/sleep_plan_equivalence_test.sh 2>&1|tail -3; zsh tests/sleep_execution_equivalence_test.sh 2>&1|tail -3; zsh tests/alert19_clears_after_report_test.sh 2>&1 | tail -2

<details>
<summary>9 lines</summary>

```
caught: mut_LED_off_after_onEnterSleep_dropped
caught: mut_LED-defer_transition_dropped
MUTATION_NOT_APPLIED: '      PowerDiagnostics::flushDiagBatch();\n#endif\n      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);' x0
caught: mut_PIR_classified_from_the_wrong_edge
caught: mut_post-wake_routing_handed_swapped_pir_and_button
sleep_execution_equivalence_test: both transcripts match the frozen pre-change output, all mutations detected
PASS: testLedgerFailureReportedOnceThenLaterAlertVisible
Event alerts clear after report test passed
<shellId: 31 completed with exit code 0>
```

</details>


---

<sub>10m 30s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && grep -n "flushDiagBatch" -B3 -A3 tests/sleep_plan_equivalence_test.sh | sed -n 1,40p**

$ cd /private/tmp/gcc-step0/dur && grep -n "flushDiagBatch" -B3 -A3 tests/sleep_plan_equivalence_test.sh | sed -n 1,40p

<details>
<summary>16 lines</summary>

```
188-            "LOG INFO FailsafeDefer: reason=closed-hours-long-sleep open=0 sleep=" \
189-            "LOG INFO FailsafeTest: cap closed sleep 3600s->60s" \
190-            "LOG INFO ModemTeardown: radioOn=1 point=heap-guard-reset" \
191:            "resetting for clean next-day start" "monitoring only" "POWER flushDiagBatch" "RESET cause=4" \
192-            "CALL hibernateAttempt overnight=1 wake=28800" "CALL hibernateAttempt overnight=0 wake=3601" \
193-            "CALL hibernateAttempt overnight=0 wake=3600" "CALL hibernateAttempt overnight=1 wake=32760" \
194-            "reason=closed" "reason=debounce" "reason=scheduled" "wake=0 reason=debounce" "wake=5 reason=debounce"; do
--
235-"
236-mutate_and_expect_diff "LED-defer transition dropped" '      transitionTo(IDLE_STATE, "sleep-counting-led-defer");
237-      return true;' "      return true;"
238:mutate_and_expect_diff "flush before the reset dropped" "      PowerDiagnostics::flushDiagBatch();
239-#endif
240-      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);" "#endif
241-      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);"
<shellId: 32 completed with exit code 0>
```

</details>


---

<sub>11m 6s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/sleep_plan_equivalence_test.sh'
t=open(p).read()
a='''"      PowerDiagnostics::flushDiagBatch();
#endif
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);" "#endif
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);"'''
b='''"      PowerDiagnostics::flushDiagBatch();
#endif
      flushAllBeforeReset();
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);" "#endif
      flushAllBeforeReset();
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);"'''
assert t.count(a)==1; open(p,'w').write(t.replace(a,b))
E
zsh tests/sleep_plan_equivalence_test.sh 2>&1|tail -3**

$ cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/sleep_plan_equivalence_test.sh'
t=open(p).read()
a='''"      PowerDiagnostics::flushDiagBatch();
#endif
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);" "#endif
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);"'''
b='''"      PowerDiagnostics::flushDiagBatch();
#endif
      flushAllBeforeReset();
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);" "#endif
      flushAllBeforeReset();
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);"'''
assert t.count(a)==1; open(p,'w').write(t.replace(a,b))
E
zsh tests/sleep_plan_equivalence_test.sh 2>&1|tail -3

```
caught: mut_test-mode_cap_dropped
caught: mut_guard_called_before_the_hibernate_attempt
sleep_plan_equivalence_test: both transcripts match the frozen output, all mutations detected
<shellId: 33 completed with exit code 0>
```


---

<sub>11m 40s</sub>

### `create`

**create a new file at /private/tmp/gcc-step0/dur/tests/deliberate_reset_flush_test.py.**

/private/tmp/gcc-step0/dur/tests/deliberate_reset_flush_test.py

```
Created file /private/tmp/gcc-step0/dur/tests/deliberate_reset_flush_test.py with 9422 characters
```


---

<sub>11m 48s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/deliberate_reset_flush_test.py'
t=open(p).read()
def sub(a,b):
    global t; assert t.count(a)==1,a; t=t.replace(a,b)
sub('''        if before not in text[:flush_at] or text.index(before) > flush_at:
            out.append(f"{name}: {before!r} must be written before the flush")
        elif name.startswith("failsafe") and text.rindex(before, 0, flush_at) > flush_at:
            out.append(f"{name}: stage record after the flush")
''','''        pos = text.rfind(before, 0, flush_at)
        if pos == -1 or flush_at - pos > 600:
            out.append(f"{name}: {before!r} must be written shortly before the flush")
''')
sub('''    check("flushAllBeforeReset" not in "".join(function_body(texts["error"]).splitlines()[1:]) or True,
          "no allocation or recursion in the body")
''','')
sub("    pre_ws = t[start - 12:start].split(\"\\n\")[-1]\n","")
sub("    # error.cpp also holds the definition line without the trailing call form\n","")
sub("    # error.cpp also holds the definition line without the trailing call form\n","") if False else None
open(p,'w').write(t)
E
python3 tests/deliberate_reset_flush_test.py**

$ cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/deliberate_reset_flush_test.py'
t=open(p).read()
def sub(a,b):
    global t; assert t.count(a)==1,a; t=t.replace(a,b)
sub('''        if before not in text[:flush_at] or text.index(before) > flush_at:
            out.append(f"{name}: {before!r} must be written before the flush")
        elif name.startswith("failsafe") and text.rindex(before, 0, flush_at) > flush_at:
            out.append(f"{name}: stage record after the flush")
''','''        pos = text.rfind(before, 0, flush_at)
        if pos == -1 or flush_at - pos > 600:
            out.append(f"{name}: {before!r} must be written shortly before the flush")
''')
sub('''    check("flushAllBeforeReset" not in "".join(function_body(texts["error"]).splitlines()[1:]) or True,
          "no allocation or recursion in the body")
''','')
sub("    pre_ws = t[start - 12:start].split(\"\\n\")[-1]\n","")
sub("    # error.cpp also holds the definition line without the trailing call form\n","")
sub("    # error.cpp also holds the definition line without the trailing call form\n","") if False else None
open(p,'w').write(t)
E
python3 tests/deliberate_reset_flush_test.py

<details>
<summary>24 lines</summary>

```
PASS: flushAllBeforeReset() is defined in State_Error.cpp
PASS: flushAllBeforeReset() is declared in state/StateMachine.h (seen by every reset site)
FAIL: it flushes C, S, N, Q, in that order (stores before the queue)
PASS: mutation caught: drop the C flush
PASS: mutation caught: drop the S flush
PASS: mutation caught: drop the N flush
PASS: mutation caught: drop the Q flush
PASS: mutation caught: queue flushed before the stores
PASS: the body allocates nothing
PASS: all seven sites call flushAllBeforeReset() immediately before the reset, after their record writes
PASS: OOM structural exclusion: the out-of-memory reset does not flush
PASS: mutation caught: call removed at thrash tier 3
PASS: mutation caught: call removed at error soft
PASS: mutation caught: call removed at error hard
PASS: mutation caught: call removed at heap guard
PASS: mutation caught: call removed at sleep exhausted
PASS: mutation caught: call removed at failsafe stage 2
PASS: mutation caught: call removed at failsafe stage 3
PASS: mutation caught: flush added at the OOM site
PASS: mutation caught: failsafe stage 2 flushes before its record
PASS: mutation caught: failsafe stage 3 flushes before its record
PASS: mutation caught: persistConnectivityFailsafeState() flushes on its own again

FAILED: 1 check(s)
<shellId: 34 completed with exit code 1>
```

</details>


---

<sub>11m 52s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 - <<'E'
import importlib.util,subprocess,tempfile,pathlib,sys
src=open('tests/deliberate_reset_flush_test.py').read().split("failures = 0")[0]
ns={}; exec(compile(src,'x','exec'),ns)
body=ns['function_body'](ns['load']()['error'])
d=tempfile.mkdtemp(); c=pathlib.Path(d)/'f.cpp'; c.write_text(ns['FAKES']+body+ns['DRIVER'].replace("{WANT}",'"C","S","N","Q"'))
r=subprocess.run(["clang++","-std=c++17",str(c),"-o",d+"/f"],capture_output=True,text=True);print(r.stderr[:800])
E**

$ cd /private/tmp/gcc-step0/dur && python3 - <<'E'
import importlib.util,subprocess,tempfile,pathlib,sys
src=open('tests/deliberate_reset_flush_test.py').read().split("failures = 0")[0]
ns={}; exec(compile(src,'x','exec'),ns)
body=ns['function_body'](ns['load']()['error'])
d=tempfile.mkdtemp(); c=pathlib.Path(d)/'f.cpp'; c.write_text(ns['FAKES']+body+ns['DRIVER'].replace("{WANT}",'"C","S","N","Q"'))
r=subprocess.run(["clang++","-std=c++17",str(c),"-o",d+"/f"],capture_output=True,text=True);print(r.stderr[:800])
E

```
Traceback (most recent call last):
  File "<stdin>", line 3, in <module>
  File "x", line 20, in <module>
NameError: name '__file__' is not defined. Did you mean: '__name__'?
<shellId: 35 completed with exit code 1>
```


---

<sub>11m 56s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 - <<'E'
import subprocess,tempfile,pathlib
src=open('tests/deliberate_reset_flush_test.py').read().split("failures = 0")[0]
ns={'__file__':str(pathlib.Path('tests/deliberate_reset_flush_test.py').resolve())}; exec(compile(src,'x','exec'),ns)
body=ns['function_body'](ns['load']()['error'])
d=tempfile.mkdtemp(); c=pathlib.Path(d)/'f.cpp'; c.write_text(ns['FAKES']+body+ns['DRIVER'].replace("{WANT}",'"C","S","N","Q"'))
r=subprocess.run(["clang++","-std=c++17",str(c),"-o",d+"/f"],capture_output=True,text=True);print(r.stderr[:800])
E**

$ cd /private/tmp/gcc-step0/dur && python3 - <<'E'
import subprocess,tempfile,pathlib
src=open('tests/deliberate_reset_flush_test.py').read().split("failures = 0")[0]
ns={'__file__':str(pathlib.Path('tests/deliberate_reset_flush_test.py').resolve())}; exec(compile(src,'x','exec'),ns)
body=ns['function_body'](ns['load']()['error'])
d=tempfile.mkdtemp(); c=pathlib.Path(d)/'f.cpp'; c.write_text(ns['FAKES']+body+ns['DRIVER'].replace("{WANT}",'"C","S","N","Q"'))
r=subprocess.run(["clang++","-std=c++17",str(c),"-o",d+"/f"],capture_output=True,text=True);print(r.stderr[:800])
E

<details>
<summary>8 lines</summary>

```
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/tmpwzcejmxk/f.cpp:20:34: error: no viable conversion from 'const char[2]' to 'const std::vector<std::string>' (aka 'const vector<basic_string<char, char_traits<char>, allocator<char>>>')
  const std::vector<std::string> want = "C","S","N","Q";
                                 ^      ~~~
/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1/vector:441:35: note: candidate constructor not viable: no known conversion from 'const char[2]' to 'const std::vector<std::string> &' for 1st argument
    _LIBCPP_CONSTEXPR_AFTER_CXX17 vector(const vector& __x);
                                  ^
/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1/vector:448:5: note: candidate constructor not viable: no known conversion
<shellId: 36 completed with exit code 0>
```

</details>


---

<sub>11m 59s</sub>

### Copilot

Braces got eaten by my placeholder; fix. Also assert is silent under mutations only via exit code, fine.

---

<sub>12m 5s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/deliberate_reset_flush_test.py'
t=open(p).read()
a='= {WANT};'; assert t.count(a)==1
t=t.replace(a,'= {@WANT@};').replace('DRIVER.replace("{WANT}"','DRIVER.replace("@WANT@"')
open(p,'w').write(t)
E
python3 tests/deliberate_reset_flush_test.py | grep -v "^PASS"; echo rc=$?**

$ cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/deliberate_reset_flush_test.py'
t=open(p).read()
a='= {WANT};'; assert t.count(a)==1
t=t.replace(a,'= {@WANT@};').replace('DRIVER.replace("{WANT}"','DRIVER.replace("@WANT@"')
open(p,'w').write(t)
E
python3 tests/deliberate_reset_flush_test.py | grep -v "^PASS"; echo rc=$?

```
deliberate_reset_flush_test: ok
rc=0
<shellId: 37 completed with exit code 0>
```


---

<sub>20m 20s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && rm -rf build-tmp/wo-2026-11-03-005-stage6/base && zsh build-tmp/wo-2026-11-03-005-stage6/runall.sh**

$ cd /private/tmp/gcc-step0/dur && rm -rf build-tmp/wo-2026-11-03-005-stage6/base && zsh build-tmp/wo-2026-11-03-005-stage6/runall.sh

```
pass=107 fail=1
tests/sleep_execution_structural_test.py
<shellId: 38 completed with exit code 0>
```


---

<sub>20m 24s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 tests/sleep_execution_structural_test.py 2>&1 | grep -v PASS | tail**

$ cd /private/tmp/gcc-step0/dur && python3 tests/sleep_execution_structural_test.py 2>&1 | grep -v PASS | tail

<details>
<summary>9 lines</summary>

```diff
+++ current
@@ -146,6 +146,7 @@
 #if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
         PowerDiagnostics::flushDiagBatch();
 #endif
+        flushAllBeforeReset();
         System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);
         
         // Should never reach here, but set ERROR_STATE as fallback
<shellId: 39 completed with exit code 0>
```

</details>


---

<sub>20m 26s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && sed -n 55,75p tests/sleep_execution_structural_test.py; grep -n "raw\b" tests/sleep_execution_structural_test.py | head -20**

$ cd /private/tmp/gcc-step0/dur && sed -n 55,75p tests/sleep_execution_structural_test.py; grep -n "raw\b" tests/sleep_execution_structural_test.py | head -20

<details>
<summary>26 lines</summary>

```
if lines[j] == "}":
            return starts[0], j
    fail(f"no closing brace for {signature!r}")


raw = SLEEP.read_text()
code = strip_comments(raw)
code_lines = code.split("\n")

# ---- 1. one declaration, in one anonymous-namespace struct ----
decl = re.findall(r"struct\s*\{[^{}]*\}\s*standbyReturn\s*;", code)
if len(decl) != 1:
    fail(f"standbyReturn must be declared once as an unnamed struct, found {len(decl)} declarations")
for field in ("preserveAfterNormalReturn", "forceModemOff", "error"):
    if len(re.findall(r"\b%s\b\s*=\s*[^;]*;\s*(?=[^{}]*\}\s*standbyReturn)" % field, decl[0])) != 1:
        fail(f"standbyReturn must declare {field} exactly once")
ns_open = raw.find("namespace {")
ns_close = raw.find("} // namespace")
decl_pos = raw.find(decl[0])
if not (0 <= ns_open < decl_pos < ns_close):
    fail("standbyReturn must be declared inside the anonymous namespace")
60:raw = SLEEP.read_text()
61:code = strip_comments(raw)
71:ns_open = raw.find("namespace {")
72:ns_close = raw.find("} // namespace")
73:decl_pos = raw.find(decl[0])
162:raw_lines = raw.split("\n")
189:if drain(raw) != drain(base_text):
<shellId: 40 completed with exit code 0>
```

</details>


---

<sub>20m 29s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/sleep_execution_structural_test.py'
t=open(p).read()
a="raw = SLEEP.read_text()\n"; assert t.count(a)==1
t=t.replace(a,a+"# WO-2026-11-03-005: the deliberate-reset flush call is the one intended addition; mapped back so every other byte is still compared.\nraw = re.sub(r\"^ *flushAllBeforeReset\\(\\);\\n\", \"\", raw, flags=re.M)\n")
open(p,'w').write(t)
E
python3 tests/sleep_execution_structural_test.py 2>&1 | grep -v PASS | tail -3; echo rc=${PIPESTATUS[0]}**

$ cd /private/tmp/gcc-step0/dur && python3 - <<'E'
p='tests/sleep_execution_structural_test.py'
t=open(p).read()
a="raw = SLEEP.read_text()\n"; assert t.count(a)==1
t=t.replace(a,a+"# WO-2026-11-03-005: the deliberate-reset flush call is the one intended addition; mapped back so every other byte is still compared.\nraw = re.sub(r\"^ *flushAllBeforeReset\\(\\);\\n\", \"\", raw, flags=re.M)\n")
open(p,'w').write(t)
E
python3 tests/sleep_execution_structural_test.py 2>&1 | grep -v PASS | tail -3; echo rc=${PIPESTATUS[0]}

```
sleep_execution_structural_test: all invariants hold
rc=0
<shellId: 41 completed with exit code 0>
```


---

<sub>20m 32s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && grep -rhn "BUILD_PATH_BASE" docs/work-orders/WO-2026-11-02-003-stage6-copilot-report.md docs/work-orders/*.md | head -5; ls /Users/chipmc/.particle/toolchains; cat project.properties; which particle**

$ cd /private/tmp/gcc-step0/dur && grep -rhn "BUILD_PATH_BASE" docs/work-orders/WO-2026-11-02-003-stage6-copilot-report.md docs/work-orders/*.md | head -5; ls /Users/chipmc/.particle/toolchains; cat project.properties; which particle

<details>
<summary>16 lines</summary>

```
15:- No other source changes. Fresh `BUILD_PATH_BASE` build; `strings` shows the version; tests `N/N (sh via zsh, py via python3)`.
17:- Fresh `BUILD_PATH_BASE` build; `strings` shows the version; record the `.bin` size (versus v42's 152558 bytes) and sha256; tests `N/N (sh via zsh, py via python3)`.
18:- Fresh `BUILD_PATH_BASE` build on the 6.6.0 toolchain; `strings` shows the version; record the `.bin` size (Step 0 predicted 152726 B) and sha256; suite `N/N`.
20:- Fresh `BUILD_PATH_BASE` build on 6.6.0; `strings` shows the version; record the `.bin` size and sha256 against v44's; suite `N/N`, serial. Open the PR; **don't merge until the soak passes.**
16:- Fresh `BUILD_PATH_BASE` build on 6.6.0 (Boron); `strings` shows the version; record the `.bin` size and sha256 against v45's (153,054 B; a small text change is expected from the S-6 move, about −8 B, and the S-5 move); suite `N/N`, serial. Open the PR; **don't merge until the soak passes.**
buildscripts
buildtools
deviceOS
gcc-arm
openocd
name=Generalized-Core-Counter
#assetOtaDir=assets
deviceOS=6.6.0
platform=boron
/Users/chipmc/bin/particle
<shellId: 42 completed with exit code 0>
```

</details>


---

<sub>20m 34s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && grep -rn "particle compile\|compile boron\|make -f\|buildscripts" docs/work-orders/WO-2026-11-02-003-stage6-copilot-report.md docs/work-orders/WO-2026-11-02-003-stage6-copilot-transcript.md tests/README.md AI_DEVELOPMENT_WORKFLOW.md | head; ls /Users/chipmc/.particle/toolchains/deviceOS /Users/chipmc/.particle/toolchains/buildscripts /Users/chipmc/.particle/toolchains/gcc-arm**

$ cd /private/tmp/gcc-step0/dur && grep -rn "particle compile\|compile boron\|make -f\|buildscripts" docs/work-orders/WO-2026-11-02-003-stage6-copilot-report.md docs/work-orders/WO-2026-11-02-003-stage6-copilot-transcript.md tests/README.md AI_DEVELOPMENT_WORKFLOW.md | head; ls /Users/chipmc/.particle/toolchains/deviceOS /Users/chipmc/.particle/toolchains/buildscripts /Users/chipmc/.particle/toolchains/gcc-arm

<details>
<summary>15 lines</summary>

```
AI_DEVELOPMENT_WORKFLOW.md:113:  Workbench's `make clean-user` (from `buildscripts/<ver>/Makefile`) only runs
AI_DEVELOPMENT_WORKFLOW.md:459:- **When:** once per release, on the **release branch head**: `particle compile
AI_DEVELOPMENT_WORKFLOW.md:483:  `particle compile boron --target 6.6.0` timed out ("Compiler timed out or
/Users/chipmc/.particle/toolchains/buildscripts:
1.10.2
1.17.2

/Users/chipmc/.particle/toolchains/deviceOS:
2.1.0
6.3.4
6.4.1
6.6.0

/Users/chipmc/.particle/toolchains/gcc-arm:
10.2.1
9.2.1
<shellId: 43 completed with exit code 0>
```

</details>


---

<sub>20m 36s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && sed -n 95,135p AI_DEVELOPMENT_WORKFLOW.md; ls /Users/chipmc/.particle/toolchains/buildscripts/1.17.2 | head**

$ cd /private/tmp/gcc-step0/dur && sed -n 95,135p AI_DEVELOPMENT_WORKFLOW.md; ls /Users/chipmc/.particle/toolchains/buildscripts/1.17.2 | head

<details>
<summary>39 lines</summary>

```
- **Delete the object file for the affected translation unit and rebuild.**
  Clearing `target/` is *not* a clean build — it holds link output only. The
  compiled objects live at
  `~/.particle/toolchains/deviceOS/<ver>/build/target/user/platform-<id>-m/<app>/`.
- **Start every build type from clean objects, or stale object files are
  reused.** A flag passed only on the command line changes no file timestamp,
  so `make` sees nothing to rebuild and silently links the previous build's
  objects. Found 2026-09-28 in WO-2026-09-28-001, when the first bench builds
  with `-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1` reused the release objects. In
  this Device OS tree (6.6.0):
  - **Agents:** use a fresh `BUILD_PATH_BASE` for every build type, in a
    directory that doesn't exist yet, for example
    `build-tmp/<wo>-stage7/obj-release` and `…/obj-bench`.
  - **Reusing a path** (for example, Chip in a terminal): run `make clean` in
    `deviceOS/6.6.0/main` with the same `PLATFORM`, `APPDIR` and
    `BUILD_PATH_BASE` arguments as the build.

  Workbench's `make clean-user` (from `buildscripts/<ver>/Makefile`) only runs
  `make clean` in `deviceOS/<ver>/main` with Workbench's own arguments, so it
  cleans only Workbench's default build path under
  `deviceOS/<ver>/build/target`. It does **not** clean a build made with the
  §3 command and its own `BUILD_PATH_BASE`. (Corrected 2026-10-07: this rule
  used to say `make clean-user`, which has no effect on agents' scratch
  builds.)
- **Keep build copies out of the repo while the host suite runs.** Structural
  tests scan the repository, and a copy of `src/` under `build-tmp/` fails
  `thermal_coupling_structural_test.py`. Run the suite before making the copy,
  or after removing it. (Temporary: remove this bullet once the test excludes
  `build-tmp/`; see the recovery plan, "Rollout and follow-ups".)
- Prove presence/absence with `nm` on the linked ELF **and** on the object
  itself. Never with `strings`.
- Record the `text`/`data` sizes at both flag values. A size that matches the
  *other* flag setting is the signature of a stale link.

**Prohibited:**

- **Restoring a build-config file in any way that rewrites its timestamp
  backwards.** `mv config.h.bak config.h` is the known example, but the rule is
  general: `cp -p`, `git stash pop`, `rsync -t`, archive extraction, and editor
  "revert file" can all restore content while preserving or backdating mtime.
Makefile
<shellId: 44 completed with exit code 0>
```

</details>


---

<sub>20m 38s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && grep -n "§3\|^## 3" AI_DEVELOPMENT_WORKFLOW.md | head; grep -n "make -f\|PLATFORM=\|APPDIR" AI_DEVELOPMENT_WORKFLOW.md | head**

$ cd /private/tmp/gcc-step0/dur && grep -n "§3\|^## 3" AI_DEVELOPMENT_WORKFLOW.md | head; grep -n "make -f\|PLATFORM=\|APPDIR" AI_DEVELOPMENT_WORKFLOW.md | head

<details>
<summary>8 lines</summary>

```
117:  §3 command and its own `BUILD_PATH_BASE`. (Corrected 2026-10-07: this rule
297:## 3. Standard workflow
555:`codex review --uncommitted` is not used: it rejects custom instructions (CLI 0.154.0). Codex's sandbox has no network, so the Particle cloud compile (a release-head check, once per release; see "Release gate" in §3) is run by Claude Code, not in Stage 7.
752:5. **Verify the binary, not the source, for anything involving vendored libraries.** A library that is both vendored in `lib/` and listed in `project.properties` may be replaced by the registry copy in a cloud build. Confirm the change is present in the binary that will be flashed (disassembly, symbols, or strings), not only in the source tree. Local and cloud builds may use different library copies. Bench-test the build type the fleet will receive. The cloud compile that checks this runs once per release, on the release branch head (§3, "Release gate").
808:- the Particle cloud compile (once per release, on the release branch head; §3);
110:    `deviceOS/6.6.0/main` with the same `PLATFORM`, `APPDIR` and
433:    make -s PLATFORM=boron APPDIR=<copy-of-tree> TARGET_DIR=<copy>/localbuild \
<shellId: 45 completed with exit code 0>
```

</details>


---

<sub>20m 40s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && sed -n 415,458p AI_DEVELOPMENT_WORKFLOW.md**

$ cd /private/tmp/gcc-step0/dur && sed -n 415,458p AI_DEVELOPMENT_WORKFLOW.md

<details>
<summary>34 lines</summary>

```
#### Mandatory: local toolchain build

Every change must be built with the **local Device OS toolchain** before Stage 7
can pass. The Particle cloud compile is **not** a per-change check (changed
2026-10-25, Chip): it runs once per release, on the release branch head (see
"Release gate: the Particle cloud compile", below).

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
same collision class in round 3 and it was recorded as an environment quirk
rather than a defect class. It was found only when the engineer tried to flash a
device.

The general lesson, which is the same one behind the linkage check above:
**verifying with an instrument that cannot see the failure is not verification.**

#### Release gate: the Particle cloud compile (once per release)

*Standing rule, Chip, 25 Oct 2026. It replaces the earlier rule that every change
needed both a local and a cloud build.*
<shellId: 46 completed with exit code 0>
```

</details>


---

<sub>24m 37s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && ls /Users/chipmc/.particle/toolchains/buildtools; W=$PWD/build-tmp/wo-2026-11-03-005-stage6; mkdir -p $W/tree && cp -R src lib project.properties $W/tree/ && export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && cd ~/.particle/toolchains/deviceOS/6.6.0/main && (time make -s PLATFORM=boron APPDIR=$W/tree TARGET_DIR=$W/tree/localbuild DEVICE_OS_PATH=$HOME/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE=$W/obj-release) > $W/build.log 2>&1; echo rc=$?; tail -15 $W/build.log**

$ cd /private/tmp/gcc-step0/dur && ls /Users/chipmc/.particle/toolchains/buildtools; W=$PWD/build-tmp/wo-2026-11-03-005-stage6; mkdir -p $W/tree && cp -R src lib project.properties $W/tree/ && export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && cd ~/.particle/toolchains/deviceOS/6.6.0/main && (time make -s PLATFORM=boron APPDIR=$W/tree TARGET_DIR=$W/tree/localbuild DEVICE_OS_PATH=$HOME/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE=$W/obj-release) > $W/build.log 2>&1; echo rc=$?; tail -15 $W/build.log

<details>
<summary>17 lines</summary>

```
1.1.1
rc=0
/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6/tree/lib/LocalTimeRK/src/LocalTimeRK.cpp: In member function 'void LocalTimeConvert::prevDay(LocalTimeHMS)':
/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6/tree/lib/LocalTimeRK/src/LocalTimeRK.cpp:1244:12: warning: unused variable 'origTime' [-Wunused-variable]
 1244 |     time_t origTime = time;
      |            ^~~~~~~~
/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6/tree/lib/LocalTimeRK/src/LocalTimeRK.cpp: In member function 'void LocalTimeConvert::nextDay(LocalTimeHMS)':
/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6/tree/lib/LocalTimeRK/src/LocalTimeRK.cpp:1255:12: warning: unused variable 'origTime' [-Wunused-variable]
 1255 |     time_t origTime = time;
      |            ^~~~~~~~
Creating /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6/tree/localbuild/platform_user_ram.ld ...
   text	   data	    bss	    dec	    hex	filename
 151692	   1114	   2200	 155006	  25d7e	/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6/tree/localbuild/tree.elf

real	3m53.677s
user	2m41.916s
sys	0m31.875s
<shellId: 47 completed with exit code 0>
```

</details>


---

<sub>24m 43s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && E=tree/localbuild/tree.elf && arm-none-eabi-nm -C $E | grep -i "flushAllBeforeReset\|flushNow"; arm-none-eabi-objdump -d -C $E | awk '/^[0-9a-f]+ <.*>:$/ {fn=$0} /bl.*<flushAllBeforeReset\(\)>/ {print fn}' ; ls -la tree/localbuild/*.bin; shasum -a 256 tree/localbuild/*.bin**

$ cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && E=tree/localbuild/tree.elf && arm-none-eabi-nm -C $E | grep -i "flushAllBeforeReset\|flushNow"; arm-none-eabi-objdump -d -C $E | awk '/^[0-9a-f]+ <.*>:$/ {fn=$0} /bl.*<flushAllBeforeReset\(\)>/ {print fn}' ; ls -la tree/localbuild/*.bin; shasum -a 256 tree/localbuild/*.bin

<details>
<summary>14 lines</summary>

```
000c1b9c T flushAllBeforeReset()
000b6d2c T SystemConfig::SensorSettings::flushNow()
000b6cac T SystemConfig::flushNow()
000b6d6e T CurrentReadings::flushNow()
000b83d8 <ThrashGuard::loop(int, unsigned long)>:
000bdbb0 <connectivityFailsafeSupervisor()>:
000bdbb0 <connectivityFailsafeSupervisor()>:
000c1bb8 <handleErrorState()>:
000c1bb8 <handleErrorState()>:
000c2f78 <handleSleepingState()>:
000c2f78 <handleSleepingState()>:
-rwxr-xr-x  1 chipmc  wheel  152810 Nov  3 16:08 tree/localbuild/tree.bin
8245ea0ce7c38b0e31cb479505ff0973dc8cef2f4a9036ecd950704e2370c38d  tree/localbuild/tree.bin
<shellId: 48 completed with exit code 0>
```

</details>


---

<sub>24m 48s</sub>

### `bash` — Failed

**cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && (arm-none-eabi-nm -C tree/localbuild/tree.elf | grep -i "flushAllBeforeReset\|flushNow"; arm-none-eabi-objdump -d -C tree/localbuild/tree.elf | awk '/^[0-9a-f]+ <.*>:$/ {fn=$0} /bl.*<flushAllBeforeReset\(\)>/ {print fn}') > nm-evidence.txt; cp tree/localbuild/tree.bin ../ 2>/dev/null; rm -f ../tree.bin; rm -rf tree obj-release; cd ../.. && zsh build-tmp/wo-2026-11-03-005-stage6/runall.sh; git status --short; git diff --numstat -- src**

$ cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && (arm-none-eabi-nm -C tree/localbuild/tree.elf | grep -i "flushAllBeforeReset\|flushNow"; arm-none-eabi-objdump -d -C tree/localbuild/tree.elf | awk '/^[0-9a-f]+ <.*>:$/ {fn=$0} /bl.*<flushAllBeforeReset\(\)>/ {print fn}') > nm-evidence.txt; cp tree/localbuild/tree.bin ../ 2>/dev/null; rm -f ../tree.bin; rm -rf tree obj-release; cd ../.. && zsh build-tmp/wo-2026-11-03-005-stage6/runall.sh; git status --short; git diff --numstat -- src

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>24m 51s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && ls; cat nm-evidence.txt 2>/dev/null | head -3**

$ cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && ls; cat nm-evidence.txt 2>/dev/null | head -3

```
build.log
obj-release
runall.sh
tree
<shellId: 50 completed with exit code 0>
```


---

<sub>24m 54s</sub>

### `bash` — Failed

**cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && arm-none-eabi-nm -C tree/localbuild/tree.elf | grep -i "flushAllBeforeReset" > nm-evidence.txt; arm-none-eabi-objdump -d -C tree/localbuild/tree.elf | awk '/^[0-9a-f]+ <.*>:$/ {fn=$0} /bl.*<flushAllBeforeReset\(\)>/ {print "caller: " fn}' >> nm-evidence.txt; cat nm-evidence.txt | wc -l; mv tree ../../../../../../tmp/wo005-tree-$$ 2>&1 | head -2; ls**

$ cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && arm-none-eabi-nm -C tree/localbuild/tree.elf | grep -i "flushAllBeforeReset" > nm-evidence.txt; arm-none-eabi-objdump -d -C tree/localbuild/tree.elf | awk '/^[0-9a-f]+ <.*>:$/ {fn=$0} /bl.*<flushAllBeforeReset\(\)>/ {print "caller: " fn}' >> nm-evidence.txt; cat nm-evidence.txt | wc -l; mv tree ../../../../../../tmp/wo005-tree-$$ 2>&1 | head -2; ls

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>24m 56s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && arm-none-eabi-nm -C tree/localbuild/tree.elf | grep -i "flushAllBeforeReset" > nm-evidence.txt; arm-none-eabi-objdump -d -C tree/localbuild/tree.elf | awk '/^[0-9a-f]+ <.*>:$/ {fn=$0} /bl.*<flushAllBeforeReset\(\)>/ {print "caller: " fn}' >> nm-evidence.txt; cat nm-evidence.txt**

$ cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && arm-none-eabi-nm -C tree/localbuild/tree.elf | grep -i "flushAllBeforeReset" > nm-evidence.txt; arm-none-eabi-objdump -d -C tree/localbuild/tree.elf | awk '/^[0-9a-f]+ <.*>:$/ {fn=$0} /bl.*<flushAllBeforeReset\(\)>/ {print "caller: " fn}' >> nm-evidence.txt; cat nm-evidence.txt

<details>
<summary>9 lines</summary>

```
000c1b9c T flushAllBeforeReset()
caller: 000b83d8 <ThrashGuard::loop(int, unsigned long)>:
caller: 000bdbb0 <connectivityFailsafeSupervisor()>:
caller: 000bdbb0 <connectivityFailsafeSupervisor()>:
caller: 000c1bb8 <handleErrorState()>:
caller: 000c1bb8 <handleErrorState()>:
caller: 000c2f78 <handleSleepingState()>:
caller: 000c2f78 <handleSleepingState()>:
<shellId: 52 completed with exit code 0>
```

</details>


---

<sub>24m 59s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && rm -r tree obj-release; ls**

$ cd /private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage6 && rm -r tree obj-release; ls

```
build.log
nm-evidence.txt
runall.sh
<shellId: 53 completed with exit code 0>
```


---

<sub>31m 0s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && zsh build-tmp/wo-2026-11-03-005-stage6/runall.sh; git status --short; git diff --numstat -- src**

$ cd /private/tmp/gcc-step0/dur && zsh build-tmp/wo-2026-11-03-005-stage6/runall.sh; git status --short; git diff --numstat -- src

<details>
<summary>28 lines</summary>

```
pass=108 fail=0

 M src/MyPersistentData.cpp
 M src/ThrashGuard.cpp
 M src/persist/CurrentReadings.h
 M src/persist/SystemConfig.h
 M src/power/ConnectivityFailsafe.cpp
 M src/state/StateMachine.h
 M src/state/State_Error.cpp
 M src/state/State_Sleep.cpp
 M tests/connectivity_failsafe_unknown_openness_test.cpp
 M tests/nightly_heap_guard_flush_test.sh
 M tests/sleep_cloud_gate_structural_test.py
 M tests/sleep_execution_structural_test.py
 M tests/sleep_plan_equivalence_test.sh
 M tests/sleep_plan_structural_test.py
 M tests/sleep_preflight_structural_test.py
 M tests/sleep_wake_structural_test.py
 M tests/stubs/sleep_teardown/SleepExecutionFakes.h
?? tests/deliberate_reset_flush_test.py
2	0	src/MyPersistentData.cpp
1	0	src/ThrashGuard.cpp
1	0	src/persist/CurrentReadings.h
1	0	src/persist/SystemConfig.h
3	1	src/power/ConnectivityFailsafe.cpp
3	0	src/state/StateMachine.h
9	0	src/state/State_Error.cpp
2	0	src/state/State_Sleep.cpp
<shellId: 54 completed with exit code 0>
```

</details>


---

<sub>31m 6s</sub>

### `bash`

**cd /private/tmp/gcc-step0/dur && awk 'NR<=234 && /^(public|protected|private)|^ *(public|protected|private):/ {l=NR": "$0} END{print l}' lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h; grep -n "static PublishQueuePosix &instance" lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h; grep -n "PublishQueuePosix::instance()\.\(setup\|withRamQueueSize\)" src/*.cpp | head -3; grep -n "flush(bool" -A12 lib/*/src/PersistentDataFile.cpp 2>/dev/null | head -20**

$ cd /private/tmp/gcc-step0/dur && awk 'NR<=234 && /^(public|protected|private)|^ *(public|protected|private):/ {l=NR": "$0} END{print l}' lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h; grep -n "static PublishQueuePosix &instance" lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h; grep -n "PublishQueuePosix::instance()\.\(setup\|withRamQueueSize\)" src/*.cpp | head -3; grep -n "flush(bool" -A12 lib/*/src/PersistentDataFile.cpp 2>/dev/null | head -20

```
48: public:
55:    static PublishQueuePosix &instance();
<shellId: 55 completed with exit code 0>
```


---

<sub>31m 27s</sub>

### `create`

**create a new file at /private/tmp/gcc-step0/dur/docs/work-orders/WO-2026-11-03-005-stage6-copilot-report.md.**

/private/tmp/gcc-step0/dur/docs/work-orders/WO-2026-11-03-005-stage6-copilot-report.md

```
Created file /private/tmp/gcc-step0/dur/docs/work-orders/WO-2026-11-03-005-stage6-copilot-report.md with 6470 characters
```


---

<sub>31m 34s</sub>

### Copilot

Implemented, but `src/` came to +21 net against the +20 limit. Nothing is committed.

- **Budget:** The extra line is the doc comment on the `flushAllBeforeReset()` declaration in `StateMachine.h`. I left it rather than trim style. Dropping it brings the total to +20.
- **Missing file:** `WO-2026-11-03-002-step0-report.md` is not in the tree, so I worked from the WO text and the cited sites.
- **Alert codes:** `RecoveryState` alert codes are in the `current` store, so they are flushed. Both the thrash alert and the Error alert are written before the flush.
- **Queue:** `PublishQueuePosix::instance().writeQueueToFiles()` is public and synchronous through the app's singleton.
- **What changed:**
  - I added the two store facades, `CurrentReadings::flushNow` and `SystemConfig::SensorSettings::flushNow`.
  - `flushAllBeforeReset()` lives in `State_Error.cpp` and flushes the three stores and then the queue.
  - It is called at the seven non-OOM sites: thrash tier 3, Error soft and hard, heap guard, sleep-exhausted, and failsafe stages 2 and 3.
  - At failsafe stages 2 and 3 it runs after the stage record, replacing the old `flushNow()`. I kept `SystemConfig::flushNow()` on the low-battery defer branch so that path behaves as before.
- **Tests:**
  - New `tests/deliberate_reset_flush_test.py` covers the four flushes and their order, the seven call sites, OOM exclusion, and the mutations.
  - I updated eight existing sleep, heap-guard and failsafe tests/stubs to account for the new call.
  - The host suite passes 108/108.
- **Build:** The Device OS 6.6.0 build with a fresh `BUILD_PATH_BASE` succeeded. `nm` shows `flushAllBeforeReset()` and exactly seven `bl` callers: `ThrashGuard::loop` (1), `connectivityFailsafeSupervisor` (2), `handleErrorState` (2) and `handleSleepingState` (2). `loop()`, where the OOM reset sits, has none.
- **Not done:** Flush durations are unmeasured, and no Dev-14 bench was run.
- **Report:** The reported model is as I know it. The CLI doesn't show me the reasoning setting, so I couldn't confirm medium.

The report is `docs/work-orders/WO-2026-11-03-005-stage6-copilot-report.md`. It lists net `src/` per file, test names, mutation results, `nm` evidence, Step 0 findings and open questions.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>
