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

No hardware testing was possible. The 8-hour nights, physical watchdog proof, post-POWER_OFF reset reason, transient alarm behavior, and v50→v49 rollback with the watchdog armed remain Chip’s bench items.