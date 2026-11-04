# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `0b98fc05-0bb2-4da3-b13f-7ca87e0d7c34`  
> - **Started:** 11/3/2026, 9:57:40 AM  
> - **Duration:** 14m 38s  
> - **Exported:** 11/3/2026, 10:12:18 AM  

---

<sub>5s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-02-004-stage6-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>7s</sub>

### `view`

**view the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/docs/work-orders/WO-2026-11-02-004-stage6-copilot-dispatch.md.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/docs/work-orders/WO-2026-11-02-004-stage6-copilot-dispatch.md

<details>
<summary>14 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/ConfigApply.cpp`, `src/ResetCause.h` (and a call-site only if the WO requires it), tests under `tests/`, local build/test per the WO, scratch directory `build-tmp/wo-2026-11-02-004-stage6/` / NOT authorized: committing, pushing, merging, flashing, deleting anything you didn't create (including `build-tmp/` itself), changing the persisted layout or `retained` RAM, any live re-apply of the timezone, malformed-string handling.

# Stage 6 dispatch — WO-2026-11-02-004 (recovery item 25: planned reset on a changed timezone)

**Binding spec:** `docs/work-orders/WO-2026-11-02-004-timezone-applies-to-running-clock.md` (architect's design: a planned reset, forced flush first). Branch `wo/2026-11-02-004-timezone-planned-reset`, base `21804e3` (main, v48). Leave all changes uncommitted. **Budget: `src/` at most +10 net lines, hard.** Stop and report rather than trimming style.

## Do
Implement the WO's Change section: `RESET_CAUSE_TIMEZONE_CHANGE = 8` in `ResetCause.h`; a file-local flag set where `ConfigApply.cpp` stores a changed timezone; in the success branch of `applyConfigurationFromLedger` after `validateStoredData()`, `SystemConfig::flushNow()` then `System.reset(RESET_CAUSE_TIMEZONE_CHANGE)` (the failsafe pairing). Re-read the cited lines on this base first: the line numbers in the WO were taken on `a7b69ce` and may have moved. Also check: is `applyConfigurationFromLedger` ever reached from `setup()` before `LocalTime` is configured (WO Stage 7 check a)? If it would reset needlessly there, report it; do not redesign.

## Tests
Per the WO's Tests section: host tests for the order (flush before reset), no reset when unchanged, no reset on a failed apply, no flag when only other sections change; extend `tests/reset_cause_structural_test.py` for code 8. Add the listed mutations as a documented check (apply each, show the test fails, revert). Update `docs/reference/alert-codes.md` only if the WO's change touches an alert (it does not expect to).

## Verify
Host suite under zsh (`*.sh`) and python3 (`*.py`), reporting `N/N (sh via zsh, py via python3)`. Local toolchain build per `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 7 "local toolchain build" with a fresh `BUILD_PATH_BASE`; confirm with `nm` that the reset path is present in the symbolised ELF (linkage check). `git diff --stat -- src/` must show ≤ +10 net.

## Stop and report if
the design can't be done in +10, the flush API differs from the failsafe's usage, or the reset would fire from a path the WO didn't expect.

## Report (write to `docs/work-orders/WO-2026-11-02-004-stage6-copilot-report.md`)
Model and reasoning actually used; interpreter line; net `src/` lines per file; test names; mutation results; build and `nm` evidence; open questions.
```

</details>


---

<sub>9s</sub>

### `bash` — Failed

**cd docs/work-orders && cat WO-2026-11-02-004-timezone-applies-to-running-clock.md; git status --short; git log --oneline -3; cat ../../src/ResetCause.h; ls ../../tests | head -50**

$ cd docs/work-orders && cat WO-2026-11-02-004-timezone-applies-to-running-clock.md; git status --short; git log --oneline -3; cat ../../src/ResetCause.h; ls ../../tests | head -50

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>11s</sub>

### `view`

**view the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/docs/work-orders/WO-2026-11-02-004-timezone-applies-to-running-clock.md.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/docs/work-orders/WO-2026-11-02-004-timezone-applies-to-running-clock.md

<details>
<summary>30 lines</summary>

```
# WO-2026-11-02-004: a changed timezone stored by ConfigApply triggers a planned reset (recovery item 25; v49 must-fix, gates wave 1; design: planned reset)

**Author:** Claude Code, 2 Nov 2026, DRAFT for the architect's review · **Status:** DRAFT for review; dispatch tomorrow (3 Nov) with the GitHub Actions WO (-003) and the spike
**Branch:** `wo/2026-11-02-004-timezone-planned-reset`, from `main`
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-7; §5 (time and boundaries: Codex `gpt-5.6-sol`, xhigh); §13 (c), (j)
**Budget:** `src/` at most **+10 net**, hard. Stop and report rather than trim style.
**Agents (architect, 2 Nov):** Stage 6 Copilot standard model, medium. Stage 7 Codex `gpt-5.6-sol`, xhigh (time and boundaries, §5), a new output file per round, **at most two rounds**.

## Plain goal
A migrated device stores `UTC0` by default, applies `EST5EDT` from the ledger on its first connect, and then runs its day logic in UTC until something reboots it (ChurchSt-2: last report 22:00Z, silent until 06:01Z, an occupancy session counted at 04:29 Eastern). When `ConfigApply` stores a changed timezone, reboot once through the deliberate-reset path so `setup()` configures `LocalTime` from it.

## The defect (OBS, main `a7b69ce`; read, not run)
- `setup()` configures the converter once from the stored string: `Generalized-Core-Counter.cpp:927-932` (`LocalTime::instance().withConfig(LocalTimePosixTimezone(tz))`).
- `ConfigApply` stores a changed timezone and logs it, nothing more: `ConfigApply.cpp:253-255` (`SystemConfig::set_timeZoneStr(timezone)`, `Log.info("Config: Timezone -> %s")`, `changed = true`). `ConfigApply.cpp` does not include `LocalTimeRK.h`.
- Fresh default is `UTC0`: `Config.h:30`, `MyPersistentData.cpp:133`.
- Every reader takes the config live from `LocalTime::instance().getConfig()` (`Clock.cpp:69`, `ClockDiagnostics.cpp:49`, `DailyBoundary.cpp:11`), and `LocalTimeCache` refreshes when that config's signature changes (`LocalTimeCache.cpp:48-60`), so one `withConfig` call updates all of them.
- Field evidence: ChurchSt-2 and LAB-01 both `config.generation` `EF590054`; LAB-01 follows Eastern only because it rebooted after the apply (pilot record, 24 h checks).

## Change (architect design ruling, 2 Nov: a planned reset, not a live re-apply)
When `ConfigApply` stores a changed timezone, **reset the device through the existing deliberate-reset path with a new reset cause**; the boot path then reads the stored timezone in `setup()`. Rationale: the boot path is field-proven (LAB-01's 15:30Z reset applied Eastern correctly), and a reboot re-reads every boot-only setting at once, so no live re-apply and no list of boot-only settings to chase.
- **New cause:** `RESET_CAUSE_TIMEZONE_CHANGE = 8` in `ResetCause.h` (+1); the reset surfaces as reset 140, data 8 (`ResetCause.h` header comment; the structural test `reset_cause_structural_test.py` counts the sites and values, extend it).
- **Where:** `ConfigApply.cpp`. Record that the timezone was stored (`ConfigApply.cpp:253-255`, a file-local flag), and in `applyConfigurationFromLedger` (`:123-171`), in the success branch after `validateStoredData()`, if the flag is set: **`SystemConfig::flushNow()` then `System.reset(RESET_CAUSE_TIMEZONE_CHANGE)`**, the same pairing the failsafe uses (`ConnectivityFailsafe.cpp:86,149,306`). The comment at `ConfigApply.cpp:144-146` says storage is normally flushed lazily by `SystemConfig::loop()`; a reset right after `set_timeZoneStr` without the forced flush would lose the value and loop (the ledger would re-apply on every boot).
- **Not in this WO:** malformed strings (architect: out of scope; the existing normalization and length check stay), any live re-apply.
- Estimate **+8 to +10** `src/` (enum +1, flag +2, reset block +4 to +5, comment). Budget at most **+10**.

## Stage 7 checks (two, architect ruling)
(a) **No reboot loop:** the reset happens only on a real change (the existing `strcmp` against the stored value, `:252`), the value is stored **and flushed** before the reset, and after the reboot the same ledger value finds no change. Walk: first connect with `UTC0` stored and `EST5EDT` in the ledger (one reset, then none); ledger equals stored (none); the apply called from `setup()` before the converter is configured (`Config.cpp` path) — does it reset needlessly?; a failed section (`success` false) must not reset; a device-level override that flips the zone back and forth (each real change resets once).
(b) **Daily totals and an open session survive,** as on failsafe and hibernate resets: `dailyoccupancy`, the open-session state and the publish queue after `System.reset(8)`; compare with the failsafe stage-2 path.

## Tests
- Host: the apply path with a changed zone flags the reset and flushes before it (order); unchanged zone does not; a failed apply does not; the flag is not set when only other sections change. `reset_cause_structural_test.py`: code 8 declared, distinct and non-zero, site passes it.
- Mutations: reset without the flush; reset on every apply; reset when the zone is unchanged; reset on a failed apply; cause 0 or a reused code.

## Field confirmation (item 25)
The v48 OTA after tonight's default decision reboots ChurchSt-2, which should then follow Eastern (hourly through 03:00Z, quiet until 11:00Z on 3 Nov). **Record the result as item 25's field confirmation** (the device has no remote reset). The fix itself is confirmed on a v49 device that applies a timezone change without rebooting.

## Waves
**Wave 1 moves to v49** (architect, 2 Nov).
```

</details>


---

<sub>11s</sub>

### `view`

**view the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/ResetCause.h.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/ResetCause.h

<details>
<summary>21 lines</summary>

```
#pragma once

#include <stdint.h>

/**
 * @file ResetCause.h
 * @brief Distinct, non-zero codes passed to System.reset(uint32_t data).
 *
 * Device OS 6.4.1 resets with RESET_REASON_USER (140) and hands `data` back to
 * the next boot as System.resetReasonData(), which the startup status already
 * publishes. So reason 140 with data 0 means the reset came from outside these
 * sites (the AB1805 library fallback, a cloud reset, or Device OS itself).
 * WO-2026-10-01-001 item C.
 */
enum ResetCause : uint32_t {
  RESET_CAUSE_APP_WATCHDOG = 1,            ///< appWatchdogHandler() (non-Wiring_Watchdog builds)
  RESET_CAUSE_CONNECTIVITY_FAILSAFE = 2,   ///< Connectivity failsafe stage 2
  RESET_CAUSE_THRASH_TIER3 = 3,            ///< ThrashGuard tier 3
  RESET_CAUSE_SLEEP_HEAP_GUARD = 4,        ///< Nightly heap guard before overnight sleep
  RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED = 5,///< All sleep attempts failed
  RESET_CAUSE_ERROR_STATE_SOFT = 6,        ///< ERROR_STATE soft reset
  RESET_CAUSE_OUT_OF_MEMORY = 7,           ///< out of memory
};
```

</details>


---

<sub>11s</sub>

### `glob`

**tests/***

"tests/*"

<details>
<summary>168 matches</summary>

```
[glob: 165 path(s) under ./tests; grouped by directory]

./tests/ (165 path(s))
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
  wake_gate_single_owner_structural_test.py
  wake_summary_led_test.cpp
  wake_summary_led_test.sh
  watchdog_ab1805_classification_test.cpp
  watchdog_ab1805_classification_test.sh
  watchdog_alert_code_test.cpp
  watchdog_alert_code_test.sh
  webhook_timeout_range_test.py

[extensions: .sh=61, .cpp=57, .py=46, .md=1]
```

</details>


---

<sub>14s</sub>

### `view` — Failed

**view the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/ConfigApply.cpp.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/ConfigApply.cpp (lines 1-300)

```
Path does not exist
```


---

<sub>14s</sub>

### `view`

**view the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/reset_cause_structural_test.py.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/reset_cause_structural_test.py

<details>
<summary>124 lines</summary>

```
#!/usr/bin/env python3
"""
WO-2026-10-01-001 item C - after any firmware-issued reset, the next startup
status must name the code that issued it.

Evidence: PCKL1 sent `resetReason=140 resetReasonData=0` after an unexplained
hour of silence. Reason 140 is `System.reset()`, and six call sites in `src/`
can issue it; nothing recorded which one did. Device OS 6.4.1's
`System.reset(uint32_t data)` hands `data` to the next boot as
`System.resetReasonData()`, which the startup status already publishes.

This is a structural test. The behavior it guards (a value surviving a reset
into the next boot) lives in Device OS, so a host harness cannot execute it;
what can be pinned is the shape:

  1. Every `System.reset(` call in `src/` passes a cause code - a bare
     `System.reset()` fails.
  2. The codes are distinct (no two sites share one) and non-zero, because 0 is
     reserved to mean "the reset came from outside our src/ reset sites".
  3. Every code used is declared in the ResetCause enum, and the enum's own
     values are distinct and non-zero.
  4. The out-of-memory site in `loop()` resets directly with
     `RESET_CAUSE_OUT_OF_MEMORY` (7) - WO-2026-10-02-003 item B.
  5. The startup status payload still carries `resetReasonData`.

Comments are stripped before each check so prose mentioning `System.reset()`
cannot produce a false positive or a false negative.
"""
import re
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


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments(code: str) -> str:
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def source_files():
    for path in sorted(SRC.rglob("*")):
        if path.suffix in (".cpp", ".h") and path.is_file():
            yield path


def main() -> None:
    if not RESET_CAUSE_HEADER.exists():
        fail(f"{RESET_CAUSE_HEADER.relative_to(REPO_ROOT)} is missing")

    # --- Enum: distinct, non-zero -------------------------------------------
    header = strip_comments(RESET_CAUSE_HEADER.read_text())
    enum_body = re.search(r"enum\s+ResetCause\s*:[^{]*\{(.*?)\}", header, re.DOTALL)
    if not enum_body:
        fail("ResetCause.h does not declare an `enum ResetCause`")
    declared = dict(
        (name, int(value))
        for name, value in re.findall(r"(RESET_CAUSE_\w+)\s*=\s*(\d+)", enum_body.group(1))
    )
    if not declared:
        fail("the ResetCause enum declares no explicitly-valued cause codes")
    zero_valued = [name for name, value in declared.items() if value == 0]
    if zero_valued:
        fail(
            f"reset cause code(s) {sorted(zero_valued)} are 0; 0 is reserved for "
            "resets issued outside src/"
        )
    seen = {}
    for name, value in declared.items():
        if value in seen:
            fail(f"reset cause codes {seen[value]} and {name} share the value {value}")
        seen[value] = name

    # --- Call sites: every one carries a distinct code -----------------------
    sites = []
    for path in source_files():
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
    if declared.get(OOM_CAUSE_NAME) != OOM_CAUSE_VALUE:
        fail(
            f"ResetCause.h must declare {OOM_CAUSE_NAME} = {OOM_CAUSE_VALUE} "
            f"(found {declared.get(OOM_CAUSE_NAME)!r})"
        )
    if OOM_CAUSE_NAME not in used:
        fail(
            f"no System.reset() site passes {OOM_CAUSE_NAME}; the out-of-memory "
            "handler's reset must name itself (WO-2026-10-02-003 item B)"
        )

    # --- The startup status still publishes the code -------------------------
    main_src = strip_comments(MAIN.read_text())
    if "System.resetReasonData()" not in main_src:
        fail("the startup path no longer reads System.resetReasonData()")
    if not re.search(r'\\"resetReasonData\\":', main_src):
        fail("the startup status payload no longer carries the resetReasonData field")

    print(
        f"Reset cause structural test passed ({len(sites)} call sites, distinct non-zero "
        "codes, resetReasonData still published)"
    )


if __name__ == "__main__":
    main()
```

</details>


---

<sub>14s</sub>

### `bash`

**Check git state and callers**

$ git log --oneline -3; git status --short; grep -rn "applyConfigurationFromLedger" src | head; ls build-tmp 2>&1 | head

<details>
<summary>13 lines</summary>

```
21804e3 Merge pull request #169 from See-Insights/release/v48
3daa503 docs: v48 full-day run PASS on Dev-09 and Dev-14; recommend v48 as the default
316acfc docs: v48 soak — 24 h grading PASS; session check now mean 9.5 ± 0.1 min over >= 40 sessions; alert 40 and slow connect on the watch list
?? docs/work-orders/WO-2026-11-02-004-stage6-copilot-dispatch.md
?? docs/work-orders/WO-2026-11-02-004-timezone-applies-to-running-clock.md
src/cloud/Cloud.h:106:     * mechanism `ConfigApply.cpp`'s `applyConfigurationFromLedger()` uses
src/cloud/Cloud.h:241:    bool applyConfigurationFromLedger(const LedgerData &defaults, const LedgerData &device);
src/cloud/Cloud.h:377:     * @brief Tracks success of last applyConfigurationFromLedger() call
src/cloud/Cloud.cpp:637:    lastApplySuccess = applyConfigurationFromLedger(defaults, device);
src/cloud/Cloud.cpp:682:        lastApplySuccess = applyConfigurationFromLedger(defaults, device);
src/cloud/ConfigApply.cpp:122:bool Cloud::applyConfigurationFromLedger(const LedgerData &defaults, const LedgerData &device) {
copilot.out
<shellId: 1 completed with exit code 0>
```

</details>


---

<sub>17s</sub>

### `view`

**view the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp (lines 1-290)

<details>
<summary>244 lines</summary>

```
#include "cloud/Cloud.h"

#include "../Config.h"
#include "power/ChargeInhibitPolicy.h"
#include "persist/PowerConfig.h"
#include "persist/SystemConfig.h"

namespace {

bool getTopLevelMap(const LedgerData &ledger, const char *key, Variant &section) {
    if (!ledger.has(key)) {
        return false;
    }

    section = ledger.get(key);
    return section.isMap();
}

bool getNestedMap(const LedgerData &ledger, const char *parentKey, const char *childKey, Variant &section) {
    Variant parent;
    if (!getTopLevelMap(ledger, parentKey, parent) || !parent.has(childKey)) {
        return false;
    }

    section = parent.get(childKey);
    return section.isMap();
}

bool getMergedIntValue(const Variant &defaultsSection, const Variant &deviceSection, const char *key, int &value) {
    if (deviceSection.isMap() && deviceSection.has(key)) {
        value = deviceSection.get(key).toInt();
        return true;
    }

    if (defaultsSection.isMap() && defaultsSection.has(key)) {
        value = defaultsSection.get(key).toInt();
        return true;
    }

    return false;
}

bool getMergedBoolValue(const Variant &defaultsSection, const Variant &deviceSection, const char *key, bool &value) {
    if (deviceSection.isMap() && deviceSection.has(key)) {
        value = deviceSection.get(key).toBool();
        return true;
    }

    if (defaultsSection.isMap() && defaultsSection.has(key)) {
        value = defaultsSection.get(key).toBool();
        return true;
    }

    return false;
}

bool getMergedFloatValue(const Variant &defaultsSection, const Variant &deviceSection, const char *key, float &value) {
    if (deviceSection.isMap() && deviceSection.has(key)) {
        value = static_cast<float>(deviceSection.get(key).toDouble());
        return true;
    }

    if (defaultsSection.isMap() && defaultsSection.has(key)) {
        value = static_cast<float>(defaultsSection.get(key).toDouble());
        return true;
    }

    return false;
}

bool getMergedStringValue(const Variant &defaultsSection,
                          const Variant &deviceSection,
                          const char *key,
                          char *buffer,
                          size_t bufferSize) {
    Variant value;
    if (deviceSection.isMap() && deviceSection.has(key)) {
        value = deviceSection.get(key);
    } else if (defaultsSection.isMap() && defaultsSection.has(key)) {
        value = defaultsSection.get(key);
    } else {
        return false;
    }

    String text = value.toString();
    if (text.length() >= bufferSize) {
        return false;
    }

    text.toCharArray(buffer, bufferSize);
    return true;
}

void normalizeTimezoneInPlace(char *timezone, size_t timezoneSize) {
    if (!timezone || timezoneSize == 0) {
        return;
    }

    const char *normalized = nullptr;
    const char *reason = nullptr;

    if (strcmp(timezone, "SGT8") == 0) {
        normalized = "SGT-8";
        reason = "posix-sign";
    } else if (strcmp(timezone, "UTC+8") == 0 ||
               strcmp(timezone, "GMT+8") == 0 ||
               strcmp(timezone, "Singapore") == 0) {
        normalized = "SGT-8";
        reason = "alias";
    }

    if (!normalized) {
        return;
    }

    Log.warn("TimezoneNormalize: from=%s to=%s reason=%s", timezone, normalized, reason);
    snprintf(timezone, timezoneSize, "%s", normalized);
}

} // namespace

bool Cloud::applyConfigurationFromLedger(const LedgerData &defaults, const LedgerData &device) {
    bool success = true;
    bool sensorChanged = false;
    bool timingChanged = false;
    bool messagingChanged = false;
    bool modesChanged = false;
    bool reportingChanged = false;
    bool powerChanged = false;
    
    // Enhanced diagnostics for Alert 41 troubleshooting - track which section fails
    bool sensorOk = applySensorConfig(defaults, device, &sensorChanged);
    bool timingOk = applyTimingConfig(defaults, device, &timingChanged);
    bool messagingOk = applyMessagingConfig(defaults, device, &messagingChanged);
    bool modesOk = applyModesConfig(defaults, device, &modesChanged);
    bool reportingOk = applyReportingConfig(defaults, device, &reportingChanged);
    bool powerOk = applyPowerConfig(defaults, device, &powerChanged);
    
    success = sensorOk && timingOk && messagingOk && modesOk && reportingOk && powerOk;
    
    if (success) {
        const bool anyChanged = sensorChanged || timingChanged || messagingChanged || modesChanged || reportingChanged || powerChanged;

        // Do not force synchronous storage flushes here; they can exceed the
        // 100 ms loop budget. Persistence is handled by SystemConfig::loop() and
        // SystemConfig::SensorSettings::loop() (called from the main loop).
        SystemConfig::validateStoredData();
        SystemConfig::SensorSettings::validateStoredData();

        // Defer device-status publishing to Cloud::loop() so it doesn't
        // execute inside CONNECTING_STATE or async callbacks.
        if (anyChanged) {
            const bool wasPending = pendingStatusPublish;
            pendingStatusPublish = true;
            pendingStatusPublishSource = "ConfigApply";
            if (!wasPending) {
                const Cloud::LedgerSyncDiagnostics diagnostics = ledgerSyncDiagnostics();
#if defined(ENABLE_LEDGER_TRACE) && ENABLE_LEDGER_TRACE
                Log.info("LedgerQueue: kind=status pending=%u syncing=%d inflight=%d",
                         diagnostics.pendingCount,
                         diagnostics.syncing ? 1 : 0,
                         diagnostics.inflight ? 1 : 0);
#else
                (void)diagnostics;
#endif
            }
        }
    } else {
        Log.warn("Configuration apply failed: sensor=%s timing=%s messaging=%s modes=%s reporting=%s power=%s",
                 sensorOk ? "OK" : "FAIL",
                 timingOk ? "OK" : "FAIL",
                 messagingOk ? "OK" : "FAIL",
                 modesOk ? "OK" : "FAIL",
                 reportingOk ? "OK" : "FAIL",
                 powerOk ? "OK" : "FAIL");
    }
    
    return success;
}

bool Cloud::applyMessagingConfig(const LedgerData &defaults, const LedgerData &device, bool *changedOut) {
    Variant defaultMessaging;
    Variant deviceMessaging;
    bool hasDefault = getTopLevelMap(defaults, "messaging", defaultMessaging);
    bool hasDevice = getTopLevelMap(device, "messaging", deviceMessaging);

    if (!hasDefault && !hasDevice) return true;
    
    if ((hasDefault && !defaultMessaging.isMap()) || (hasDevice && !deviceMessaging.isMap())) return true;

    bool success = true;
    bool changed = false;
    bool serialEnabled = false;
    bool verboseMode = false;
    int verboseTimeoutMin = 0;

    if (getMergedBoolValue(defaultMessaging, deviceMessaging, "serial", serialEnabled)) {
        if (SystemConfig::get_serialConnected() != serialEnabled) {
            SystemConfig::set_serialConnected(serialEnabled);
            Log.info("Config: Serial → %s", serialEnabled ? "ON" : "OFF");
            changed = true;
        }
    }

    if (getMergedBoolValue(defaultMessaging, deviceMessaging, "verboseMode", verboseMode)) {
        if (SystemConfig::get_verboseMode() != verboseMode) {
            SystemConfig::set_verboseMode(verboseMode);
            Log.info("Config: Verbose -> %s", verboseMode ? "ON" : "OFF");
            changed = true;
        }
    }

    if (getMergedIntValue(defaultMessaging, deviceMessaging, "verboseTimeoutMin", verboseTimeoutMin)) {
        if (validateRange(verboseTimeoutMin, 0, 1440, "verboseTimeoutMin")) {
            if (SystemConfig::get_verboseTimeoutMin() != (uint16_t)verboseTimeoutMin) {
                SystemConfig::set_verboseTimeoutMin((uint16_t)verboseTimeoutMin);
                Log.info("Config: Verbose timeout -> %d min", verboseTimeoutMin);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    if (changedOut) {
        *changedOut = changed;
    }
    if (changed) Log.info("Messaging config updated");
    return success;
}

bool Cloud::applyTimingConfig(const LedgerData &defaults, const LedgerData &device, bool *changedOut) {
    Variant defaultTiming;
    Variant deviceTiming;
    bool hasDefault = getTopLevelMap(defaults, "timing", defaultTiming);
    bool hasDevice = getTopLevelMap(device, "timing", deviceTiming);
    
    if (!hasDefault && !hasDevice) return true;
    if ((hasDefault && !defaultTiming.isMap()) || (hasDevice && !deviceTiming.isMap())) return true;

    bool success = true;
    bool changed = false;
    int reportingInterval = 0;
    int openHour = 0;
    int closeHour = 0;
    int connectAttemptBudgetSec = 0;
    char timezone[SystemConfig::kTimeZoneCapacity] = {0};

    if (getMergedStringValue(defaultTiming, deviceTiming, "timezone", timezone, sizeof(timezone))) {
        normalizeTimezoneInPlace(timezone, sizeof(timezone));
        size_t timezoneLen = strlen(timezone);
        if (timezoneLen > 0 && timezoneLen < sizeof(timezone)) {
            if (strcmp(SystemConfig::get_timeZoneStrCStr(), timezone) != 0) {
                SystemConfig::set_timeZoneStr(timezone);
                Log.info("Config: Timezone -> %s", timezone);
                changed = true;
            }
        } else {
            Log.warn("Invalid timezone length: %d", (int)timezoneLen);
            success = false;
        }
    }

    if (getMergedIntValue(defaultTiming, deviceTiming, "reportingIntervalSec", reportingInterval)) {
        if (validateRange(reportingInterval, 300, 65535, "timing.reportingIntervalSec")) {
            if (SystemConfig::get_reportingInterval() != reportingInterval) {
                SystemConfig::set_reportingInterval(reportingInterval);
                Log.info("Config: Reporting interval -> %ds", reportingInterval);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Note: pollingRate is no longer used in v3.23 - sensor-specific timing
    // is now configured via sensor.setting1-4 fields in the sensor section

    // WO-2026-09-24-004: the hours are checked and applied as a pair. An absent
    // value falls back to the current one; a pair that breaks the rules is
    // rejected whole, so the device keeps its last valid hours.
    const bool haveOpenHour = getMergedIntValue(defaultTiming, deviceTiming, "openHour", openHour);
    const bool haveCloseHour = getMergedIntValue(defaultTiming, deviceTiming, "closeHour", closeHour);
    if (haveOpenHour || haveCloseHour) {
        const int mergedOpen = haveOpenHour ? openHour : (int)SystemConfig::get_openTime();
        const int mergedClose = haveCloseHour ? closeHour : (int)SystemConfig::get_closeTime();
        const char *hoursRule = Config::hoursRuleFailure(mergedOpen, mergedClose);
        if (hoursRule == nullptr) {
            if (SystemConfig::get_openTime() != mergedOpen || SystemConfig::get_closeTime() != mergedClose) {
                SystemConfig::set_openTime(mergedOpen);
```

</details>


---

<sub>17s</sub>

### `bash`

**Check flush usage and call sites**

$ git branch --show-current; grep -n "flushNow\|System.reset" src/cloud/*.cpp src/*.cpp | head -30; sed -n 625,690p src/cloud/Cloud.cpp

<details>
<summary>73 lines</summary>

```
wo/2026-11-02-004-timezone-planned-reset
src/cloud/DeviceStatusPublisher.cpp:527:    writer.name("resetReason").value((int)System.resetReason());
src/cloud/DeviceStatusPublisher.cpp:528:    writer.name("resetReasonData").value((unsigned long)System.resetReasonData());
src/Config.cpp:159:    SystemConfig::flushNow();
src/Generalized-Core-Counter.cpp:304:  const int reason = System.resetReason();
src/Generalized-Core-Counter.cpp:305:  const uint32_t reasonData = System.resetReasonData();
src/Generalized-Core-Counter.cpp:323:  // explicit System.reset(), or any other reset reason) - the call stack
src/Generalized-Core-Counter.cpp:412:                                 // System.reset() is done.
src/Generalized-Core-Counter.cpp:957:  // `System.resetReason() == RESET_REASON_POWER_MANAGEMENT`) intended to
src/Generalized-Core-Counter.cpp:992:    if (System.resetReason() == RESET_REASON_POWER_MANAGEMENT) {
src/Generalized-Core-Counter.cpp:1074:  if (System.resetReason() == RESET_REASON_POWER_MANAGEMENT) {
src/Generalized-Core-Counter.cpp:1213:    System.reset(RESET_CAUSE_OUT_OF_MEMORY);
src/Generalized-Core-Counter.cpp:1283:  int resetReason = System.resetReason();
src/Generalized-Core-Counter.cpp:1284:  uint32_t resetReasonData = System.resetReasonData();
src/Generalized-Core-Counter.cpp:1416:  const int osResetReason = System.resetReason();
src/MyPersistentData.cpp:918:void flushNow() { sysStatus.flush(true); }
src/ThrashGuard.cpp:16: * - Tier 3: System.reset() after recent repeated trips, raises alert 18
src/ThrashGuard.cpp:152:  System.reset(RESET_CAUSE_THRASH_TIER3);
        logConfigDiagFailure(configFailureReason);
    }

    Log.warn("%s", invalidLogPrefix);
    return false;
}

} // namespace

bool Cloud::loadConfigurationFromCloud() {
    LedgerData defaults = defaultSettingsLedger.get();
    LedgerData device = deviceSettingsLedger.get();
    lastApplySuccess = applyConfigurationFromLedger(defaults, device);

    if (lastApplySuccess) {
        lastApplySuccess = finalizeLedgerAppliedConfig(
            "Configuration apply completed but resulting config is invalid");
    }

    return lastApplySuccess;
}

const char *Cloud::getWebhookName() const {
    // Priority 1: Cloud configuration (explicitly set)
    const char *cloudWebhookName = SystemConfig::get_webhookNameCStr();
    if (cloudWebhookName && cloudWebhookName[0] != '\0') {
        return cloudWebhookName;
    }

    // Priority 2: Convention-based (mode-specific)
    uint8_t mode = SystemConfig::get_sensorMode();
    const char *conventionName = "unknown-webhook-v1";

    switch (mode) {
        case SystemConfig::COUNTING:
            conventionName = "counting-webhook-v1";
            break;
        case SystemConfig::OCCUPANCY:
            conventionName = "occupancy-webhook-v1";
            break;
        case SystemConfig::MEASUREMENT:
            conventionName = "measurement-webhook-v1";
            break;
        default:
            break;
    }

    return conventionName;
}

void Cloud::loop() {
    // Apply any newly-synced configuration outside callback context.
    // Do at most one deferred operation per loop() pass.
    if (pendingConfigApply && Particle.connected()) {
        pendingConfigApply = false;
        LedgerData defaults = defaultSettingsLedger.get();
        LedgerData device = deviceSettingsLedger.get();
        lastApplySuccess = applyConfigurationFromLedger(defaults, device);

        if (lastApplySuccess) {
            lastApplySuccess = finalizeLedgerAppliedConfig(
                "Deferred configuration apply completed but resulting config is invalid");
        }

        return;
    }
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>20s</sub>

### `bash`

**Trace setup-time reachability**

$ grep -rn "loadConfigurationFromCloud" src | grep -v "^src/cloud/Cloud.h"; grep -n "flushNow\|System.reset" -B2 src/ConnectivityFailsafe.cpp src/*/ConnectivityFailsafe.cpp 2>/dev/null| head -30; sed -n 150,162p src/Config.cpp; grep -n "withConfig\|loadConfigurationFromCloud\|Cloud::instance().setup\|cloud.*setup\|Config::" src/Generalized-Core-Counter.cpp | head -30

<details>
<summary>56 lines</summary>

```
src/state/State_Connect.cpp:536:      bool configOk = Cloud::instance().loadConfigurationFromCloud();
src/state/State_Connect.cpp:695:      Cloud::instance().loadConfigurationFromCloud();
src/cloud/Cloud.cpp:634:bool Cloud::loadConfigurationFromCloud() {
src/power/ConnectivityFailsafe.cpp-84-    }
src/power/ConnectivityFailsafe.cpp-85-  }
src/power/ConnectivityFailsafe.cpp:86:  SystemConfig::flushNow();
--
src/power/ConnectivityFailsafe.cpp-147-  RecoveryState::set_lastConnectivityRecoveryAction(0);
src/power/ConnectivityFailsafe.cpp-148-  RecoveryState::set_connectivityRecoveryCount(0);
src/power/ConnectivityFailsafe.cpp:149:  SystemConfig::flushNow();
--
src/power/ConnectivityFailsafe.cpp-304-    Log.info("Failsafe: stage=2 action=system-reset age=%lds",
src/power/ConnectivityFailsafe.cpp-305-             (long)connectionAgeSec);
src/power/ConnectivityFailsafe.cpp:306:    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);
    uint32_t debounceMs = SystemConfig::SensorSettings::get_sensorSetting1();
    return debounceMs == 0 ? DEFAULT_OCCUPANCY_DEBOUNCE_MS : debounceMs;
}

void markLedgerConfigurationValid() {
    if (!SystemConfig::get_hasValidLedgerConfig()) {
        SystemConfig::set_hasValidLedgerConfig(true);
    }
    setSource(CONFIG_SOURCE_LEDGER, "ledger-apply");
    SystemConfig::flushNow();
}

void markStorageConfigurationLoaded() {
493:  SystemConfig::setup();    // Initialize persistent storage
494:  SystemConfig::SensorSettings::setup(); // Initialize the sensor configuration
504:      ? std::max(CurrentReadings::get_occupancyStartTime(), SystemConfig::get_lastReport()) : 0;
528:  if (SystemConfig::get_hasValidLedgerConfig()) {
529:    Config::markStorageConfigurationLoaded();
531:    Config::markFactoryDefaultsActive();
556:  if (SystemConfig::get_serialConnected() || (ALLOW_BLOCKING_SERIAL_WAITS != 0)) {
575:  if (SystemConfig::get_testConnectionDurationOverride() == 0) {
576:    SystemConfig::set_testConnectionDurationOverride(0xFFFF);  // Disabled (max uint16_t)
639:  SensorType configuredType = static_cast<SensorType>(SystemConfig::SensorSettings::get_sensorType());
803:  // Moved here (from immediately after SystemConfig::setup(), before AB1805 init)
848:      if (SystemConfig::get_verboseMode()) {
853:    } else if (SystemConfig::get_verboseMode()) {
907:  Cloud::instance().setup(); // Initialize the cloud functions
927:  const char *tz = SystemConfig::get_timeZoneStrCStr();
929:    tz = Config::DEFAULT_TIMEZONE;
930:    SystemConfig::set_timeZoneStr(tz);
932:  LocalTime::instance().withConfig(LocalTimePosixTimezone(tz));
934:  Config::logDiagnostics("ConfigDiag");
935:  if (!Config::isValid(true)) {
943:  // be true here with zero confirmed cloud syncs ever - ab1805.setup() seeds
946:  // SystemConfig::get_lastTimeSync() (persisted across reboots, stamped only by
983:  const bool neverConfirmedSyncEver = (SystemConfig::get_lastTimeSync() == 0);
994:      if (localHour == SystemConfig::get_openTime()) {
1052:  // cloud/ledger setup, timezone config, connection-mode logging, and sensor
1086:  if (PowerConfig::get_lowBatteryMode()) {
1170:  SystemConfig::loop();
1171:  SystemConfig::SensorSettings::loop();
1194:    unsigned long timeoutMs = SystemConfig::get_webhookTimeoutMs();
1195:    if (timeoutMs < SystemConfig::kWebhookTimeoutMinMs || timeoutMs > SystemConfig::kWebhookTimeoutMaxMs) {
<shellId: 3 completed with exit code 0>
```

</details>


---

<sub>24s</sub>

### `bash`

**Find ConfigApply test patterns and build instructions**

$ grep -ln "ConfigApply" tests/*; cat tests/config_window_hold_test.sh; ls tests/../test* 2>/dev/null | head; grep -n "Stage 7\|local toolchain build" -A12 AI_DEVELOPMENT_WORKFLOW.md | grep -in "toolchain\|BUILD_PATH\|compile\|nm " | head -20; sed -n 1,200p tests/README.md | head -60

<details>
<summary>246 lines</summary>

````
grep: tests/fixtures: Is a directory
grep: tests/stubs: Is a directory
tests/connection_mode_downgrade_structural_test.py
tests/connection_mode_downgrade_test.cpp
tests/connection_mode_downgrade_test.sh
tests/hour_rules_test.sh
tests/persistence_facade_behavior_test.cpp
tests/reporting_interval_cap_test.cpp
tests/reporting_interval_cap_test.sh
tests/sensor_type_source_test.sh
tests/sensor_type_validation_test.cpp
tests/webhook_timeout_range_test.py
#!/bin/zsh
set -euo pipefail

# WO-2026-10-09-002: the once-a-day config window.
#
# Part 1 extracts the real Cloud::areLedgersSynced() from
# src/cloud/LedgerClient.cpp, checks the copy byte-for-byte against the source
# (COPY_MISMATCH on any difference) and compiles it into
# config_window_hold_test.cpp. Each behaviour case runs in its own process,
# because the function keeps its state in function-local statics.
# Part 2 checks the structure of the CONNECTED+open abort in State_Sleep.cpp
# (it resets cloudGate.cloudSyncStartMs, a field of the cloudGate struct declared before the abort).
# Part 3 plants mutations on COPIES of the two files; each must be caught by a
# run-time or structure check, not by a compile error.
#
# Usage: config_window_hold_test.sh [LedgerClient.cpp [State_Sleep.cpp]]
# (default: this repo's src/)

script="${0:A}"
repo_root="${script:h:h}"
ledger_src="${1:-$repo_root/src/cloud/LedgerClient.cpp}"
sleep_src="${2:-$repo_root/src/state/State_Sleep.cpp}"
work="${TMPDIR:-/tmp}/config_window_hold_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

extract() {
  python3 - "$ledger_src" "$work" <<'PY'
import re, sys

path, work = sys.argv[1], sys.argv[2]
app = open(path).read()
m = re.search(r"^bool Cloud::areLedgersSynced\(\) const \{.*?^\}\n", app, re.S | re.M)
if not m:
    sys.exit("EXTRACT_FAILED: Cloud::areLedgersSynced() not found")
block = m.group(0)
if block != app[m.start():m.end()] or block not in app:
    print("COPY_MISMATCH: extracted block differs from the source", file=sys.stderr)
    sys.exit(1)
open(work + "/are_ledgers_synced.h", "w").write(block)
open(work + "/copy_back.h", "w").write(app[m.start():m.end()])
PY
  cmp -s "$work/are_ledgers_synced.h" "$work/copy_back.h" || { echo "COPY_MISMATCH" >&2; return 1; }
}

build() {
  clang++ -std=c++17 -Wall -Wextra -pedantic -Wno-unused-variable -Wno-unused-function \
    -DAREA_LEDGERS_SYNCED_H="\"$work/are_ledgers_synced.h\"" \
    "$repo_root/tests/config_window_hold_test.cpp" -o "$work/hold_test"
}

cases=(boot_trusted boot_untrusted boot_no_change anchor same_day first_after_open
       first_after_open_untrusted cut_short backward_clock_unequal zero_connection_epoch warm_outside_hold partial_outside_hold
       empty_device_outside_hold cold_boot)

run_cases() {
  extract
  build
  local c rc=0
  for c in $cases; do
    "$work/hold_test" "$c" || rc=1
  done
  return $rc
}

structural_abort() {
  python3 - "$sleep_src" <<'PY'
import re, sys
text = open(sys.argv[1]).read()
decl = -1
sm = re.search(r'struct \{[^{}]*?\bunsigned long cloudSyncStartMs = 0;[^{}]*?\} cloudGate;', text, re.S)
if sm:
    decl = sm.start()
m = re.search(r'ensureSensorEnabled\("SLEEP abort: CONNECTED\+OPEN"\);\n(.*?)transitionTo\(IDLE_STATE, "sleep-abort-open-hours"\);', text, re.S)
if not m:
    sys.exit("the CONNECTED+open abort was not found")
if "cloudGate.cloudSyncStartMs = 0;" not in m.group(1):
    sys.exit("the CONNECTED+open abort does not reset cloudGate.cloudSyncStartMs")
if decl < 0 or decl > m.start():
    sys.exit("cloudSyncStartMs is not declared in the cloudGate struct before the CONNECTED+open abort")
PY
}

echo "=== Part 1: the real areLedgersSynced(), one process per case ==="
run_cases || fail "a behaviour case failed"
echo ""
echo "=== Part 2: the CONNECTED+open abort resets cloudSyncStartMs ==="
structural_abort || fail "the abort does not reset cloudSyncStartMs"
echo "OK: the abort resets cloudSyncStartMs"

# When called on a mutated copy, stop here: the caller wants the exit status.
[[ -n "${1:-}" ]] && { echo "config_window_hold_test passed"; exit 0; }

echo ""
echo "=== Part 3: mutations (planted on copies) ==="
plant() {
  # $1 = name, $2 = file (ledger|sleep), then pairs of: old literal, new literal
  local name="$1" which="$2"; shift 2
  local dir="$work/mut_${name// /_}"
  mkdir -p "$dir"
  cp "$repo_root/src/cloud/LedgerClient.cpp" "$dir/LedgerClient.cpp"
  cp "$repo_root/src/state/State_Sleep.cpp" "$dir/State_Sleep.cpp"
  local target="$dir/LedgerClient.cpp"
  [[ "$which" == sleep ]] && target="$dir/State_Sleep.cpp"
  python3 - "$target" "$@" <<'PY'
import sys
path, pairs = sys.argv[1], sys.argv[2:]
text = open(path).read()
for old, new in zip(pairs[::2], pairs[1::2]):
    if text.count(old) != 1:
        sys.exit("MUTATION_FAILED: literal not found exactly once: %r" % old)
    text = text.replace(old, new, 1)
open(path, "w").write(text)
PY
  if "$script" "$dir/LedgerClient.cpp" "$dir/State_Sleep.cpp" >"$dir/out" 2>"$dir/err"; then
    rm -rf "$dir"
    fail "mutation '$name' was NOT detected"
  fi
  if grep -q "error:\|MUTATION_FAILED\|EXTRACT_FAILED" "$dir/err"; then
    cat "$dir/err" >&2
    fail "mutation '$name' broke the build or the mutation instead of being caught by a check"
  fi
  local caught
  caught="$(grep -h -m1 '^FAIL:\|^the \|^cloudSyncStartMs' "$dir/out" "$dir/err" | head -1 | cut -d: -f2-)"
  rm -rf "$dir"
  echo "OK (mutation): '$name' detected ($caught)"
}

plant "boot hold gated by clock trust" ledger \
  "configHold = (holdDoneEpoch == 0) ||
                         (Clock::isTrusted() && holdDoneEpoch < DailyBoundary::todayAt(SystemConfig::get_openTime()));" \
  "configHold = (Clock::isTrusted() && holdDoneEpoch < DailyBoundary::todayAt(SystemConfig::get_openTime()));"
plant "remove the anchor" ledger \
  "        if (configHold && hasPendingOutputLedgerSync()) {
            firstConnectedTime = nowMs;
        }" \
  "        (void)0;"
plant "end the hold on timeout only" ledger \
  "(inputLanded || nowMs - firstConnectedTime" \
  "(nowMs - firstConnectedTime"
plant "set holdDoneEpoch when cut short" ledger \
  "            holdBaseDevice = deviceSync;" \
  "            holdBaseDevice = deviceSync;
            holdDoneEpoch = currentConnectionEpoch;"
plant "early return not gated by the hold" ledger \
  "if (defaultSynced && deviceSynced && !configHold) {" \
  "if (defaultSynced && deviceSynced) {"
plant "F1: one maximum for both input ledgers" ledger \
  "            holdBaseDefault = defaultSync;
            holdBaseDevice = deviceSync;" \
  "            holdBaseDefault = std::max(defaultSync, deviceSync);
            holdBaseDevice = holdBaseDefault;" \
  "(defaultSync != holdBaseDefault) || (deviceSync != holdBaseDevice)" \
  "(std::max(defaultSync, deviceSync) != holdBaseDefault)"
plant "F1: != replaced by > (backward clock step)" ledger \
  "(defaultSync != holdBaseDefault) || (deviceSync != holdBaseDevice)" \
  "(defaultSync > holdBaseDefault) || (deviceSync > holdBaseDevice)"
plant "F2: done stored as a zero epoch" ledger \
  "holdDoneEpoch = currentConnectionEpoch ? currentConnectionEpoch : 1;" \
  "holdDoneEpoch = currentConnectionEpoch;"
plant "remove the CONNECTED+open abort reset" sleep \
  "    ensureSensorEnabled(\"SLEEP abort: CONNECTED+OPEN\");
    cloudGate.cloudSyncStartMs = 0;" \
  "    ensureSensorEnabled(\"SLEEP abort: CONNECTED+OPEN\");"

echo ""
echo "config_window_hold_test passed (real areLedgersSynced() cases, abort structure, mutations)"
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
67:416:#### Mandatory: local toolchain build
69:418:Every change must be built with the **local Device OS toolchain** before Stage 7
70:419-can pass. The Particle cloud compile is **not** a per-change check (changed
72:421-"Release gate: the Particle cloud compile", below).
74:423-The two use different include-search orders. A source file can compile cleanly in
84:446-acceptance test, on every side, used the cloud compiler. Codex had flagged the
92:454-#### Release gate: the Particle cloud compile (once per release)
103:475-- **Why a release gate and not a per-change one:** the cloud compile exists for the
107:479-  **shipped release**, and one compile of the release head covers it. The
111:545:`codex review --uncommitted` is not used: it rejects custom instructions (CLI 0.154.0). Codex's sandbox has no network, so the Particle cloud compile (a release-head check, once per release; see "Release gate" in §3) is run by Claude Code, not in Stage 7.
118:552-  --allow-all-tools --add-dir ~/.particle/toolchains \
125:561:The deny rules enforce the Implementer's restrictions at the CLI, not only in the prompt. `--add-dir ~/.particle/toolchains` gives Copilot the local Device OS toolchain, which the local build in Stage 7's mandatory checks needs.
191:740-5. **Verify the binary, not the source, for anything involving vendored libraries.** A library that is both vendored in `lib/` and listed in `project.properties` may be replaced by the registry copy in a cloud build. Confirm the change is present in the binary that will be flashed (disassembly, symbols, or strings), not only in the source tree. Local and cloud builds may use different library copies. Bench-test the build type the fleet will receive. The cloud compile that checks this runs once per release, on the release branch head (§3, "Release gate").
218:790-  - **The candidate:** it always gets a fresh `BUILD_PATH_BASE`, with no object reuse between the two builds.
223:795-- the local ARM toolchain build (with its `BUILD_PATH_BASE`);
224:796-- the Particle cloud compile (once per release, on the release branch head; §3);
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

## WO-2026-08-11-001 (SLEEP_PREP threshold exclusion) host test

`noteLoopStageDuration()`/`setLoopStage()`/`transitionTo()`
(`src/Generalized-Core-Counter.cpp`) and `handleSleepingState()`
(`src/state/State_Sleep.cpp`) pull in heavy Particle/PublishQueuePosix
dependencies and can't be compiled standalone on the host. This test parses
the actual shipped source directly to confirm the corrected design:

- `LOOP_STAGE_SLEEP_PREP` is still excluded from the
<shellId: 4 completed with exit code 0>
````

</details>


---

<sub>28s</sub>

### `bash`

**View sample test and build workflow**

$ cat tests/reporting_interval_cap_test.sh | head -60; ls tests/stubs; sed -n 410,455p AI_DEVELOPMENT_WORKFLOW.md

<details>
<summary>109 lines</summary>

```
#!/bin/zsh
set -euo pipefail

# WO-2026-10-08-001 item T: reportingIntervalSec is capped at 65535 (the width
# of the uint16_t store) in ConfigApply, so a larger value is rejected instead
# of wrapping. The real validateRange() and the real reportingIntervalSec block
# are extracted from src/cloud/ConfigApply.cpp. A mutation (the 86400 bound
# restored, on a COPY) must fail at run time, not at compile time.
#
# Usage: reporting_interval_cap_test.sh [ConfigApply.cpp]

script="${0:A}"
repo_root="${script:h:h}"
config_src="${1:-$repo_root/src/cloud/ConfigApply.cpp}"
work="${TMPDIR:-/tmp}/reporting_interval_cap_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

extract() {
  # $1 = file, $2 = literal on the block's first line, $3 = output
  python3 - "$1" "$2" "$3" <<'PY'
import sys
path, needle, out = sys.argv[1:4]
lines = open(path).readlines()
start = next((i for i, l in enumerate(lines) if needle in l), None)
if start is None:
    sys.exit("EXTRACT_FAILED: %r not found in %s" % (needle, path))
if needle.startswith("bool Cloud::validateRange"):
    start -= 1  # include the template<typename T> line
depth = 0
opened = False
for i in range(start, len(lines)):
    for ch in lines[i]:
        if ch == '{':
            depth += 1
            opened = True
        elif ch == '}':
            depth -= 1
    if opened and depth == 0:
        open(out, "w").write("".join(lines[start:i + 1]))
        break
else:
    sys.exit("EXTRACT_FAILED: no closing brace")
PY
}

run_with() {
  extract "$1" 'if (getMergedIntValue(defaultTiming, deviceTiming, "reportingIntervalSec", reportingInterval)) {' "$work/interval_block.inc"
  extract "$1" 'bool Cloud::validateRange(T value' "$work/validate_range.inc"
  grep -q set_reportingInterval "$work/interval_block.inc" || fail "could not extract the interval block"
  clang++ -std=c++17 -Wall -Wextra -pedantic -I"$work" \
    "$repo_root/tests/reporting_interval_cap_test.cpp" -o "$work/bin"
  "$work/bin"
}

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
it is *connected*.

This check was added on 2026-08-25 after WO-2026-08-25-001 reached Stage 7 with
two of its four functions absent from the artifact, having passed every other
verification.

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
<shellId: 5 completed with exit code 0>
```

</details>


---

<sub>41s</sub>

### `bash` — Failed

**Apply source edits**

$ cd src && python3 - <<'E'
import re
p='ResetCause.h'; t=open(p).read()
t=t.replace("  RESET_CAUSE_OUT_OF_MEMORY = 7,           ///< out of memory\n","  RESET_CAUSE_OUT_OF_MEMORY = 7,           ///< out of memory\n  RESET_CAUSE_TIMEZONE_CHANGE = 8,         ///< ConfigApply stored a changed timezone\n")
open(p,'w').write(t)
p='cloud/ConfigApply.cpp'; t=open(p).read()
t=t.replace('#include "../Config.h"\n','#include "../Config.h"\n#include "../ResetCause.h"\n',1)
t=t.replace('namespace {\n\nbool getTopLevelMap','namespace {\n\nbool timezoneStored = false;\n\nbool getTopLevelMap',1)
t=t.replace('                Log.info("Config: Timezone -> %s", timezone);\n','                Log.info("Config: Timezone -> %s", timezone);\n                timezoneStored = true;\n',1)
t=t.replace('        SystemConfig::SensorSettings::validateStoredData();\n\n','''        SystemConfig::SensorSettings::validateStoredData();

        // LocalTime reads the zone only at boot: flush it, then reboot once.
        if (timezoneStored) {
            SystemConfig::flushNow();
            System.reset(RESET_CAUSE_TIMEZONE_CHANGE);
        }

''',1)
open(p,'w').write(t)
E
cd ..; git diff --stat -- src/; git diff -- src | grep -c "^+[^+]"

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>47s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/ResetCause.h.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/ResetCause.h

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/ResetCause.h updated with changes.
```


---

<sub>47s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp updated with changes.
```


---

<sub>47s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp updated with changes.
```


---

<sub>47s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp updated with changes.
```


---

<sub>47s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp updated with changes.
```


---

<sub>55s</sub>

### `bash`

**Inspect existing harness**

$ git diff --stat -- src; cat tests/reporting_interval_cap_test.cpp | head -120; wc -l tests/stubs/Particle.h; sed -n 60,140p tests/reporting_interval_cap_test.sh

<details>
<summary>96 lines</summary>

```
src/ResetCause.h          |  1 +
 src/cloud/ConfigApply.cpp | 10 ++++++++++
 2 files changed, 11 insertions(+)
// WO-2026-10-08-001 item T: the reportingIntervalSec range in ConfigApply.
// Part of tests/reporting_interval_cap_test.sh.
//
// The real block from Cloud::applyTimingConfig() and the real
// Cloud::validateRange() are extracted from src/cloud/ConfigApply.cpp and run
// against a stub store with the real uint16_t width.

#include <cstdint>
#include <cstdio>

namespace {

int failures = 0;

struct TestLog {
  template <typename... Args>
  void info(const char *, Args...) {}
  template <typename... Args>
  void warn(const char *, Args...) {}
} Log;

uint16_t storedInterval = 3600;
int injectedValue = 0;

namespace SystemConfig {
uint16_t get_reportingInterval() { return storedInterval; }
void set_reportingInterval(uint16_t value) { storedInterval = value; }
} // namespace SystemConfig

struct Cloud {
  template <typename T>
  bool validateRange(T value, T min, T max, const char *name);
  bool applyInterval(bool &changed);
};

bool getMergedIntValue(int, int, const char *, int &out) {
  out = injectedValue;
  return true;
}

#include "validate_range.inc"

bool Cloud::applyInterval(bool &changed) {
  bool success = true;
  int reportingInterval = 0;
  int defaultTiming = 0;
  int deviceTiming = 0;
#include "interval_block.inc"
  return success;
}

template bool Cloud::validateRange<int>(int, int, int, const char *);

// Returns whether the apply as a whole succeeded.
bool apply(int value, bool &changed) {
  injectedValue = value;
  changed = false;
  Cloud cloud;
  return cloud.applyInterval(changed);
}

void check(const char *name, bool ok) {
  printf("  %s %s\n", ok ? "ok  " : "FAIL", name);
  if (!ok) {
    failures++;
  }
}

} // namespace

int main() {
  bool changed = false;

  storedInterval = 3600;
  check("65535 is accepted", apply(65535, changed) && changed);
  check("65535 is stored without wrapping", storedInterval == 65535);

  storedInterval = 3600;
  check("65536 is rejected", !apply(65536, changed));
  check("65536 leaves the stored interval unchanged", storedInterval == 3600 && !changed);

  check("86400 is rejected", !apply(86400, changed));
  check("86400 leaves the stored interval unchanged", storedInterval == 3600 && !changed);

  check("300 is still accepted", apply(300, changed) && storedInterval == 300);
  check("299 is still rejected", !apply(299, changed) && storedInterval == 300);

  if (failures != 0) {
    printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  printf("\nreporting interval cap test passed\n");
  return 0;
}
      15 tests/stubs/Particle.h

run_with "$config_src"

[[ -n "${1:-}" ]] && exit 0

echo "--- mutation: restore the 86400 upper bound (on a copy) ---"
python3 - "$config_src" "$work/ConfigApply.mut.cpp" <<'PY'
import sys
text = open(sys.argv[1]).read()
old = "reportingInterval, 300, 65535,"
if text.count(old) != 1:
    sys.exit("MUTATION_FAILED: bound not found exactly once")
open(sys.argv[2], "w").write(text.replace(old, "reportingInterval, 300, 86400,"))
PY
if "$script" "$work/ConfigApply.mut.cpp" >/dev/null 2>"$work/err"; then
  fail "mutation 'restore the 86400 bound' was NOT detected"
fi
grep -q "error:" "$work/err" && { cat "$work/err" >&2; fail "mutation broke the build instead of being caught"; }
echo "OK (mutation): 'restore the 86400 bound' detected"

echo "reporting_interval_cap_test passed"
<shellId: 7 completed with exit code 0>
```

</details>


---

<sub>1m 3s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/src/cloud/ConfigApply.cpp updated with changes.
```


---

<sub>1m 3s</sub>

### `bash`

**Check diff size**

$ git diff --stat -- src; git diff -- src/cloud/ConfigApply.cpp

<details>
<summary>48 lines</summary>

```diff
src/ResetCause.h          |  1 +
 src/cloud/ConfigApply.cpp | 15 ++++++++++++---
 2 files changed, 13 insertions(+), 3 deletions(-)
diff --git a/src/cloud/ConfigApply.cpp b/src/cloud/ConfigApply.cpp
index 42a1996..f203b85 100644
--- a/src/cloud/ConfigApply.cpp
+++ b/src/cloud/ConfigApply.cpp
@@ -1,12 +1,15 @@
 #include "cloud/Cloud.h"
 
 #include "../Config.h"
+#include "../ResetCause.h"
 #include "power/ChargeInhibitPolicy.h"
 #include "persist/PowerConfig.h"
 #include "persist/SystemConfig.h"
 
 namespace {
 
+bool timezoneStored = false;
+
 bool getTopLevelMap(const LedgerData &ledger, const char *key, Variant &section) {
     if (!ledger.has(key)) {
         return false;
@@ -141,12 +144,17 @@ bool Cloud::applyConfigurationFromLedger(const LedgerData &defaults, const Ledge
     if (success) {
         const bool anyChanged = sensorChanged || timingChanged || messagingChanged || modesChanged || reportingChanged || powerChanged;
 
-        // Do not force synchronous storage flushes here; they can exceed the
-        // 100 ms loop budget. Persistence is handled by SystemConfig::loop() and
-        // SystemConfig::SensorSettings::loop() (called from the main loop).
+        // Do not force synchronous storage flushes here (they can exceed the
+        // 100 ms loop budget) except before the one-off timezone reset below;
+        // otherwise SystemConfig::loop() persists lazily.
         SystemConfig::validateStoredData();
         SystemConfig::SensorSettings::validateStoredData();
 
+        if (timezoneStored) {
+            SystemConfig::flushNow();
+            System.reset(RESET_CAUSE_TIMEZONE_CHANGE);
+        }
+
         // Defer device-status publishing to Cloud::loop() so it doesn't
         // execute inside CONNECTING_STATE or async callbacks.
         if (anyChanged) {
@@ -253,6 +261,7 @@ bool Cloud::applyTimingConfig(const LedgerData &defaults, const LedgerData &devi
             if (strcmp(SystemConfig::get_timeZoneStrCStr(), timezone) != 0) {
                 SystemConfig::set_timeZoneStr(timezone);
                 Log.info("Config: Timezone -> %s", timezone);
+                timezoneStored = true;
                 changed = true;
             }
         } else {
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>1m 28s</sub>

### `create`

**create a new file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.cpp.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.cpp

```
Created file /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.cpp with 5426 characters
```


---

<sub>1m 44s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.cpp.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.cpp

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.cpp updated with changes.
```


---

<sub>1m 44s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.cpp.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.cpp

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.cpp updated with changes.
```


---

<sub>1m 44s</sub>

### `create`

**create a new file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh

```
Created file /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh with 4532 characters
```


---

<sub>1m 55s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh updated with changes.
```


---

<sub>1m 55s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/reset_cause_structural_test.py.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/reset_cause_structural_test.py

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/reset_cause_structural_test.py updated with changes.
```


---

<sub>1m 55s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/reset_cause_structural_test.py.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/reset_cause_structural_test.py

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/reset_cause_structural_test.py updated with changes.
```


---

<sub>1m 55s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/reset_cause_structural_test.py.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/reset_cause_structural_test.py

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/reset_cause_structural_test.py updated with changes.
```


---

<sub>1m 57s</sub>

### Copilot

Also update the docstring and header comment mention, then run.

---

<sub>2m 6s</sub>

### `bash`

**Run new tests**

$ chmod +x tests/timezone_planned_reset_test.sh; zsh tests/timezone_planned_reset_test.sh 2>&1 | tail -50; python3 tests/reset_cause_structural_test.py

<details>
<summary>30 lines</summary>

```
--- case: changed_zone ---
  ok   apply succeeds
  ok   changed zone: stored, flushed, then reset(8), in that order
  ok   the zone is stored
--- case: after_reboot ---
  ok   same ledger value after the reboot: no store, no reset
--- case: unchanged_zone ---
  ok   unchanged zone: no reset and no flush
--- case: failed_apply ---
  ok   apply fails
  ok   failed apply: no reset
  ok   failed apply: no flush
--- case: failed_timing ---
  ok   timing section fails after storing the zone: no reset
--- case: other_section_only ---
  ok   only another section changed: no reset, no flush
  ok   the status publish is still deferred
--- case: zone_absent ---
  ok   no timezone in the ledger: no reset
timezone_planned_reset behaviour cases passed

=== Part 2: mutations (planted on copies) ===
OK (mutation): 'reset without the flush' detected (FAIL changed zone: stored, flushed, then reset(8), in that order)
OK (mutation): 'reset on every apply' detected (FAIL same ledger value after the reboot: no store, no reset)
OK (mutation): 'reset when the zone is unchanged' detected (FAIL same ledger value after the reboot: no store, no reset)
OK (mutation): 'reset on a failed apply' detected (FAIL failed apply: no reset)
OK (mutation): 'flag set when only other sections change' detected (FAIL changed zone: stored, flushed, then reset(8), in that order)
OK (mutation): 'reset before the flush (order)' detected (FAIL changed zone: stored, flushed, then reset(8), in that order)

timezone_planned_reset_test passed (real apply path, one process per case, mutations)
Reset cause structural test passed (8 call sites, distinct non-zero codes, resetReasonData still published)
<shellId: 9 completed with exit code 0>
```

</details>


---

<sub>2m 15s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh updated with changes.
```


---

<sub>2m 15s</sub>

### `edit`

**edit the file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh

```
File /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/tests/timezone_planned_reset_test.sh updated with changes.
```


---

<sub>2m 23s</sub>

### `bash`

**Rerun test**

$ zsh tests/timezone_planned_reset_test.sh 2>&1 | tail -8; git status --short

<details>
<summary>14 lines</summary>

```
OK (mutation): 'the flag is never set (the changed zone does not reset)' detected (FAIL changed zone: stored, flushed, then reset(8), in that order)
OK (mutation): 'reset before the flush (order)' detected (FAIL changed zone: stored, flushed, then reset(8), in that order)

=== Part 3: cause-code mutations (structural test on a copy of src/) ===
OK (mutation): 'cause code 0' detected (FAIL: reset cause code(s) ['RESET_CAUSE_TIMEZONE_CHANGE'] are 0; 0 is reserved for resets issued outside src/)
OK (mutation): 'reused code' detected (FAIL: reset cause codes RESET_CAUSE_OUT_OF_MEMORY and RESET_CAUSE_TIMEZONE_CHANGE share the value 7)

timezone_planned_reset_test passed (real apply path, one process per case, mutations)
 M src/ResetCause.h
 M src/cloud/ConfigApply.cpp
 M tests/reset_cause_structural_test.py
?? docs/work-orders/WO-2026-11-02-004-stage6-copilot-dispatch.md
?? docs/work-orders/WO-2026-11-02-004-timezone-applies-to-running-clock.md
?? tests/timezone_planned_reset_test.cpp
?? tests/timezone_planned_reset_test.sh
<shellId: 10 completed with exit code 0>
```

</details>


---

<sub>2m 28s</sub>

### Copilot

Now the full host suite and the toolchain build.

---

<sub>2m 28s</sub>

### `bash` — Failed

**Run host suite**

$ pass=0; fail=0; failed=""; for f in tests/*.sh; do if zsh $f >/tmp/o.$$ 2>&1; then pass=$((pass+1)); else fail=$((fail+1)); failed="$failed $f"; fi; done; for f in tests/*.py; do if python3 $f >/tmp/o.$$ 2>&1; then pass=$((pass+1)); else fail=$((fail+1)); failed="$failed $f"; fi; done; echo "pass=$pass fail=$fail $failed"; ls tests/*.sh | wc -l; ls tests/*.py | wc -l

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>9m 1s</sub>

### `bash`

**Run host suite**

$ pass=0; fail=0; failed=""; for f in tests/*.sh; do if zsh $f >/dev/null 2>&1; then pass=$((pass+1)); else fail=$((fail+1)); failed="$failed $f"; fi; done; for f in tests/*.py; do if python3 $f >/dev/null 2>&1; then pass=$((pass+1)); else fail=$((fail+1)); failed="$failed $f"; fi; done; echo "pass=$pass fail=$fail $failed"

```
pass=108 fail=0
<shellId: 12 completed with exit code 0>
```


---

<sub>9m 6s</sub>

### `bash`

**Read build/linkage instructions**

$ grep -n "linkage check" -B2 -A18 AI_DEVELOPMENT_WORKFLOW.md | sed -n 1,60p; ls ~/.particle/toolchains/deviceOS ~/.particle/toolchains/gcc-arm ~/.particle/toolchains/buildtools; ls; cat .gitignore | head -20

<details>
<summary>98 lines</summary>

```
435-         BUILD_PATH_BASE=<fresh scratch dir that doesn't exist yet>
436-
437:This is also the build that produces the symbolised ELF the linkage check above
438-requires, so the two checks share the work.
439-
440-Use a new `BUILD_PATH_BASE` for each build type (see §2, "Verifying compile-time
441-flags"); `make clean-user` does not clean this path.
442-
443-Added 2026-08-26 after WO-2026-08-25-001. An include rewritten from
444-`"../Config.h"` to `"Config.h"` broke the local build and survived **seven
445-implementation rounds and three independent Stage 7 reviews** — because every
446-acceptance test, on every side, used the cloud compiler. Codex had flagged the
447-same collision class in round 3 and it was recorded as an environment quirk
448-rather than a defect class. It was found only when the engineer tried to flash a
449-device.
450-
451:The general lesson, which is the same one behind the linkage check above:
452-**verifying with an instrument that cannot see the failure is not verification.**
453-
454-#### Release gate: the Particle cloud compile (once per release)
455-
456-*Standing rule, Chip, 25 Oct 2026. It replaces the earlier rule that every change
457-needed both a local and a cloud build.*
458-
459-- **When:** once per release, on the **release branch head**: `particle compile
460-  boron . --target <Device OS version of the release>` (for example `--target
461-  6.6.0`), run by Claude Code (Codex's sandbox has no network). It covers **every
462-  WO in that release**; no WO or PR needs its own cloud compile.
463-- **Record:** the result, with **flash and RAM**, goes in the release record
464-  (the release WO, `docs/work-orders/<date>-vNN-release-and-soak.md`, and the
465-  release PR body), beside the `.bin` size and sha256 of §13
466-  (j). The compile must be of the head that is uploaded, not an earlier commit.
467-- **What stays per change:** the local ARM build above, with a fresh
468:  `BUILD_PATH_BASE`, and the linkage check (`nm` on the symbolised ELF). A WO's
469-  Stage 7 does not wait for the cloud.
470-- **If the service is down at release time:** record that (the date, the error
471-  text, and a control build of the previous release if one was tried), and
472-  **release from the verified local build** as usual (§13 (j)). Do not hold the
473-  release for it; retry once the service is back and add the numbers to the
474-  record.
475-- **Why a release gate and not a per-change one:** the cloud compile exists for the
476-  risk in §12.5 (item 5): a library both vendored in `lib/` and listed in
477-  `project.properties` can be replaced by the registry copy in a cloud build, so
478-  the shipped binary can differ from the local one. That risk concerns the
479-  **shipped release**, and one compile of the release head covers it. The
480-  include-order collision that the local build catches (above) is a per-change
481-  risk, so the local build stays per change. A per-change cloud gate also turns a
482-  Particle service failure into a block on every WO: on 25 Oct 2026
483-  `particle compile boron --target 6.6.0` timed out ("Compiler timed out or
484-  encountered an error", about 2 minutes, no diagnostics) on the candidate of
485-  WO-2026-10-25-001 and, as a control, on the unmodified base.
486-
/Users/chipmc/.particle/toolchains/buildtools:
1.1.1

/Users/chipmc/.particle/toolchains/deviceOS:
2.1.0
6.3.4
6.4.1
6.6.0

/Users/chipmc/.particle/toolchains/gcc-arm:
10.2.1
9.2.1
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
# macOS
.DS_Store

# VS Code
.vscode/

# Particle / build outputs
*.bin
release/artifacts/*.bin
firmware*.bin
boron_firmware_*.bin
p2_firmware_*.bin

# Local build artifacts
/target/

# Visible scratch dir for temporary host-test build artifacts (must stay
# visible - no leading dot - per AI_DEVELOPMENT_WORKFLOW.md's "Temporary
# build and test artifacts" rule)
/build-tmp/
<shellId: 13 completed with exit code 0>
```

</details>


---

<sub>13m 51s</sub>

### `bash`

**Local toolchain build**

$ S=$PWD/build-tmp/wo-2026-11-02-004-stage6; mkdir -p $S/app && for x in src lib project.properties; do cp -R $x $S/app/; done
export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH
cd ~/.particle/toolchains/deviceOS/6.6.0/main && make -s PLATFORM=boron APPDIR=$S/app TARGET_DIR=$S/app/localbuild DEVICE_OS_PATH=~/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE=$S/bpb-$(date +%s) > $S/build.log 2>&1; echo rc=$?; tail -15 $S/build.log; ls $S/app/localbuild

<details>
<summary>27 lines</summary>

```
rc=0
/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/build-tmp/wo-2026-11-02-004-stage6/app/lib/LocalTimeRK/src/LocalTimeRK.cpp: In member function 'bool LocalTimeSchedule::getNextScheduledTime(LocalTimeConvert&, std::function<bool(LocalTimeScheduleItem&)>) const':
/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/build-tmp/wo-2026-11-02-004-stage6/app/lib/LocalTimeRK/src/LocalTimeRK.cpp:924:25: warning: suggest parentheses around '&&' within '||' [-Wparentheses]
  924 |             if (bResult && closestTime == 0 || tmpConvert.time < closestTime) {
      |                 ~~~~~~~~^~~~~~~~~~~~~~~~~~~
/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/build-tmp/wo-2026-11-02-004-stage6/app/lib/LocalTimeRK/src/LocalTimeRK.cpp: In member function 'void LocalTimeConvert::prevDay(LocalTimeHMS)':
/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/build-tmp/wo-2026-11-02-004-stage6/app/lib/LocalTimeRK/src/LocalTimeRK.cpp:1244:12: warning: unused variable 'origTime' [-Wunused-variable]
 1244 |     time_t origTime = time;
      |            ^~~~~~~~
/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/build-tmp/wo-2026-11-02-004-stage6/app/lib/LocalTimeRK/src/LocalTimeRK.cpp: In member function 'void LocalTimeConvert::nextDay(LocalTimeHMS)':
/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/build-tmp/wo-2026-11-02-004-stage6/app/lib/LocalTimeRK/src/LocalTimeRK.cpp:1255:12: warning: unused variable 'origTime' [-Wunused-variable]
 1255 |     time_t origTime = time;
      |            ^~~~~~~~
Creating /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/build-tmp/wo-2026-11-02-004-stage6/app/localbuild/platform_user_ram.ld ...
   text	   data	    bss	    dec	    hex	filename
 151636	   1114	   2200	 154950	  25d46	/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/build-tmp/wo-2026-11-02-004-stage6/app/localbuild/app.elf
app.bin
app.bin.crc_updated
app.bin.no_crc
app.bin.product
app.bin.suffix_updated
app.elf
app.hex
app.lst
app.map
obj
<shellId: 14 completed with exit code 0>
```

</details>


---

<sub>13m 57s</sub>

### `bash`

**nm linkage check**

$ cd build-tmp/wo-2026-11-02-004-stage6/app/localbuild; export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH; nm -C app.elf | grep -E "applyConfigurationFromLedger|SystemConfig::flushNow|timezoneStored|system_reset"; arm-none-eabi-objdump -d -C app.elf | awk '/<Cloud::applyConfigurationFromLedger/,/^$/' | grep -E "flushNow|system_reset|movs.*#8|r0, #8|r1, #8" | head

<details>
<summary>7 lines</summary>

```
000b6cac T SystemConfig::flushNow()
2003da84 b (anonymous namespace)::timezoneStored
000baf78 T Cloud::applyConfigurationFromLedger(particle::LedgerData const&, particle::LedgerData const&)
000c88b4 T system_reset
   bb042:	f7fb fe33 	bl	b6cac <SystemConfig::flushNow()>
   bb048:	2008      	movs	r0, #8
<shellId: 15 completed with exit code 0>
```

</details>


---

<sub>14m 0s</sub>

### `bash`

**Show flush→reset sequence**

$ cd build-tmp/wo-2026-11-02-004-stage6/app/localbuild; export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH; arm-none-eabi-objdump -d -C app.elf | awk '/^000baf78/,/^$/' | grep -n -A6 "b6cac" | head -12; cd ../../..; ls; git -C .. diff --stat -- src

<details>
<summary>13 lines</summary>

```
75:   bb042:	f7fb fe33 	bl	b6cac <SystemConfig::flushNow()>
76-   bb046:	2100      	movs	r1, #0
77-   bb048:	2008      	movs	r0, #8
78-   bb04a:	f011 fa13 	bl	cc474 <SystemClass::reset(unsigned long, particle::Flags<SystemResetFlagType, unsigned long>)>
79-   bb04e:	b924      	cbnz	r4, bb05a <Cloud::applyConfigurationFromLedger(particle::LedgerData const&, particle::LedgerData const&)+0xe2>
80-   bb050:	4644      	mov	r4, r8
81-   bb052:	4620      	mov	r0, r4
copilot.out
wo-2026-11-02-004-stage6
 src/ResetCause.h          |  1 +
 src/cloud/ConfigApply.cpp | 15 ++++++++++++---
 2 files changed, 13 insertions(+), 3 deletions(-)
<shellId: 16 completed with exit code 0>
```

</details>


---

<sub>14m 11s</sub>

### `bash`

**Check call-site context**

$ sed -n 528,545p src/state/State_Connect.cpp; sed -n 688,698p src/state/State_Connect.cpp

<details>
<summary>27 lines</summary>

```
if (SystemConfig::get_verboseMode()) {
        char data[64];
        snprintf(data, sizeof(data), "Connected in %i secs", SystemConfig::get_lastConnectionDuration());
        publishDiagnosticSafe("Cellular", data, PRIVATE);
      }

      Cloud::instance().logLedgerConnectState();

      bool configOk = Cloud::instance().loadConfigurationFromCloud();
      if (!configOk) {
        // Enhanced diagnostics for Alert 41 troubleshooting
        bool ledgersSynced = Cloud::instance().areLedgersSynced();
        unsigned long connectDuration = millis() - connectedStartMs;
        Log.warn("Configuration apply failed (will raise alert 41)");
        Log.warn("Alert 41 context: ledgersSynced=%s connectDuration=%lu ms",
                 ledgersSynced ? "true" : "false",
                 connectDuration);
        RecoveryState::raiseAlert(41);
    session.webhookAwaitStartMs = millis();
  }

  // Once connected, ensure configuration is loaded at least once
  if (Particle.connected()) {
    if (!configLoadedInUpdateMode) {
      Log.info("Connected in FIRMWARE_UPDATE_STATE - loading configuration from cloud");
      Cloud::instance().loadConfigurationFromCloud();
      configLoadedInUpdateMode = true;
    }
  }
<shellId: 17 completed with exit code 0>
```

</details>


---

<sub>14m 30s</sub>

### `create`

**create a new file at /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/docs/work-orders/WO-2026-11-02-004-stage6-copilot-report.md.**

/Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/docs/work-orders/WO-2026-11-02-004-stage6-copilot-report.md

```
Created file /Users/chipmc/Documents/Maker/Particle/Projects/gcc-wt/wo004/docs/work-orders/WO-2026-11-02-004-stage6-copilot-report.md with 5657 characters
```


---

<sub>14m 38s</sub>

### Copilot

The timezone planned reset is implemented and verified, and nothing is committed. `src/` is +10 net, at the hard budget, and the full host suite and the local toolchain build both pass.

**Change**
- **Reset cause:** `RESET_CAUSE_TIMEZONE_CHANGE = 8` added in `ResetCause.h`.
- **Flag and reset:** `src/cloud/ConfigApply.cpp` records a stored timezone change in a file-local flag. After `validateStoredData()` it calls `SystemConfig::flushNow()` and then `System.reset(RESET_CAUSE_TIMEZONE_CHANGE)`, the same pairing the failsafe uses.
- **File path:** the file is `src/cloud/ConfigApply.cpp`, not the `src/ConfigApply.cpp` the dispatch names.
- **Comment:** I reworded the existing "no forced flush" comment in place, because it would otherwise contradict the new block.

**Tests**
- **New test:** `tests/timezone_planned_reset_test.sh` covers the apply-path cases and the mutations. It extracts the real apply code and runs seven cases:
  - changed zone resets in the order store, flush, reset
  - same ledger value after the reboot does not reset
  - unchanged zone does not reset
  - failed apply does not reset
  - failed timing section does not reset
  - only another section changed does not reset
  - no timezone in the ledger does not reset
- **Mutations:** all eight planted mutations are caught at run time, none by a compile error. Six are on the apply path and two are on the cause code (0, and reused 7).
- **Structural test:** `tests/reset_cause_structural_test.py` now expects 8 reset sites and checks that code 8 is issued from `ConfigApply.cpp`.
- **Host suite:** 108/108 (sh via zsh, py via python3).

**Build**
- **Local toolchain build:** the Device OS 6.6.0 build used a fresh `BUILD_PATH_BASE` and returned 0.
- **`nm` check:** `nm` shows the apply function, `flushNow` and `system_reset`. The disassembly shows `flushNow`, then reset with code 8.

**Stage 7 check (a):** `applyConfigurationFromLedger` is not reached from `setup()`, so there is no needless reset before `LocalTime` is configured. It runs only after connect, from `State_Connect.cpp` and the deferred apply in `Cloud::loop()`.

**Open questions (more in the report):**
- **Reasoning level:** the dispatch asked for medium reasoning. The session ran at low, so medium was not what ran.
- **`finalizeLedgerAppliedConfig()` delay:** it runs after the apply returns, so on a timezone change it only runs after the reboot. This is not a loop, but it delays the valid-ledger marking by one connect.
- **Stage 7 check (b):** survival of daily totals, the open session and the publish queue across the reset is not covered by host tests and needs a bench check.

Report: `docs/work-orders/WO-2026-11-02-004-stage6-copilot-report.md`. Scratch output is left in `build-tmp/wo-2026-11-02-004-stage6/`.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>
