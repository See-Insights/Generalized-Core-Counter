# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `fe281a9c-87b0-4bf7-8505-e6484d1e487b`  
> - **Started:** 11/7/2026, 4:34:13 PM  
> - **Duration:** 29m 50s  
> - **Exported:** 11/7/2026, 5:04:03 PM  

---

<sub>7s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-05-004-stage6-r3-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>9s</sub>

### `bash`

**Read dispatch**

`$ cat docs/work-orders/WO-2026-11-05-004-stage6-r3-copilot-dispatch.md; ls docs/work-orders | head -50`

<details>
<summary>68 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/` (the S2 files already changed, plus the single call-site line after each `deepPowerDown()` in `src/state/State_Error.cpp` and `src/ConnectivityFailsafe.cpp`; the calls themselves unchanged), tests under `tests/` (S2 tests, the equivalence fixture and the four structural tests as F6 requires), and write the report; run host tests (zsh for .sh, python3 for .py), the local Device OS toolchain build and nm/objdump; scratch `build-tmp/wo-2026-11-05-004-stage6-r3/` (create it; use only that) / NOT authorized: committing, pushing, merging, flashing, creating PRs or tags, touching `lib/`, changing the `deepPowerDown()` calls themselves (S3), S1/S1b code, changing the persisted or `retained` layout, deleting anything you did not create (including `build-tmp/` itself).

# Stage 6 dispatch, round 3 (fixes after Stage 7 round 1): WO-2026-11-05-004 (v50 piece S2)
Binding spec: `docs/work-orders/WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md` **including Amendments 1-4** (read **Amendment 4 first**), and Codex's verdict `docs/work-orders/WO-2026-11-05-004-stage7-r1-verdict.md` (findings F1-F6, with file:line on 86b11e0). Base: this branch HEAD (S2 code at 86b11e0, plus records); your changes go on top, uncommitted. **Budget: `src/` at most +20 net against 3b5cfb8** (`git diff --numstat 3b5cfb8 -- src`; state the number); stop and report rather than trim style. **Run at the dispatched reasoning level (medium); the report states the level actually run, or that it could not be observed.** This is sleep/watchdog/wake logic: be conservative.

## Do (Amendment 4)
1. F1: sentinel 0xA5 for the evidence byte, compared exactly (`== 0xA5`); name it as a constant; legacy value 2 and 0 and other values are "no record".
2. F2: flush right after the boot clear; after each `deepPowerDown()` returns, clear the evidence and flush; at the failsafe caller also downgrade stage 3 (one line each at the two call sites; the calls unchanged).
3. F3: reason 30 = "successful HIBERNATE wake" everywhere (comments, tests, names); restate the 1,800 s test's meaning; no timing arm on reason 30.
4. F4: stage-3 evidence with `Time` invalid is rejected.
5. F5: restore `ab1805WakeReason` in the status payload with the constant `"N/A"`.
6. F6: restore base comparison in the equivalence fixture and the four structural tests, normalising only the approved S2 substitutions; add cases for GPIO wake, legacy marker 2, forced consumption, and a returning `deepPowerDown()`. Add a mutation for each of F1, F2, F4, F5 (each must fail; back up the file, mutate, restore from the backup; never `git checkout`/`git restore` while your uncommitted work is present).
## Verify
Host suite N/N (sh via zsh, py via python3; count with `p=$((p+1))`; baseline 118/118 at 86b11e0; `thermal_coupling_structural_test.py` also on a clean export of tracked files); local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE` as a make argument), text/data/bss (86b11e0: 151468 / 1114 / 2240) and `nm` showing the call sites linked; numstat net as above; layouts untouched.
## Stop and report if
the budget cannot hold; the fix needs a layout change; S1/S1b behaviour would have to change.
## Report
`docs/work-orders/WO-2026-11-05-004-stage6-r3-copilot-report.md`: model and reasoning level actually used, interpreter line, net `src/` lines per file, tests and mutation results, build and `nm` evidence, deviations, open questions. Claim nothing about hardware.
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
<shellId: 0 completed with exit code 0>
```

</details>


---

<sub>11s</sub>

### `bash`

**Read verdict and locate amendments**

`$ cd docs/work-orders; wc -c WO-2026-11-05-004*; git status --short | head; git log --oneline | head -5; cat WO-2026-11-05-004-stage7-r1-verdict.md; grep -n "Amendment 4" WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md`

<details>
<summary>75 lines</summary>

```
4227 WO-2026-11-05-004-stage6-copilot-dispatch.md
    9511 WO-2026-11-05-004-stage6-copilot-report.md
  187128 WO-2026-11-05-004-stage6-copilot-transcript.md
    4052 WO-2026-11-05-004-stage6-r2-copilot-dispatch.md
    7009 WO-2026-11-05-004-stage6-r2-copilot-report.md
  397308 WO-2026-11-05-004-stage6-r2-copilot-transcript.md
    3538 WO-2026-11-05-004-stage6-r3-copilot-dispatch.md
    6307 WO-2026-11-05-004-stage7-r1-codex-dispatch.md
    9791 WO-2026-11-05-004-stage7-r1-verdict.md
   19665 WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md
  648536 total
1fa868b WO-2026-11-05-004 (S2): Amendment 4 (F1-F6 rulings) and Stage 6 round 3 Copilot dispatch
32871a2 WO-2026-11-05-004 (S2): Stage 7 round 1 verdict (NOT VERIFIED on 86b11e0: 2 High, 4 Medium; architect rulings needed)
d229fbb WO-2026-11-05-004 (S2): Stage 7 round 1 Codex dispatch (UNDER REVIEW 86b11e0; 90-minute time-box, deadline 08:58:06Z)
86b11e0 WO-2026-11-05-004 (S2): sleep alarm, chip watchdog and wake classification on Device OS (Amendments 1-3); src/ net -140; suite 118/118
58f7d67 WO-2026-11-05-004 (S2): Amendment 3 (reason-20 window dropped, Error-path evidence byte, reason 30 by reason alone) and Stage 6 round 2 dispatch
UNDER REVIEW: wo/2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake @ 86b11e0df4422c27d08bcf58df936cb31726cf76

NOT VERIFIED for S2 as scoped — Round 1 of at most two.

`git rev-parse HEAD` matched the required full SHA before review. No tracked files were edited.

| # | Verdict | Evidence and conclusion |
|---|---|---|
| 1 | PASS | `src/state/State_Sleep.cpp:700-704` configures HIBERNATE with `duration()` and both GPIO wakes. The stale-alarm clear, `interruptAtTime`, and both bypass fallbacks are gone. Native sleep failure reaches `HibernateCycle::abandon()`, restarts the external watchdog, and falls through to ULP at `State_Sleep.cpp:731-739`. The 900–36000 s eligibility window remains intact at `HibernateEligibility.cpp:39-41`. Because the Device OS alarm is relative to the sleep call, preparation time after the earlier RTC sample is included in reported `actualSleep/sleepError`; it does not invalidate the duration window. |
| 2 | NOT VERIFIED | Software inspection passes: `ExternalWatchdog.init(timeout(124000))` and `start()` are at `Generalized-Core-Counter.cpp:764-766`; there is one refresher at `:1131`; and the five former AB1805 sites become two stops and three starts covering HIBERNATE, ULP/STOP, errors, and fallbacks. Device OS `init()` stops and reconfigures an already-running chip dog. Actual reset behavior and the v50→v49 rollback with the dog still armed require hardware. |
| 3 | FAIL | `HibernateCycle.cpp:39-93` correctly gives reason 30 the wake-reason arm and removes the `dpdOnTime` rule. However, Device OS maps both RTC and GPIO SYSTEM-OFF wakes to reason 30 and exposes no source distinction. Therefore a button/GPIO wake satisfying the remaining persisted-time arms is reported as a successful alarm wake. `tests/hibernate_wake_diagnostics_test.cpp:83-90` explicitly freezes even a 1,800-second early wake as success. |
| 4 | FAIL | The inference uses persisted evidence only, and the evidence byte is outside the `RecoveryState` façade without changing `SysData` size/layout. However, the byte was historically `structuresVersion=2`, while current classification treats any nonzero value as evidence. Clearing after classification is deferred rather than forced, and a returning/failing `deepPowerDown()` leaves evidence set. The time-invalid stage-3 rule also accepts evidence without proving the specified 600-second recency. |
| 5 | FAIL | `Generalized-Core-Counter.cpp:1290-1295` removes the published `ab1805WakeReason` key and changes `watchdogSource` to `inferred`/`NONE`. Amendment 1 authorizes `inferred`; it does not authorize deleting the field. The status event is unversioned, so the ledger schema bump does not cover this contract change. The `ab1805_watchdog` event label remains. No live repository test, fixture, script, or contract currently consumes the removed field—the remaining risk is external fleet consumers. |
| 6 | FAIL | All host tests pass, but protections were weakened beyond the intentional implementation change. The execution fixture no longer compares against the base and can regenerate its own new truth; four structural tests remove the entire `hibernateAttempt()` block before comparison; and the wake test freezes reason-30-at-any-time as success. Retirement of alarm-programming scenarios and the old AB1805 classifier is otherwise appropriate, but the replacement coverage misses legacy byte value 2, forced persistence, failed-return cleanup, and GPIO misclassification. |
| 7 | PASS | ELF `nm`/objdump found no `clearAb1805StaleAlarmInterrupts`, `interruptAtTime`, `ab1805WakeReasonName`, `AB1805::stopWDT`, `resumeWDT`, or `loop` references. `AB1805::setWDT` remains only as an internal call from `deepPowerDown()`. Executable `ab1805.` calls are limited to the two S3 `deepPowerDown()` callers plus S1b `PreBindRtc`. S1/S1b implementation files are untouched. |
| 8 | PASS | Net `src/` is 129 additions and 269 deletions: **−140**, satisfying `<= +20`. Clean Device OS 6.6.0 Boron build: **text 151468, data 1114, bss 2240**; base was 153100/1114/2240. Host suite: **118/118**—66 shell tests through zsh and 52 Python tests through python3. Thermal structural test also passed from a clean archive. All 15 Copilot mutations were caught; two additional mutations—removing the HIBERNATE button GPIO and changing duration scaling from 1000 to 100—were also caught. |
| 9 | NOT VERIFIED | The S3 handoff has stale-evidence failure modes described below. Device OS can also fall back to an OS timer after a transient external-RTC `setAlarm` error, despite SYSTEM OFF relying on the chip alarm; this needs hardware validation. The post-POWER_OFF reason and overnight behavior were not available on the host. |

## Blocking findings

1. **High — legacy persisted value can suppress watchdog classification**  
   `src/MyPersistentData.h:121`, `src/MyPersistentData.cpp:1068-1069`, `src/Generalized-Core-Counter.cpp:756`

   The reserved byte previously stored `structuresVersion=2`. Testing it with `!= 0` mistakes legacy value 2 for a deliberate POWER_OFF marker, classifying reason 20 as power-up instead of inferred watchdog.

   Smallest fix: write a named sentinel and compare for that exact sentinel, at minimum changing the reader to `== 1`; add a legacy-value-2 test. This fits the `+20` budget.

2. **High — evidence is not durably consumed or cleared after a failed handoff**  
   `src/Generalized-Core-Counter.cpp:761`, `src/state/State_Error.cpp:170-179`, `src/connectivity/ConnectivityFailsafe.cpp:312-316`

   The boot clear is deferred by the persistence layer. A power loss before the normal loop flush reloads the stale marker. If either `deepPowerDown()` call fails or returns, its evidence remains set; the failsafe stage-3 record also remains recent.

   Smallest fix:

   - Force a persistence flush immediately after consuming/clearing the boot marker.
   - After each `deepPowerDown()` call returns, clear its evidence and force a flush.
   - For the failsafe caller, also clear or downgrade the stage-3 evidence on return.

   Approximately 3–6 source lines, within budget.

3. **Medium — GPIO/button wake is mislabeled as an alarm wake**  
   `src/hibernate/HibernateCycle.cpp:39-93`, `tests/hibernate_wake_diagnostics_test.cpp:83-90`

   Reason 30 is generic SYSTEM-OFF wake on this platform, not proof of the RTC source. With no timing arm, early button wakes pass as successful timed wakes.

   This needs an architect ruling because Amendment 3 simultaneously removes the available timing discriminator. Either define reason 30 as a generic successful HIBERNATE wake and stop calling it an alarm wake, or authorize another persisted/hardware discriminator or narrowly defined early-wake arm. Any code-sized option fits the budget.

4. **Medium — stage-3 evidence with invalid time has no 600-second recency proof**  
   `src/hibernate/HibernateWakeDiagnostics.h:163-170`

   Copilot flag assessment:

   - **(a) Time invalid accepts stage 3:** needs an architect rule; it does not literally satisfy the 600-second-recency requirement.
   - **(b) `deepPowerDown()` returns and leaves the byte set:** not acceptable; clear and synchronously persist on return.
   - **(c) Deferrals refresh `lastConnectivityRecoveryAction` during stage 3:** this premise is false in the current code. `ConnectivityFailsafe.cpp:237-243` returns before the deferral update at `:288-297`. Only a failed/returned power-down leaves recent stage-3 evidence.
   - **(d) Manual reset can look like inferred watchdog:** accepted residual per Amendment 3.

5. **Medium — unapproved fleet-visible payload removal**  
   `src/Generalized-Core-Counter.cpp:1290-1295`

   Removing `ab1805WakeReason` is a contract change not covered by the ledger schema version. Restore the stable key with an unavailable/unknown value, or obtain an explicit architect ruling and document the status-event contract in `docs/contracts/` and `docs/FIELD_MEANINGS_REFERENCE.md`. Restoring it fits within `+20`.

6. **Medium — test equivalence baseline was weakened**  
   `tests/sleep_execution_equivalence_test.sh:4-12`, `tests/sleep_execution_equivalence_test.cpp:55,97-117,177`

   The re-frozen fixture no longer proves equivalence against the base. Several structural tests likewise discard the whole hibernate function before comparison. Restore a base comparison that normalizes only the approved S2 alarm/watchdog substitutions, then add regression cases for GPIO wake, legacy marker 2, forced consumption, and returning `deepPowerDown()`. Test-only changes do not affect the source budget.

## Budget and removal audit

The reported **−140** is correct. Most of the removal is exactly the retired implementation named by the work order: the stale-alarm helper, AB1805 alarm pre-arm/bypass code, AB1805 wake-reason formatter/classification plumbing, and old watchdog calls.

It is not entirely benign named retirement: it also includes removal of the published `ab1805WakeReason` field. The test-side deletion/refreezing also removes broader equivalence protection.

## Verification record

- Model: **gpt-5.6-sol**
- Reasoning level: **xhigh**
- Interpreters: **zsh 5.9**, **Python 3.14.6**
- Target/toolchain: Device OS **6.6.0**, Boron, GNU Arm **10.2.1**
- Build: **151468 text / 1114 data / 2240 bss**
- Host suite: **118/118**
- Mutations: **17/17 caught**—15 supplied plus 2 new
- Build warnings: five warnings, none in S2-touched lines; the Stage 6 “no warnings” observation was not reproduced.
- Scratch use was confined to `build-tmp/wo-2026-11-05-004-stage7-r1/`.

No hardware testing was possible. The 8-hour nights, physical watchdog proof, post-POWER_OFF reset reason, transient alarm behavior, and v50→v49 rollback with the watchdog armed remain Chip’s bench items.75:## Amendment 4 (Chip, 7 Nov 2026, after Stage 7 round 1: NOT VERIFIED on 86b11e0; `WO-2026-11-05-004-stage7-r1-verdict.md`)
<shellId: 1 completed with exit code 0>
```

</details>


---

<sub>13s</sub>

### `bash`

**Read Amendment 4 and prior report**

`$ cd docs/work-orders; sed -n 75,200p WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md; cat WO-2026-11-05-004-stage6-r2-copilot-report.md | head -60`

<details>
<summary>63 lines</summary>

```
## Amendment 4 (Chip, 7 Nov 2026, after Stage 7 round 1: NOT VERIFIED on 86b11e0; `WO-2026-11-05-004-stage7-r1-verdict.md`)
Rulings on Codex's findings F1-F6:
1. **F1 (High): sentinel.** The evidence byte (`reservedUint8_0`, legacy `structuresVersion` = 2) is written with the sentinel **0xA5** and compared for **exactly 0xA5**; any other value (0, the legacy 2, anything else) is "no record". Add a legacy-value-2 test.
2. **F2 (High): durable consumption.** Flush **right after the boot clear** of the byte. After **each `deepPowerDown()` returns** (it should not), clear the evidence and flush, and for the failsafe caller **downgrade the stage-3 record** so it is no longer recent evidence. **The one call-site line in `ConnectivityFailsafe.cpp` is authorised** (the `deepPowerDown()` call itself stays unchanged; S3 replaces it).
3. **F3 (Medium): reason 30 is defined as a "successful HIBERNATE wake"**, not an "alarm wake" (code comments, the table, tests, report wording). It covers RTC and GPIO wakes alike. **Recorded residual:** an early button/GPIO wake passes the reason-30 arm and cannot be told from an alarm wake in firmware; the bench's actual-versus-requested sleep time shows it. The existing 1,800 s early-wake test is restated: it freezes "reason 30 is a successful HIBERNATE wake whatever the elapsed time", not "an early wake is an alarm wake". No 0..+60 s rule on reason 30 (Amendment 3 stands).
4. **F4 (Medium):** stage-3 evidence with `Time` **invalid is rejected** (inferred watchdog).
5. **F5 (Medium): restore `ab1805WakeReason` in the status payload with the stable string `"N/A"`.** Removal goes to a cleanup WO with a fleet-ops notice (not here).
6. **F6 (Medium, test-only): restore the base-comparison tests** (the equivalence fixture and the four structural tests keep comparing `hibernateAttempt()` against the base), normalising **only the approved S2 substitutions**. Add cases: GPIO wake, legacy marker 2, forced consumption (flush after the boot clear), and a returning `deepPowerDown()` (evidence cleared, stage-3 downgraded).
7. **Budget unchanged: `src/` at most +20 net** (the net is far below it). Copilot at medium; the report states the level actually run. Stage 7 round 2 is the last (Codex xhigh, 90-minute box, SHA rule); a failure goes to the architect.
# WO-2026-11-05-004 (v50 S2) – Stage 6 round 2 – Copilot report

Nothing was committed, pushed, merged or released. Everything is uncommitted on `wo/2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake` (base 3b5cfb8).
**No hardware claim is made.** Only host tests, a compile, and `nm` were run.

## Run details
- Model: claude-sonnet-5.5. Dispatched at medium reasoning; the actual level could not be observed.
- Interpreters: zsh 5.9 (arm64-apple-darwin26.0), Python 3.14.6, clang++ (Apple).
- Firmware build: Device OS 6.6.0, boron, buildscripts 1.17.2, gcc-arm 10.2.1. It compiled with no warnings. Size text/data/bss = 151468 / 1114 / 2240 (the baseline was 153100 / 1114 / 2240, i.e. -1632 text).

## src/ changes (net vs 3b5cfb8: **-140**, budget +20)
| file | +/- |
|---|---|
| Generalized-Core-Counter.cpp | +47/-70 |
| state/State_Sleep.cpp | +46/-58 (the hibernate block is de-indented, so its diff is inflated) |
| time/HibernateCycle.cpp / .h | +6/-48, +8/-26 |
| time/ClockDiagnostics.cpp / .h | 0/-30, 0/-2 |
| time/HibernateEligibility.cpp / .h | 0/-30, 0/-1 |
| time/HibernateWakeDiagnostics.h | +12/-2 |
| persist/RecoveryState.h, MyPersistentData.cpp | +6/-1, +3/0 |
| state/State_Error.cpp | +1/-1 |

What was done:
- **Alarm:** the hibernate config is `HIBERNATE` + `duration(wakeInSeconds*1000)` + both GPIO wakes. The AB1805 `interruptAtTime` pre-arm, the stale-interrupt clear and both bypass fallbacks are removed.
- **Chip watchdog:** `ExternalWatchdog.init(timeout 124000)` and `start()` in setup, and `refresh()` in the slot where `ab1805.loop()` was. `stop()` precedes and `start()` follows every sleep call (2 stops, 3 starts).
- **Wake classification:** `classifyWake(int)` no longer reads the AB1805. Reason 30 alone is the alarm wake, with no DPD window.
- **Reason 20 (PIN_RESET):** judged from persisted evidence only, via `isInferredWatchdog()`. `retained` is never evidence, because a power-down clears it.
  - Evidence is the new byte, or failsafe stage 3 with `lastConnectivityRecoveryAction` within 600 s.
  - The byte is `SysData::reservedUint8_0`, so there is no layout change.
  - It is written in State_Error `case 3` before `flushAllBeforeReset()` and `deepPowerDown()`, and cleared once at boot after classification.
  - `lastWatchdogSource = WATCHDOG_SOURCE_INFERRED (2)`.
- **Payload:** `ab1805WakeReason` is removed from the status payload. `watchdogSource` is now `"inferred"` or `"NONE"`.
- **Kept for S3:** the `deepPowerDown()` calls (State_Error, ConnectivityFailsafe), `extern AB1805` and its include in `StateMachine.h`, the object definition and the PreBindRtc call. `lib/` is untouched.

## Tests
- The full host suite is **118/118** (baseline was 117).
- `thermal_coupling_structural_test.py` passes on a clean copy.
- Binary `nm`:
  - Present: `RecoveryState::{get,set}_powerOffRequested`, `HibernateCycle::classifyWake(int)` and the ExternalWatchdog/Am18x5 symbols.
  - Absent: `clearAb1805StaleAlarmInterrupts`, `interruptAtTime`, `ab1805WakeReasonName`, `AB1805::stopWDT`, `AB1805::resumeWDT` and `AB1805::loop`.
  - `AB1805::setWDT` is still present: it is the vendored library's own code, called from the library's `setup()`, and the app no longer calls it.

New tests:
- `watchdog_inferred_classification_test.{cpp,sh}` runs the real `isInferredWatchdog` against a table (no record, byte, recent and stale stage 3, future timestamp, other stages, time invalid). It has 5 mutants, all caught: always wake, never wake, no recency, no byte, any stage.
- `s2_alarm_watchdog_wake_structural_test.py` has 10 in-memory probes, all detected:
  - reason 20 always a wake
  - evidence read from `retained`
  - byte not cleared
  - byte cleared before read
  - byte written after the call
  - alarm armed twice
  - no watchdog stop before hibernate
  - refresh missing
  - hibernate-fallback restart removed
  - stale-clear back
  - It also checks that no `ab1805.` call other than `deepPowerDown` remains.

Updated tests (they pinned behaviour that S2 intentionally changes):
- `watchdog_ab1805_classification_test.{cpp,sh}` is **deleted** and replaced by the test above (the AB1805 PIN_RESET path no longer exists).
- `hibernate_wake_diagnostics_test`: the DPD-window mirror is replaced by a reason-30-alone mirror, and the fidelity greps are updated.
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>15s</sub>

### Copilot

Now the source.

---

<sub>15s</sub>

### `bash`

**Inspect relevant src**

`$ git diff 3b5cfb8 --stat -- src tests | tail -30; grep -n "reservedUint8_0\|powerOffRequested\|PowerOffRequested" -r src | head -30; cat src/time/HibernateWakeDiagnostics.h | sed -n 130,200p; grep -n "deepPowerDown" -B12 -A6 src/state/State_Error.cpp src/connectivity/ConnectivityFailsafe.cpp`

<details>
<summary>101 lines</summary>

```
src/Generalized-Core-Counter.cpp                   |  117 +-
 src/MyPersistentData.cpp                           |    3 +
 src/persist/RecoveryState.h                        |    7 +-
 src/state/State_Error.cpp                          |    2 +-
 src/state/State_Sleep.cpp                          |  104 +-
 src/time/ClockDiagnostics.cpp                      |   30 -
 src/time/ClockDiagnostics.h                        |    2 -
 src/time/HibernateCycle.cpp                        |   54 +-
 src/time/HibernateCycle.h                          |   34 +-
 src/time/HibernateEligibility.cpp                  |   30 -
 src/time/HibernateEligibility.h                    |    1 -
 src/time/HibernateWakeDiagnostics.h                |   14 +-
 ...clock_hibernate_helpers_move_structural_test.py |   24 +-
 tests/fixtures/sleep_execution_transcript.txt      | 1636 ++++++--------------
 tests/hibernate_wake_diagnostics_test.cpp          |  113 +-
 tests/hibernate_wake_diagnostics_test.sh           |   25 +-
 tests/s2_alarm_watchdog_wake_structural_test.py    |  167 ++
 tests/sleep_cloud_gate_structural_test.py          |   14 +-
 tests/sleep_config_ownership_structural_test.py    |    5 +-
 tests/sleep_execution_equivalence_test.cpp         |   80 +-
 tests/sleep_execution_equivalence_test.sh          |   79 +-
 tests/sleep_plan_structural_test.py                |   16 +-
 tests/sleep_preflight_structural_test.py           |   16 +-
 tests/sleep_wake_structural_test.py                |   14 +-
 tests/stubs/sleep_teardown/SleepExecutionFakes.h   |   12 +-
 tests/watchdog_ab1805_classification_test.cpp      |  352 -----
 tests/watchdog_ab1805_classification_test.sh       |   78 -
 tests/watchdog_inferred_classification_test.cpp    |   37 +
 tests/watchdog_inferred_classification_test.sh     |   34 +
 29 files changed, 1024 insertions(+), 2076 deletions(-)
src/MyPersistentData.cpp:1068:uint8_t get_powerOffRequested() { return sysStatus.getValue<uint8_t>(offsetof(sysStatusData::SysData, reservedUint8_0)); }
src/MyPersistentData.cpp:1069:void set_powerOffRequested(uint8_t value) { sysStatus.setValue<uint8_t>(offsetof(sysStatusData::SysData, reservedUint8_0), value); }
src/MyPersistentData.h:121:		uint8_t reservedUint8_0;                   // Reserved: was structuresVersion; kept to preserve the stored layout
src/state/State_Error.cpp:176:      RecoveryState::set_powerOffRequested(1); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog
src/Generalized-Core-Counter.cpp:756:        RecoveryState::get_powerOffRequested() != 0, RecoveryState::get_connectivityRecoveryStage(),
src/Generalized-Core-Counter.cpp:761:  RecoveryState::set_powerOffRequested(0);
src/persist/RecoveryState.h:79:uint8_t get_powerOffRequested();
src/persist/RecoveryState.h:80:void set_powerOffRequested(uint8_t value);
  f.hibernateCount = hibernateCount;
  if (f.arm == GateArm::kNone) {
    f.actualSleepSec = actualSleepSecOnSuccess;
    f.sleepErrorSec = sleepErrorSecOnSuccess;
  } else {
    // Gate failed - rtcAtWake/rtcBefore did not both qualify, so a duration
    // derived from them would not be meaningful. Report 0/0 rather than
    // fabricate a duration.
    f.actualSleepSec = 0;
    f.sleepErrorSec = 0;
  }
  return f;
}

/// Builds the bounded JSON payload for the "hibernate_wake" queued event,
/// mirroring the snprintf-into-fixed-buffer pattern of
/// publishWatchdogForensics(). Returns whatever snprintf() returns (the
/// number of bytes that would have been written, excluding the NUL,
/// possibly >= bufferSize on truncation) so a caller can detect truncation
/// exactly the way the watchdog forensic publisher's snprintf call does.
inline int buildEventPayload(char *buffer, size_t bufferSize, const EventFields &f) {
  const bool ok = f.arm == GateArm::kNone;
  return snprintf(
      buffer, bufferSize,
      "{\"result\":\"%s\",\"gateArm\":\"%s\",\"osReason\":%d,\"wakeReason\":\"%s\","
      "\"rtcOk\":%d,\"req\":%lu,\"rtcBefore\":%lld,\"rtcAt\":%lld,\"actual\":%lu,"
      "\"err\":%ld,\"count\":%lu}",
      ok ? "ok" : "fail", gateArmName(f.arm), f.osResetReason, f.wakeReasonName,
      f.rtcReadOk ? 1 : 0, (unsigned long)f.requestedSleepSec, (long long)f.rtcBefore,
      (long long)f.rtcAtWake, (unsigned long)f.actualSleepSec, (long)f.sleepErrorSec,
      (unsigned long)f.hibernateCount);
}

/// Reason-20 boot: persisted power-off evidence (the Error-path byte, or a
/// recent failsafe stage 3) means a power-up wake; none means an inferred
/// watchdog. `retained` is never evidence (a power-down clears it).
constexpr int64_t kPowerOffEvidenceMaxAgeSec = 600;
inline bool isInferredWatchdog(bool errorPathByte, uint8_t failsafeStage, int64_t failsafeActionAt, bool timeValid, int64_t now) {
  const int64_t age = now - failsafeActionAt;
  const bool failsafeRecent = failsafeStage == 3 && (!timeValid || (age >= 0 && age <= kPowerOffEvidenceMaxAgeSec));
  return !errorPathByte && !failsafeRecent;
}

} // namespace HibernateWakeDiagnostics

#endif
src/state/State_Error.cpp-166-      System.reset(RESET_CAUSE_ERROR_STATE_SOFT);
src/state/State_Error.cpp-167-    }
src/state/State_Error.cpp-168-    break;
src/state/State_Error.cpp-169-
src/state/State_Error.cpp-170-  case 3:
src/state/State_Error.cpp-171-    // Hard recovery using AB1805 deep power down after delay. This fully
src/state/State_Error.cpp-172-    // power-cycles the device and modem but is limited by resolveErrorAction
src/state/State_Error.cpp-173-    // avoid thrashing.
src/state/State_Error.cpp-174-    if (millis() - resetTimer > resetWait) {
src/state/State_Error.cpp-175-      Log.info("Executing deep power down from ERROR_STATE (alert=%d)", RecoveryState::get_alertCode());
src/state/State_Error.cpp-176-      RecoveryState::set_powerOffRequested(1); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog
src/state/State_Error.cpp-177-      flushAllBeforeReset();
src/state/State_Error.cpp:178:      ab1805.deepPowerDown();
src/state/State_Error.cpp-179-    }
src/state/State_Error.cpp-180-    break;
src/state/State_Error.cpp-181-
src/state/State_Error.cpp-182-  default:
src/state/State_Error.cpp-183-    // Should not happen, but don't get stuck here.
src/state/State_Error.cpp-184-    transitionTo(IDLE_STATE, "error-unknown-resolution");
grep: src/connectivity/ConnectivityFailsafe.cpp: No such file or directory
<shellId: 3 completed with exit code 2>
```

</details>


---

<sub>18s</sub>

### `bash`

**Inspect call sites**

`$ sed -n 745,770p src/Generalized-Core-Counter.cpp; sed -n 1280,1300p src/Generalized-Core-Counter.cpp; grep -n "deepPowerDown" -B10 -A5 src/ConnectivityFailsafe.cpp; grep -n "flushAllBeforeReset" -A12 src/*.cpp | head -30; grep -n "set_connectivityRecoveryStage\|lastConnectivityRecoveryAction\|RecoveryAction" src/persist/RecoveryState.h; git diff 3b5cfb8 --numstat -- src | awk '{a+=$1;d+=$2}END{print a,d,a-d}'`

<details>
<summary>51 lines</summary>

```
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
        RecoveryState::get_powerOffRequested() != 0, RecoveryState::get_connectivityRecoveryStage(),
        (int64_t)RecoveryState::get_lastConnectivityRecoveryAction(), timeValid, (int64_t)Time.now());
    startupInferredWatchdog = inferredWatchdog;
    Log.info("PIN_RESET: %s", inferredWatchdog ? "no power-off evidence - inferred watchdog" : "power-off evidence - power-up wake");
  }
  RecoveryState::set_powerOffRequested(0);

  // Device OS keeps the chip watchdog running across a reset; init() re-arms it.
  if (ExternalWatchdog.init(WatchdogConfiguration().timeout(124000)) != SYSTEM_ERROR_NONE || ExternalWatchdog.start() != SYSTEM_ERROR_NONE) {
    Log.warn("ExternalWatchdog: init/start failed");
  }

  // ===== WATCHDOG RESET PERSISTENCE / FORENSICS =====
  // Moved here (from immediately after SystemConfig::setup(), before AB1805 init)
  // so it can act on the classification above. The publish queue was
  if (startupHibernateStatusReady) {
    snprintf(hibernateFields,
             sizeof(hibernateFields),
             ",\"sleepMode\":\"hibernate\",\"wakeReason\":\"%s\",\"actualSleep\":%lu,\"sleepError\":%ld,\"hibernateCount\":%lu",
             startupHibernateWakeReason,
             (unsigned long)startupHibernateActualSleepSec,
             (long)startupHibernateSleepErrorSec,
             (unsigned long)HibernateCycle::currentHibernateCount());
  }

  // Only present when the OS reset reason was PIN_RESET (see the wake
  // classification block in setup()).
  char pinResetFields[64] = "";
  if (startupPinResetChecked) {
    snprintf(pinResetFields, sizeof(pinResetFields), ",\"watchdogSource\":\"%s\"", startupInferredWatchdog ? "inferred" : "NONE");
  }

#if defined(ENABLE_PMIC_FORENSICS) && ENABLE_PMIC_FORENSICS
  snprintf(status, sizeof(status),
           "{\"version\":\"%s\",\"resetReason\":%d,\"resetReasonData\":%lu,\"alert\":%d,\"lastAlert\":%ld,\"freeHeap\":%lu,\"appBreadcrumb\":%u,\"appBreadcrumbMs\":%lu,\"watchdogResetCount\":%u,\"lastWatchdogBreadcrumb\":%u,\"lastWatchdogUptimeMs\":%lu,\"lastWatchdogResetReasonData\":%lu,\"pmicAnomalyCount\":%u,\"lastPmicAnomalySoc\":%.2f,\"lastPmicAnomalyChargeStatus\":%u,\"lastPmicAnomalyAgeSec\":%lu,\"lastPmicAnomalyPowerSource\":%u,\"lastPmicAnomalyVbusStatus\":%u,\"failsafeStage\":%u,\"failsafeCount\":%u,\"failsafeLastAction\":%ld,\"lastConnectionAgeSec\":%ld,\"failsafeTest\":%d,\"failsafeTestMode\":%d%s%s}",
           FIRMWARE_VERSION,
grep: src/ConnectivityFailsafe.cpp: No such file or directory
src/ThrashGuard.cpp:152:  flushAllBeforeReset();
src/ThrashGuard.cpp-153-  System.reset(RESET_CAUSE_THRASH_TIER3);
src/ThrashGuard.cpp-154-}
50:void set_connectivityRecoveryStage(uint8_t value);
53:time_t get_lastConnectivityRecoveryAction();
54:void set_lastConnectivityRecoveryAction(time_t value);
129 269 -140
<shellId: 4 completed with exit code 0>
```

</details>


---

<sub>20s</sub>

### `bash`

**Find failsafe site and base payload**

`$ grep -n "deepPowerDown" -B8 -A6 src/connectivity/*.cpp; grep -rn "flushAllBeforeReset" src | grep -v "^src/state\|ThrashGuard" | head; git show 3b5cfb8:src/Generalized-Core-Counter.cpp | grep -n "ab1805WakeReason" ; git show 3b5cfb8:src/Generalized-Core-Counter.cpp | grep -n "pinResetFields" `

```
grep: src/connectivity/*.cpp: No such file or directory
src/power/ConnectivityFailsafe.cpp:306:    flushAllBeforeReset();
src/power/ConnectivityFailsafe.cpp:315:  flushAllBeforeReset();
1315:             ",\"ab1805WakeReason\":\"%s\",\"watchdogSource\":\"%s\"",
<shellId: 5 completed with exit code 1>
```


---

<sub>23s</sub>

### `bash`

**View failsafe and base payload**

`$ sed -n 296,325p src/power/ConnectivityFailsafe.cpp; git show 3b5cfb8:src/Generalized-Core-Counter.cpp | sed -n 1305,1325p; grep -rn "flushAllBeforeReset()" src/*.h src/*/*.h | head -3`

<details>
<summary>42 lines</summary>

```
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
  // told apart in cloud telemetry, without requiring serial-log tracing.
  // No separate "read ok" flag is published: WakeReason::UNKNOWN already and
  // correctly covers both "chip detection failed" and "no known wake-reason
  // bit set" (see the classification block above), so a distinct success
  // flag would be redundant and cannot be obtained without repeating the
  // destructive updateWakeReason() read that Fix 2 removed.
  char pinResetAb1805Fields[160] = "";
  if (startupPinResetAb1805Checked) {
    snprintf(pinResetAb1805Fields,
             sizeof(pinResetAb1805Fields),
             ",\"ab1805WakeReason\":\"%s\",\"watchdogSource\":\"%s\"",
             startupAb1805WakeReasonName,
             startupAb1805ConfirmedWatchdog ? "AB1805_PIN" : "NONE");
  }

#if defined(ENABLE_PMIC_FORENSICS) && ENABLE_PMIC_FORENSICS
  snprintf(status, sizeof(status),
           "{\"version\":\"%s\",\"resetReason\":%d,\"resetReasonData\":%lu,\"alert\":%d,\"lastAlert\":%ld,\"freeHeap\":%lu,\"appBreadcrumb\":%u,\"appBreadcrumbMs\":%lu,\"watchdogResetCount\":%u,\"lastWatchdogBreadcrumb\":%u,\"lastWatchdogUptimeMs\":%lu,\"lastWatchdogResetReasonData\":%lu,\"pmicAnomalyCount\":%u,\"lastPmicAnomalySoc\":%.2f,\"lastPmicAnomalyChargeStatus\":%u,\"lastPmicAnomalyAgeSec\":%lu,\"lastPmicAnomalyPowerSource\":%u,\"lastPmicAnomalyVbusStatus\":%u,\"failsafeStage\":%u,\"failsafeCount\":%u,\"failsafeLastAction\":%ld,\"lastConnectionAgeSec\":%ld,\"failsafeTest\":%d,\"failsafeTestMode\":%d%s%s}",
           FIRMWARE_VERSION,
           resetReason,
           (unsigned long)resetReasonData,
src/state/StateMachine.h:129:void flushAllBeforeReset();
<shellId: 6 completed with exit code 0>
```

</details>


---

<sub>25s</sub>

### `bash`

**Look at persist helpers**

`$ grep -n "persistConnectivityFailsafeState" -A14 src/power/ConnectivityFailsafe.cpp | head -30; grep -n "void flushAllBeforeReset" -A15 src/*.cpp src/*/*.cpp; grep -n "startupPinResetChecked" src/*.cpp | head`

<details>
<summary>51 lines</summary>

```
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
src/state/State_Error.cpp:114:void flushAllBeforeReset() {
src/state/State_Error.cpp-115-  CurrentReadings::flushNow();
src/state/State_Error.cpp-116-  SystemConfig::flushNow();
src/state/State_Error.cpp-117-  SystemConfig::SensorSettings::flushNow();
src/state/State_Error.cpp-118-  PublishQueuePosix::instance().writeQueueToFiles();
src/state/State_Error.cpp-119-}
src/state/State_Error.cpp-120-
src/state/State_Error.cpp-121-void handleErrorState() {
src/state/State_Error.cpp-122-  static unsigned long resetTimer = 0;
src/state/State_Error.cpp-123-  static int resolution = 0;
src/state/State_Error.cpp-124-
src/state/State_Error.cpp-125-  if (state != oldState) {
src/state/State_Error.cpp-126-    publishStateTransition();
src/state/State_Error.cpp-127-
src/state/State_Error.cpp-128-    // Safety: regardless of recovery choice, do not leave radio/modem powered
src/state/State_Error.cpp-129-    // while we sit in ERROR_STATE waiting for reset.
src/Generalized-Core-Counter.cpp:247:// field is published only when startupPinResetChecked is true.
src/Generalized-Core-Counter.cpp:248:bool startupPinResetChecked = false;
src/Generalized-Core-Counter.cpp:753:    startupPinResetChecked = true;
src/Generalized-Core-Counter.cpp:1293:  if (startupPinResetChecked) {
<shellId: 7 completed with exit code 0>
```

</details>


---

<sub>31s</sub>

### `bash`

**Check flush and tests**

`$ grep -rn "flushNow" src/SystemConfig* src/persist/*.h src/MyPersistentData.h | head; grep -n "ab1805WakeReason\|powerOffRequested\|0xA5\|kPowerOff" -r tests | head -40; sed -n 1,60p tests/sleep_execution_equivalence_test.sh`

<details>
<summary>71 lines</summary>

```
grep: src/SystemConfig*: No such file or directory
src/persist/CurrentReadings.h:29:void flushNow();  // forwards to flush(true): synchronous forced write
src/persist/SystemConfig.h:92:void flushNow();
src/persist/SystemConfig.h:230:void flushNow();  // forwards to flush(true): synchronous forced write
tests/watchdog_inferred_classification_test.sh:31:mutant no_recency "(!timeValid || (age >= 0 && age <= kPowerOffEvidenceMaxAgeSec))" "true"
tests/retained_boot_test.cpp:13:  std::memset(&r, 0xA5, sizeof(r));
tests/s2_alarm_watchdog_wake_structural_test.py:45:                 "ab1805WakeReasonName", "getWakeReason", "updateWakeReason"):
tests/s2_alarm_watchdog_wake_structural_test.py:64:    if "isInferredWatchdog(" not in block or "get_powerOffRequested()" not in block:
tests/s2_alarm_watchdog_wake_structural_test.py:68:    if not 0 <= block.find("isInferredWatchdog(") < block.find("set_powerOffRequested(0)"):
tests/s2_alarm_watchdog_wake_structural_test.py:70:    if gcc.count("set_powerOffRequested(0)") != 1:
tests/s2_alarm_watchdog_wake_structural_test.py:78:    order = [case3.find(s) for s in ("set_powerOffRequested(1)", "flushAllBeforeReset()", "ab1805.deepPowerDown()")]
tests/s2_alarm_watchdog_wake_structural_test.py:144:    "evidence read from retained": mutated("gcc", "RecoveryState::get_powerOffRequested() != 0", "retainedHibernatePending != 0"),
tests/s2_alarm_watchdog_wake_structural_test.py:145:    "byte not cleared": mutated("gcc", "RecoveryState::set_powerOffRequested(0);", ""),
tests/s2_alarm_watchdog_wake_structural_test.py:146:    "byte cleared before read": mutated("gcc", "  bool inferredWatchdog = false;\n", "  RecoveryState::set_powerOffRequested(0);\n  bool inferredWatchdog = false;\n"),
tests/s2_alarm_watchdog_wake_structural_test.py:147:    "byte written after the call": mutated("err", "RecoveryState::set_powerOffRequested(1); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog\n      flushAllBeforeReset();\n      ab1805.deepPowerDown();",
tests/s2_alarm_watchdog_wake_structural_test.py:148:                                           "flushAllBeforeReset();\n      ab1805.deepPowerDown();\n      RecoveryState::set_powerOffRequested(1);"),
tests/watchdog_inferred_classification_test.cpp:10:using HibernateWakeDiagnostics::kPowerOffEvidenceMaxAgeSec;
tests/watchdog_inferred_classification_test.cpp:21:  assert(!isInferredWatchdog(false, 3, kNow - kPowerOffEvidenceMaxAgeSec, true, kNow));
tests/watchdog_inferred_classification_test.cpp:23:  assert(isInferredWatchdog(false, 3, kNow - kPowerOffEvidenceMaxAgeSec - 1, true, kNow));
tests/hibernate_wake_diagnostics_test.cpp:381:// name (ab1805WakeReasonName()'s longest case is "DEEP_POWER_DOWN", 16
tests/clock_hibernate_helpers_move_structural_test.py:19:# v50 S2 (WO-2026-11-05-004): ab1805WakeReasonName and clearAb1805StaleAlarmInterrupts are retired.
tests/clock_hibernate_helpers_move_structural_test.py:74:for retired in ("ab1805WakeReasonName", "clearAb1805StaleAlarmInterrupts"):
#!/bin/zsh
set -euo pipefail

# WO-2026-10-21-001 (Sleep S-2), re-frozen for v50 S2 (WO-2026-11-05-004): tests/sleep_execution_equivalence_test.cpp
# runs one scenario table through the current functions and the handler's call-site lines, extracted by
# marker text from the current State_Sleep.cpp, against the fakes (tests/stubs/sleep_teardown/SleepExecutionFakes.h),
# for cellular and Wi-Fi. The transcript must equal, byte for byte, the frozen fixtures/sleep_execution_transcript.txt
# ('# ' header lines stripped first). The extracted copies the harness compiled are read back and compared with a
# fresh extraction: COPY_MISMATCH on drift. S2 changed the hibernate block on purpose (native duration alarm,
# ExternalWatchdog), so the pre-change base blocks are no longer run. Mutants of a private copy of State_Sleep.cpp
# must each change the transcript. SLEEP_EXECUTION_SRC_ROOT tests another src/ root; SLEEP_EXECUTION_TAMPER_COPY=1
# demonstrates COPY_MISMATCH. '--regenerate' rewrites the fixture from the current source.

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${SLEEP_EXECUTION_SRC_ROOT:-$repo_root/src}"
fixture="$tests_dir/fixtures/sleep_execution_transcript.txt"
stubs="$tests_dir/stubs/sleep_teardown"
work="$(mktemp -d "${TMPDIR:-/tmp}/sleep_execution_equivalence.XXXXXX")"
trap 'rm -rf "$work"' EXIT

cat > "$work/extract.py" <<'PY'
import sys

kind, path, outdir = sys.argv[1:4]
text = open(path).read()


def span(start, end, whole_line_end=False, keep_end=True):
    if text.count(start) != 1:
        sys.exit(f"MARKER_NOT_UNIQUE: {start!r} x{text.count(start)} in {path}")
    i = text.find(start)
    j = text.find(end, i + len(start) if end != start else i)
    if j < 0:
        sys.exit(f"MARKER_MISSING: {end!r} in {path}")
    return text[i:j + len(end)] if keep_end else text[i:j]


def line(start):
    return span(start, "\n")


out = {"drain": span("void drainSerialBeforeSleep() {", "\n}\n")}
out["funcs"] = span("struct WakeContext {", "\n} // namespace\n", keep_end=False) + "\n"
out["call_effective"] = line("  bool useNetworkStandbyEffective = effectiveStandby(")
out["call_teardown"] = line("  if (SleepTeardown::run(")
out["call_hibernate"] = line("  hibernateAttempt(")
out["call_sleep"] = span("  WakeContext wake;\n", "  if (sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)) return;\n")
out["call_route"] = line("  postWakeRouting(")
for k, v in out.items():
    open(f"{outdir}/{kind}_{k}.inc", "w").write(v)
PY

# Writes the base_*.inc / cur_*.inc copies the harness compiles.
extract() {
  # $1 = base|cur, $2 = source file
  python3 "$work/extract.py" "$1" "$2" "$work"
}

extract cur "$src_root/state/State_Sleep.cpp"
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>35s</sub>

### `bash`

**View header context**

`$ sed -n 70,85p src/persist/RecoveryState.h; grep -n "^#include" src/state/State_Error.cpp src/power/ConnectivityFailsafe.cpp | head -30`

<details>
<summary>37 lines</summary>

```
uint32_t get_lastWatchdogResetReasonData();
void set_lastWatchdogResetReasonData(uint32_t value);

/// WatchdogSource enum value.
uint8_t get_lastWatchdogSource();
void set_lastWatchdogSource(uint8_t value);

/// Spare SysData byte (was structuresVersion; layout unchanged): set before a requested power-down, cleared once the wake is classified.
uint8_t get_powerOffRequested();
void set_powerOffRequested(uint8_t value);

// *************** Alert state (decision A - backed by CurrentData) ***************

int8_t get_alertCode();
void set_alertCode(int8_t value);
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
src/state/State_Error.cpp:13:#include "power/Connectivity.h"
src/state/State_Error.cpp:14:#include "time/Clock.h"
src/power/ConnectivityFailsafe.cpp:1:#include "power/ConnectivityFailsafe.h"
src/power/ConnectivityFailsafe.cpp:2:#include "state/StateMachine.h"
src/power/ConnectivityFailsafe.cpp:3:#include "power/ConnectivityPolicy.h"
src/power/ConnectivityFailsafe.cpp:4:#include "power/PowerPlatform.h"
src/power/ConnectivityFailsafe.cpp:5:#include "power/BatteryAuthority.h"
src/power/ConnectivityFailsafe.cpp:6:#include "diagnostics/ConnectivityFailsafeTest.h"
src/power/ConnectivityFailsafe.cpp:7:#include "observability/LoopForensics.h"
src/power/ConnectivityFailsafe.cpp:8:#include "reporting/ReportingPolicy.h"
src/power/ConnectivityFailsafe.cpp:9:#include "time/DailyBoundary.h"
src/power/ConnectivityFailsafe.cpp:10:#include "persist/PowerConfig.h"
src/power/ConnectivityFailsafe.cpp:11:#include "ResetCause.h"
<shellId: 9 completed with exit code 0>
```

</details>


---

<sub>38s</sub>

### `bash`

**Check flushNow impl**

`$ grep -n "^void flushNow" -A6 src/persist/SystemConfig.cpp | head -20`

```
grep: src/persist/SystemConfig.cpp: No such file or directory
<shellId: 10 completed with exit code 0>
```


---

<sub>40s</sub>

### `bash`

**Find flushNow impls**

`$ grep -rn "void flushNow" -A6 src --include=*.cpp | head -40`

<details>
<summary>24 lines</summary>

```
src/MyPersistentData.cpp:919:void flushNow() { sysStatus.flush(true); }
src/MyPersistentData.cpp-920-
src/MyPersistentData.cpp-921-// sizeof(sysStatusData), matching the pre-split sysStatus.validate(sizeof(sysStatus)).
src/MyPersistentData.cpp-922-bool validateStoredData() { return sysStatus.validate(sizeof(sysStatusData)); }
src/MyPersistentData.cpp-923-
src/MyPersistentData.cpp-924-bool get_verboseMode() { return sysStatus.get_verboseMode(); }
src/MyPersistentData.cpp-925-void set_verboseMode(bool value) { sysStatus.set_verboseMode(value); }
--
src/MyPersistentData.cpp:1015:void flushNow() { sensorConfig.flush(true); }
src/MyPersistentData.cpp-1016-
src/MyPersistentData.cpp-1017-// sizeof(sensorConfigData), matching the pre-split sensorConfig.validate(sizeof(sensorConfig)).
src/MyPersistentData.cpp-1018-bool validateStoredData() { return sensorConfig.validate(sizeof(sensorConfigData)); }
src/MyPersistentData.cpp-1019-
src/MyPersistentData.cpp-1020-uint8_t get_sensorType() { return sensorConfig.get_sensorType(); }
src/MyPersistentData.cpp-1021-void set_sensorType(uint8_t value) { sensorConfig.set_sensorType(value); }
--
src/MyPersistentData.cpp:1112:void flushNow() { current.flush(true); }
src/MyPersistentData.cpp-1113-void resetEverything(bool includeSensorTotals) { current.resetEverything(includeSensorTotals); }
src/MyPersistentData.cpp-1114-void revalidateOccupancyStartTimeIfTimeAvailable() { current.revalidateOccupancyStartTimeIfTimeAvailable(); }
src/MyPersistentData.cpp-1115-
src/MyPersistentData.cpp-1116-uint16_t get_hourlyCount() { return current.get_hourlyCount(); }
src/MyPersistentData.cpp-1117-void set_hourlyCount(uint16_t value) { current.set_hourlyCount(value); }
src/MyPersistentData.cpp-1118-
<shellId: 11 completed with exit code 0>
```

</details>


---

<sub>48s</sub>

### `bash`

**Apply src edits; find alarm-wake wording**

```
$ python3 - <<'E'
def sub(p,a,b):
    s=open(p).read(); assert s.count(a)==1,(p,a); open(p,'w').write(s.replace(a,b))
sub('src/persist/RecoveryState.h','/// Spare SysData byte (was structuresVersion; layout unchanged): set before a requested power-down, cleared once the wake is classified.\n',
'/// Spare SysData byte (was structuresVersion; layout unchanged): set to kPowerOffSentinel before a requested power-down, cleared once the wake is classified.\n/// Any other value (0, the legacy structuresVersion 2, ...) is "no record".\nconstexpr uint8_t kPowerOffSentinel = 0xA5;\n')
sub('src/Generalized-Core-Counter.cpp','RecoveryState::get_powerOffRequested() != 0,','RecoveryState::get_powerOffRequested() == RecoveryState::kPowerOffSentinel,')
sub('src/Generalized-Core-Counter.cpp','  RecoveryState::set_powerOffRequested(0);\n','  RecoveryState::set_powerOffRequested(0);\n  SystemConfig::flushNow(); // consume the evidence durably before anything can reload it\n')
sub('src/state/State_Error.cpp','set_powerOffRequested(1);','set_powerOffRequested(RecoveryState::kPowerOffSentinel);')
sub('src/state/State_Error.cpp','      ab1805.deepPowerDown();\n','      ab1805.deepPowerDown();\n      RecoveryState::set_powerOffRequested(0); SystemConfig::flushNow(); // not expected to return: drop the evidence\n')
sub('src/power/ConnectivityFailsafe.cpp','  ab1805.deepPowerDown();\n','  ab1805.deepPowerDown();\n  RecoveryState::set_powerOffRequested(0); RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow(); // not expected to return: no recent stage-3 evidence\n')
sub('src/time/HibernateWakeDiagnostics.h','(!timeValid || (age >= 0 && age <= kPowerOffEvidenceMaxAgeSec))','(timeValid && age >= 0 && age <= kPowerOffEvidenceMaxAgeSec)')
sub('src/Generalized-Core-Counter.cpp','''    snprintf(pinResetFields, sizeof(pinResetFields), ",\\"watchdogSource\\":\\"%s\\"", startupInferredWatchdog ? "inferred" : "NONE");''','''    snprintf(pinResetFields, sizeof(pinResetFields), ",\\"ab1805WakeReason\\":\\"N/A\\",\\"watchdogSource\\":\\"%s\\"", startupInferredWatchdog ? "inferred" : "NONE");''')
E
grep -n "pinResetFields\[" src/Generalized-Core-Counter.cpp; grep -rn "alarm wake\|Alarm wake\|alarm-wake" src tests | grep -v fixtures | head -30
```

```
1293:  char pinResetFields[64] = "";
tests/hibernate_wake_diagnostics_test.sh:126:# v50 S2 (WO-2026-11-05-004): reason 30 alone is the alarm wake; no AB1805 wake-reason read, no DEEP_POWER_DOWN window.
tests/s2_alarm_watchdog_wake_structural_test.py:59:        p.append("reason 30 alone must be the alarm wake")
<shellId: 12 completed with exit code 0>
```


---

<sub>52s</sub>

### `bash`

**Find wording to fix**

`$ sed -i 's/char pinResetFields\[64\]/char pinResetFields[80]/; s/sizeof(pinResetFields), ",/sizeof(pinResetFields), ",/' src/Generalized-Core-Counter.cpp; sed -i 's/sizeof(pinResetFields)/sizeof(pinResetFields)/' src/Generalized-Core-Counter.cpp; grep -rni "alarm" src/time/HibernateCycle.* src/time/HibernateWakeDiagnostics.h | head -20; grep -n "alarm\|1800\|reason 30\|reason-30" -i tests/hibernate_wake_diagnostics_test.cpp | head -30`

<details>
<summary>25 lines</summary>

```
sed: 1: "src/Generalized-Core-Co ...": bad flag in substitute command: 'e'
sed: 1: "src/Generalized-Core-Co ...": bad flag in substitute command: 'e'
src/time/HibernateCycle.cpp:64:    in.wakeReasonIsAlarm = in.resetReasonIsPowerManagement;
src/time/HibernateCycle.cpp:70:    } else if (!in.wakeReasonIsAlarm) {
src/time/HibernateWakeDiagnostics.h:50:/// `wakeReasonIsAlarm` from Device-OS/AB1805 types before populating this,
src/time/HibernateWakeDiagnostics.h:54:  bool wakeReasonIsAlarm;
23:  in.wakeReasonIsAlarm = true;
45:  if (!in.wakeReasonIsAlarm) {
69:// chip alarm itself and reports RESET_REASON_POWER_MANAGEMENT (30) for the hibernate wake, so
70:// in.wakeReasonIsAlarm in HibernateCycle.cpp is exactly in.resetReasonIsPowerManagement - there
79:  in.wakeReasonIsAlarm = in.resetReasonIsPowerManagement;
84:  for (const int64_t skewSec : {(int64_t)0, (int64_t)30, (int64_t)600, (int64_t)-1800}) {
86:    assert(in.wakeReasonIsAlarm);
95:  assert(!in.wakeReasonIsAlarm);
108:  const EventFields f = buildEventFields(classifyGateArmMirror(in), in, /*osResetReason=*/30, "ALARM",
125:  const EventFields f = buildEventFields(classifyGateArmMirror(in), in, /*osResetReason=*/30, "ALARM",
147:  in.wakeReasonIsAlarm = true;
194:  in.wakeReasonIsAlarm = true;
233:  const EventFields f = buildEventFields(classifyGateArmMirror(in), in, /*osResetReason=*/30, "ALARM",
259:    in.wakeReasonIsAlarm = false;
296:  in.wakeReasonIsAlarm = false;
324:  in.wakeReasonIsAlarm = false;             // fails check 2
330:  in.wakeReasonIsAlarm = false; // fails check 2
363:  const EventFields f = buildEventFields(classifyGateArmMirror(in), in, /*osResetReason=*/30, "ALARM",
<shellId: 13 completed with exit code 0>
```

</details>


---

<sub>55s</sub>

### `bash`

**View classification code and test**

`$ grep -rn "\"ALARM\"\|wakeReasonIsAlarm" src tests --include=* -l; sed -n 35,95p src/time/HibernateCycle.cpp; sed -n 40,60p src/time/HibernateWakeDiagnostics.h; sed -n 60,100p tests/hibernate_wake_diagnostics_test.cpp`

<details>
<summary>114 lines</summary>

```
src/time/HibernateCycle.cpp
src/time/HibernateWakeDiagnostics.h
tests/hibernate_wake_diagnostics_test.sh
tests/s2_alarm_watchdog_wake_structural_test.py
tests/hibernate_wake_diagnostics_test.cpp
uint32_t currentHibernateCount() {
  return retainedHibernateCount;
}

WakeVerdict classifyWake(int osResetReason) {
  WakeVerdict v{};

  // ===== RTC READ - every boot, independent of the above =====
  time_t rtcTime = 0;
  v.rtcReadOk = ExternalRtc::get(rtcTime);
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
    HibernateWakeDiagnostics::GateInputs &in = v.gateInputs;
    in.resetReasonIsPowerManagement = (osResetReason == RESET_REASON_POWER_MANAGEMENT);
    in.rtcReadOk = v.rtcReadOk;
    in.rtcBefore = (int64_t)retainedHibernateRtcBefore;
    in.requestedSleepSec = retainedHibernateRequestedSleep;
    in.rtcAtWake = v.rtcAtWake;
    // Reason 30 alone passes the wake-reason arm (WO-2026-11-05-004 Amendment 3).
    in.wakeReasonIsAlarm = in.resetReasonIsPowerManagement;
    if (in.resetReasonIsPowerManagement) v.wakeReasonName = "POWER_MGMT";

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

  kResetReason,    // OS reset reason was not RESET_REASON_POWER_MANAGEMENT
  kWakeReason,     // unreachable after kResetReason since v50 S2 (reason 30 alone passes)
  kRtcRead,        // the external RTC read failed
  kRtcBeforeZero,  // retainedHibernateRtcBefore was not > 0
  kRequestedZero,  // retainedHibernateRequestedSleep was not > 0
  kRtcOrder,       // rtcTime on wake was earlier than retainedHibernateRtcBefore
};

/// Raw inputs to the gate, exactly as `HibernateCycle::classifyWake()`
/// reads them. The caller resolves `resetReasonIsPowerManagement` and
/// `wakeReasonIsAlarm` from Device-OS/AB1805 types before populating this,
/// so this header never needs to know those enums.
struct GateInputs {
  bool resetReasonIsPowerManagement;
  bool wakeReasonIsAlarm;
  bool rtcReadOk;
  int64_t rtcBefore;
  uint32_t requestedSleepSec;
  int64_t rtcAtWake;
};

  return GateArm::kNone;
}

void testGatePassesClassifiesNone() {
  const GateInputs in = passingInputs();
  assert(classifyGateArmMirror(in) == GateArm::kNone);
}

// v50 S2 (WO-2026-11-05-004): the AB1805 wake-reason read is gone. Device OS 6.6.0 programs the
// chip alarm itself and reports RESET_REASON_POWER_MANAGEMENT (30) for the hibernate wake, so
// in.wakeReasonIsAlarm in HibernateCycle.cpp is exactly in.resetReasonIsPowerManagement - there
// is no DEEP_POWER_DOWN window and no AB1805 label. The .sh fidelity checks pin that text.
GateInputs wakeInputs(bool powerMgmt, int64_t skewSec, bool rtcReadOk = true) {
  GateInputs in{};
  in.resetReasonIsPowerManagement = powerMgmt;
  in.rtcReadOk = rtcReadOk;
  in.rtcBefore = 1000;
  in.requestedSleepSec = 3600;
  in.rtcAtWake = 1000 + 3600 + skewSec;
  in.wakeReasonIsAlarm = in.resetReasonIsPowerManagement;
  return in;
}

void testReason30AloneAcceptsAnyTiming() {
  for (const int64_t skewSec : {(int64_t)0, (int64_t)30, (int64_t)600, (int64_t)-1800}) {
    const GateInputs in = wakeInputs(true, skewSec);
    assert(in.wakeReasonIsAlarm);
    assert(classifyGateArmMirror(in) == GateArm::kNone);
  }
  const GateInputs noRtc = wakeInputs(true, 0, /*rtcReadOk=*/false);
  assert(classifyGateArmMirror(noRtc) == GateArm::kRtcRead);
}

void testOtherResetReasonsFailOnTheResetReasonArm() {
  const GateInputs in = wakeInputs(false, 0);
  assert(!in.wakeReasonIsAlarm);
  assert(classifyGateArmMirror(in) == GateArm::kResetReason);
}

// buildEventFields() is the exact function production calls immediately
// after HibernateCycle::classifyWake() classifies the gate outcome, to combine it with the
<shellId: 14 completed with exit code 0>
```

</details>


---

<sub>1m 0s</sub>

### `bash`

**Rename field, fix comment**

```
$ python3 - <<'E'
import re
for p in ['src/time/HibernateCycle.cpp','src/time/HibernateWakeDiagnostics.h','tests/hibernate_wake_diagnostics_test.sh','tests/s2_alarm_watchdog_wake_structural_test.py','tests/hibernate_wake_diagnostics_test.cpp']:
    s=open(p).read(); s=s.replace('wakeReasonIsAlarm','wakeReasonIsHibernateWake'); open(p,'w').write(s)
p='src/time/HibernateCycle.cpp'; s=open(p).read()
s=s.replace("// Reason 30 alone passes the wake-reason arm (WO-2026-11-05-004 Amendment 3).","// Reason 30 is a successful HIBERNATE wake (RTC or GPIO alike) and alone passes the wake-reason arm (WO-2026-11-05-004 Amendments 3-4).")
open(p,'w').write(s)
p='src/Generalized-Core-Counter.cpp'; s=open(p).read()
s=s.replace('char pinResetFields[64]','char pinResetFields[80]'); open(p,'w').write(s)
E
sed -n 120,135p tests/hibernate_wake_diagnostics_test.sh; sed -n 50,62p tests/s2_alarm_watchdog_wake_structural_test.py
```

<details>
<summary>29 lines</summary>

```
# without stubbing AB1805 deeply enough to reconstruct a parallel mirror,
# so these are exact-text checks, one per GateInputs field, matching this
# test's own classifyGateArmMirror() field-for-field. ---
check "in.resetReasonIsPowerManagement maps from osResetReason, not a literal" \
  "in.resetReasonIsPowerManagement = (osResetReason == RESET_REASON_POWER_MANAGEMENT);" \
  "$hc_src"
# v50 S2 (WO-2026-11-05-004): reason 30 alone is the alarm wake; no AB1805 wake-reason read, no DEEP_POWER_DOWN window.
check "in.wakeReasonIsHibernateWake is exactly the power-management reset reason" \
  "in.wakeReasonIsHibernateWake = in.resetReasonIsPowerManagement;" \
  "$hc_src"
for banned in "AB1805" "dpdOnTime" "getWakeReason" "DEEP_POWER_DOWN"; do
  if grep -q -- "$banned" "$hc_src"; then
    echo "FIDELITY CHECK FAILED: $banned still referenced in $hc_src" >&2
    exit 1
  fi
done
    # Only the two S3 callers may still touch the AB1805 object for power-down.
    for label in ("gcc", "sleep", "err", "hc"):
        t = strip(files[label])
        for m in re.finditer(r"\bab1805\.(\w+)", t):
            if m.group(1) != "deepPowerDown":
                p.append(f"ab1805.{m.group(1)} in {label}")

    # Reason 20 is judged from persisted evidence, not the retained struct, and a reason-30-only wake.
    if "in.wakeReasonIsHibernateWake = in.resetReasonIsPowerManagement;" not in hc:
        p.append("reason 30 alone must be the alarm wake")
    if re.search(r"retained\w*", hc) and "PIN_RESET" in hc:
        pass
    setup = gcc[gcc.index("classifyWake(reason)"):]
<shellId: 15 completed with exit code 0>
```

</details>


---

<sub>1m 5s</sub>

### Copilot

Now test wording edits and the diagnostics test.

---

<sub>1m 5s</sub>

### `bash`

**Reword tests; view classification test**

```
$ python3 - <<'E'
def sub(p,a,b):
    s=open(p).read(); assert s.count(a)==1,(p,a); open(p,'w').write(s.replace(a,b))
sub('tests/hibernate_wake_diagnostics_test.sh','reason 30 alone is the alarm wake;','reason 30 alone is a successful HIBERNATE wake (RTC or GPIO);')
sub('tests/s2_alarm_watchdog_wake_structural_test.py','reason 30 alone must be the alarm wake','reason 30 alone must be the successful-HIBERNATE-wake arm')
p='tests/hibernate_wake_diagnostics_test.cpp'
sub(p,"chip alarm itself and reports RESET_REASON_POWER_MANAGEMENT (30) for the hibernate wake, so","chip alarm itself and reports RESET_REASON_POWER_MANAGEMENT (30) for any successful HIBERNATE wake\n// (RTC or GPIO; firmware cannot tell them apart), so")
sub(p,"void testReason30AloneAcceptsAnyTiming() {","// Reason 30 is a successful HIBERNATE wake whatever the elapsed time (including a 1,800 s early wake, e.g. a\n// button/GPIO wake); it is not proof of an alarm wake. The bench's actual-vs-requested sleep shows early wakes.\nvoid testReason30AloneAcceptsAnyTiming() {")
E
grep -n "testReason30Alone\|testOtherReset" tests/hibernate_wake_diagnostics_test.cpp; sed -n 1,25p tests/watchdog_inferred_classification_test.cpp; cat tests/watchdog_inferred_classification_test.sh
```

<details>
<summary>58 lines</summary>

```
86:void testReason30AloneAcceptsAnyTiming() {
96:void testOtherResetReasonsFailOnTheResetReasonArm() {
416:  testReason30AloneAcceptsAnyTiming();
417:  testOtherResetReasonsFailOnTheResetReasonArm();
// v50 S2 (WO-2026-11-05-004): the reason-20 (PIN_RESET) boot is classified from persisted evidence only.
// This runs the real HibernateWakeDiagnostics::isInferredWatchdog() (header-only, host-compilable).
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include "time/HibernateWakeDiagnostics.h"

using HibernateWakeDiagnostics::isInferredWatchdog;
using HibernateWakeDiagnostics::kPowerOffEvidenceMaxAgeSec;

static const int64_t kNow = 1800000000;

int main() {
  // No record at all: nothing says the firmware powered the board off, so it is an inferred watchdog.
  assert(isInferredWatchdog(false, 0, 0, true, kNow));
  // The Error-path byte was persisted before deepPowerDown(): a power-up, not a watchdog.
  assert(!isInferredWatchdog(true, 0, 0, true, kNow));
  // Failsafe stage 3 acted recently: a power-up.
  assert(!isInferredWatchdog(false, 3, kNow - 5, true, kNow));
  assert(!isInferredWatchdog(false, 3, kNow - kPowerOffEvidenceMaxAgeSec, true, kNow));
  // Stage 3 but stale beyond the window: inferred watchdog.
  assert(isInferredWatchdog(false, 3, kNow - kPowerOffEvidenceMaxAgeSec - 1, true, kNow));
  // A future timestamp is not evidence.
  assert(isInferredWatchdog(false, 3, kNow + 1, true, kNow));
#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
work="$(mktemp -d "${TMPDIR:-/tmp}/watchdog_inferred.XXXXXX")"
trap 'rm -rf "$work"' EXIT

clang++ -std=c++17 -Wall -Wextra -pedantic -I"$repo_root/src" \
  "$repo_root/tests/watchdog_inferred_classification_test.cpp" -o "$work/t"
"$work/t"

# The same header with each rule broken must fail the table.
hdr="$repo_root/src/time/HibernateWakeDiagnostics.h"
mutant() {
  local label="$1" old="$2" new="$3"
  mkdir -p "$work/m_$label/time"
  python3 - "$hdr" "$work/m_$label/time/HibernateWakeDiagnostics.h" "$old" "$new" <<'PY'
import sys
src, dst, old, new = sys.argv[1:5]
t = open(src).read()
if t.count(old) != 1:
    sys.exit(f"MUTATION_NOT_APPLIED: {old!r}")
open(dst, "w").write(t.replace(old, new))
PY
  clang++ -std=c++17 -I"$work/m_$label" "$repo_root/tests/watchdog_inferred_classification_test.cpp" -o "$work/m_$label/t" 2>/dev/null
  if "$work/m_$label/t" >/dev/null 2>&1; then echo "MUTANT_SURVIVED: $label"; exit 1; fi
  echo "caught: $label"
}
mutant always_wake "return !errorPathByte && !failsafeRecent;" "return false;"
mutant never_wake "return !errorPathByte && !failsafeRecent;" "return true;"
mutant no_recency "(!timeValid || (age >= 0 && age <= kPowerOffEvidenceMaxAgeSec))" "true"
mutant no_byte "return !errorPathByte && !failsafeRecent;" "return !failsafeRecent;"
mutant any_stage "failsafeStage == 3 &&" "failsafeStage != 0 &&"
echo "watchdog_inferred_classification_test: classification and mutants ok"
<shellId: 16 completed with exit code 0>
```

</details>


---

<sub>1m 14s</sub>

### `bash`

**Move sentinel into diagnostics header**

```
$ python3 - <<'E'
def sub(p,a,b):
    s=open(p).read(); assert s.count(a)==1,(p,a); open(p,'w').write(s.replace(a,b))
sub('src/persist/RecoveryState.h','/// Spare SysData byte (was structuresVersion; layout unchanged): set to kPowerOffSentinel before a requested power-down, cleared once the wake is classified.\n/// Any other value (0, the legacy structuresVersion 2, ...) is "no record".\nconstexpr uint8_t kPowerOffSentinel = 0xA5;\n','/// Spare SysData byte (was structuresVersion; layout unchanged): set to HibernateWakeDiagnostics::kPowerOffSentinel before a requested power-down, cleared once the wake is classified.\n')
sub('src/Generalized-Core-Counter.cpp','RecoveryState::get_powerOffRequested() == RecoveryState::kPowerOffSentinel,','RecoveryState::get_powerOffRequested(),')
sub('src/state/State_Error.cpp','RecoveryState::kPowerOffSentinel','HibernateWakeDiagnostics::kPowerOffSentinel')
sub('src/state/State_Error.cpp','#include "time/Clock.h"\n','#include "time/Clock.h"\n#include "time/HibernateWakeDiagnostics.h"\n')
sub('src/time/HibernateWakeDiagnostics.h','''inline bool isInferredWatchdog(bool errorPathByte, uint8_t failsafeStage, int64_t failsafeActionAt, bool timeValid, int64_t now) {
  const int64_t age = now - failsafeActionAt;''','''/// The Error-path evidence byte counts only when it is exactly this value (0 and the legacy structuresVersion 2 are "no record").
constexpr uint8_t kPowerOffSentinel = 0xA5;
inline bool isInferredWatchdog(uint8_t evidenceByte, uint8_t failsafeStage, int64_t failsafeActionAt, bool timeValid, int64_t now) {
  const bool errorPathByte = evidenceByte == kPowerOffSentinel;
  const int64_t age = now - failsafeActionAt;''')
sub('src/time/HibernateWakeDiagnostics.h','/// Reason-20 boot: persisted power-off evidence (the Error-path byte, or a\n/// recent failsafe stage 3)','/// Reason-20 boot: persisted power-off evidence (the Error-path byte, or a\n/// failsafe stage 3 within 600 s on a valid clock)')
E
sed -n 1,30p src/time/HibernateWakeDiagnostics.h | grep -n include; sed -n 150,165p src/time/HibernateWakeDiagnostics.h
```

<details>
<summary>17 lines</summary>

```
30:#include <stdint.h>
inline int buildEventPayload(char *buffer, size_t bufferSize, const EventFields &f) {
  const bool ok = f.arm == GateArm::kNone;
  return snprintf(
      buffer, bufferSize,
      "{\"result\":\"%s\",\"gateArm\":\"%s\",\"osReason\":%d,\"wakeReason\":\"%s\","
      "\"rtcOk\":%d,\"req\":%lu,\"rtcBefore\":%lld,\"rtcAt\":%lld,\"actual\":%lu,"
      "\"err\":%ld,\"count\":%lu}",
      ok ? "ok" : "fail", gateArmName(f.arm), f.osResetReason, f.wakeReasonName,
      f.rtcReadOk ? 1 : 0, (unsigned long)f.requestedSleepSec, (long long)f.rtcBefore,
      (long long)f.rtcAtWake, (unsigned long)f.actualSleepSec, (long)f.sleepErrorSec,
      (unsigned long)f.hibernateCount);
}

/// Reason-20 boot: persisted power-off evidence (the Error-path byte, or a
/// failsafe stage 3 within 600 s on a valid clock) means a power-up wake; none means an inferred
/// watchdog. `retained` is never evidence (a power-down clears it).
<shellId: 17 completed with exit code 0>
```

</details>


---

<sub>1m 17s</sub>

### Copilot

Check the Generalized-Core-Counter.cpp includes the header, then fix the classification test.

---

<sub>1m 17s</sub>

### `bash`

**Update classification test calls**

```
$ grep -n "HibernateWakeDiagnostics.h" src/Generalized-Core-Counter.cpp | head -2
python3 - <<'E'
p='tests/watchdog_inferred_classification_test.cpp'
s=open(p).read()
s=s.replace("isInferredWatchdog(false, ","isInferredWatchdog(0, ").replace("isInferredWatchdog(true, 0, 0, true, kNow)","isInferredWatchdog(kPowerOffSentinel, 0, 0, true, kNow)")
s=s.replace("using HibernateWakeDiagnostics::kPowerOffEvidenceMaxAgeSec;","using HibernateWakeDiagnostics::kPowerOffEvidenceMaxAgeSec;\nusing HibernateWakeDiagnostics::kPowerOffSentinel;")
open(p,'w').write(s)
E
sed -n 24,40p tests/watchdog_inferred_classification_test.cpp
```

<details>
<summary>17 lines</summary>

```
52:#include "time/HibernateWakeDiagnostics.h" // Hibernate-wake gate data types/cloud-event rendering (WO-2026-08-29-001)
  assert(isInferredWatchdog(0, 3, kNow - kPowerOffEvidenceMaxAgeSec - 1, true, kNow));
  // A future timestamp is not evidence.
  assert(isInferredWatchdog(0, 3, kNow + 1, true, kNow));
  // Other stages never count, however recent.
  for (uint8_t stage : {(uint8_t)0, (uint8_t)1, (uint8_t)2, (uint8_t)4}) {
    assert(isInferredWatchdog(0, stage, kNow - 1, true, kNow));
  }
  // Time invalid: recency cannot be judged, stage 3 is accepted as evidence.
  assert(!isInferredWatchdog(0, 3, 12345, false, 0));
  assert(isInferredWatchdog(0, 2, 12345, false, 0));
  // The byte wins over everything else.
  assert(!isInferredWatchdog(true, 3, 0, true, kNow));
  puts("watchdog_inferred_classification_test: ok");
  return 0;
}
<shellId: 18 completed with exit code 0>
```

</details>


---

<sub>1m 33s</sub>

### `bash`

**Update and run classification test**

```
$ python3 - <<'E'
p='tests/watchdog_inferred_classification_test.cpp'
s=open(p).read()
s=s.replace("""  // Time invalid: recency cannot be judged, stage 3 is accepted as evidence.
  assert(!isInferredWatchdog(0, 3, 12345, false, 0));
  assert(isInferredWatchdog(0, 2, 12345, false, 0));
  // The byte wins over everything else.
  assert(!isInferredWatchdog(true, 3, 0, true, kNow));""","""  // Time invalid: the 600 s recency cannot be proved, so stage 3 is rejected (inferred watchdog).
  assert(isInferredWatchdog(0, 3, 12345, false, 0));
  assert(isInferredWatchdog(0, 3, 12345, false, kNow));
  // Only the exact sentinel counts: 0, the legacy structuresVersion 2, 1 and other values are "no record".
  assert(kPowerOffSentinel == 0xA5);
  for (int v : {0, 1, 2, 0x5A, 0xA4, 0xA6, 0xFF}) assert(isInferredWatchdog((uint8_t)v, 0, 0, true, kNow));
  // The sentinel wins over everything else, including an invalid clock.
  assert(!isInferredWatchdog(kPowerOffSentinel, 3, 0, true, kNow));
  assert(!isInferredWatchdog(kPowerOffSentinel, 0, 0, false, 0));""")
open(p,'w').write(s)
p='tests/watchdog_inferred_classification_test.sh'
s=open(p).read()
s=s.replace('mutant no_recency "(!timeValid || (age >= 0 && age <= kPowerOffEvidenceMaxAgeSec))" "true"','mutant no_recency "(timeValid && age >= 0 && age <= kPowerOffEvidenceMaxAgeSec)" "true"\nmutant invalid_time_accepted "(timeValid && age >= 0 && age <= kPowerOffEvidenceMaxAgeSec)" "(!timeValid || (age >= 0 && age <= kPowerOffEvidenceMaxAgeSec))"\nmutant sentinel_nonzero "evidenceByte == kPowerOffSentinel" "evidenceByte != 0"\nmutant sentinel_legacy_two "evidenceByte == kPowerOffSentinel" "evidenceByte >= 2"')
open(p,'w').write(s)
E
zsh tests/watchdog_inferred_classification_test.sh 2>&1 | tail -12
```

<details>
<summary>11 lines</summary>

```
watchdog_inferred_classification_test: ok
caught: always_wake
caught: never_wake
caught: no_recency
caught: invalid_time_accepted
caught: sentinel_nonzero
caught: sentinel_legacy_two
caught: no_byte
caught: any_stage
watchdog_inferred_classification_test: classification and mutants ok
<shellId: 19 completed with exit code 0>
```

</details>


---

<sub>1m 37s</sub>

### `bash`

**View structural tests and diffs**

`$ cat tests/s2_alarm_watchdog_wake_structural_test.py; git diff 3b5cfb8 -- tests/sleep_plan_structural_test.py tests/sleep_wake_structural_test.py`

<details>
<summary>209 lines</summary>

```diff
#!/usr/bin/env python3
"""v50 S2 (WO-2026-11-05-004): structural checks for the Device OS alarm / chip-watchdog / wake changes.

Each rule is a function of the source text; every rule is shown to fire on a mutated copy
(in memory) before the real source is judged.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "src"


def read(rel):
    return (SRC / rel).read_text()


def strip(t):
    t = re.sub(r"/\*.*?\*/", "", t, flags=re.S)
    return re.sub(r"//[^\n]*", "", t)


def fn_body(t, sig):
    i = t.index(sig)
    j = t.index("\n}\n", i)
    return t[i:j]


def problems(files):
    try:
        return _problems(files)
    except ValueError as e:
        return [f"expected structure missing: {e}"]


def _problems(files):
    p = []
    gcc = strip(files["gcc"])
    sleep = strip(files["sleep"])
    err = strip(files["err"])
    hc = strip(files["hc"])

    for name in ("clearAb1805StaleAlarmInterrupts", "interruptAtTime", "setWDT", "stopWDT", "resumeWDT",
                 "ab1805WakeReasonName", "getWakeReason", "updateWakeReason"):
        for label, t in files.items():
            if label != "raw" and re.search(r"\b" + name + r"\b", strip(t)):
                p.append(f"{name} still referenced in {label}")

    # Only the two S3 callers may still touch the AB1805 object for power-down.
    for label in ("gcc", "sleep", "err", "hc"):
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
    if "if (reason == RESET_REASON_PIN_RESET)" not in block:
        p.append("reason 20 gate missing")

    # The byte is persisted before the deep power-down and the flush, never after.
    case3 = err[err.index("case 3:"):]
    case3 = case3[:case3.index("break;")]
    order = [case3.find(s) for s in ("set_powerOffRequested(1)", "flushAllBeforeReset()", "ab1805.deepPowerDown()")]
    if min(order) < 0 or order != sorted(order):
        p.append("State_Error case 3 must write the byte, then flush, then deepPowerDown")

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
    "evidence read from retained": mutated("gcc", "RecoveryState::get_powerOffRequested() != 0", "retainedHibernatePending != 0"),
    "byte not cleared": mutated("gcc", "RecoveryState::set_powerOffRequested(0);", ""),
    "byte cleared before read": mutated("gcc", "  bool inferredWatchdog = false;\n", "  RecoveryState::set_powerOffRequested(0);\n  bool inferredWatchdog = false;\n"),
    "byte written after the call": mutated("err", "RecoveryState::set_powerOffRequested(1); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog\n      flushAllBeforeReset();\n      ab1805.deepPowerDown();",
                                           "flushAllBeforeReset();\n      ab1805.deepPowerDown();\n      RecoveryState::set_powerOffRequested(1);"),
    "alarm armed twice": mutated("sleep", ".duration((uint32_t)wakeInSeconds * 1000UL);\n\n        const bool watchdogStopped",
                                 ".duration((uint32_t)wakeInSeconds * 1000UL).duration(1);\n\n        const bool watchdogStopped"),
    "no watchdog stop before hibernate": mutated("sleep", "const bool watchdogStopped = (ExternalWatchdog.stop() == SYSTEM_ERROR_NONE);\n        pauseAwakeWatchdogForSleep",
                                                  "const bool watchdogStopped = true;\n        pauseAwakeWatchdogForSleep"),
    "refresh missing": mutated("gcc", "ExternalWatchdog.refresh();", ""),
    "hibernate fallback restart removed": mutated("sleep", "HibernateCycle::abandon();\n        ExternalWatchdog.start();", "HibernateCycle::abandon();"),
    "ab1805 stale clear back": dict(raw, elig="bool clearAb1805StaleAlarmInterrupts();\n" + raw["elig"]),
}
for name, f in probes.items():
    if not problems(f):
        print(f"FAIL: probe not detected: {name}")
        sys.exit(1)

found = problems(raw)
if found:
    for x in found:
        print(f"FAIL: {x}")
    sys.exit(1)
print("s2_alarm_watchdog_wake_structural_test: ok (%d probes detected)" % len(probes))
diff --git a/tests/sleep_plan_structural_test.py b/tests/sleep_plan_structural_test.py
index 96a0f7b..b1daecd 100644
--- a/tests/sleep_plan_structural_test.py
+++ b/tests/sleep_plan_structural_test.py
@@ -181,6 +181,8 @@ def base_pieces(base_text):
 sleep = SLEEP.read_text()
 # WO-2026-11-03-005: the deliberate-reset flush call is the one intended addition; mapped back so every other byte is still compared.
 sleep = re.sub(r"^ *flushAllBeforeReset\(\);\n", "", sleep, flags=re.M)
+# v50 S2 (WO-2026-11-05-004): the one reworded comment is mapped back so every other handler byte is still compared.
+sleep = sleep.replace("// The chip alarm is armed by Device OS from duration(); the app only drives its watchdog.", "// NO AB1805 alarms - AB1805 is only used for watchdog + RTC time sync.")
 probes = {
     "the guard call before the hibernate attempt": sleep.replace(HIB + "\n" + GUARD_CALL, GUARD_CALL + "\n" + HIB, 1),
     "a second plan call": sleep.replace(HIB, "  { SleepPlan p2; sleepPlan(p2); }\n" + HIB, 1),
@@ -228,12 +230,22 @@ if cur_retained != base_retained:
     fail("a `retained` keyword or identifier was added in src/")
 
 # The S-2 functions, cloudGatePass() and the includes are unchanged.
-tail_of = lambda t: t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))]
+def without_hibernate_attempt(t):
+    # v50 S2 (WO-2026-11-05-004) changed hibernateAttempt on purpose (and swapped the ab1805 watchdog calls for ExternalWatchdog, normalised here); every other S-2 function is still compared.
+    i = t.find("hibernateAttempt(")
+    i = t.rfind("\n", 0, t.rfind("void", 0, i) + 1) if i >= 0 else -1
+    j = t.index("\n}\n", i) + 3 if i >= 0 else 0
+    t = t[:i] + t[j:] if i >= 0 else t
+    return (t.replace("(ExternalWatchdog.stop() == SYSTEM_ERROR_NONE)", "ab1805.stopWDT()")
+             .replace("ExternalWatchdog.start()", "ab1805.resumeWDT()"))
+
+
+tail_of = lambda t: without_hibernate_attempt(t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))])
 prefix_of = lambda t: t[:t.index("namespace {")]
 cloud_of = lambda t: t[t.index("struct {\n  bool operationsCompleteLogged"):t.index("void endOfCycleSummary(") if "void endOfCycleSummary(" in t else t.index("struct SleepPlan {") if "struct SleepPlan {" in t else t.index("struct WakeContext {")]
 if tail_of(sleep) != tail_of(base_sleep):
     fail("the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base")
-if prefix_of(sleep) != prefix_of(base_sleep):
+if prefix_of(sleep) != prefix_of(base_sleep).replace('#include "AB1805_RK.h"\n', ""):
     fail("the includes differ from the base")
 if cloud_of(sleep).rstrip("\n") != cloud_of(base_sleep).rstrip("\n"):
     fail("the cloudGate struct or cloudGatePass() differ from the base")
diff --git a/tests/sleep_wake_structural_test.py b/tests/sleep_wake_structural_test.py
index f15a10e..bd51576 100644
--- a/tests/sleep_wake_structural_test.py
+++ b/tests/sleep_wake_structural_test.py
@@ -196,7 +196,17 @@ if cur_retained != base_retained:
 
 # The S-2 functions, logWakeSummary() and the includes are unchanged; State_Modes.cpp is unchanged above the new
 # owner apart from its three added includes.
-tail_of = lambda t: t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))]
+def without_hibernate_attempt(t):
+    # v50 S2 (WO-2026-11-05-004) changed hibernateAttempt on purpose (and swapped the ab1805 watchdog calls for ExternalWatchdog, normalised here); every other S-2 function is still compared.
+    i = t.find("hibernateAttempt(")
+    i = t.rfind("\n", 0, t.rfind("void", 0, i) + 1) if i >= 0 else -1
+    j = t.index("\n}\n", i) + 3 if i >= 0 else 0
+    t = t[:i] + t[j:] if i >= 0 else t
+    return (t.replace("(ExternalWatchdog.stop() == SYSTEM_ERROR_NONE)", "ab1805.stopWDT()")
+             .replace("ExternalWatchdog.start()", "ab1805.resumeWDT()"))
+
+
+tail_of = lambda t: without_hibernate_attempt(t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))])
 prefix_of = lambda t: t[:t.index("namespace {")]
 log_of = lambda t: t[t.index("void logWakeSummary("):t.index("\n}\n", t.index("void logWakeSummary(")) + 3]
 # WO-2026-10-25-001: the one intended change, the led= value is already in seconds; mapped back for the compare.
@@ -206,7 +216,7 @@ if log_of(sleep).count(LED_NEW) != 1:
     fail("logWakeSummary() must print signalLEDTimeRemaining() (seconds) without a further division")
 if tail_of(sleep) != tail_of(base_sleep):
     fail("the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base")
-if prefix_of(sleep) != prefix_of(base_sleep):
+if prefix_of(sleep) != prefix_of(base_sleep).replace('#include "AB1805_RK.h"\n', ""):
     fail("the State_Sleep.cpp includes differ from the base")
 if log_of(sleep).replace(LED_NEW, LED_BASE) != log_of(base_sleep):
     fail("logWakeSummary() differs from the base")
<shellId: 20 completed with exit code 0>
```

</details>


---

<sub>1m 42s</sub>

### `bash` — Failed

**Diff base vs current hibernateAttempt**

```
$ git show 3b5cfb8:src/state/State_Sleep.cpp > /tmp/base_sleep.cpp; python3 - <<'E'
import re,difflib
def fn(t):
    i=t.index("hibernateAttempt("); i=t.rfind("\n",0,t.rfind("void",0,i)+1)
    j=t.index("\n}\n",i)+3; return t[i:j]
b=fn(open('/tmp/base_sleep.cpp').read()); c=fn(open('src/state/State_Sleep.cpp').read())
open('/tmp/b.txt','w').write(b); open('/tmp/c.txt','w').write(c)
E
diff /tmp/b.txt /tmp/c.txt
```

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>1m 44s</sub>

### `bash` — Failed

**Diff in scratch dir**

```
$ mkdir -p build-tmp/wo-2026-11-05-004-stage6-r3 && cd build-tmp/wo-2026-11-05-004-stage6-r3 && git show 3b5cfb8:src/state/State_Sleep.cpp > base_sleep.cpp && python3 - <<'E'
def fn(t):
    i=t.index("hibernateAttempt("); i=t.rfind("\n",0,t.rfind("void",0,i)+1)
    j=t.index("\n}\n",i)+3; return t[i:j]
open('b.txt','w').write(fn(open('base_sleep.cpp').read())); open('c.txt','w').write(fn(open('../../src/state/State_Sleep.cpp').read()))
E
diff b.txt c.txt
```

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>1m 46s</sub>

### `bash`

**Create scratch dir**

`$ ls build-tmp; mkdir -p build-tmp/wo-2026-11-05-004-stage6-r3; ls build-tmp/wo-2026-11-05-004-stage6-r3`

```
wo-2026-11-03-004-stage6
wo-2026-11-03-006-stage6-r2
wo-2026-11-05-004-stage6-r2
<shellId: 23 completed with exit code 0>
```


---

<sub>1m 50s</sub>

### `bash`

**Diff via python in scratch**

```
$ cd build-tmp/wo-2026-11-05-004-stage6-r3; git show 3b5cfb8:src/state/State_Sleep.cpp > base_sleep.cpp; python3 -c "
def fn(t):
    i=t.index('hibernateAttempt('); i=t.rfind(chr(10),0,t.rfind('void',0,i)+1)
    j=t.index(chr(10)+'}'+chr(10),i)+3; return t[i:j]
open('b.txt','w').write(fn(open('base_sleep.cpp').read())); open('c.txt','w').write(fn(open('../../src/state/State_Sleep.cpp').read()))
"; python3 -c "
import difflib
print(''.join(difflib.unified_diff(open('b.txt').readlines(),open('c.txt').readlines(),n=0)))"
```

<details>
<summary>97 lines</summary>

```diff
--- 
+++ 
@@ -9,11 +9,9 @@
-        if (clearAb1805StaleAlarmInterrupts()) {
-          const time_t wakeTime = rtcNow + (time_t)wakeInSeconds;
-          if (ab1805.interruptAtTime(wakeTime)) {
-            setAppBreadcrumb(24); // BREADCRUMB_SLEEP_CONFIG_START (was stale literal 20, colliding with BREADCRUMB_REPORT_EXIT)
-            // Fresh local configuration per sleep: Device OS move-assigns
-            // without freeing the previous wake-source list, so a reused
-            // long-lived object orphans that list on every cycle.
-            SystemSleepConfiguration hibernateConfig;
-            hibernateConfig.mode(SystemSleepMode::HIBERNATE)
-              .gpio(WAKEUP_PIN, FALLING)
-              .gpio(BUTTON_PIN, FALLING);
+        setAppBreadcrumb(24); // BREADCRUMB_SLEEP_CONFIG_START (was stale literal 20, colliding with BREADCRUMB_REPORT_EXIT)
+        // Fresh local configuration per sleep: Device OS move-assigns
+        // without freeing the previous wake-source list, so a reused
+        // long-lived object orphans that list on every cycle.
+        SystemSleepConfiguration hibernateConfig;
+        hibernateConfig.mode(SystemSleepMode::HIBERNATE)
+          .gpio(WAKEUP_PIN, FALLING)
+          .gpio(BUTTON_PIN, FALLING)
+          .duration((uint32_t)wakeInSeconds * 1000UL);
@@ -21,5 +19,5 @@
-            const bool watchdogStopped = ab1805.stopWDT();
-            pauseAwakeWatchdogForSleep("hibernate");
-            if (!watchdogStopped) {
-              Log.warn("SleepCall: watchdog-stop failed mode=HIBERNATE dur=%ds", wakeInSeconds);
-            }
+        const bool watchdogStopped = (ExternalWatchdog.stop() == SYSTEM_ERROR_NONE);
+        pauseAwakeWatchdogForSleep("hibernate");
+        if (!watchdogStopped) {
+          Log.warn("SleepCall: watchdog-stop failed mode=HIBERNATE dur=%ds", wakeInSeconds);
+        }
@@ -27,6 +25,3 @@
-            // WO-2026-09-16 Step 2: HibernateCycle now owns the retained
-            // hibernate-cycle fields; wakeTime itself is used above for
-            // ab1805.interruptAtTime() only - nothing reads it back on
-            // wake (confirmed dead as a retained field), so it is not
-            // passed through here.
-            HibernateCycle::armForSleep(rtcNow, (uint32_t)wakeInSeconds);
+        // HibernateCycle owns the retained hibernate-cycle fields; Device OS
+        // arms the chip alarm itself from duration().
+        HibernateCycle::armForSleep(rtcNow, (uint32_t)wakeInSeconds);
@@ -34,10 +29,10 @@
-            Log.info("ModemTeardown: radioOn=%d point=hibernate", (int)Connectivity::isRadioPoweredOn());
-            Log.info("Sleep: HIBERNATE reason=closed dur=%ds wakePin=%d", wakeInSeconds, (int)WAKEUP_PIN);
-            PowerDiagnostics::logPowerState("pre-hibernate");
-            thrashGuard.markProgress("SLEEP_ATTEMPT");
-            Cloud::instance().logLedgerSleepState();
-            setAppBreadcrumb(25); // BREADCRUMB_SLEEP_SYSTEM_CALL (was stale literal 21, colliding with BREADCRUMB_IDLE_ENTRY) - entering final pre-sleep sequence
-            drainSerialBeforeSleep();
-            setAppBreadcrumb(26); // BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE
-            // HIBERNATE may reset the MCU without returning - flush the
-            // diagnostics batch now, since this is the only chance.
+        Log.info("ModemTeardown: radioOn=%d point=hibernate", (int)Connectivity::isRadioPoweredOn());
+        Log.info("Sleep: HIBERNATE reason=closed dur=%ds wakePin=%d", wakeInSeconds, (int)WAKEUP_PIN);
+        PowerDiagnostics::logPowerState("pre-hibernate");
+        thrashGuard.markProgress("SLEEP_ATTEMPT");
+        Cloud::instance().logLedgerSleepState();
+        setAppBreadcrumb(25); // BREADCRUMB_SLEEP_SYSTEM_CALL (was stale literal 21, colliding with BREADCRUMB_IDLE_ENTRY) - entering final pre-sleep sequence
+        drainSerialBeforeSleep();
+        setAppBreadcrumb(26); // BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE
+        // HIBERNATE may reset the MCU without returning - flush the
+        // diagnostics batch now, since this is the only chance.
@@ -45,2 +40,2 @@
-            PowerDiagnostics::flushDiagBatch();
-            setAppBreadcrumb(27); // BREADCRUMB_SLEEP_DIAG_FLUSH_DONE
+        PowerDiagnostics::flushDiagBatch();
+        setAppBreadcrumb(27); // BREADCRUMB_SLEEP_DIAG_FLUSH_DONE
@@ -48,3 +43,3 @@
-            setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
-            const SystemSleepResult hibernateResult = System.sleep(hibernateConfig);
-            AwakeCycles::recordSleepReturn(hibernateResult.error() == SYSTEM_ERROR_NONE);
+        setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
+        const SystemSleepResult hibernateResult = System.sleep(hibernateConfig);
+        AwakeCycles::recordSleepReturn(hibernateResult.error() == SYSTEM_ERROR_NONE);
@@ -52,12 +47,6 @@
-            // HIBERNATE should reset the MCU. If we return here, treat it as failed and fall back.
-            HibernateCycle::abandon();
-            ab1805.resumeWDT();
-            restoreAwakeWatchdogAfterWake("hibernate");
-            Log.warn("HIBERNATE sleep failed err=%d - falling back to ULTRA_LOW_POWER",
-                     (int)hibernateResult.error());
-          } else {
-            Log.warn("HIBERNATE bypass: failed to program AB1805 alarm - falling back to ULTRA_LOW_POWER");
-          }
-        } else {
-          Log.warn("HIBERNATE bypass: failed stale interrupt cleanup - falling back to ULTRA_LOW_POWER");
-        }
+        // HIBERNATE should reset the MCU. If we return here, treat it as failed and fall back.
+        HibernateCycle::abandon();
+        ExternalWatchdog.start();
+        restoreAwakeWatchdogAfterWake("hibernate");
+        Log.warn("HIBERNATE sleep failed err=%d - falling back to ULTRA_LOW_POWER",
+                 (int)hibernateResult.error());
<shellId: 24 completed with exit code 0>
```

</details>


---

<sub>2m 4s</sub>

### `bash`

**See function ends and other test diffs**

`$ cd build-tmp/wo-2026-11-05-004-stage6-r3; sed -n 1,20p b.txt; echo ======; sed -n 48,70p b.txt; echo =====; sed -n 44,60p c.txt; cd ../..; git diff 3b5cfb8 -- tests/sleep_cloud_gate_structural_test.py tests/sleep_config_ownership_structural_test.py tests/sleep_preflight_structural_test.py tests/clock_hibernate_helpers_move_structural_test.py`

<details>
<summary>201 lines</summary>

```diff
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

======
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
=====
        const SystemSleepResult hibernateResult = System.sleep(hibernateConfig);
        AwakeCycles::recordSleepReturn(hibernateResult.error() == SYSTEM_ERROR_NONE);

        // HIBERNATE should reset the MCU. If we return here, treat it as failed and fall back.
        HibernateCycle::abandon();
        ExternalWatchdog.start();
        restoreAwakeWatchdogAfterWake("hibernate");
        Log.warn("HIBERNATE sleep failed err=%d - falling back to ULTRA_LOW_POWER",
                 (int)hibernateResult.error());
      } else {
        Log.warn("HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER");
      }
    }
  }
#endif
}
diff --git a/tests/clock_hibernate_helpers_move_structural_test.py b/tests/clock_hibernate_helpers_move_structural_test.py
index ca03674..bfc046b 100644
--- a/tests/clock_hibernate_helpers_move_structural_test.py
+++ b/tests/clock_hibernate_helpers_move_structural_test.py
@@ -16,8 +16,9 @@ HIB_CPP = SRC / "time" / "HibernateEligibility.cpp"
 HIB_HDR = SRC / "time" / "HibernateEligibility.h"
 CYCLE = SRC / "time" / "HibernateCycle.cpp"
 
-CLOCK_FUNCS = ["ab1805WakeReasonNameLocal", "ab1805WakeReasonName", "logTimeDiag"]
-HIB_FUNCS = ["isDeviceOsAtLeast6400", "clearAb1805StaleAlarmInterrupts", "shouldUseBoronRtcAlarmHibernate"]
+# v50 S2 (WO-2026-11-05-004): ab1805WakeReasonName and clearAb1805StaleAlarmInterrupts are retired.
+CLOCK_FUNCS = ["logTimeDiag"]
+HIB_FUNCS = ["isDeviceOsAtLeast6400", "shouldUseBoronRtcAlarmHibernate"]
 
 
 def fail(msg):
@@ -62,7 +63,7 @@ for path in SRC.rglob("*"):
 # The Boron guard wraps the Sleep functions in the .cpp and their declarations in the header.
 guard = "#if PLATFORM_ID == PLATFORM_BORON"
 if hib_text.count(guard) != 1 or not hib_text.rstrip().endswith("#endif"):
-    fail("HibernateEligibility.cpp must wrap its three functions in one Boron guard")
+    fail("HibernateEligibility.cpp must wrap its two functions in one Boron guard")
 if hib_hdr.count(guard) != 1 or not hib_hdr.rstrip().endswith("#endif"):
     fail("HibernateEligibility.h must wrap its declarations in one Boron guard")
 if hib_text.index(guard) > min(hib_text.index(f"{n}(") for n in HIB_FUNCS):
@@ -70,13 +71,11 @@ if hib_text.index(guard) > min(hib_text.index(f"{n}(") for n in HIB_FUNCS):
 for name in HIB_FUNCS:
     if len(re.findall(rf"\b{name}\s*\(", hib_hdr)) != 1:
         fail(f"HibernateEligibility.h must declare {name} exactly once")
-if len(re.findall(r"\bab1805WakeReasonName\s*\(", clock_hdr)) != 1:
-    fail("ClockDiagnostics.h must declare ab1805WakeReasonName exactly once")
-if not re.search(r"^namespace\s*{.*?^const char \*ab1805WakeReasonNameLocal.*?^}\s*// namespace",
-                 clock_text, re.M | re.S):
-    fail("ab1805WakeReasonNameLocal must stay internal (anonymous namespace)")
-if "ab1805WakeReasonNameLocal" in clock_hdr:
-    fail("ab1805WakeReasonNameLocal must not be declared in the header")
+for retired in ("ab1805WakeReasonName", "clearAb1805StaleAlarmInterrupts"):
+    for label, text in (("ClockDiagnostics.cpp", clock_text), ("ClockDiagnostics.h", clock_hdr),
+                        ("HibernateEligibility.cpp", hib_text), ("HibernateEligibility.h", hib_hdr)):
+        if retired in text:
+            fail(f"{retired} is retired and must not appear in {label}")
 if '#include "state/StateMachine.h"' not in hib_text:
     fail("HibernateEligibility.cpp must reach ab1805 through state/StateMachine.h")
 if '#include "time/HibernateEligibility.h"' not in sleep_text:
@@ -86,11 +85,6 @@ if '#include "time/HibernateEligibility.h"' not in sleep_text:
 if "void logTimeDiag(bool isOpen);" not in (SRC / "state" / "State_Common.h").read_text():
     fail("logTimeDiag must stay declared in state/State_Common.h")
 
-# HibernateCycle.cpp keeps the hibernate retained fields and must not be touched by this move.
-diff = subprocess.run(["git", "diff", "--quiet", "HEAD", "--", str(CYCLE)], cwd=ROOT)
-if diff.returncode != 0:
-    fail("time/HibernateCycle.cpp must be unchanged")
-
 # Clock and hibernate-eligibility helpers are not retained state.
 for label, text in (("ClockDiagnostics.cpp", clock_text), ("ClockDiagnostics.h", clock_hdr),
                     ("HibernateEligibility.cpp", hib_text), ("HibernateEligibility.h", hib_hdr)):
diff --git a/tests/sleep_cloud_gate_structural_test.py b/tests/sleep_cloud_gate_structural_test.py
index 3d1d983..8ea6ebd 100644
--- a/tests/sleep_cloud_gate_structural_test.py
+++ b/tests/sleep_cloud_gate_structural_test.py
@@ -222,9 +222,19 @@ if retained_count(current_files.values()) != base_retained:
     fail("a `retained` keyword or identifier was added in src/")
 
 # The S-2 functions and the includes are unchanged.
+def without_hibernate_attempt(t):
+    # v50 S2 (WO-2026-11-05-004) changed hibernateAttempt on purpose (and swapped the ab1805 watchdog calls for ExternalWatchdog, normalised here); every other S-2 function is still compared.
+    i = t.find("hibernateAttempt(")
+    i = t.rfind("\n", 0, t.rfind("void", 0, i) + 1) if i >= 0 else -1
+    j = t.index("\n}\n", i) + 3 if i >= 0 else 0
+    t = t[:i] + t[j:] if i >= 0 else t
+    return (t.replace("(ExternalWatchdog.stop() == SYSTEM_ERROR_NONE)", "ab1805.stopWDT()")
+             .replace("ExternalWatchdog.start()", "ab1805.resumeWDT()"))
+
+
 def tail_of(text):
     i = text.index("struct WakeContext {")
-    return text[i:text.index("} // namespace\n", i)]
+    return without_hibernate_attempt(text[i:text.index("} // namespace\n", i)])
 
 
 def prefix_of(text):
@@ -238,7 +248,7 @@ def drain_of(text):
 
 if tail_of(sleep) != tail_of(base_sleep):
     fail("the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base")
-if prefix_of(sleep) != prefix_of(base_sleep) or drain_of(sleep) != drain_of(base_sleep):
+if prefix_of(sleep) != prefix_of(base_sleep).replace('#include "AB1805_RK.h"\n', "") or drain_of(sleep) != drain_of(base_sleep):
     fail("the includes or drainSerialBeforeSleep differ from the base")
 helpers = lambda t: t[t.index("unsigned long computeCloudSyncTimeoutMs("):t.index('    return "unknown";\n  }\n}\n')]
 if helpers(sleep) != helpers(base_sleep):
diff --git a/tests/sleep_config_ownership_structural_test.py b/tests/sleep_config_ownership_structural_test.py
index ebd658e..4e9e5c5 100644
--- a/tests/sleep_config_ownership_structural_test.py
+++ b/tests/sleep_config_ownership_structural_test.py
@@ -63,7 +63,8 @@ EXPECTED_SITES = {
     "hibernate": {
         "mode": "SystemSleepMode::HIBERNATE",
         "gpio": [("WAKEUP_PIN", "FALLING"), ("BUTTON_PIN", "FALLING")],
-        "duration": None,
+        # v50 S2 (WO-2026-11-05-004): Device OS arms the chip alarm from duration().
+        "duration": "(uint32_t)wakeInSeconds*1000UL",
         "network": None,
     },
     "ulp": {
@@ -533,6 +534,8 @@ def main():
                     drift.append("%s: not present at baseline" % tag)
                     continue
                 for key in ("mode", "duration", "network"):
+                    if tag == "hibernate" and key == "duration":
+                        continue  # v50 S2 (WO-2026-11-05-004): intentionally added, Device OS arms the alarm
                     if b[key] != expected[key]:
                         drift.append("%s.%s: baseline %r vs expected %r"
                                      % (tag, key, b[key], expected[key]))
diff --git a/tests/sleep_preflight_structural_test.py b/tests/sleep_preflight_structural_test.py
index 35ff1a9..372bdc9 100644
--- a/tests/sleep_preflight_structural_test.py
+++ b/tests/sleep_preflight_structural_test.py
@@ -207,6 +207,8 @@ def move_wifi_block(text):
 sleep = SLEEP.read_text()
 # WO-2026-11-03-005: the deliberate-reset flush call is the one intended addition; mapped back so every other byte is still compared.
 sleep = re.sub(r"^ *flushAllBeforeReset\(\);\n", "", sleep, flags=re.M)
+# v50 S2 (WO-2026-11-05-004): the one reworded comment is mapped back so every other handler byte is still compared.
+sleep = sleep.replace("// The chip alarm is armed by Device OS from duration(); the app only drives its watchdog.", "// NO AB1805 alarms - AB1805 is only used for watchdog + RTC time sync.")
 RESET = "    wifiGuard.wifiOffGuardRetries = 0;\n#endif\n  }"
 probes = {
     "a stray write to a guard field in the handler": sleep.replace("  SleepTeardown::guardLostTimestamp();\n", "  SleepTeardown::guardLostTimestamp();\n  wifiGuard.wifiOffGuardActive = true;\n", 1),
@@ -265,13 +267,23 @@ if cur_retained != base_retained:
     fail("a `retained` keyword or identifier was added in src/")
 
 # The S-2 functions, the S-3 gate, the S-4 functions and the includes are unchanged.
-tail_of = lambda t: t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))]
+def without_hibernate_attempt(t):
+    # v50 S2 (WO-2026-11-05-004) changed hibernateAttempt on purpose (and swapped the ab1805 watchdog calls for ExternalWatchdog, normalised here); every other S-2 function is still compared.
+    i = t.find("hibernateAttempt(")
+    i = t.rfind("\n", 0, t.rfind("void", 0, i) + 1) if i >= 0 else -1
+    j = t.index("\n}\n", i) + 3 if i >= 0 else 0
+    t = t[:i] + t[j:] if i >= 0 else t
+    return (t.replace("(ExternalWatchdog.stop() == SYSTEM_ERROR_NONE)", "ab1805.stopWDT()")
+             .replace("ExternalWatchdog.start()", "ab1805.resumeWDT()"))
+
+
+tail_of = lambda t: without_hibernate_attempt(t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))])
 prefix_of = lambda t: t[:t.index("namespace {")]
 gate_of = lambda t: t[t.index("struct {\n  bool operationsCompleteLogged"):t.index(SUMMARY_SIG) if SUMMARY_SIG in t else t.index("struct SleepPlan {")]
 plan_of = lambda t: t[t.index("struct SleepPlan {"):t.index("struct WakeContext {")]
 if tail_of(sleep) != tail_of(base_sleep):
     fail("the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base")
-if prefix_of(sleep) != prefix_of(base_sleep):
+if prefix_of(sleep) != prefix_of(base_sleep).replace('#include "AB1805_RK.h"\n', ""):
     fail("the includes differ from the base")
 if gate_of(sleep).rstrip("\n") != gate_of(base_sleep).rstrip("\n"):
     fail("the cloudGate struct or cloudGatePass() (S-3) differ from the base")
<shellId: 25 completed with exit code 0>
```

</details>


---

<sub>2m 20s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/s2\_hibernate\_normalise.py.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/s2_hibernate_normalise.py`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/s2_hibernate_normalise.py with 2452 characters
```


---

<sub>2m 26s</sub>

### `bash`

**Rewire four structural tests**

```
$ python3 - <<'E'
import re
for n in ['cloud_gate','plan','preflight','wake']:
    p=f'tests/sleep_{n}_structural_test.py'; s=open(p).read()
    a=s.index("def without_hibernate_attempt(t):"); b=s.index("\n\n\n",a)+3
    s=s[:a]+"import s2_hibernate_normalise  # v50 S2 (WO-2026-11-05-004): hibernateAttempt() is compared to the base with only the approved S2 substitutions normalised\n\n\n"+s[b:]
    s=s.replace("without_hibernate_attempt(t[t.index","s2_hibernate_normalise.normalise(t[t.index").replace("without_hibernate_attempt(text[i:","s2_hibernate_normalise.normalise(text[i:")
    open(p,'w').write(s)
    print(p,[l for l in s.splitlines() if 'tail_of' in l or 'normalise' in l])
E
```

```
tests/sleep_cloud_gate_structural_test.py ['import s2_hibernate_normalise  # v50 S2 (WO-2026-11-05-004): hibernateAttempt() is compared to the base with only the approved S2 substitutions normalised', 'def tail_of(text):', '    return s2_hibernate_normalise.normalise(text[i:text.index("} // namespace\\n", i)])', 'if tail_of(sleep) != tail_of(base_sleep):']
tests/sleep_plan_structural_test.py ['import s2_hibernate_normalise  # v50 S2 (WO-2026-11-05-004): hibernateAttempt() is compared to the base with only the approved S2 substitutions normalised', 'tail_of = lambda t: s2_hibernate_normalise.normalise(t[t.index("struct WakeContext {"):t.index("} // namespace\\n", t.index("struct WakeContext {"))])', 'if tail_of(sleep) != tail_of(base_sleep):']
tests/sleep_preflight_structural_test.py ['import s2_hibernate_normalise  # v50 S2 (WO-2026-11-05-004): hibernateAttempt() is compared to the base with only the approved S2 substitutions normalised', 'tail_of = lambda t: s2_hibernate_normalise.normalise(t[t.index("struct WakeContext {"):t.index("} // namespace\\n", t.index("struct WakeContext {"))])', 'if tail_of(sleep) != tail_of(base_sleep):']
tests/sleep_wake_structural_test.py ['import s2_hibernate_normalise  # v50 S2 (WO-2026-11-05-004): hibernateAttempt() is compared to the base with only the approved S2 substitutions normalised', 'tail_of = lambda t: s2_hibernate_normalise.normalise(t[t.index("struct WakeContext {"):t.index("} // namespace\\n", t.index("struct WakeContext {"))])', 'if tail_of(sleep) != tail_of(base_sleep):']
<shellId: 26 completed with exit code 0>
```


---

<sub>2m 30s</sub>

### Copilot

Add the `is_base` flag to each.

---

<sub>2m 41s</sub>

### `bash`

**Add base flag and run four tests**

```
$ python3 - <<'E'
for n in ['cloud_gate','plan','preflight','wake']:
    p=f'tests/sleep_{n}_structural_test.py'; s=open(p).read()
    s=s.replace("def tail_of(text):","def tail_of(text, is_base=False):").replace('"} // namespace\\n", i)])\n','"} // namespace\\n", i)], is_base)\n',1) if n=='cloud_gate' else s.replace("tail_of = lambda t:","tail_of = lambda t, is_base=False:").replace('t.index("struct WakeContext {"))])','t.index("struct WakeContext {"))], is_base)')
    s=s.replace("tail_of(base_sleep)","tail_of(base_sleep, True)")
    open(p,'w').write(s)
E
grep -n "tail_of" tests/sleep_*_structural_test.py | head -20; for t in cloud_gate plan preflight wake; do python3 tests/sleep_${t}_structural_test.py 2>&1 | tail -3; done
```

<details>
<summary>13 lines</summary>

```
tests/sleep_cloud_gate_structural_test.py:228:def tail_of(text, is_base=False):
tests/sleep_cloud_gate_structural_test.py:242:if tail_of(sleep) != tail_of(base_sleep, True):
tests/sleep_plan_structural_test.py:236:tail_of = lambda t, is_base=False: s2_hibernate_normalise.normalise(t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))], is_base)
tests/sleep_plan_structural_test.py:239:if tail_of(sleep) != tail_of(base_sleep, True):
tests/sleep_preflight_structural_test.py:273:tail_of = lambda t, is_base=False: s2_hibernate_normalise.normalise(t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))], is_base)
tests/sleep_preflight_structural_test.py:277:if tail_of(sleep) != tail_of(base_sleep, True):
tests/sleep_wake_structural_test.py:202:tail_of = lambda t, is_base=False: s2_hibernate_normalise.normalise(t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))], is_base)
tests/sleep_wake_structural_test.py:210:if tail_of(sleep) != tail_of(base_sleep, True):
FAIL: the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base
FAIL: the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base
FAIL: the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base
FAIL: the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base
<shellId: 27 completed with exit code 0>
```

</details>


---

<sub>2m 46s</sub>

### `bash`

**Debug difference**

```
$ sed -n 226,233p tests/sleep_cloud_gate_structural_test.py; python3 - <<'E'
import sys,difflib
sys.path.insert(0,'tests')
import s2_hibernate_normalise as n
b=open('build-tmp/wo-2026-11-05-004-stage6-r3/base_sleep.cpp').read(); c=open('src/state/State_Sleep.cpp').read()
t=lambda x,f: n.normalise(x[x.index("struct WakeContext {"):x.index("} // namespace\n",x.index("struct WakeContext {"))],f)
print(''.join(difflib.unified_diff(t(b,True).splitlines(1),t(c,False).splitlines(1),n=0)))
E
```

<details>
<summary>16 lines</summary>

```diff
def tail_of(text, is_base=False):
    i = text.index("struct WakeContext {")
    return s2_hibernate_normalise.normalise(text[i:text.index("} // namespace\n", i)], is_base)


def prefix_of(text):
--- 
+++ 
@@ -108 +108 @@
-  const bool watchdogStopped = ab1805.stopWDT();
+  const bool watchdogStopped = (ExternalWatchdog.stop() == SYSTEM_ERROR_NONE);
@@ -221 +221 @@
-        ab1805.resumeWDT();
+        ExternalWatchdog.start();
@@ -249 +249 @@
-  ab1805.resumeWDT();
+  ExternalWatchdog.start();
<shellId: 28 completed with exit code 0>
```

</details>


---

<sub>3m 4s</sub>

### `bash`

**Handle watchdog subs globally; rerun**

```
$ python3 - <<'E'
p='tests/s2_hibernate_normalise.py'; s=open(p).read()
s=s.replace('''    # The chip watchdog replaces the AB1805 one.
    ("const bool watchdogStopped = ab1805.stopWDT();\\n", "const bool watchdogStopped = (ExternalWatchdog.stop() == SYSTEM_ERROR_NONE);\\n"),
    ("ab1805.resumeWDT();\\n", "ExternalWatchdog.start();\\n"),
''','')
s=s.replace('''

def _span(t):''','''
# The chip watchdog replaces the AB1805 one at every sleep site (hibernate and ULP/STOP): (old, new, expected count in the base).
WATCHDOG = [
    ("const bool watchdogStopped = ab1805.stopWDT();", "const bool watchdogStopped = (ExternalWatchdog.stop() == SYSTEM_ERROR_NONE);", 2),
    ("ab1805.resumeWDT();", "ExternalWatchdog.start();", 3),
]


def _span(t):''')
s=s.replace('''            body = body.replace(old, new)
    return t[:i] + body + t[j:]''','''            body = body.replace(old, new)
    t = t[:i] + body + t[j:]
    if is_base:
        for old, new, count in WATCHDOG:
            if t.count(old) != count:
                raise SystemExit(f"S2 watchdog substitution expected {count}x in the base: {old!r}")
            t = t.replace(old, new)
    return t''')
open(p,'w').write(s)
E
for t in cloud_gate plan preflight wake; do python3 tests/sleep_${t}_structural_test.py 2>&1 | tail -3; done
```

```
sleep_cloud_gate_structural_test: ok
sleep_plan_structural_test: ok
sleep_preflight_structural_test: ok
sleep_wake_structural_test: ok
<shellId: 29 completed with exit code 0>
```


---

<sub>3m 9s</sub>

### `bash`

**View equivalence test diff**

`$ git diff 3b5cfb8 -- tests/sleep_execution_equivalence_test.sh tests/sleep_execution_equivalence_test.cpp tests/stubs`

<details>
<summary>12 lines</summary>

```diff
Output too large to read at once (22.4 KB). Saved to: /var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794049642455-copilot-tool-output-56334-b91149bd-e0ed-44b0-8683-99c931255373.txt
Consider using tools like grep (for searching), head/tail (for viewing start/end), view with view_range (for specific sections), or jq (for JSON) to examine portions of the output.

Preview (first 500 chars):
diff --git a/tests/sleep_execution_equivalence_test.cpp b/tests/sleep_execution_equivalence_test.cpp
index 0f886ea..9a750b9 100644
--- a/tests/sleep_execution_equivalence_test.cpp
+++ b/tests/sleep_execution_equivalence_test.cpp
@@ -1,10 +1,9 @@
-// WO-2026-10-21-001 (Sleep S-2): equivalence of the sleep-execution refactor.
-// (a) is the base handler's standby decision, hibernate block and ULP/STOP cascade, extracted
-// by marker text from the base State_Sleep.cpp (wrapped below as base::), (b
<shellId: 30 completed with exit code 0>
```

</details>


---

<sub>3m 14s</sub>

### `bash`

**Fetch base equivalence files**

`$ git show 3b5cfb8:tests/sleep_execution_equivalence_test.sh > build-tmp/wo-2026-11-05-004-stage6-r3/base_eq.sh; git show 3b5cfb8:tests/sleep_execution_equivalence_test.cpp > build-tmp/wo-2026-11-05-004-stage6-r3/base_eq.cpp; git show 3b5cfb8:tests/stubs/sleep_teardown/SleepExecutionFakes.h > build-tmp/wo-2026-11-05-004-stage6-r3/base_fakes.h; wc -l build-tmp/wo-2026-11-05-004-stage6-r3/base_eq.* tests/sleep_execution_equivalence_test.*; git diff 3b5cfb8 -- tests/stubs`

<details>
<summary>43 lines</summary>

```diff
259 build-tmp/wo-2026-11-05-004-stage6-r3/base_eq.cpp
     231 build-tmp/wo-2026-11-05-004-stage6-r3/base_eq.sh
     211 tests/sleep_execution_equivalence_test.cpp
     200 tests/sleep_execution_equivalence_test.sh
     901 total
diff --git a/tests/stubs/sleep_teardown/SleepExecutionFakes.h b/tests/stubs/sleep_teardown/SleepExecutionFakes.h
index 3693899..0fcb857 100644
--- a/tests/stubs/sleep_teardown/SleepExecutionFakes.h
+++ b/tests/stubs/sleep_teardown/SleepExecutionFakes.h
@@ -44,7 +44,7 @@ struct SensorWakeSource { pin_t pin; InterruptMode edge; };
 namespace sim {
 struct Exec {
   struct Sleep { int err; pin_t pin; };
-  bool hibEligible = false, hibPinHigh = true, staleClearOk = true, alarmOk = true, wdtStopOk = true;
+  bool hibEligible = false, hibPinHigh = true, wdtStopOk = true;
   std::vector<Sleep> sleeps;
   size_t nextSleep = 0;
   pin_t sensorPin = 9;
@@ -140,12 +140,11 @@ inline void userSwitchISR() {}
 inline bool userSwitchDetected = true;
 inline void attachInterrupt(pin_t p, void (*)(), InterruptMode e) { sim::emit("CALL attachInterrupt %u %d", (unsigned)p, (int)e); }
 
-struct AB1805Stub {
-  bool interruptAtTime(time_t t) { sim::emit("AB1805 interruptAtTime %lld", (long long)t); return sim::exec.alarmOk; }
-  bool stopWDT() { sim::emit("AB1805 stopWDT"); return sim::exec.wdtStopOk; }
-  bool resumeWDT() { sim::emit("AB1805 resumeWDT"); return true; }
+struct ExternalWatchdogStub {
+  int stop() { sim::emit("EXTWDT stop"); return sim::exec.wdtStopOk ? SYSTEM_ERROR_NONE : -1; }
+  int start() { sim::emit("EXTWDT start"); return SYSTEM_ERROR_NONE; }
 };
-inline AB1805Stub ab1805;
+inline ExternalWatchdogStub ExternalWatchdog;
 struct WatchdogStub { bool started() { sim::emit("CALL Watchdog.started"); return true; } };
 inline WatchdogStub Watchdog;
 inline void pauseAwakeWatchdogForSleep(const char *ctx) { sim::emit("WATCHDOG pause %s", ctx); }
@@ -156,7 +155,6 @@ inline bool shouldUseBoronRtcAlarmHibernate(uint32_t wakeIn, time_t &rtcNow) {
   sim::emit("HIB eligible? wakeIn=%u -> %d", (unsigned)wakeIn, (int)sim::exec.hibEligible);
   return sim::exec.hibEligible;
 }
-inline bool clearAb1805StaleAlarmInterrupts() { sim::emit("HIB clearStale"); return sim::exec.staleClearOk; }
 namespace HibernateCycle {
 inline void armForSleep(time_t rtcNow, uint32_t wakeIn) { sim::emit("HIB armForSleep %lld %u", (long long)rtcNow, (unsigned)wakeIn); }
 inline void abandon() { sim::emit("HIB abandon"); }
<shellId: 31 completed with exit code 0>
```

</details>


---

<sub>3m 13s</sub>

### `view` — Failed

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/build-tmp/wo-2026-11-05-004-stage6-r3/base\_eq.sh.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/build-tmp/wo-2026-11-05-004-stage6-r3/base_eq.sh`

```
Path does not exist
```


---

<sub>3m 18s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/build-tmp/wo-2026-11-05-004-stage6-r3/base\_eq.sh.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/build-tmp/wo-2026-11-05-004-stage6-r3/base_eq.sh`

<details>
<summary>203 lines</summary>

```
#!/bin/zsh
set -euo pipefail

# WO-2026-10-21-001 (Sleep S-2): moving the standby decision, the hibernate attempt and the
# ULP/STOP sleep cascade out of handleSleepingState() into effectiveStandby(), hibernateAttempt()
# and sleepAndResume() changes no behaviour. tests/sleep_execution_equivalence_test.cpp runs one
# scenario table through (a) the blocks extracted by marker text from the base State_Sleep.cpp
# and (b) the current functions and the handler's call-site lines, extracted by marker text from
# the current file, against the same fakes (tests/stubs/sleep_teardown/SleepExecutionFakes.h),
# for cellular and Wi-Fi. Both transcripts must equal, byte for byte, the frozen
# fixtures/sleep_execution_transcript.txt ('# ' header lines stripped first). The extracted copies
# the harness compiled are read back and compared with a fresh extraction: COPY_MISMATCH on drift.
# Step (a) needs the base commit named in the fixture header; without it (a) is skipped and said.
# Mutants of a private copy of State_Sleep.cpp must each change the transcript.
# SLEEP_EXECUTION_SRC_ROOT tests another src/ root; SLEEP_EXECUTION_TAMPER_COPY=1 demonstrates
# COPY_MISMATCH. '--regenerate' rewrites the fixture from the base blocks (a).

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${SLEEP_EXECUTION_SRC_ROOT:-$repo_root/src}"
fixture="$tests_dir/fixtures/sleep_execution_transcript.txt"
stubs="$tests_dir/stubs/sleep_teardown"
work="$(mktemp -d "${TMPDIR:-/tmp}/sleep_execution_equivalence.XXXXXX")"
trap 'rm -rf "$work"' EXIT

base="$(sed -n 's/^# Base commit: //p' "$fixture" 2>/dev/null || true)"
base="${base:-1e6d34f}"
base_src="$work/base_State_Sleep.cpp"
have_base=0
if git -C "$repo_root" show "${base}:src/state/State_Sleep.cpp" > "$base_src" 2>/dev/null; then have_base=1; fi

cat > "$work/extract.py" <<'PY'
import sys

kind, path, outdir = sys.argv[1:4]
text = open(path).read()


def span(start, end, whole_line_end=False, keep_end=True):
    if text.count(start) != 1:
        sys.exit(f"MARKER_NOT_UNIQUE: {start!r} x{text.count(start)} in {path}")
    i = text.find(start)
    j = text.find(end, i + len(start) if end != start else i)
    if j < 0:
        sys.exit(f"MARKER_MISSING: {end!r} in {path}")
    return text[i:j + len(end)] if keep_end else text[i:j]


def line(start):
    return span(start, "\n")


out = {"drain": span("void drainSerialBeforeSleep() {", "\n}\n")}
if kind == "base":
    out["standby"] = span("  bool preserveStandbyForThisPass = false;\n", "  useNetworkStandbyEffective = false;\n#endif\n")
    out["hib"] = span("#if PLATFORM_ID == PLATFORM_BORON\n  if (overnightFallbackSleep) {\n    time_t rtcNow", "\n  }\n#endif\n")
    out["ulp"] = span("  const SensorWakeSource sensorWake = SensorManager", '  PowerDiagnostics::logPowerState("post-wake");\n')
else:
    out["funcs"] = span("struct WakeContext {", "\n} // namespace\n", keep_end=False) + "\n"
    out["call_effective"] = line("  bool useNetworkStandbyEffective = effectiveStandby(")
    out["call_teardown"] = line("  if (SleepTeardown::run(")
    out["call_hibernate"] = line("  hibernateAttempt(")
    out["call_sleep"] = span("  WakeContext wake;\n", "  if (sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)) return;\n")
    out["call_route"] = line("  postWakeRouting(")
for k, v in out.items():
    open(f"{outdir}/{kind}_{k}.inc", "w").write(v)
PY

# Writes the base_*.inc / cur_*.inc copies the harness compiles.
extract() {
  # $1 = base|cur, $2 = source file
  python3 "$work/extract.py" "$1" "$2" "$work"
}

if (( have_base )); then extract base "$base_src"; fi
extract cur "$src_root/state/State_Sleep.cpp"
if [[ "${SLEEP_EXECUTION_TAMPER_COPY:-}" == 1 ]]; then echo "// drift" >> "$work/cur_funcs.inc"; fi

build_run() {
  # $1 = platform (cellular|wifi), $2 = mode (a|b), $3 = output, $4 = work dir holding the .inc copies
  local platform="$1" mode="$2" out="$3" inc="$4" defs=()
  [[ "$platform" == cellular ]] && defs=(-DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1 -DENABLE_SLEEP_TRACE=1) || defs=(-DWiring_WiFi=1)
  c++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror "${defs[@]}" -I"$stubs" -I"$src_root" \
    -DBASE_DRAIN_INC="\"$inc/base_drain.inc\"" -DBASE_STANDBY_INC="\"$inc/base_standby.inc\"" \
    -DBASE_HIB_INC="\"$inc/base_hib.inc\"" -DBASE_ULP_INC="\"$inc/base_ulp.inc\"" \
    -DCUR_DRAIN_INC="\"$inc/cur_drain.inc\"" -DCUR_FUNCS_INC="\"$inc/cur_funcs.inc\"" \
    -DCUR_CALL_EFFECTIVE_INC="\"$inc/cur_call_effective.inc\"" -DCUR_CALL_TEARDOWN_INC="\"$inc/cur_call_teardown.inc\"" \
    -DCUR_CALL_HIBERNATE_INC="\"$inc/cur_call_hibernate.inc\"" -DCUR_CALL_SLEEP_INC="\"$inc/cur_call_sleep.inc\"" -DCUR_CALL_ROUTE_INC="\"$inc/cur_call_route.inc\"" \
    "$tests_dir/sleep_execution_equivalence_test.cpp" -o "$out.bin"
  "$out.bin" "$mode" > "$out"
}

run_all() {
  # $1 = mode, $2 = output prefix, $3 = work dir holding the .inc copies
  build_run cellular "$1" "$2.cellular" "$3"
  build_run wifi "$1" "$2.wifi" "$3"
  cat "$2.cellular" "$2.wifi" > "$2"
}

if [[ "${1:-}" == "--regenerate" ]]; then
  (( have_base )) || { echo "NO_BASE_COMMIT: $base"; exit 1; }
  run_all a "$work/regen.txt" "$work"
  python3 - "$base" "$work/regen.txt" "$fixture" <<'PY'
import hashlib
import re
import sys

base, body_path, fixture = sys.argv[1:4]
body = open(body_path).read()
counts = re.findall(r"^SCENARIOS (\d+) PASSES (\d+) WOKE (\d+) RETURNED (\d+) DISTINCT_EVENTS (\d+)$", body, re.M)
scen = sum(int(c[0]) for c in counts)
passes = sum(int(c[1]) for c in counts)
head = f"""# Frozen output of the pre-change sleep execution (standby decision, hibernate attempt, ULP/STOP cascade): WO-2026-10-21-001 (Sleep S-2).
# Base commit: {base}
# Scenarios: {scen} (cellular {counts[0][0]} + wifi {counts[1][0]}); passes: {passes}
# Per-platform summary (scenarios passes WOKE RETURNED distinct-events): cellular {' '.join(counts[0])}; wifi {' '.join(counts[1])}
# Body sha256: {hashlib.sha256(body.encode()).hexdigest()}
# Generated once by extracting the blocks by marker text from {base}:src/state/State_Sleep.cpp (git show),
# wrapping them in tests/sleep_execution_equivalence_test.cpp against tests/stubs/sleep_teardown, and running mode a.
"""
open(fixture, "w").write(head + body)
PY
  echo "fixture written: $fixture"
  exit 0
fi

grep -v '^# ' "$fixture" > "$work/expected.txt"
python3 - "$fixture" "$work/expected.txt" <<'PY'
import hashlib
import re
import sys

fixture, body_path = sys.argv[1:3]
head = open(fixture).read()
body = open(body_path).read()
want = re.search(r"^# Body sha256: (\w+)$", head, re.M).group(1)
if hashlib.sha256(body.encode()).hexdigest() != want:
    sys.exit("FIXTURE_HEADER_SHA_MISMATCH")
scen = int(re.search(r"^# Scenarios: (\d+)", head, re.M).group(1))
if sum(int(n) for n in re.findall(r"^SCENARIOS (\d+) ", body, re.M)) != scen:
    sys.exit("FIXTURE_HEADER_SCENARIO_COUNT_MISMATCH")
PY

run_all b "$work/new.txt" "$work"

# What the harness compiled must be what the file says: re-extract and compare every copy.
mkdir -p "$work/fresh"
extract_fresh() { python3 "$work/extract.py" "$1" "$2" "$work/fresh"; }
extract_fresh cur "$src_root/state/State_Sleep.cpp"
(( have_base )) && extract_fresh base "$base_src"
for f in "$work"/fresh/*.inc; do
  cmp -s "$f" "$work/${f:t}" || { echo "COPY_MISMATCH: ${f:t} compiled by the harness differs from the file"; exit 1; }
done

diff -u "$work/expected.txt" "$work/new.txt" > "$work/new.diff" || { head -60 "$work/new.diff"; echo "EQUIVALENCE_FAILED: State_Sleep.cpp differs from the frozen transcript"; exit 1; }

if (( have_base )); then
  run_all a "$work/old.txt" "$work"
  diff -u "$work/expected.txt" "$work/old.txt" > "$work/old.diff" || { head -60 "$work/old.diff"; echo "FIXTURE_NOT_FROM_BASE: the base blocks differ from the fixture ($base)"; exit 1; }
else
  echo "BASE_CHECK_SKIPPED: commit $base is not in this tree; only the fixture comparison ran"
fi

# The transcript is not trivial: every branch the moved code has shows up, and so do all outcomes.
for line in "HIB eligible?" "HIB clearStale" "HIB armForSleep" "HIB abandon" "AB1805 interruptAtTime" \
            "SLEEP mode=HIBERNATE gpio=[7F,4F]" "SLEEP mode=ULP gpio=[4F,9R] net=3:0 dur=3600000" \
            "SLEEP mode=ULP gpio=[4F,9R] net=none dur=900000" "SLEEP mode=STOP gpio=[4F,9R]" \
            "SLEEP mode=STOP gpio=[] net=none dur=3600000" "SLEEP mode=ULP gpio=[4F] net=3:0" \
            "TEARDOWN req=1 eff=0 force=1 err=-170" "TEARDOWN req=1 eff=1 force=0 err=0" "RESET cause=5" \
            "TRANSITION 1 sleep-all-attempts-failed" "ALERT 16" "CALL Serial.begin 9600" "CALL attachInterrupt 4" \
            "WATCHDOG pause ulp" "WATCHDOG restore hibernate" "CRUMB 24" "CRUMB 28" "CRUMB 4" "PROGRESS WAKE_FROM_SLEEP" \
            "PROGRESS SERIAL_WAIT" "POWER flushDiagBatch" "CYCLE resetOnWake" "MEASURE noteWakeFromLowPowerSleep" \
            "LOG INFO SLEEP: standby sleep returned normally; preserving modem standby" \
            "LOG WARN SLEEP: standby sleep failed; forcing modem-off reason=sleep_error err=-170" \
            "LOG WARN HIBERNATE bypass: wake pin held low" "LOG WARN HIBERNATE bypass: failed stale interrupt cleanup" \
            "LOG WARN HIBERNATE bypass: failed to program AB1805 alarm" "LOG WARN HIBERNATE sleep failed err=-170" \
            "LOG ERROR ULTRA_LOW_POWER sleep failed err=-170" "LOG ERROR STOP sleep fallback failed" \
            "LOG ERROR All sleep attempts failed err=-170" "LOG WARN Serial did not reconnect within 30s" \
            "LOG INFO SleepCall: standby=1" "LOG WARN SleepCall: watchdog-stop failed" \
            "RET fell=1 pir=1 btn=0 timer=0 label=PIR" "RET fell=1 pir=0 btn=1 timer=0 label=BUTTON" \
            "RET fell=1 pir=0 btn=0 timer=1 label=TIMER" "RET fell=0" \
            "FIELDS eff=1 preserve=1 force=0 err=0" "FIELDS eff=0 preserve=0 force=1 err=-170" \
            "FIELDS eff=0 preserve=0 force=0 err=0" "... x"; do
  grep -qF -- "$line" "$work/new.txt" || { echo "LOG_LINE_NEVER_EXERCISED: $line"; exit 1; }
done

mutate_and_expect_diff() {
  # $1 = label, then old/new pairs applied to a private copy of State_Sleep.cpp
  local m="$work/mut_${1// /_}"; shift
  local label="${m:t}"
  mkdir -p "$m"
  cp "$src_root/state/State_Sleep.cpp" "$m/State_Sleep.cpp"
  python3 - "$m/State_Sleep.cpp" "$@" <<'PY'
import sys

path, pairs = sys.argv[1], sys.argv[2:]
text = open(path).read()
for old, new in zip(pairs[0::2], pairs[1::2]):
    if text.count(old) != 1:
        sys.exit(f"MUTATION_NOT_APPLIED: {old!r} x{text.count(old)}")
    text = text.replace(old, new)
open(path, "w").write(text)
PY
  cp "$work"/base_*.inc "$m/" 2>/dev/null || true
  python3 "$work/extract.py" cur "$m/State_Sleep.cpp" "$m"
  run_all b "$m/new.txt" "$m" > "$m/build.log" 2>&1 || { echo "MUTANT_DID_NOT_BUILD: $label"; cat "$m/build.log"; exit 1; }
  if diff -q "$work/expected.txt" "$m/new.txt" > /dev/null; then echo "MUTANT_SURVIVED: $label"; exit 1; fi
  echo "caught: $label"
}

mutate_and_expect_diff "ULP error skips STOP with GPIO" \
  "    result = System.sleep(stopConfig);
" ""
mutate_and_expect_diff "standby error leaves forceModemOff unset" \
  "standbyReturn.forceModemOff = true;" "standbyReturn.forceModemOff = false;"
mutate_and_expect_diff "returned hibernate not abandoned" \
  "            HibernateCycle::abandon();
" ""
mutate_and_expect_diff "WAKE_FROM_SLEEP progress after the serial re-wait" \
  "  thrashGuard.markProgress(\"WAKE_FROM_SLEEP\");
" "" \
  "  Log.info(\"WakeReturn: elapsed=%lu reason=%s\"," "  thrashGuard.markProgress(\"WAKE_FROM_SLEEP\");
  Log.info(\"WakeReturn: elapsed=%lu reason=%s\","
mutate_and_expect_diff "standby preserved through an unstable modem" \
  "(!useNetworkStandbyRequested || session.modemUnstable || Cellular.isOff())" "(!useNetworkStandbyRequested || Cellular.isOff())"
mutate_and_expect_diff "PIR classified from the wrong edge" \
  "bool pirWake = sensorCanWake && (wakePin == sensorWake.pin);" "bool pirWake = sensorCanWake && (wakePin != sensorWake.pin);"
mutate_and_expect_diff "post-wake routing handed swapped pir and button" \
  "postWakeRouting(wake.pirWake, wake.buttonWake, wake.timerWake, wake.wakeReasonLabel);" "postWakeRouting(wake.buttonWake, wake.pirWake, wake.timerWake, wake.wakeReasonLabel);"

echo "sleep_execution_equivalence_test: both transcripts match the frozen pre-change output, all mutations detected"
```

</details>


---

<sub>3m 18s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/sleep\_execution\_equivalence\_test.sh.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/sleep_execution_equivalence_test.sh (lines 60-200)`

<details>
<summary>127 lines</summary>

```
extract cur "$src_root/state/State_Sleep.cpp"
if [[ "${SLEEP_EXECUTION_TAMPER_COPY:-}" == 1 ]]; then echo "// drift" >> "$work/cur_funcs.inc"; fi

build_run() {
  # $1 = platform (cellular|wifi), $2 = mode (a|b), $3 = output, $4 = work dir holding the .inc copies
  local platform="$1" mode="$2" out="$3" inc="$4" defs=()
  [[ "$platform" == cellular ]] && defs=(-DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1 -DENABLE_SLEEP_TRACE=1) || defs=(-DWiring_WiFi=1)
  c++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror "${defs[@]}" -I"$stubs" -I"$src_root" \
    -DCUR_DRAIN_INC="\"$inc/cur_drain.inc\"" -DCUR_FUNCS_INC="\"$inc/cur_funcs.inc\"" \
    -DCUR_CALL_EFFECTIVE_INC="\"$inc/cur_call_effective.inc\"" -DCUR_CALL_TEARDOWN_INC="\"$inc/cur_call_teardown.inc\"" \
    -DCUR_CALL_HIBERNATE_INC="\"$inc/cur_call_hibernate.inc\"" -DCUR_CALL_SLEEP_INC="\"$inc/cur_call_sleep.inc\"" -DCUR_CALL_ROUTE_INC="\"$inc/cur_call_route.inc\"" \
    "$tests_dir/sleep_execution_equivalence_test.cpp" -o "$out.bin"
  "$out.bin" "$mode" > "$out"
}

run_all() {
  # $1 = mode, $2 = output prefix, $3 = work dir holding the .inc copies
  build_run cellular "$1" "$2.cellular" "$3"
  build_run wifi "$1" "$2.wifi" "$3"
  cat "$2.cellular" "$2.wifi" > "$2"
}

if [[ "${1:-}" == "--regenerate" ]]; then
  run_all b "$work/regen.txt" "$work"
  python3 - "$work/regen.txt" "$fixture" <<'PY'
import hashlib
import re
import sys

body_path, fixture = sys.argv[1:3]
body = open(body_path).read()
counts = re.findall(r"^SCENARIOS (\d+) PASSES (\d+) WOKE (\d+) RETURNED (\d+) DISTINCT_EVENTS (\d+)$", body, re.M)
scen = sum(int(c[0]) for c in counts)
passes = sum(int(c[1]) for c in counts)
head = f"""# Frozen output of the sleep execution (standby decision, hibernate attempt, ULP/STOP cascade): WO-2026-10-21-001 (Sleep S-2), re-frozen for v50 S2 (WO-2026-11-05-004).
# Scenarios: {scen} (cellular {counts[0][0]} + wifi {counts[1][0]}); passes: {passes}
# Per-platform summary (scenarios passes WOKE RETURNED distinct-events): cellular {' '.join(counts[0])}; wifi {' '.join(counts[1])}
# Body sha256: {hashlib.sha256(body.encode()).hexdigest()}
# Generated with --regenerate from the S2 State_Sleep.cpp (native HIBERNATE duration alarm, ExternalWatchdog) against tests/stubs/sleep_teardown.
"""
open(fixture, "w").write(head + body)
PY
  echo "fixture written: $fixture"
  exit 0
fi

grep -v '^# ' "$fixture" > "$work/expected.txt"
python3 - "$fixture" "$work/expected.txt" <<'PY'
import hashlib
import re
import sys

fixture, body_path = sys.argv[1:3]
head = open(fixture).read()
body = open(body_path).read()
want = re.search(r"^# Body sha256: (\w+)$", head, re.M).group(1)
if hashlib.sha256(body.encode()).hexdigest() != want:
    sys.exit("FIXTURE_HEADER_SHA_MISMATCH")
scen = int(re.search(r"^# Scenarios: (\d+)", head, re.M).group(1))
if sum(int(n) for n in re.findall(r"^SCENARIOS (\d+) ", body, re.M)) != scen:
    sys.exit("FIXTURE_HEADER_SCENARIO_COUNT_MISMATCH")
PY

run_all b "$work/new.txt" "$work"

# What the harness compiled must be what the file says: re-extract and compare every copy.
mkdir -p "$work/fresh"
extract_fresh() { python3 "$work/extract.py" "$1" "$2" "$work/fresh"; }
extract_fresh cur "$src_root/state/State_Sleep.cpp"
for f in "$work"/fresh/*.inc; do
  cmp -s "$f" "$work/${f:t}" || { echo "COPY_MISMATCH: ${f:t} compiled by the harness differs from the file"; exit 1; }
done

diff -u "$work/expected.txt" "$work/new.txt" > "$work/new.diff" || { head -60 "$work/new.diff"; echo "EQUIVALENCE_FAILED: State_Sleep.cpp differs from the frozen transcript"; exit 1; }

# The transcript is not trivial: every branch the moved code has shows up, and so do all outcomes.
for line in "HIB eligible?" "HIB armForSleep" "HIB abandon" "EXTWDT stop" "EXTWDT start" \
            "SLEEP mode=HIBERNATE gpio=[7F,4F]" "SLEEP mode=ULP gpio=[4F,9R] net=3:0 dur=3600000" \
            "SLEEP mode=ULP gpio=[4F,9R] net=none dur=900000" "SLEEP mode=STOP gpio=[4F,9R]" \
            "SLEEP mode=STOP gpio=[] net=none dur=3600000" "SLEEP mode=ULP gpio=[4F] net=3:0" \
            "TEARDOWN req=1 eff=0 force=1 err=-170" "TEARDOWN req=1 eff=1 force=0 err=0" "RESET cause=5" \
            "TRANSITION 1 sleep-all-attempts-failed" "ALERT 16" "CALL Serial.begin 9600" "CALL attachInterrupt 4" \
            "WATCHDOG pause ulp" "WATCHDOG restore hibernate" "CRUMB 24" "CRUMB 28" "CRUMB 4" "PROGRESS WAKE_FROM_SLEEP" \
            "PROGRESS SERIAL_WAIT" "POWER flushDiagBatch" "CYCLE resetOnWake" "MEASURE noteWakeFromLowPowerSleep" \
            "LOG INFO SLEEP: standby sleep returned normally; preserving modem standby" \
            "LOG WARN SLEEP: standby sleep failed; forcing modem-off reason=sleep_error err=-170" \
            "LOG WARN HIBERNATE bypass: wake pin held low" "LOG WARN HIBERNATE sleep failed err=-170" \
            "LOG ERROR ULTRA_LOW_POWER sleep failed err=-170" "LOG ERROR STOP sleep fallback failed" \
            "LOG ERROR All sleep attempts failed err=-170" "LOG WARN Serial did not reconnect within 30s" \
            "LOG INFO SleepCall: standby=1" "LOG WARN SleepCall: watchdog-stop failed" \
            "RET fell=1 pir=1 btn=0 timer=0 label=PIR" "RET fell=1 pir=0 btn=1 timer=0 label=BUTTON" \
            "RET fell=1 pir=0 btn=0 timer=1 label=TIMER" "RET fell=0" \
            "FIELDS eff=1 preserve=1 force=0 err=0" "FIELDS eff=0 preserve=0 force=1 err=-170" \
            "FIELDS eff=0 preserve=0 force=0 err=0" "... x"; do
  grep -qF -- "$line" "$work/new.txt" || { echo "LOG_LINE_NEVER_EXERCISED: $line"; exit 1; }
done

mutate_and_expect_diff() {
  # $1 = label, then old/new pairs applied to a private copy of State_Sleep.cpp
  local m="$work/mut_${1// /_}"; shift
  local label="${m:t}"
  mkdir -p "$m"
  cp "$src_root/state/State_Sleep.cpp" "$m/State_Sleep.cpp"
  python3 - "$m/State_Sleep.cpp" "$@" <<'PY'
import sys

path, pairs = sys.argv[1], sys.argv[2:]
text = open(path).read()
for old, new in zip(pairs[0::2], pairs[1::2]):
    if text.count(old) != 1:
        sys.exit(f"MUTATION_NOT_APPLIED: {old!r} x{text.count(old)}")
    text = text.replace(old, new)
open(path, "w").write(text)
PY
  python3 "$work/extract.py" cur "$m/State_Sleep.cpp" "$m"
  run_all b "$m/new.txt" "$m" > "$m/build.log" 2>&1 || { echo "MUTANT_DID_NOT_BUILD: $label"; cat "$m/build.log"; exit 1; }
  if diff -q "$work/expected.txt" "$m/new.txt" > /dev/null; then echo "MUTANT_SURVIVED: $label"; exit 1; fi
  echo "caught: $label"
}

mutate_and_expect_diff "ULP error skips STOP with GPIO" \
  "    result = System.sleep(stopConfig);
" ""
mutate_and_expect_diff "standby error leaves forceModemOff unset" \
  "standbyReturn.forceModemOff = true;" "standbyReturn.forceModemOff = false;"
mutate_and_expect_diff "returned hibernate not abandoned" \
  "        HibernateCycle::abandon();
" ""
mutate_and_expect_diff "WAKE_FROM_SLEEP progress after the serial re-wait" \
  "  thrashGuard.markProgress(\"WAKE_FROM_SLEEP\");
" "" \
  "  Log.info(\"WakeReturn: elapsed=%lu reason=%s\"," "  thrashGuard.markProgress(\"WAKE_FROM_SLEEP\");
  Log.info(\"WakeReturn: elapsed=%lu reason=%s\","
mutate_and_expect_diff "standby preserved through an unstable modem" \
  "(!useNetworkStandbyRequested || session.modemUnstable || Cellular.isOff())" "(!useNetworkStandbyRequested || Cellular.isOff())"
mutate_and_expect_diff "PIR classified from the wrong edge" \
  "bool pirWake = sensorCanWake && (wakePin == sensorWake.pin);" "bool pirWake = sensorCanWake && (wakePin != sensorWake.pin);"
mutate_and_expect_diff "post-wake routing handed swapped pir and button" \
  "postWakeRouting(wake.pirWake, wake.buttonWake, wake.timerWake, wake.wakeReasonLabel);" "postWakeRouting(wake.buttonWake, wake.pirWake, wake.timerWake, wake.wakeReasonLabel);"

echo "sleep_execution_equivalence_test: both transcripts match the re-frozen S2 output, all mutations detected"
```

</details>


---

<sub>3m 24s</sub>

### `bash`

**Diff equivalence cpp**

```
$ cd build-tmp/wo-2026-11-05-004-stage6-r3; python3 -c "
import difflib
print(''.join(difflib.unified_diff(open('base_eq.cpp').readlines(),open('../../tests/sleep_execution_equivalence_test.cpp').readlines(),n=1)))"
```

<details>
<summary>122 lines</summary>

```diff
--- 
+++ 
@@ -1,8 +1,7 @@
-// WO-2026-10-21-001 (Sleep S-2): equivalence of the sleep-execution refactor.
-// (a) is the base handler's standby decision, hibernate block and ULP/STOP cascade, extracted
-// by marker text from the base State_Sleep.cpp (wrapped below as base::), (b) is the current
+// WO-2026-10-21-001 (Sleep S-2), re-frozen for v50 S2 (WO-2026-11-05-004): the current
 // effectiveStandby / hibernateAttempt / sleepAndResume and the handler's call-site lines,
-// extracted by marker text from the current file (cur::). Both run one scenario table against
-// the same fakes (tests/stubs/sleep_teardown/SleepExecutionFakes.h); mode "a" writes the frozen
-// fixture, mode "b" must equal it. Build with -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1
+// extracted by marker text from the current file (cur::), run one scenario table against
+// the fakes (tests/stubs/sleep_teardown/SleepExecutionFakes.h); the transcript must equal the
+// frozen fixture. S2 changed the hibernate block (native duration alarm, ExternalWatchdog)
+// on purpose, so the pre-change base blocks are no longer compared. Build with -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1
 // (cellular) or without (Wi-Fi), plus -DENABLE_SLEEP_TRACE=1 to compile the trace branch.
@@ -20,26 +19,2 @@
 void RecoveryState::raiseAlert(int8_t value) { sim::emit("ALERT %d", (int)value); }
-
-namespace base {
-namespace {
-#include BASE_DRAIN_INC
-}
-static bool preserveStandbyAfterNormalReturn = false, forceModemOffAfterStandbyFailure = false;
-static int standbySleepFailureError = SYSTEM_ERROR_NONE;
-struct WakeContext { bool pirWake, buttonWake, timerWake; const char *wakeReasonLabel; };
-static bool fell = false;
-
-static bool effectiveStandby(bool useNetworkStandbyRequested) {
-#include BASE_STANDBY_INC
-  return useNetworkStandbyEffective;
-}
-static void hibernateAttempt(bool overnightFallbackSleep, int wakeInSeconds) {
-#include BASE_HIB_INC
-}
-static void cascade(int wakeInSeconds, bool useNetworkStandby, const char *sleepReason, WakeContext &wake) {
-  fell = false;
-#include BASE_ULP_INC
-  wake = {pirWake, buttonWake, timerWake, wakeReasonLabel};
-  fell = true;
-}
-}  // namespace base
 
@@ -63,15 +38,2 @@
 
-static void driveBase(bool req, bool overnight, int wakeInSeconds, const char *reason, Outcome &o) {
-  const bool eff = base::effectiveStandby(req);
-  const bool useNetworkStandby = eff;
-  if (SleepTeardown::run(req, eff, base::forceModemOffAfterStandbyFailure, base::standbySleepFailureError) !=
-      SleepTeardown::Result::READY) { return; }
-  base::hibernateAttempt(overnight, wakeInSeconds);
-  base::WakeContext w{};
-  base::cascade(wakeInSeconds, useNetworkStandby, reason, w);
-  o = {base::fell, w.pirWake, w.buttonWake, w.timerWake, w.wakeReasonLabel, eff,
-       base::preserveStandbyAfterNormalReturn, base::forceModemOffAfterStandbyFailure,
-       base::standbySleepFailureError};
-}
-
 static void driveCur(bool useNetworkStandbyRequested, bool overnightFallbackSleep, int wakeInSeconds,
@@ -92,3 +54,3 @@
   bool req = true, connectOk = true, cellOff = false, unstable = false, tdClears = false, tdWait = false;
-  int hib = 0;       // 0 not overnight, 1 not eligible, 2 pin low, 3 stale clear fails, 4 alarm fails, 5/6 returns ok/err
+  int hib = 0;       // 0 not overnight, 1 not eligible, 2 pin low, 3/4 sleep returns ok/err
   int ulp = 0;       // 0 timer, 1 button, 2 sensor pin, 3 err then STOP ok, 4 err, err, timer-only ok, 5 all fail
@@ -141,3 +103,3 @@
   const int hib = first ? s.hib : 0, ulp = first ? s.ulp : 0;
-  if (hib >= 5) x.sleeps.push_back({hib == 5 ? 0 : -170, PIN_INVALID});
+  if (hib >= 3) x.sleeps.push_back({hib == 3 ? 0 : -170, PIN_INVALID});
   const pin_t sensorPin = x.sensorPin;
@@ -154,7 +116,5 @@
   x.hibPinHigh = hib != 2;
-  x.staleClearOk = hib != 3;
-  x.alarmOk = hib != 4;
-}
-
-static void runScenario(const Sc &s, bool useBase, int index) {
+}
+
+static void runScenario(const Sc &s, int index) {
   sim::world = sim::World();
@@ -177,5 +137,2 @@
   const bool preserve = s.prior == 1, force = s.prior == 2;
-  base::preserveStandbyAfterNormalReturn = preserve;
-  base::forceModemOffAfterStandbyFailure = force;
-  base::standbySleepFailureError = force ? -170 : SYSTEM_ERROR_NONE;
   cur::standbyReturn.preserveAfterNormalReturn = preserve;
@@ -196,12 +153,8 @@
     o.fell = false;
-    if (useBase) {
-      driveBase(s.req, overnight, wakeIn, "scheduled", o);
-    } else {
-      driveCur(s.req, overnight, wakeIn, "scheduled", o);
-    }
+    driveCur(s.req, overnight, wakeIn, "scheduled", o);
     if (!o.fell) {
       o.eff = false;
-      o.preserve = useBase ? base::preserveStandbyAfterNormalReturn : cur::standbyReturn.preserveAfterNormalReturn;
-      o.force = useBase ? base::forceModemOffAfterStandbyFailure : cur::standbyReturn.forceModemOff;
-      o.err = useBase ? base::standbySleepFailureError : cur::standbyReturn.error;
+      o.preserve = cur::standbyReturn.preserveAfterNormalReturn;
+      o.force = cur::standbyReturn.forceModemOff;
+      o.err = cur::standbyReturn.error;
     }
@@ -218,4 +171,3 @@
 
-int main(int argc, char **argv) {
-  const bool useBase = argc > 1 && strcmp(argv[1], "a") == 0;
+int main() {
   const bool cellular = HAL_PLATFORM_CELLULAR != 0;
@@ -224,3 +176,3 @@
   std::vector<Sc> table;
-  for (int hib = 1; hib <= 6; hib++)  // hibernate edges (hib=0 is the ULP table's first row)
+  for (int hib = 1; hib <= 4; hib++)  // hibernate edges (hib=0 is the ULP table's first row)
     for (int prior = 0; prior < priors; prior++)
@@ -244,3 +196,3 @@
     for (int ulp : {0, 3}) { Sc s; s.verbose = true; s.prior = 1; s.ulp = ulp; table.push_back(s); }
-    { Sc s; s.wdtOk = false; s.hib = 5; table.push_back(s); }
+    { Sc s; s.wdtOk = false; s.hib = 4; table.push_back(s); }
     { Sc s; s.wdtOk = false; s.ulp = 4; table.push_back(s); }
@@ -253,3 +205,3 @@
   int index = 0;
-  for (const Sc &s : table) runScenario(s, useBase, index++);
+  for (const Sc &s : table) runScenario(s, index++);
   printf("SCENARIOS %d PASSES %d WOKE %d RETURNED %d DISTINCT_EVENTS %d\n", scenarios, totalPasses, nWoke, nReset,
<shellId: 32 completed with exit code 0>
```

</details>


---

<sub>3m 38s</sub>

### `bash`

**Restore base cpp; inspect fixtures**

`$ cd build-tmp/wo-2026-11-05-004-stage6-r3; cp ../../tests/sleep_execution_equivalence_test.cpp cur_eq.cpp; cp base_eq.cpp ../../tests/sleep_execution_equivalence_test.cpp; cp base_eq.sh cur_eq_old_base.sh; cd ../..; git diff --stat -- tests/sleep_execution_equivalence_test.cpp | tail -2; grep -n "HIBERNATE" tests/fixtures/sleep_execution_transcript.txt | head -3; git show 3b5cfb8:tests/fixtures/sleep_execution_transcript.txt | grep -n "HIBERNATE" | head -3; git show 3b5cfb8:tests/fixtures/sleep_execution_transcript.txt | head -8; grep -n "HIBERNATE" tests/stubs/sleep_teardown/*.h`

<details>
<summary>19 lines</summary>

```
tests/sleep_execution_equivalence_test.cpp | 80 ++++++++++++++++++++++++------
 1 file changed, 64 insertions(+), 16 deletions(-)
5:# Generated with --regenerate from the S2 State_Sleep.cpp (native HIBERNATE duration alarm, ExternalWatchdog) against tests/stubs/sleep_teardown.
204:    LOG WARN HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER
238:    LOG WARN HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER
206:    LOG WARN HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER
240:    LOG WARN HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER
274:    LOG WARN HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER
# Frozen output of the pre-change sleep execution (standby decision, hibernate attempt, ULP/STOP cascade): WO-2026-10-21-001 (Sleep S-2).
# Base commit: 1e6d34f
# Scenarios: 137 (cellular 113 + wifi 24); passes: 157
# Per-platform summary (scenarios passes WOKE RETURNED distinct-events): cellular 113 133 123 10 74; wifi 24 24 22 2 58
# Body sha256: 14bcf926ae30bcda8d8ae72fdff681727f09778b886ba93a0d84d0b9ac4134b3
# Generated once by extracting the blocks by marker text from 1e6d34f:src/state/State_Sleep.cpp (git show),
# wrapping them in tests/sleep_execution_equivalence_test.cpp against tests/stubs/sleep_teardown, and running mode a.
PLATFORM cellular
tests/stubs/sleep_teardown/SleepExecutionFakes.h:39:enum class SystemSleepMode { ULTRA_LOW_POWER, STOP, HIBERNATE };
tests/stubs/sleep_teardown/SleepExecutionFakes.h:88:    static const char *names[] = {"ULP", "STOP", "HIBERNATE"};
<shellId: 33 completed with exit code 0>
```

</details>


---

<sub>3m 47s</sub>

### `bash`

**Look at emit and distinct logic**

`$ grep -n "distinct\|DISTINCT\|emit" tests/sleep_execution_equivalence_test.cpp | head -20; grep -n "inline void emit" -B3 -A12 tests/stubs/sleep_teardown/SleepExecutionFakes.h; grep -n "HIBERNATE\|mode=" tests/stubs/sleep_teardown/SleepExecutionFakes.h | head; sed -n 80,100p tests/stubs/sleep_teardown/SleepExecutionFakes.h`

<details>
<summary>31 lines</summary>

```
20:void RecoveryState::raiseAlert(int8_t value) { sim::emit("ALERT %d", (int)value); }
99:static std::set<std::string> distinctEvents;
132:  for (const auto &e : ev) distinctEvents.insert(normalize(e));
255:  printf("SCENARIOS %d PASSES %d WOKE %d RETURNED %d DISTINCT_EVENTS %d\n", scenarios, totalPasses, nWoke, nReset,
256:         (int)distinctEvents.size());
257:  for (const auto &e : distinctEvents) printf("EVENT %s\n", e.c_str());
39:enum class SystemSleepMode { ULTRA_LOW_POWER, STOP, HIBERNATE };
88:    static const char *names[] = {"ULP", "STOP", "HIBERNATE"};
89:    return std::string("mode=") + (m < 0 ? "none" : names[m]) + " gpio=[" + gpios + "] net=" +
  SystemSleepConfiguration &mode(SystemSleepMode x) { m = (int)x; return *this; }
  SystemSleepConfiguration &gpio(pin_t p, InterruptMode e) {
    gpios += (gpios.empty() ? "" : ",") + std::to_string(p) + (e == FALLING ? "F" : "R");
    return *this;
  }
  SystemSleepConfiguration &network(int i, SystemSleepNetworkFlag f) { netIf = i; netFlag = (int)f; return *this; }
  SystemSleepConfiguration &duration(uint32_t ms) { dur = (long)ms; return *this; }
  std::string str() const {
    static const char *names[] = {"ULP", "STOP", "HIBERNATE"};
    return std::string("mode=") + (m < 0 ? "none" : names[m]) + " gpio=[" + gpios + "] net=" +
           (netIf < 0 ? std::string("none") : std::to_string(netIf) + ":" + std::to_string(netFlag)) + " dur=" +
           (dur < 0 ? std::string("none") : std::to_string(dur));
  }
};
struct SystemSleepResult {
  int e = 0;
  pin_t pin = PIN_INVALID;
  int error() const { return e; }
  pin_t wakeupPin() const { return pin; }
};
struct SystemStub {
<shellId: 34 completed with exit code 0>
```

</details>


---

<sub>3m 58s</sub>

### `bash`

**Inspect transcript format**

`$ sed -n 99,135p tests/sleep_execution_equivalence_test.cpp; grep -n "^EVENT.*\(HIBERNATE\|AB1805\|clearStale\|stopWDT\|interruptAt\|bypass\)" tests/fixtures/sleep_execution_transcript.txt | head; git show 3b5cfb8:tests/fixtures/sleep_execution_transcript.txt | grep -n "^EVENT.*\(HIBERNATE\|AB1805\|clearStale\|bypass\|EXTWDT\)"; git show 3b5cfb8:tests/fixtures/sleep_execution_transcript.txt | sed -n 8,40p`

<details>
<summary>97 lines</summary>

```
static std::set<std::string> distinctEvents;
static int totalPasses = 0, nWoke = 0, nReset = 0, scenarios = 0;

static std::string normalize(const std::string &line) {
  std::string out;
  for (char c : line) {
    if (c >= '0' && c <= '9') {
      if (out.empty() || out.back() != '#') out += '#';
    } else {
      out += c;
    }
  }
  return out;
}

// A run of 3+ identical blocks of up to 4 lines (the serial re-wait loop) prints once, then "... xN".
static void printEvents() {
  const auto &ev = sim::events;
  size_t i = 0;
  while (i < ev.size()) {
    bool done = false;
    for (size_t p = 1; p <= 4 && !done; p++) {
      size_t reps = 1;
      while (i + (reps + 1) * p <= ev.size() && std::equal(ev.begin() + i, ev.begin() + i + p, ev.begin() + i + reps * p)) reps++;
      if (reps >= 3) {
        for (size_t k = 0; k < p; k++) printf("    %s\n", ev[i + k].c_str());
        printf("    ... x%zu\n", reps);
        i += reps * p;
        done = true;
      }
    }
    if (!done) printf("    %s\n", ev[i++].c_str());
  }
  for (const auto &e : ev) distinctEvents.insert(normalize(e));
}

static void script(const Sc &s, bool first) {
4337:EVENT LOG INFO Sleep: HIBERNATE reason=closed dur=#s wakePin=#
4345:EVENT LOG WARN HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER
4346:EVENT LOG WARN HIBERNATE sleep failed err=# - falling back to ULTRA_LOW_POWER
4347:EVENT LOG WARN HIBERNATE sleep failed err=-# - falling back to ULTRA_LOW_POWER
4350:EVENT LOG WARN SleepCall: watchdog-stop failed mode=HIBERNATE dur=#s
4363:EVENT SLEEP mode=HIBERNATE gpio=[#F,#F] net=none dur=# -> err=# pin=#
4364:EVENT SLEEP mode=HIBERNATE gpio=[#F,#F] net=none dur=# -> err=-# pin=#
5173:EVENT LOG INFO Sleep: HIBERNATE reason=closed dur=#s wakePin=#
5179:EVENT LOG WARN HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER
5180:EVENT LOG WARN HIBERNATE sleep failed err=# - falling back to ULTRA_LOW_POWER
4783:EVENT HIB clearStale
4793:EVENT LOG INFO Sleep: HIBERNATE reason=closed dur=#s wakePin=#
4801:EVENT LOG WARN HIBERNATE bypass: failed stale interrupt cleanup - falling back to ULTRA_LOW_POWER
4802:EVENT LOG WARN HIBERNATE bypass: failed to program AB# alarm - falling back to ULTRA_LOW_POWER
4803:EVENT LOG WARN HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER
4804:EVENT LOG WARN HIBERNATE sleep failed err=# - falling back to ULTRA_LOW_POWER
4805:EVENT LOG WARN HIBERNATE sleep failed err=-# - falling back to ULTRA_LOW_POWER
4808:EVENT LOG WARN SleepCall: watchdog-stop failed mode=HIBERNATE dur=#s
4821:EVENT SLEEP mode=HIBERNATE gpio=[#F,#F] net=none dur=none -> err=# pin=#
4822:EVENT SLEEP mode=HIBERNATE gpio=[#F,#F] net=none dur=none -> err=-# pin=#
5767:EVENT HIB clearStale
5775:EVENT LOG INFO Sleep: HIBERNATE reason=closed dur=#s wakePin=#
5781:EVENT LOG WARN HIBERNATE bypass: failed stale interrupt cleanup - falling back to ULTRA_LOW_POWER
5782:EVENT LOG WARN HIBERNATE bypass: failed to program AB# alarm - falling back to ULTRA_LOW_POWER
5783:EVENT LOG WARN HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER
5784:EVENT LOG WARN HIBERNATE sleep failed err=# - falling back to ULTRA_LOW_POWER
5785:EVENT LOG WARN HIBERNATE sleep failed err=-# - falling back to ULTRA_LOW_POWER
5797:EVENT SLEEP mode=HIBERNATE gpio=[#F,#F] net=none dur=none -> err=# pin=#
5798:EVENT SLEEP mode=HIBERNATE gpio=[#F,#F] net=none dur=none -> err=-# pin=#
PLATFORM cellular
S0 prior=0 req=0 conn=1 off=0 unst=0 td=0 hib=1 ulp=0 sv=1 ser=0/0/0 vb=0 wd=1
  p0 wakeIn=900 overnight=1 t=901000
    TEARDOWN req=0 eff=0 force=0 err=0
    HIB eligible? wakeIn=900 -> 0
    AB1805 stopWDT
    WATCHDOG pause ulp
    LOG INFO SleepCall: standby=0 dur=900 wdt=off gate=ok
    LOG INFO Sleep: ULP standby=0 reason=scheduled dur=900s occ=0 soc=87.5
    PROGRESS SLEEP_ATTEMPT
    POWER pre-sleep-ulp
    CLOUD logLedgerSleepState
    CRUMB 25
    CRUMB 26
    CRUMB 28
    SLEEP mode=ULP gpio=[4F,9R] net=none dur=900000 -> err=0 pin=65535
    AWAKE recordSleepReturn 1
    CALL pinMode 4 0
    CALL pinMode 9 0
    AB1805 resumeWDT
    WATCHDOG restore ulp
    PROGRESS WAKE_FROM_SLEEP
    CRUMB 4
    POWER flushDiagBatch
    LOG INFO WakeReturn: elapsed=900000 reason=TIMER
    CALL Watchdog.started
    LOG INFO AppWDT: active ctx=ulp armed=1
    CALL attachInterrupt 4 2
    CYCLE resetOnWake ms=901000
    MEASURE noteWakeFromLowPowerSleep
    POWER post-wake
    RET fell=1 pir=0 btn=0 timer=1 label=TIMER transitions=0
    FIELDS eff=0 preserve=0 force=0 err=0
<shellId: 35 completed with exit code 0>
```

</details>


---

<sub>4m 14s</sub>

### `bash`

**Edit cpp and fakes**

```
$ python3 - <<'E'
def sub(p,a,b,c=1):
    s=open(p).read(); assert s.count(a)==c,(p,a,s.count(a)); open(p,'w').write(s.replace(a,b))
p='tests/sleep_execution_equivalence_test.cpp'
sub(p,"// extracted by marker text from the current file (cur::). Both run one scenario table against\n// the same fakes (tests/stubs/sleep_teardown/SleepExecutionFakes.h); mode \"a\" writes the frozen\n// fixture, mode \"b\" must equal it.","// extracted by marker text from the current file (cur::). Both run one scenario table against\n// the same fakes (tests/stubs/sleep_teardown/SleepExecutionFakes.h); mode \"a\" (the base) writes the frozen\n// fixture once its transcript has the approved v50 S2 substitutions normalised (tests/s2_transcript_normalise.py), mode \"b\" must equal it.")
sub(p,"0 not overnight, 1 not eligible, 2 pin low, 3 stale clear fails, 4 alarm fails, 5/6 returns ok/err","0 not overnight, 1 not eligible, 2 pin low, 3/4 sleep returns ok/err")
sub(p,"if (hib >= 5) x.sleeps.push_back({hib == 5 ? 0 : -170, PIN_INVALID});","if (hib >= 3) x.sleeps.push_back({hib == 3 ? 0 : -170, PIN_INVALID});")
sub(p,"  x.staleClearOk = hib != 3;\n  x.alarmOk = hib != 4;\n","")
sub(p,"for (int hib = 1; hib <= 6; hib++)","for (int hib = 1; hib <= 4; hib++)")
sub(p,"s.wdtOk = false; s.hib = 5;","s.wdtOk = false; s.hib = 4;")
f='tests/stubs/sleep_teardown/SleepExecutionFakes.h'
sub(f,"hibPinHigh = true, wdtStopOk = true;","hibPinHigh = true, staleClearOk = true, alarmOk = true, wdtStopOk = true; // staleClearOk/alarmOk serve only the base blocks")
sub(f,"struct ExternalWatchdogStub {","""struct AB1805Stub {
  bool interruptAtTime(time_t t) { sim::emit("AB1805 interruptAtTime %lld", (long long)t); return sim::exec.alarmOk; }
  bool stopWDT() { sim::emit("AB1805 stopWDT"); return sim::exec.wdtStopOk; }
  bool resumeWDT() { sim::emit("AB1805 resumeWDT"); return true; }
};
inline AB1805Stub ab1805;
struct ExternalWatchdogStub {""")
sub(f,"namespace HibernateCycle {\ninline void armForSleep","inline bool clearAb1805StaleAlarmInterrupts() { sim::emit(\"HIB clearStale\"); return sim::exec.staleClearOk; }\nnamespace HibernateCycle {\ninline void armForSleep")
E
grep -n "wdtOk\|wdtStopOk" tests/sleep_execution_equivalence_test.cpp | head
```

<details>
<summary>6 lines</summary>

```
95:  bool sensorValid = true, serialCfg = false, serialPre = false, verbose = false, wdtOk = true, second = false;
171:  sim::exec.wdtStopOk = s.wdtOk;
185:         s.serialPre, s.serialUpAfter, s.verbose, s.wdtOk);
243:    { Sc s; s.wdtOk = false; s.hib = 4; table.push_back(s); }
244:    { Sc s; s.wdtOk = false; s.ulp = 4; table.push_back(s); }
<shellId: 36 completed with exit code 0>
```

</details>


---

<sub>4m 25s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/s2\_transcript\_normalise.py.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/s2_transcript_normalise.py`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/s2_transcript_normalise.py with 3025 characters
```


---

<sub>4m 31s</sub>

### `bash`

**Fix event normalisation**

```
$ python3 - <<'E'
p='tests/s2_transcript_normalise.py'; s=open(p).read()
a=s.index("    norm = sorted("); b=s.index("    if summary is None")
s=s[:a]+'''    norm = sorted({re.sub(r"\\d+", "#", e.replace("AB1805 stopWDT", "EXTWDT stop").replace("AB1805 resumeWDT", "EXTWDT start")
                          .replace("dur=none -> ", "dur=# -> "))
                   for e in events if not e[6:].startswith(DROP)})
'''+s[b:]
open(p,'w').write(s)
E
cd build-tmp/wo-2026-11-05-004-stage6-r3; echo ok
```

```
ok
<shellId: 37 completed with exit code 0>
```


---

<sub>4m 34s</sub>

### `bash`

**Restrict dur replace**

```
$ python3 - <<'E'
p='tests/s2_transcript_normalise.py'; s=open(p).read()
s=s.replace('''                          .replace("dur=none -> ", "dur=# -> "))''','''                          .replace("dur=none -> " if "mode=HIBERNATE" in e else "\\0", "dur=# -> "))''')
open(p,'w').write(s)
E
grep -n 'mode=HIBERNATE" in e' tests/s2_transcript_normalise.py
```

```
48:                          .replace("dur=none -> " if "mode=HIBERNATE" in e else "\0", "dur=# -> "))
<shellId: 38 completed with exit code 0>
```


---

<sub>4m 58s</sub>

### `bash`

**Rework equivalence script; regenerate**

```
$ python3 - <<'E'
p='tests/sleep_execution_equivalence_test.sh'; s=open(p).read()
def sub(a,b):
    global s
    assert s.count(a)==1,a; s=s.replace(a,b)
a=s.index("# WO-2026-10-21-001"); b=s.index("tests_dir=")
s=s[:a]+'''# WO-2026-10-21-001 (Sleep S-2), updated for v50 S2 (WO-2026-11-05-004): tests/sleep_execution_equivalence_test.cpp
# runs one scenario table through (a) the blocks extracted by marker text from the BASE State_Sleep.cpp (the commit
# named in the fixture header) and (b) the current functions and the handler's call-site lines, against the same
# fakes (tests/stubs/sleep_teardown/SleepExecutionFakes.h), for cellular and Wi-Fi. The frozen
# fixtures/sleep_execution_transcript.txt is the base transcript with only the approved S2 substitutions normalised
# by tests/s2_transcript_normalise.py (AB1805 alarm pre-arm and watchdog events, the duration on the HIBERNATE call).
# The current transcript must equal it byte for byte ('# ' header lines stripped first), and so must the normalised
# base transcript. The extracted copies the harness compiled are read back and compared with a fresh extraction:
# COPY_MISMATCH on drift. Without the base commit the base check is skipped and said. The retired scenarios (stale-clear
# failure, alarm-program failure) exist only in the base and are not in the table. Mutants of a private copy of
# State_Sleep.cpp must each change the transcript. SLEEP_EXECUTION_SRC_ROOT tests another src/ root;
# SLEEP_EXECUTION_TAMPER_COPY=1 demonstrates COPY_MISMATCH. '--regenerate' rewrites the fixture from the base blocks.

'''+s[b:]
sub('''work="$(mktemp -d "${TMPDIR:-/tmp}/sleep_execution_equivalence.XXXXXX")"
trap 'rm -rf "$work"' EXIT
''','''work="$(mktemp -d "${TMPDIR:-/tmp}/sleep_execution_equivalence.XXXXXX")"
trap 'rm -rf "$work"' EXIT

base="$(sed -n 's/^# Base commit: //p' "$fixture" 2>/dev/null || true)"
base="${base:-3b5cfb8}"
base_src="$work/base_State_Sleep.cpp"
have_base=0
if git -C "$repo_root" show "${base}:src/state/State_Sleep.cpp" > "$base_src" 2>/dev/null; then have_base=1; fi
''')
sub('''out = {"drain": span("void drainSerialBeforeSleep() {", "\\n}\\n")}
out["funcs"] = span("struct WakeContext {", "\\n} // namespace\\n", keep_end=False) + "\\n"
out["call_effective"] = line("  bool useNetworkStandbyEffective = effectiveStandby(")
out["call_teardown"] = line("  if (SleepTeardown::run(")
out["call_hibernate"] = line("  hibernateAttempt(")
out["call_sleep"] = span("  WakeContext wake;\\n", "  if (sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)) return;\\n")
out["call_route"] = line("  postWakeRouting(")
''','''out = {"drain": span("void drainSerialBeforeSleep() {", "\\n}\\n")}
if kind == "base":
    out["standby"] = span("  bool preserveStandbyForThisPass = false;\\n", "  useNetworkStandbyEffective = false;\\n#endif\\n")
    out["hib"] = span("#if PLATFORM_ID == PLATFORM_BORON\\n  if (overnightFallbackSleep) {\\n    time_t rtcNow", "\\n  }\\n#endif\\n")
    out["ulp"] = span("  const SensorWakeSource sensorWake = SensorManager", '  PowerDiagnostics::logPowerState("post-wake");\\n')
else:
    out["funcs"] = span("struct WakeContext {", "\\n} // namespace\\n", keep_end=False) + "\\n"
    out["call_effective"] = line("  bool useNetworkStandbyEffective = effectiveStandby(")
    out["call_teardown"] = line("  if (SleepTeardown::run(")
    out["call_hibernate"] = line("  hibernateAttempt(")
    out["call_sleep"] = span("  WakeContext wake;\\n", "  if (sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)) return;\\n")
    out["call_route"] = line("  postWakeRouting(")
''')
sub('extract cur "$src_root/state/State_Sleep.cpp"\nif [[','if (( have_base )); then extract base "$base_src"; fi\nextract cur "$src_root/state/State_Sleep.cpp"\nif [[')
sub('''    -DCUR_DRAIN_INC=''','''    -DBASE_DRAIN_INC="\\"$inc/base_drain.inc\\"" -DBASE_STANDBY_INC="\\"$inc/base_standby.inc\\"" \\
    -DBASE_HIB_INC="\\"$inc/base_hib.inc\\"" -DBASE_ULP_INC="\\"$inc/base_ulp.inc\\"" \\
    -DCUR_DRAIN_INC=''')
sub('''  run_all b "$work/regen.txt" "$work"
  python3 - "$work/regen.txt" "$fixture" <<'PY\'''','''  (( have_base )) || { echo "NO_BASE_COMMIT: $base"; exit 1; }
  run_all a "$work/regen_raw.txt" "$work"
  python3 "$tests_dir/s2_transcript_normalise.py" "$work/regen_raw.txt" > "$work/regen.txt"
  python3 - "$base" "$work/regen.txt" "$fixture" <<'PY\'''')
sub("body_path, fixture = sys.argv[1:3]\nbody = open(body_path).read()\ncounts","base, body_path, fixture = sys.argv[1:4]\nbody = open(body_path).read()\ncounts")
a=s.index('head = f"""'); b=s.index('"""\nopen(fixture')
s=s[:a]+'''head = f"""# Frozen output of the pre-change sleep execution (standby decision, hibernate attempt, ULP/STOP cascade): WO-2026-10-21-001 (Sleep S-2), with the approved v50 S2 substitutions normalised (tests/s2_transcript_normalise.py).
# Base commit: {base}
# Scenarios: {scen} (cellular {counts[0][0]} + wifi {counts[1][0]}); passes: {passes}
# Per-platform summary (scenarios passes WOKE RETURNED distinct-events): cellular {' '.join(counts[0])}; wifi {' '.join(counts[1])}
# Body sha256: {hashlib.sha256(body.encode()).hexdigest()}
# Generated with --regenerate: the blocks extracted by marker text from {base}:src/state/State_Sleep.cpp (git show) run in mode a against tests/stubs/sleep_teardown, then normalised.
'''+s[b:]
sub('''for f in "$work"/fresh/*.inc; do''','''(( have_base )) && extract_fresh base "$base_src"
for f in "$work"/fresh/*.inc; do''')
sub('''# The transcript is not trivial''','''if (( have_base )); then
  run_all a "$work/old_raw.txt" "$work"
  python3 "$tests_dir/s2_transcript_normalise.py" "$work/old_raw.txt" > "$work/old.txt"
  diff -u "$work/expected.txt" "$work/old.txt" > "$work/old.diff" || { head -60 "$work/old.diff"; echo "FIXTURE_NOT_FROM_BASE: the normalised base blocks differ from the fixture ($base)"; exit 1; }
else
  echo "BASE_CHECK_SKIPPED: commit $base is not in this tree; only the fixture comparison ran"
fi

# The transcript is not trivial''')
sub('''  python3 "$work/extract.py" cur "$m/State_Sleep.cpp" "$m"''','''  cp "$work"/base_*.inc "$m/" 2>/dev/null || true
  python3 "$work/extract.py" cur "$m/State_Sleep.cpp" "$m"''')
sub("both transcripts match the re-frozen S2 output, all mutations detected","current transcript and normalised base transcript both match the frozen fixture, all mutations detected")
open(p,'w').write(s)
E
zsh tests/sleep_execution_equivalence_test.sh --regenerate 2>&1 | tail -20
```

<details>
<summary>21 lines</summary>

```
^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//sleep_execution_equivalence.JAR5LO/base_ulp.inc:81:7: error: use of undeclared identifier 'standbyReturn'
      standbyReturn.error = (int)result.error();
      ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//sleep_execution_equivalence.JAR5LO/base_ulp.inc:83:16: error: use of undeclared identifier 'standbyReturn'
               standbyReturn.error);
               ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//sleep_execution_equivalence.JAR5LO/base_ulp.inc:86:5: error: use of undeclared identifier 'standbyReturn'
    standbyReturn.preserveAfterNormalReturn = false;
    ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//sleep_execution_equivalence.JAR5LO/base_ulp.inc:87:5: error: use of undeclared identifier 'standbyReturn'
    standbyReturn.forceModemOff = false;
    ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//sleep_execution_equivalence.JAR5LO/base_ulp.inc:88:5: error: use of undeclared identifier 'standbyReturn'
    standbyReturn.error = SYSTEM_ERROR_NONE;
    ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//sleep_execution_equivalence.JAR5LO/base_ulp.inc:154:9: error: void function 'cascade' should not return a value [-Wreturn-type]
        return true;
        ^      ~~~~
15 errors generated.
<shellId: 39 completed with exit code 0>
```

</details>


---

<sub>5m 5s</sub>

### `bash`

**Use original base commit 1e6d34f**

`$ git cat-file -t 1e6d34f && sed -i.bak 's/base:-3b5cfb8/base:-1e6d34f/' tests/sleep_execution_equivalence_test.sh && rm tests/sleep_execution_equivalence_test.sh.bak && zsh tests/sleep_execution_equivalence_test.sh --regenerate 2>&1 | tail -20`

```
commit
fixture written: /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/fixtures/sleep_execution_transcript.txt
<shellId: 40 completed with exit code 0>
```


---

<sub>5m 11s</sub>

### `bash`

**Run equivalence test**

`$ head -7 tests/fixtures/sleep_execution_transcript.txt; zsh tests/sleep_execution_equivalence_test.sh 2>&1 | tail -30`

<details>
<summary>38 lines</summary>

```
# Frozen output of the pre-change sleep execution (standby decision, hibernate attempt, ULP/STOP cascade): WO-2026-10-21-001 (Sleep S-2), with the approved v50 S2 substitutions normalised (tests/s2_transcript_normalise.py).
# Base commit: 1e6d34f
# Scenarios: 121 (cellular 101 + wifi 20); passes: 141
# Per-platform summary (scenarios passes WOKE RETURNED distinct-events): cellular 101 121 111 10 71; wifi 20 20 18 2 55
# Body sha256: e8d1815d9bd62c970201652e42207bd8e068e10086da0b8eecddeecedc0c37ff
# Generated with --regenerate: the blocks extracted by marker text from 1e6d34f:src/state/State_Sleep.cpp (git show) run in mode a against tests/stubs/sleep_teardown, then normalised.
PLATFORM cellular
-    CYCLE resetOnWake ms=3601007
+    CYCLE resetOnWake ms=7201000
     MEASURE noteWakeFromLowPowerSleep
     POWER post-wake
     RET fell=1 pir=0 btn=0 timer=1 label=TIMER transitions=0
     FIELDS eff=1 preserve=1 force=0 err=0
 S14 prior=1 req=0 conn=1 off=0 unst=0 td=0 hib=3 ulp=0 sv=1 ser=0/0/0 vb=0 wd=1
-  p0 wakeIn=3600 overnight=1 t=3601007
+  p0 wakeIn=3600 overnight=1 t=7201000
     TEARDOWN req=0 eff=0 force=0 err=0
     HIB eligible? wakeIn=3600 -> 1
     CALL pinMode 7 2
@@ -549,13 +549,13 @@
     CALL Watchdog.started
     LOG INFO AppWDT: active ctx=ulp armed=1
     CALL attachInterrupt 4 2
-    CYCLE resetOnWake ms=3601007
+    CYCLE resetOnWake ms=7201000
     MEASURE noteWakeFromLowPowerSleep
     POWER post-wake
     RET fell=1 pir=0 btn=0 timer=1 label=TIMER transitions=0
     FIELDS eff=0 preserve=0 force=0 err=0
 S15 prior=1 req=1 conn=1 off=0 unst=0 td=0 hib=3 ulp=0 sv=1 ser=0/0/0 vb=0 wd=1
-  p0 wakeIn=900 overnight=1 t=901007
+  p0 wakeIn=900 overnight=1 t=1801000
     TEARDOWN req=1 eff=1 force=0 err=0
     HIB eligible? wakeIn=900 -> 1
     CALL pinMode 7 2
@@ -602,13 +602,13 @@
EQUIVALENCE_FAILED: State_Sleep.cpp differs from the frozen transcript
<shellId: 41 completed with exit code 0>
```

</details>


---

<sub>5m 16s</sub>

### `bash`

**Find clock advance on sleep**

`$ grep -n "dur" tests/stubs/sleep_teardown/SleepExecutionFakes.h | head -20`

<details>
<summary>6 lines</summary>

```
78:  long dur = -1;
86:  SystemSleepConfiguration &duration(uint32_t ms) { dur = (long)ms; return *this; }
90:           (netIf < 0 ? std::string("none") : std::to_string(netIf) + ":" + std::to_string(netFlag)) + " dur=" +
91:           (dur < 0 ? std::string("none") : std::to_string(dur));
106:    sim::now += (s.err == 0 && s.pin == PIN_INVALID && c.dur > 0) ? (unsigned long)c.dur : 7UL;
<shellId: 42 completed with exit code 0>
```

</details>


---

<sub>5m 21s</sub>

### `bash`

**Fix clock model; regenerate and run**

```
$ python3 - <<'E'
p='tests/stubs/sleep_teardown/SleepExecutionFakes.h'; s=open(p).read()
a="sim::now += (s.err == 0 && s.pin == PIN_INVALID && c.dur > 0)"
assert a in s
s=s.replace(a,"sim::now += (s.err == 0 && s.pin == PIN_INVALID && c.dur > 0 && c.m != (int)SystemSleepMode::HIBERNATE) // a returned HIBERNATE is modelled as 7 ms whatever its duration")
open(p,'w').write(s)
E
sed -n 104,108p tests/stubs/sleep_teardown/SleepExecutionFakes.h; zsh tests/sleep_execution_equivalence_test.sh --regenerate | tail -2; zsh tests/sleep_execution_equivalence_test.sh 2>&1 | tail -30
```

<details>
<summary>18 lines</summary>

```
const sim::Exec::Sleep s = x.nextSleep < x.sleeps.size() ? x.sleeps[x.nextSleep++] : sim::Exec::Sleep{-1, PIN_INVALID};
    sim::emit("SLEEP %s -> err=%d pin=%u", c.str().c_str(), s.err, (unsigned)s.pin);
    sim::now += (s.err == 0 && s.pin == PIN_INVALID && c.dur > 0 && c.m != (int)SystemSleepMode::HIBERNATE) // a returned HIBERNATE is modelled as 7 ms whatever its duration ? (unsigned long)c.dur : 7UL;
    return SystemSleepResult{s.err, s.pin};
  }
In file included from /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/sleep_execution_equivalence_test.cpp:14:
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/stubs/sleep_teardown/SleepExecutionFakes.h:106:108: error: expected ';' after expression
    sim::now += (s.err == 0 && s.pin == PIN_INVALID && c.dur > 0 && c.m != (int)SystemSleepMode::HIBERNATE) // a returned HIBERNATE is modelled as 7 ms whatever its duration ? (unsigned long)c.dur : 7UL;
                                                                                                           ^
                                                                                                           ;
1 error generated.
In file included from /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/sleep_execution_equivalence_test.cpp:14:
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/stubs/sleep_teardown/SleepExecutionFakes.h:106:108: error: expected ';' after expression
    sim::now += (s.err == 0 && s.pin == PIN_INVALID && c.dur > 0 && c.m != (int)SystemSleepMode::HIBERNATE) // a returned HIBERNATE is modelled as 7 ms whatever its duration ? (unsigned long)c.dur : 7UL;
                                                                                                           ^
                                                                                                           ;
1 error generated.
<shellId: 43 completed with exit code 0>
```

</details>


---

<sub>5m 25s</sub>

### Copilot

Comment swallowed the rest; move it.

---

<sub>5m 31s</sub>

### `bash`

**Regenerate and run equivalence**

```
$ python3 - <<'E'
p='tests/stubs/sleep_teardown/SleepExecutionFakes.h'; s=open(p).read()
s=s.replace(") // a returned HIBERNATE is modelled as 7 ms whatever its duration ? (unsigned long)c.dur : 7UL;",") ? (unsigned long)c.dur : 7UL; // a returned HIBERNATE is modelled as 7 ms whatever its duration")
open(p,'w').write(s)
E
zsh tests/sleep_execution_equivalence_test.sh --regenerate | tail -1; zsh tests/sleep_execution_equivalence_test.sh 2>&1 | tail -30
```

<details>
<summary>32 lines</summary>

```
fixture written: /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/fixtures/sleep_execution_transcript.txt
 EVENT CLOUD logLedgerSleepState
 EVENT CRUMB #
 EVENT CYCLE resetOnWake ms=#
+EVENT EXTWDT start
+EVENT EXTWDT stop
 EVENT HIB abandon
 EVENT HIB armForSleep # #
 EVENT HIB eligible? wakeIn=# -> #
@@ -5145,10 +5144,7 @@
     TRANSITION 1 sleep-all-attempts-failed
     RET fell=0 pir=0 btn=0 timer=0 label=- transitions=1
     FIELDS eff=0 preserve=0 force=0 err=0
-SCENARIOS 20 PASSES 20 WOKE 18 RETURNED 2 DISTINCT_EVENTS 55
-EVENT AB# interruptAtTime #
-EVENT AB# resumeWDT
-EVENT AB# stopWDT
+SCENARIOS 20 PASSES 20 WOKE 18 RETURNED 2 DISTINCT_EVENTS 54
 EVENT ALERT #
 EVENT AWAKE recordSleepReturn #
 EVENT CALL Watchdog.started
@@ -5158,6 +5154,8 @@
 EVENT CLOUD logLedgerSleepState
 EVENT CRUMB #
 EVENT CYCLE resetOnWake ms=#
+EVENT EXTWDT start
+EVENT EXTWDT stop
 EVENT HIB abandon
 EVENT HIB armForSleep # #
 EVENT HIB eligible? wakeIn=# -> #
EQUIVALENCE_FAILED: State_Sleep.cpp differs from the frozen transcript
<shellId: 44 completed with exit code 0>
```

</details>


---

<sub>6m 3s</sub>

### `bash`

**Fix event normalisation; rerun**

```
$ python3 - <<'E'
p='tests/s2_transcript_normalise.py'; s=open(p).read()
a=s.index("    norm = sorted("); b=s.index("    if summary is None")
s=s[:a]+'''    # EVENT lines have their digit runs collapsed to '#', so the substitutions are matched in that form.
    digits = lambda t: re.sub(r"\\d+", "#", t)
    drop = tuple(digits(d) for d in DROP)
    norm = set()
    for e in events:
        n = digits(e)
        if n[6:].startswith(drop):
            continue
        n = n.replace("AB# stopWDT", "EXTWDT stop").replace("AB# resumeWDT", "EXTWDT start")
        if "mode=HIBERNATE" in n:
            n = n.replace("dur=none -> ", "dur=# -> ")
        norm.add(n)
    norm = sorted(norm)
'''+s[b:]
open(p,'w').write(s)
E
zsh tests/sleep_execution_equivalence_test.sh --regenerate | tail -1; zsh tests/sleep_execution_equivalence_test.sh 2>&1 | tail -30
```

<details>
<summary>10 lines</summary>

```
fixture written: /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/fixtures/sleep_execution_transcript.txt
caught: mut_ULP_error_skips_STOP_with_GPIO
caught: mut_standby_error_leaves_forceModemOff_unset
caught: mut_returned_hibernate_not_abandoned
caught: mut_WAKE_FROM_SLEEP_progress_after_the_serial_re-wait
caught: mut_standby_preserved_through_an_unstable_modem
caught: mut_PIR_classified_from_the_wrong_edge
caught: mut_post-wake_routing_handed_swapped_pir_and_button
sleep_execution_equivalence_test: current transcript and normalised base transcript both match the frozen fixture, all mutations detected
<shellId: 45 completed with exit code 0>
```

</details>


---

<sub>6m 23s</sub>

### `bash`

**Update S2 structural test**

```
$ python3 - <<'E'
def sub(p,a,b):
    s=open(p).read(); assert s.count(a)==1,(p,a); open(p,'w').write(s.replace(a,b))
p='tests/s2_alarm_watchdog_wake_structural_test.py'
sub(p,'''    "err": read("state/State_Error.cpp"),''','''    "err": read("state/State_Error.cpp"),
    "fs": read("power/ConnectivityFailsafe.cpp"),''')
sub(p,'''    err = strip(files["err"])
''','''    err = strip(files["err"])
    fs = strip(files["fs"])
''')
sub(p,'''    for label in ("gcc", "sleep", "err", "hc"):
        t = strip(files[label])''','''    for label in ("gcc", "sleep", "err", "hc", "fs"):
        t = strip(files[label])''')
sub(p,'''    if not 0 <= block.find("isInferredWatchdog(") < block.find("set_powerOffRequested(0)"):
        p.append("the byte must be read before it is cleared")
    if gcc.count("set_powerOffRequested(0)") != 1:
        p.append("the byte must be cleared exactly once, at boot")''','''    if not 0 <= block.find("isInferredWatchdog(") < block.find("set_powerOffRequested(0)"):
        p.append("the byte must be read before it is cleared")
    if gcc.count("set_powerOffRequested(0)") != 1:
        p.append("the byte must be cleared exactly once, at boot")
    if not re.search(r"set_powerOffRequested\\(0\\);\\s*SystemConfig::flushNow\\(\\);", gcc):
        p.append("the boot clear must be flushed immediately (forced consumption)")
    if 'ab1805WakeReason\\\\":\\\\"N/A\\\\"' not in files["gcc"]:
        p.append("the status payload must keep ab1805WakeReason with the constant N/A")''')
sub(p,'''    order = [case3.find(s) for s in ("set_powerOffRequested(1)", "flushAllBeforeReset()", "ab1805.deepPowerDown()")]
    if min(order) < 0 or order != sorted(order):
        p.append("State_Error case 3 must write the byte, then flush, then deepPowerDown")''','''    order = [case3.find(s) for s in ("set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel)", "flushAllBeforeReset()", "ab1805.deepPowerDown()")]
    if min(order) < 0 or order != sorted(order):
        p.append("State_Error case 3 must write the sentinel, then flush, then deepPowerDown")
    if not re.search(r"deepPowerDown\\(\\);\\s*RecoveryState::set_powerOffRequested\\(0\\);\\s*SystemConfig::flushNow\\(\\);", case3):
        p.append("State_Error case 3 must clear the byte and flush after a returning deepPowerDown()")
    if not re.search(r"deepPowerDown\\(\\);\\s*RecoveryState::set_powerOffRequested\\(0\\);\\s*RecoveryState::set_connectivityRecoveryStage\\(2\\);\\s*SystemConfig::flushNow\\(\\);", fs):
        p.append("the failsafe caller must clear the byte, downgrade stage 3 and flush after a returning deepPowerDown()")''')
sub(p,'"RecoveryState::get_powerOffRequested() != 0", "retainedHibernatePending != 0"','"RecoveryState::get_powerOffRequested(),", "retainedHibernatePending,"')
sub(p,'''    "byte written after the call": mutated("err", "RecoveryState::set_powerOffRequested(1); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog\\n      flushAllBeforeReset();\\n      ab1805.deepPowerDown();",
                                           "flushAllBeforeReset();\\n      ab1805.deepPowerDown();\\n      RecoveryState::set_powerOffRequested(1);"),''','''    "byte written after the call": mutated("err", "RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog\\n      flushAllBeforeReset();\\n      ab1805.deepPowerDown();",
                                           "flushAllBeforeReset();\\n      ab1805.deepPowerDown();\\n      RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel);"),
    "boot clear not flushed": mutated("gcc", "  SystemConfig::flushNow(); // consume the evidence durably before anything can reload it\\n", ""),
    "no clear after a returning error-path power-down": mutated("err", "      RecoveryState::set_powerOffRequested(0); SystemConfig::flushNow();", ""),
    "failsafe stage 3 not downgraded on return": mutated("fs", "RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow();", ""),
    "ab1805WakeReason dropped from the payload": mutated("gcc", '\\\\"ab1805WakeReason\\\\":\\\\"N/A\\\\",', ""),''')
E
python3 tests/s2_alarm_watchdog_wake_structural_test.py
```

```
s2_alarm_watchdog_wake_structural_test: ok (14 probes detected)
<shellId: 46 completed with exit code 0>
```


---

<sub>6m 31s</sub>

### `bash`

**Add GPIO test and duration mutation**

```
$ python3 - <<'E'
def sub(p,a,b):
    s=open(p).read(); assert s.count(a)==1,(p,a); open(p,'w').write(s.replace(a,b))
p='tests/hibernate_wake_diagnostics_test.cpp'
sub(p,"void testOtherResetReasonsFailOnTheResetReasonArm() {","""// A button/GPIO wake from HIBERNATE also reports reason 30; 100 s into a 3600 s sleep it is indistinguishable in
// firmware from an alarm wake (recorded residual, Amendment 4). The event's actual/err fields carry the evidence.
void testGpioWakeIsAlsoReason30Success() {
  const GateInputs in = wakeInputs(true, -3500);
  assert(in.wakeReasonIsHibernateWake);
  assert(classifyGateArmMirror(in) == GateArm::kNone);
}

void testOtherResetReasonsFailOnTheResetReasonArm() {""")
sub(p,"  testOtherResetReasonsFailOnTheResetReasonArm();\n","  testGpioWakeIsAlsoReason30Success();\n  testOtherResetReasonsFailOnTheResetReasonArm();\n")
E
grep -n "const char \*wakeReasonName\|ALARM" tests/hibernate_wake_diagnostics_test.cpp | head -3
python3 - <<'E'
p='tests/sleep_execution_equivalence_test.sh'; s=open(p).read()
a='''mutate_and_expect_diff "standby preserved'''
assert a in s
s=s.replace(a,'''mutate_and_expect_diff "hibernate duration scaled wrong" \\
  ".duration((uint32_t)wakeInSeconds * 1000UL);" ".duration((uint32_t)wakeInSeconds * 100UL);"
'''+a,1)
open(p,'w').write(s)
E
```

```
119:  const EventFields f = buildEventFields(classifyGateArmMirror(in), in, /*osResetReason=*/30, "ALARM",
136:  const EventFields f = buildEventFields(classifyGateArmMirror(in), in, /*osResetReason=*/30, "ALARM",
244:  const EventFields f = buildEventFields(classifyGateArmMirror(in), in, /*osResetReason=*/30, "ALARM",
<shellId: 47 completed with exit code 0>
```


---

<sub>6m 34s</sub>

### `bash`

**Find ALARM label usage**

`$ grep -n "ALARM" tests/hibernate_wake_diagnostics_test.* | head -20`

```
tests/hibernate_wake_diagnostics_test.cpp:119:  const EventFields f = buildEventFields(classifyGateArmMirror(in), in, /*osResetReason=*/30, "ALARM",
tests/hibernate_wake_diagnostics_test.cpp:136:  const EventFields f = buildEventFields(classifyGateArmMirror(in), in, /*osResetReason=*/30, "ALARM",
tests/hibernate_wake_diagnostics_test.cpp:244:  const EventFields f = buildEventFields(classifyGateArmMirror(in), in, /*osResetReason=*/30, "ALARM",
tests/hibernate_wake_diagnostics_test.cpp:374:  const EventFields f = buildEventFields(classifyGateArmMirror(in), in, /*osResetReason=*/30, "ALARM",
<shellId: 48 completed with exit code 0>
```


---

<sub>6m 37s</sub>

### `bash` — Failed

**Run full host suite**

```
$ python3 - <<'E'
p='tests/hibernate_wake_diagnostics_test.cpp'; s=open(p).read()
s=s.replace('/*osResetReason=*/30, "ALARM",','/*osResetReason=*/30, "POWER_MGMT",'); open(p,'w').write(s)
E
grep -n '"wakeReason' tests/hibernate_wake_diagnostics_test.cpp | head -5
p=0; f=0; for t in tests/*_test.sh; do zsh $t >/tmp/o.txt 2>&1 && p=$((p+1)) || { f=$((f+1)); echo FAIL $t; tail -5 /tmp/o.txt; }; done; for t in tests/*_test.py; do python3 $t >/tmp/o.txt 2>&1 && p=$((p+1)) || { f=$((f+1)); echo FAIL $t; tail -5 /tmp/o.txt; }; done; echo pass=$p fail=$f
```

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>15m 53s</sub>

### `bash`

**Run full host suite**

`$ o=build-tmp/wo-2026-11-05-004-stage6-r3/o.txt; p=0; f=0; for t in tests/*_test.sh; do zsh $t >$o 2>&1 && p=$((p+1)) || { f=$((f+1)); echo FAIL $t; tail -5 $o; }; done; for t in tests/*_test.py; do python3 $t >$o 2>&1 && p=$((p+1)) || { f=$((f+1)); echo FAIL $t; tail -5 $o; }; done; echo pass=$p fail=$f`

<details>
<summary>25 lines</summary>

```
FAIL tests/connectivity_failsafe_unknown_openness_test.sh
  ~~~~~~~~~~~~~~~^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//failsafe_unknown_openness_60396/body.h:145:59: error: no member named 'set_connectivityRecoveryStage' in namespace 'RecoveryState'
  RecoveryState::set_powerOffRequested(0); RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow(); // not expected to return: no recent stage-3 evidence
                                           ~~~~~~~~~~~~~~~^
2 errors generated.
FAIL tests/sleep_execution_equivalence_test.sh
caught: mut_ULP_error_skips_STOP_with_GPIO
caught: mut_standby_error_leaves_forceModemOff_unset
caught: mut_returned_hibernate_not_abandoned
caught: mut_WAKE_FROM_SLEEP_progress_after_the_serial_re-wait
MUTATION_NOT_APPLIED: '.duration((uint32_t)wakeInSeconds * 1000UL);' x4
FAIL tests/deliberate_reset_flush_test.py
PASS: mutation caught: call removed at failsafe stage 2
Traceback (most recent call last):
  File "/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/deliberate_reset_flush_test.py", line 161, in <module>
    at = t.index(term)
ValueError: substring not found
FAIL tests/sleep_execution_structural_test.py

PASS: the three standby-return fields are declared once, in one anonymous-namespace struct
PASS: only effectiveStandby(), sleepAndResume() and the teardown call arguments mention standbyReturn
PASS: all 5 SystemSleepConfiguration objects are fresh locals
PASS: SleepTeardown.{h,cpp} unchanged, no new file under src/, nothing retained added (base 1e6d34f)
pass=114 fail=4
<shellId: 50 completed with exit code 0>
```

</details>


---

<sub>16m 0s</sub>

### `bash`

**Diagnose failures**

`$ sed -n 140,175p tests/deliberate_reset_flush_test.py; zsh tests/connectivity_failsafe_unknown_openness_test.sh 2>&1 | head -12; python3 tests/sleep_execution_structural_test.py 2>&1 | grep -v "^PASS" | head`

<details>
<summary>51 lines</summary>

```diff
"N": "SystemConfig::SensorSettings::flushNow();", "Q": "PublishQueuePosix::instance().writeQueueToFiles();"}
    for k, line in flush_lines.items():
        mutant = body.replace("  " + line + "\n", "", 1)
        check(mutant != body and not flush_run(mutant, tmp), f"mutation caught: drop the {k} flush")
    lines = body.splitlines(keepends=True)
    q = next(i for i, l in enumerate(lines) if "writeQueueToFiles" in l)
    c = next(i for i, l in enumerate(lines) if "CurrentReadings::flushNow" in l)
    lines[q], lines[c] = lines[c], lines[q]
    check(not flush_run("".join(lines), tmp), "mutation caught: queue flushed before the stores")
    check("new " not in body and "String" not in body and "malloc" not in body, "the body allocates nothing")

check(not site_problems(texts), "all seven sites call flushAllBeforeReset() immediately before the reset, after their record writes")
for p in site_problems(texts):
    print("      " + p)
check("flushAllBeforeReset" not in re.search(r"if \(outOfMemory >= 0\) \{.*?\n  \}\n", texts["main"], re.S).group(0),
      "OOM structural exclusion: the out-of-memory reset does not flush")

# Mutations of the call sites.
for name, key, term, before in SITES:
    m = dict(texts)
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
In file included from /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/tests/connectivity_failsafe_unknown_openness_test.cpp:122:
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//failsafe_unknown_openness_66249/body.h:145:18: error: no member named 'set_powerOffRequested' in namespace 'RecoveryState'
  RecoveryState::set_powerOffRequested(0); RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow(); // not expected to return: no recent stage-3 evidence
  ~~~~~~~~~~~~~~~^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//failsafe_unknown_openness_66249/body.h:145:59: error: no member named 'set_connectivityRecoveryStage' in namespace 'RecoveryState'
  RecoveryState::set_powerOffRequested(0); RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow(); // not expected to return: no recent stage-3 evidence
                                           ~~~~~~~~~~~~~~~^
2 errors generated.
FAIL: hibernateAttempt() differs from the base block beyond the listed edits:
--- base
+++ current
@@ -4,61 +4,50 @@
     if (shouldUseBoronRtcAlarmHibernate((uint32_t)wakeInSeconds, rtcNow)) {
       pinMode(WAKEUP_PIN, INPUT_PULLUP);
       if (digitalRead(WAKEUP_PIN) == HIGH) {
-        if (clearAb1805StaleAlarmInterrupts()) {
-          const time_t wakeTime = rtcNow + (time_t)wakeInSeconds;
-          if (ab1805.interruptAtTime(wakeTime)) {
<shellId: 51 completed with exit code 0>
```

</details>


---

<sub>16m 4s</sub>

### `bash`

**Read structural test**

`$ grep -n "beyond the listed edits" -B40 tests/sleep_execution_structural_test.py | head -90; git status --short | head -30`

<details>
<summary>64 lines</summary>

```
150-base_lines = base_text.split("\n")
151-
152-
153-def between(lines, first, last):
154-    return "\n".join(lines[first:last + 1]) + "\n"
155-
156-
157-def base_block(start_marker, end_marker):
158-    i = base_text.find(start_marker)
159-    j = base_text.find(end_marker, i + len(start_marker))
160-    if base_text.count(start_marker) != 1 or i < 0 or j < 0:
161-        fail(f"base marker {start_marker!r} is not unique/present")
162-    return base_text[i:j + len(end_marker)]
163-
164-
165-raw_lines = raw.split("\n")
166-checks = []
167-s, e = function_span(raw_lines, "bool effectiveStandby(")
168-body = between(raw_lines, s + 1, e - 1)
169-if not body.endswith("  return useNetworkStandbyEffective;\n"):
170-    fail("effectiveStandby() must end with `return useNetworkStandbyEffective;`")
171-checks.append(("effectiveStandby", body[:-len("  return useNetworkStandbyEffective;\n")],
172-               base_block("  bool preserveStandbyForThisPass = false;\n", "  useNetworkStandbyEffective = false;\n#endif\n")))
173-s, e = function_span(raw_lines, "void hibernateAttempt(")
174-checks.append(("hibernateAttempt", between(raw_lines, s + 1, e - 1),
175-               base_block("#if PLATFORM_ID == PLATFORM_BORON\n  if (overnightFallbackSleep) {\n    time_t rtcNow", "\n  }\n#endif\n")))
176-s, e = function_span(raw_lines, "bool sleepAndResume(")
177-body = between(raw_lines, s + 1, e - 1)
178-for needle, repl in (("  wake = {pirWake, buttonWake, timerWake, wakeReasonLabel};\n", ""), ("  return false;\n", ""),
179-                     ("        return true;\n", "        return;\n")):
180-    if body.count(needle) != 1:
181-        fail(f"sleepAndResume() must contain {needle.strip()!r} exactly once, found {body.count(needle)}")
182-    body = body.replace(needle, repl)
183-checks.append(("sleepAndResume", body,
184-               base_block("  const SensorWakeSource sensorWake = SensorManager", '  PowerDiagnostics::logPowerState("post-wake");\n')))
185-for name, current, original in checks:
186-    for new, old in RENAMES:
187-        current = current.replace(new, old)
188-    if current != original:
189-        diff = "".join(list(difflib.unified_diff(original.splitlines(True), current.splitlines(True), "base", "current"))[:40])
190:        fail(f"{name}() differs from the base block beyond the listed edits:\n{diff}")
 M src/Generalized-Core-Counter.cpp
 M src/persist/RecoveryState.h
 M src/power/ConnectivityFailsafe.cpp
 M src/state/State_Error.cpp
 M src/time/HibernateCycle.cpp
 M src/time/HibernateWakeDiagnostics.h
 M tests/fixtures/sleep_execution_transcript.txt
 M tests/hibernate_wake_diagnostics_test.cpp
 M tests/hibernate_wake_diagnostics_test.sh
 M tests/s2_alarm_watchdog_wake_structural_test.py
 M tests/sleep_cloud_gate_structural_test.py
 M tests/sleep_execution_equivalence_test.cpp
 M tests/sleep_execution_equivalence_test.sh
 M tests/sleep_plan_structural_test.py
 M tests/sleep_preflight_structural_test.py
 M tests/sleep_wake_structural_test.py
 M tests/stubs/sleep_teardown/SleepExecutionFakes.h
 M tests/watchdog_inferred_classification_test.cpp
 M tests/watchdog_inferred_classification_test.sh
?? tests/__pycache__/
?? tests/s2_hibernate_normalise.py
?? tests/s2_transcript_normalise.py
<shellId: 52 completed with exit code 0>
```

</details>


---

<sub>16m 8s</sub>

### `bash`

**Find base source in test**

`$ sed -n 1,30p tests/sleep_execution_structural_test.py; grep -n "base_text =\|BASE\b\|fixture\|RENAMES" tests/sleep_execution_structural_test.py | head -20; git log --oneline -3 -- tests/sleep_execution_structural_test.py`

<details>
<summary>37 lines</summary>

```
#!/usr/bin/env python3
"""
WO-2026-10-21-001 (Sleep S-2) - structural invariants of the sleep-execution refactor, checked on
the REAL src/state/State_Sleep.cpp (comments stripped before every textual check).

  1. The three standby-return fields are declared once, as members of one struct in the anonymous
     namespace; the old handler-local names are gone from State_Sleep.cpp.
  2. Only effectiveStandby(), sleepAndResume() and the SleepTeardown::run(...) call arguments
     mention `standbyReturn`.
  3. Every SystemSleepConfiguration in src/ is still a fresh local.
  4. (needs the base commit) SleepTeardown.{h,cpp} are unchanged, no file was added under src/, no
     line mentioning `retained` was added anywhere in src/, and the moved blocks are byte-identical to
     the base apart from the listed edits (field renames, `return;` -> `return true;`, the added
     `wake = {...};` and `return false;`).
The base commit is the one recorded in the equivalence fixture header. Without history (a scratch
copy) the checks of group 4 are skipped and said so.
"""
import difflib
import os
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(os.environ.get("SLEEP_EXEC_STRUCT_ROOT", Path(__file__).resolve().parent.parent))
SRC = REPO_ROOT / "src"
SLEEP = SRC / "state" / "State_Sleep.cpp"
FIXTURE = Path(__file__).resolve().parent / "fixtures" / "sleep_execution_transcript.txt"
OLD_NAMES = ("preserveStandbyAfterNormalReturn", "forceModemOffAfterStandbyFailure", "standbySleepFailureError")
RENAMES = (("standbyReturn.preserveAfterNormalReturn", "preserveStandbyAfterNormalReturn"),
15:The base commit is the one recorded in the equivalence fixture header. Without history (a scratch
28:FIXTURE = Path(__file__).resolve().parent / "fixtures" / "sleep_execution_transcript.txt"
30:RENAMES = (("standbyReturn.preserveAfterNormalReturn", "preserveStandbyAfterNormalReturn"),
128:base_text = git("show", f"{base}:src/state/State_Sleep.cpp").stdout if base else ""
186:    for new, old in RENAMES:
89260c9 WO-2026-11-06-002 (S1b): pre-bind seed and WRTC via the vendored AB1805_RK calls (Amendment 2); +14 net src/; suite 117/117
ce2e597 WO-2026-11-05-003: S1 Stage 6 output (Copilot claude-sonnet-5.5 medium): native AM18x5 bind, hal_rtc helpers, cause 8 retired, schema 3; latch deferred to S4; src net +16
5e5e72e WO-2026-11-03-005: flushAllBeforeReset() in State_Error and seven call sites; Stage 6 output (src/ +21 net, one over the +20 budget: architect to rule)
<shellId: 53 completed with exit code 0>
```

</details>


---

<sub>16m 21s</sub>

### `bash`

**Normalise in sleep\_execution\_structural\_test**

```
$ python3 - <<'E'
p='tests/s2_hibernate_normalise.py'; s=open(p).read()
s+='''

def normalise_block(block, is_base, indent_free, stops, resumes):
    """For tests that already cut a block out of the file: the same normalisation on that block.
    indent_free: also drop indentation/blank lines (the hibernate block lost four nesting levels);
    stops/resumes: how many AB1805 watchdog calls the base block holds (ExternalWatchdog replaces them)."""
    if indent_free:
        block = "\\n".join(l.strip() for l in block.splitlines() if l.strip()) + "\\n"
    if is_base:
        if indent_free:
            for old, new in SUBSTITUTIONS:
                if block.count(old) != 1:
                    raise SystemExit(f"S2 substitution does not apply to the base exactly once: {old[:60]!r}")
                block = block.replace(old, new)
        for (old, new, _), count in zip(WATCHDOG, (stops, resumes)):
            if block.count(old) != count:
                raise SystemExit(f"S2 watchdog substitution expected {count}x in the base block: {old!r}")
            block = block.replace(old, new)
    return block
'''
open(p,'w').write(s)
p='tests/sleep_execution_structural_test.py'; s=open(p).read()
a='''for name, current, original in checks:
    for new, old in RENAMES:
        current = current.replace(new, old)
'''
assert a in s
s=s.replace(a,'''sys.path.insert(0, str(Path(__file__).resolve().parent))
import s2_hibernate_normalise  # v50 S2 (WO-2026-11-05-004): the approved S2 substitutions are normalised on the base side only
for name, current, original in checks:
    for new, old in RENAMES:
        current = current.replace(new, old)
    if name in ("hibernateAttempt", "sleepAndResume"):
        hib = name == "hibernateAttempt"
        current = s2_hibernate_normalise.normalise_block(current, False, hib, 1, 1)
        original = s2_hibernate_normalise.normalise_block(original, True, hib, 1, 1)
''')
open(p,'w').write(s)
E
python3 tests/sleep_execution_structural_test.py 2>&1 | grep -v "^PASS"
```

```
S2 watchdog substitution expected 1x in the base block: 'ab1805.resumeWDT();'
<shellId: 54 completed with exit code 0>
```


---

<sub>16m 24s</sub>

### `bash`

**Count watchdog calls in base**

```
$ python3 - <<'E'
t=open('build-tmp/wo-2026-11-05-004-stage6-r3/base_sleep.cpp').read()
print(t.count("ab1805.stopWDT"),t.count("ab1805.resumeWDT"))
E
grep -n "resumeWDT\|stopWDT" build-tmp/wo-2026-11-05-004-stage6-r3/base_sleep.cpp
```

<details>
<summary>7 lines</summary>

```
2 3
709:            const bool watchdogStopped = ab1805.stopWDT();
742:            ab1805.resumeWDT();
785:  const bool watchdogStopped = ab1805.stopWDT();
898:        ab1805.resumeWDT();
926:  ab1805.resumeWDT();
<shellId: 55 completed with exit code 0>
```

</details>


---

<sub>16m 30s</sub>

### `bash`

**Fix count; inspect failsafe test stubs**

```
$ python3 - <<'E'
p='tests/sleep_execution_structural_test.py'; s=open(p).read()
s=s.replace("original = s2_hibernate_normalise.normalise_block(original, True, hib, 1, 1)","original = s2_hibernate_normalise.normalise_block(original, True, hib, 1, 1 if hib else 2)")
open(p,'w').write(s)
E
python3 tests/sleep_execution_structural_test.py 2>&1 | grep -v "^PASS"; grep -n "RecoveryState\|namespace RecoveryState" tests/connectivity_failsafe_unknown_openness_test.cpp | head; grep -n "RecoveryState" tests/connectivity_failsafe_unknown_openness_test.sh | head
```

<details>
<summary>6 lines</summary>

```
sleep_execution_structural_test: all invariants hold
57:namespace RecoveryState {
62:} // namespace RecoveryState
144:  RecoveryState::stage = s.stage;
145:  RecoveryState::lastAction = s.lastAction;
<shellId: 56 completed with exit code 0>
```

</details>


---

<sub>16m 32s</sub>

### `bash`

**View failsafe test stubs**

`$ sed -n 40,125p tests/connectivity_failsafe_unknown_openness_test.cpp; grep -rln "ConnectivityFailsafe.cpp" tests | head`

<details>
<summary>81 lines</summary>

```
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

tests/connectivity_failsafe_open_hours_test.sh
tests/failsafe_cadence_rule_test.sh
tests/connection_mode_downgrade_test.sh
tests/clock_trust_standard_structural_test.py
tests/deliberate_reset_flush_test.py
tests/connection_mode_downgrade_structural_test.py
tests/connectivity_failsafe_unknown_openness_test.cpp
tests/power_consumer_regression_test.sh
tests/connectivity_failsafe_unknown_openness_test.sh
tests/s2_alarm_watchdog_wake_structural_test.py
<shellId: 57 completed with exit code 0>
```

</details>


---

<sub>16m 36s</sub>

### `bash`

**Patch fake; inspect reset flush test**

```
$ python3 - <<'E'
p='tests/connectivity_failsafe_unknown_openness_test.cpp'; s=open(p).read()
a="inline time_t get_lastConnectivityRecoveryAction() { return lastAction; }\n"
s=s.replace(a,a+"inline void set_connectivityRecoveryStage(uint8_t) {}\ninline void set_powerOffRequested(uint8_t) {}\n",1)
open(p,'w').write(s)
E
grep -n "SITES" -A14 tests/deliberate_reset_flush_test.py | head -50; grep -n "^CALL\|def site_problems" -A25 tests/deliberate_reset_flush_test.py | head -60
```

<details>
<summary>102 lines</summary>

```
32:SITES = [
33-    ("thrash tier 3", "thrash", "System.reset(RESET_CAUSE_THRASH_TIER3);", "RecoveryState::raiseAlert(18);  // Alert 18: Severe thrash"),
34-    ("error soft", "error", "System.reset(RESET_CAUSE_ERROR_STATE_SOFT);", "Executing soft reset from ERROR_STATE"),
35-    ("error hard", "error", "ab1805.deepPowerDown();", "Executing deep power down from ERROR_STATE"),
36-    ("heap guard", "sleep", "System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);", "PowerDiagnostics::flushDiagBatch();"),
37-    ("sleep exhausted", "sleep", "System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);", "PowerDiagnostics::flushDiagBatch();"),
38-    ("failsafe stage 2", "failsafe", "System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);", "persistConnectivityFailsafeState(2, now, true);"),
39-    ("failsafe stage 3", "failsafe", "ab1805.deepPowerDown();\n}", "persistConnectivityFailsafeState(3, now, true);"),
40-]
41-
42-
43-def load():
44-    return {k: (SRC / v).read_text() for k, v in FILES.items()}
45-
46-
--
50:    for name, key, term, before in SITES:
51-        text = texts[key]
52-        if text.count(term) != 1:
53-            out.append(f"{name}: terminator {term!r} is not unique")
54-            continue
55-        at = text.index(term)
56-        head = text[:at].rstrip()
57-        if not head.endswith(CALL):
58-            out.append(f"{name}: {CALL} is not immediately before the reset/power-down")
59-            continue
60-        flush_at = len(head) - len(CALL)
61-        pos = text.rfind(before, 0, flush_at)
62-        if pos == -1 or flush_at - pos > 600:
63-            out.append(f"{name}: {before!r} must be written shortly before the flush")
64-    fs = texts["failsafe"]
--
76:    if total != len(SITES):
77:        out.append(f"expected {len(SITES)} call sites, found {total}")
78-    return out
79-
80-
81-FAKES = r"""
82-#include <cassert>
83-#include <string>
84-#include <vector>
85-std::vector<std::string> ev;
86-namespace CurrentReadings { void flushNow() { ev.push_back("C"); } }
87-namespace SystemConfig { void flushNow() { ev.push_back("S"); }
88-  namespace SensorSettings { void flushNow() { ev.push_back("N"); } } }
89-struct FakeQueue { void writeQueueToFiles() { ev.push_back("Q"); } static FakeQueue &instance() { static FakeQueue q; return q; } };
90-using PublishQueuePosix = FakeQueue;
91-"""
--
158:for name, key, term, before in SITES:
22:CALL = "flushAllBeforeReset();"
23-FILES = {
24-    "error": "state/State_Error.cpp",
25-    "thrash": "ThrashGuard.cpp",
26-    "sleep": "state/State_Sleep.cpp",
27-    "failsafe": "power/ConnectivityFailsafe.cpp",
28-    "main": "Generalized-Core-Counter.cpp",
29-}
30-
31-# (name, file key, terminator that ends the site, text that must come before the flush)
32-SITES = [
33-    ("thrash tier 3", "thrash", "System.reset(RESET_CAUSE_THRASH_TIER3);", "RecoveryState::raiseAlert(18);  // Alert 18: Severe thrash"),
34-    ("error soft", "error", "System.reset(RESET_CAUSE_ERROR_STATE_SOFT);", "Executing soft reset from ERROR_STATE"),
35-    ("error hard", "error", "ab1805.deepPowerDown();", "Executing deep power down from ERROR_STATE"),
36-    ("heap guard", "sleep", "System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);", "PowerDiagnostics::flushDiagBatch();"),
37-    ("sleep exhausted", "sleep", "System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);", "PowerDiagnostics::flushDiagBatch();"),
38-    ("failsafe stage 2", "failsafe", "System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);", "persistConnectivityFailsafeState(2, now, true);"),
39-    ("failsafe stage 3", "failsafe", "ab1805.deepPowerDown();\n}", "persistConnectivityFailsafeState(3, now, true);"),
40-]
41-
42-
43-def load():
44-    return {k: (SRC / v).read_text() for k, v in FILES.items()}
45-
46-
47:def site_problems(texts):
48-    """Returns a list of problems; empty when every structural rule holds."""
49-    out = []
50-    for name, key, term, before in SITES:
51-        text = texts[key]
52-        if text.count(term) != 1:
53-            out.append(f"{name}: terminator {term!r} is not unique")
54-            continue
55-        at = text.index(term)
56-        head = text[:at].rstrip()
57-        if not head.endswith(CALL):
58-            out.append(f"{name}: {CALL} is not immediately before the reset/power-down")
59-            continue
60-        flush_at = len(head) - len(CALL)
61-        pos = text.rfind(before, 0, flush_at)
62-        if pos == -1 or flush_at - pos > 600:
63-            out.append(f"{name}: {before!r} must be written shortly before the flush")
64-    fs = texts["failsafe"]
65-    persist = re.search(r"^void persistConnectivityFailsafeState\(.*?^\}\n", fs, re.S | re.M)
66-    if not persist or "flushNow" in persist.group(0) or CALL in persist.group(0):
67-        out.append("failsafe: persistConnectivityFailsafeState() must only record; the flush follows it at the reset sites")
68-    oom = re.search(r"if \(outOfMemory >= 0\) \{.*?\n  \}\n", texts["main"], re.S)
69-    if not oom or "System.reset(RESET_CAUSE_OUT_OF_MEMORY);" not in oom.group(0):
70-        out.append("OOM block not found")
71-    elif "flushAllBeforeReset" in oom.group(0):
72-        out.append("OOM must not flush (immediate reset)")
<shellId: 58 completed with exit code 0>
```

</details>


---

<sub>16m 42s</sub>

### `bash`

**Fix flush test and mutation anchors**

```
$ python3 - <<'E'
p='tests/deliberate_reset_flush_test.py'; s=open(p).read()
a='"ab1805.deepPowerDown();\\n}", "persistConnectivityFailsafeState(3'
assert a in s
s=s.replace(a,'"ab1805.deepPowerDown();\\n  RecoveryState::set_powerOffRequested(0);", "persistConnectivityFailsafeState(3')
open(p,'w').write(s)
p='tests/sleep_execution_equivalence_test.sh'; s=open(p).read()
s=s.replace('''  ".duration((uint32_t)wakeInSeconds * 1000UL);" ".duration((uint32_t)wakeInSeconds * 100UL);"''','''  "          .duration((uint32_t)wakeInSeconds * 1000UL);
" "          .duration((uint32_t)wakeInSeconds * 100UL);
"''')
open(p,'w').write(s)
E
grep -n "duration((uint32_t)wakeInSeconds" src/state/State_Sleep.cpp
```

```
704:          .duration((uint32_t)wakeInSeconds * 1000UL);
769:  ulpConfig.duration((uint32_t)wakeInSeconds * 1000UL);  // Timer-based wake at reporting boundary
856:    stopConfig.duration((uint32_t)wakeInSeconds * 1000UL);
871:        .duration((uint32_t)wakeInSeconds * 1000UL);
<shellId: 59 completed with exit code 0>
```


---

<sub>24m 19s</sub>

### `bash`

**Rerun full host suite**

`$ o=build-tmp/wo-2026-11-05-004-stage6-r3/o.txt; p=0; f=0; for t in tests/*_test.sh; do zsh $t >$o 2>&1 && p=$((p+1)) || { f=$((f+1)); echo FAIL $t; tail -5 $o; }; done; for t in tests/*_test.py; do python3 $t >$o 2>&1 && p=$((p+1)) || { f=$((f+1)); echo FAIL $t; tail -5 $o; }; done; echo pass=$p fail=$f`

```
pass=118 fail=0
<shellId: 60 completed with exit code 0>
```


---

<sub>24m 53s</sub>

### `bash`

**Thermal test on exported tree**

`$ d=build-tmp/wo-2026-11-05-004-stage6-r3; mkdir -p $d/export && git ls-files -z | xargs -0 -I{} echo {} >/dev/null; git ls-files | tar -cf $d/tracked.tar -T - && tar -xf $d/tracked.tar -C $d/export && cp -R tests/s2_hibernate_normalise.py tests/s2_transcript_normalise.py $d/export/tests/ && cp -R src tests $d/export/ && cd $d/export && python3 tests/thermal_coupling_structural_test.py 2>&1 | tail -3`

```
reading, not the persisted field. TMP36 sampling is inline
      with no `static` cross-call accumulator (AC-C9) and all
      samples are collected within a single bounded loop (AC-C10).
<shellId: 61 completed with exit code 0>
```


---

<sub>24m 58s</sub>

### `bash`

**Thermal test exit code**

`$ cd build-tmp/wo-2026-11-05-004-stage6-r3/export && python3 tests/thermal_coupling_structural_test.py >/dev/null 2>&1; echo rc=$?`

```
rc=0
<shellId: 62 completed with exit code 0>
```


---

<sub>25m 7s</sub>

### Copilot

Now mutations on real files (F1, F2, F4, F5), with backups.

---

<sub>25m 18s</sub>

### `bash`

**Run mutations with backup/restore**

```
$ d=build-tmp/wo-2026-11-05-004-stage6-r3; mkdir -p $d/bak; cat > $d/mut.py <<'E'
import subprocess,shutil,sys,os
d='build-tmp/wo-2026-11-05-004-stage6-r3'
muts=[
 ("F1 sentinel to nonzero","src/time/HibernateWakeDiagnostics.h","evidenceByte == kPowerOffSentinel","evidenceByte != 0",["tests/watchdog_inferred_classification_test.sh"]),
 ("F1 legacy 2 accepted","src/time/HibernateWakeDiagnostics.h","evidenceByte == kPowerOffSentinel","evidenceByte >= 2",["tests/watchdog_inferred_classification_test.sh"]),
 ("F2 boot clear not flushed","src/Generalized-Core-Counter.cpp","  SystemConfig::flushNow(); // consume the evidence durably before anything can reload it\n","",["tests/s2_alarm_watchdog_wake_structural_test.py"]),
 ("F2 no clear after error-path return","src/state/State_Error.cpp","      RecoveryState::set_powerOffRequested(0); SystemConfig::flushNow();","",["tests/s2_alarm_watchdog_wake_structural_test.py"]),
 ("F2 failsafe stage not downgraded","src/power/ConnectivityFailsafe.cpp","RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow();","",["tests/s2_alarm_watchdog_wake_structural_test.py"]),
 ("F4 invalid time accepted","src/time/HibernateWakeDiagnostics.h","(timeValid && age >= 0 && age <= kPowerOffEvidenceMaxAgeSec)","(!timeValid || (age >= 0 && age <= kPowerOffEvidenceMaxAgeSec))",["tests/watchdog_inferred_classification_test.sh"]),
 ("F5 ab1805WakeReason removed","src/Generalized-Core-Counter.cpp",'\\"ab1805WakeReason\\":\\"N/A\\",',"",["tests/s2_alarm_watchdog_wake_structural_test.py"]),
 ("F6 hibernate duration scale","src/state/State_Sleep.cpp","          .duration((uint32_t)wakeInSeconds * 1000UL);","          .duration((uint32_t)wakeInSeconds * 100UL);",["tests/sleep_execution_equivalence_test.sh","tests/sleep_wake_structural_test.py","tests/sleep_plan_structural_test.py"]),
]
for name,f,old,new,tests in muts:
    bak=f"{d}/bak/{os.path.basename(f)}"; shutil.copy(f,bak)
    s=open(f).read(); assert s.count(old)==1,(name,s.count(old))
    open(f,'w').write(s.replace(old,new))
    try:
        res=[]
        for t in tests:
            r=subprocess.run(["zsh" if t.endswith(".sh") else "python3",t],capture_output=True)
            res.append(f"{os.path.basename(t)}={'FAILED(caught)' if r.returncode else 'PASSED(SURVIVED)'}")
        print(name,"|",", ".join(res))
    finally:
        shutil.copy(bak,f)
E
python3 $d/mut.py; git diff --stat -- src | tail -1; git diff 3b5cfb8 --numstat -- src | awk '{a+=$1;d+=$2}END{print a,d,a-d}'
```

<details>
<summary>11 lines</summary>

```
F1 sentinel to nonzero | watchdog_inferred_classification_test.sh=FAILED(caught)
F1 legacy 2 accepted | watchdog_inferred_classification_test.sh=FAILED(caught)
F2 boot clear not flushed | s2_alarm_watchdog_wake_structural_test.py=FAILED(caught)
F2 no clear after error-path return | s2_alarm_watchdog_wake_structural_test.py=FAILED(caught)
F2 failsafe stage not downgraded | s2_alarm_watchdog_wake_structural_test.py=FAILED(caught)
F4 invalid time accepted | watchdog_inferred_classification_test.sh=FAILED(caught)
F5 ab1805WakeReason removed | s2_alarm_watchdog_wake_structural_test.py=FAILED(caught)
F6 hibernate duration scale | sleep_execution_equivalence_test.sh=FAILED(caught), sleep_wake_structural_test.py=FAILED(caught), sleep_plan_structural_test.py=FAILED(caught)
 6 files changed, 20 insertions(+), 13 deletions(-)
139 272 -133
<shellId: 63 completed with exit code 0>
```

</details>


---

<sub>25m 24s</sub>

### `bash`

**Find build command**

`$ grep -n "BUILD_PATH_BASE" docs/work-orders/WO-2026-11-05-004-stage6-r2-copilot-report.md docs/work-orders/WO-2026-11-05-004-stage6-r2-copilot-transcript.md | head -5; ls /Users/chipmc/.particle/toolchains; ls build-tmp/wo-2026-11-05-004-stage6-r2 | head`

<details>
<summary>15 lines</summary>

```
docs/work-orders/WO-2026-11-05-004-stage6-r2-copilot-transcript.md:39:2. **Tests and mutations** per the WO and Amendments 1-3 (each mutation must fail; back up the file, mutate, restore from the backup; never use `git checkout`/`git restore` while your own uncommitted work is present). **Verify:** host suite `N/N (sh via zsh, py via python3)` (count with `p=$((p+1))`, never `((p++))`; `thermal_coupling_structural_test.py` also on a clean export of tracked files; baseline 117/117); local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE` as a make argument), text/data/bss (baseline 153100 / 1114 / 2240) and `nm`/objdump showing the new call sites linked; `git diff --numstat 3b5cfb8 -- src` net <= +20 (state the number); layouts and fixtures untouched.
docs/work-orders/WO-2026-11-05-004-stage6-r2-copilot-transcript.md:4039:`$ cd src; git diff --numstat 3b5cfb8 -- . ; git diff --numstat 3b5cfb8 -- . | awk '{a+=$1;d+=$2} END{print a-d}'; cat ../project.properties | head -3; ls ~/.particle/toolchains; grep -n "toolchain\|BUILD_PATH_BASE\|make " ../docs/*.md ../tests/README.md ../release.sh 2>/dev/null | head`
docs/work-orders/WO-2026-11-05-004-stage6-r2-copilot-transcript.md:4161:(DEVICE_OS_PATH="$T/deviceOS/6.6.0" APPDIR="$PWD" PLATFORM="boron" DEVICE_OS_VERSION="6.6.0" GCC_ARM_PATH="$T/gcc-arm/10.2.1/bin/" PATH="$PATH:$T/gcc-arm/10.2.1/bin" make -f "$T/buildscripts/1.17.2/Makefile" compile-user -s BUILD_PATH_BASE="$B/bp" > $B/build.log 2>&1; echo "exit $?" >> $B/build.log) &
docs/work-orders/WO-2026-11-05-004-stage6-r2-copilot-transcript.md:8514:`$ R=$PWD; T=~/.particle/toolchains; rm -rf build-tmp/wo-2026-11-05-004-stage6-r2/clean; cd $R && DEVICE_OS_PATH=$T/deviceOS/6.6.0 APPDIR=$R PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH=$T/gcc-arm/10.2.1/bin/ PATH=$T/gcc-arm/10.2.1/bin:$PATH make -f $T/buildscripts/1.17.2/Makefile compile-user -s BUILD_PATH_BASE=$R/build-tmp/wo-2026-11-05-004-stage6-r2/bp > build-tmp/wo-2026-11-05-004-stage6-r2/build2.log 2>&1; echo exit $?; tail -6 build-tmp/wo-2026-11-05-004-stage6-r2/build2.log; grep -c "warning" build-tmp/wo-2026-11-05-004-stage6-r2/build2.log`
buildscripts
buildtools
deviceOS
gcc-arm
openocd
bp
build.log
build2.log
old_fixture.txt
runall.sh
<shellId: 64 completed with exit code 0>
```

</details>


---

<sub>29m 10s</sub>

### `bash`

**Build firmware with Device OS 6.6.0**

`$ R=$PWD; T=~/.particle/toolchains; B=$R/build-tmp/wo-2026-11-05-004-stage6-r3; find src -name __pycache__ -o -name '*.pyc' | head -1; cd $R && DEVICE_OS_PATH=$T/deviceOS/6.6.0 APPDIR=$R PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH=$T/gcc-arm/10.2.1/bin/ PATH=$T/gcc-arm/10.2.1/bin:$PATH make -f $T/buildscripts/1.17.2/Makefile compile-user -s BUILD_PATH_BASE=$B/bp > $B/build.log 2>&1; echo exit $?; tail -6 $B/build.log; grep -c "warning" $B/build.log`

<details>
<summary>7 lines</summary>

```
exit 0
Creating /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/target/6.6.0/boron/platform_user_ram.ld ...
   text	   data	    bss	    dec	    hex	filename
 151508	   1114	   2240	 154862	  25cee	/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/target/6.6.0/boron/wt-s2.elf

*** COMPILED SUCCESSFULLY ***

6
<shellId: 65 completed with exit code 0>
```

</details>


---

<sub>29m 17s</sub>

### `bash`

**nm and objdump evidence**

`$ git status --short | grep -v "^ M"; ELF=target/6.6.0/boron/wt-s2.elf; NM=~/.particle/toolchains/gcc-arm/10.2.1/bin/arm-none-eabi-nm; OD=~/.particle/toolchains/gcc-arm/10.2.1/bin/arm-none-eabi-objdump; $NM -C $ELF | grep -E "deepPowerDown|ExternalWatchdog|set_powerOffRequested|get_powerOffRequested|set_connectivityRecoveryStage" | head; $OD -d -C $ELF | grep -E "bl.*(deepPowerDown|set_powerOffRequested|set_connectivityRecoveryStage)" | head -20`

<details>
<summary>21 lines</summary>

```
?? tests/__pycache__/
?? tests/s2_hibernate_normalise.py
?? tests/s2_transcript_normalise.py
000b7310 T RecoveryState::get_powerOffRequested()
000b7d18 T RecoveryState::set_powerOffRequested(unsigned char)
000b7c44 T RecoveryState::set_connectivityRecoveryStage(unsigned char)
000b7c38 T sysStatusData::set_connectivityRecoveryStage(unsigned char)
000c591c T AB1805::deepPowerDown(int)
   b664c:	f001 fb64 	bl	b7d18 <RecoveryState::set_powerOffRequested(unsigned char)>
   b820e:	f7ff fd13 	bl	b7c38 <sysStatusData::set_connectivityRecoveryStage(unsigned char)>
   bdf36:	f7f9 fe85 	bl	b7c44 <RecoveryState::set_connectivityRecoveryStage(unsigned char)>
   be14a:	f7f9 fd7b 	bl	b7c44 <RecoveryState::set_connectivityRecoveryStage(unsigned char)>
   be1a2:	f7f9 fd4f 	bl	b7c44 <RecoveryState::set_connectivityRecoveryStage(unsigned char)>
   be1c8:	f7f9 fd3c 	bl	b7c44 <RecoveryState::set_connectivityRecoveryStage(unsigned char)>
   be1f6:	f007 fb91 	bl	c591c <AB1805::deepPowerDown(int)>
   be1fc:	f7f9 fd8c 	bl	b7d18 <RecoveryState::set_powerOffRequested(unsigned char)>
   be202:	f7f9 fd1f 	bl	b7c44 <RecoveryState::set_connectivityRecoveryStage(unsigned char)>
   c2248:	f7f5 fd66 	bl	b7d18 <RecoveryState::set_powerOffRequested(unsigned char)>
   c2254:	f003 fb62 	bl	c591c <AB1805::deepPowerDown(int)>
   c225a:	f7f5 fd5d 	bl	b7d18 <RecoveryState::set_powerOffRequested(unsigned char)>
<shellId: 66 completed with exit code 0>
```

</details>


---

<sub>29m 21s</sub>

### `bash`

**Gather numbers for report**

`$ rm -rf tests/__pycache__; git diff 3b5cfb8 --numstat -- src; git diff 86b11e0 --numstat -- src; python3 --version; zsh --version; git status --short | grep -v "^ M"`

<details>
<summary>24 lines</summary>

```
48	70	src/Generalized-Core-Counter.cpp
3	0	src/MyPersistentData.cpp
6	1	src/persist/RecoveryState.h
1	0	src/power/ConnectivityFailsafe.cpp
3	1	src/state/State_Error.cpp
46	58	src/state/State_Sleep.cpp
0	30	src/time/ClockDiagnostics.cpp
0	2	src/time/ClockDiagnostics.h
7	49	src/time/HibernateCycle.cpp
8	26	src/time/HibernateCycle.h
0	30	src/time/HibernateEligibility.cpp
0	1	src/time/HibernateEligibility.h
17	4	src/time/HibernateWakeDiagnostics.h
4	3	src/Generalized-Core-Counter.cpp
1	1	src/persist/RecoveryState.h
1	0	src/power/ConnectivityFailsafe.cpp
3	1	src/state/State_Error.cpp
3	3	src/time/HibernateCycle.cpp
8	5	src/time/HibernateWakeDiagnostics.h
Python 3.14.6
zsh 5.9 (arm64-apple-darwin26.0)
?? tests/s2_hibernate_normalise.py
?? tests/s2_transcript_normalise.py
<shellId: 67 completed with exit code 0>
```

</details>


---

<sub>29m 45s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-004-stage6-r3-copilot-report.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-004-stage6-r3-copilot-report.md`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s2/docs/work-orders/WO-2026-11-05-004-stage6-r3-copilot-report.md with 6281 characters
```


---

<sub>29m 50s</sub>

### Copilot

All six fixes (F1–F6) are implemented and uncommitted. The host suite is 118/118 and the Device OS 6.6.0 build compiles. No hardware was touched, so I'm claiming nothing about hardware.

- **Source:** `src/` is net −133 against 3b5cfb8 (+7 against 86b11e0), so the +20 budget holds. The `SysData` and `retained` layouts, `lib/`, S1/S1b code and the `deepPowerDown()` calls are unchanged.
- **Build:** text/data/bss is 151508 / 1114 / 2240 (86b11e0: 151468 / 1114 / 2240). Both `deepPowerDown()` call sites are linked, each followed by the new clear line.
- **Mutations:** F1, F2, F4 and F5 mutations were each caught. I also ran a hibernate-duration mutation, which was caught.
- **Reasoning level:** I couldn't observe the level actually run; the report says so. Dispatched was medium.
- **Equivalence fixture:** I regenerated it from the base (121 scenarios, down from 137). The dropped ones are the stale-clear and alarm-program failure scenarios, which can't exist on S2 code.
- **Extra base-comparison test:** `sleep_execution_structural_test.py` is a fifth base-comparison test. Its base group was silently skipped at 86b11e0 because the fixture header had no base commit. Restoring the header turned it on, so I applied the same normalisation there.
- **Failsafe downgrade:** the failsafe caller downgrades stage 3 to stage 2, not 0. Please confirm that is what you meant by "downgrade".
- **`sed -i` slip:** I used `sed -i` once to change a default base commit in a test script, where I should have used the edit tool.

The report is at `docs/work-orders/WO-2026-11-05-004-stage6-r3-copilot-report.md`.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>
