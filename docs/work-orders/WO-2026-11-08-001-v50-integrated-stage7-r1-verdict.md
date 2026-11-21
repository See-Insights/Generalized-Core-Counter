UNDER REVIEW: integration/v50-ab1805 @ ae2619b3ce646d0b2ba59e73e60f9a4e66b31073

NOT VERIFIED for the integrated head.

`git rev-parse HEAD` matched the full SHA. The tracked worktree remained clean. Build and host tests pass, but the integrated source has two release-significant cross-piece defects and one lower-severity stale-evidence risk. Escalate to the architect.

| # | Result | Integrated judgment |
|---|---|---|
| 1 | PASS | Chip ownership transfers cleanly from the pre-bind vendored calls to Device OS. No post-bind vendored transaction or unprotected Wire access was found. |
| 2 | FAIL | A previously armed 124-second external watchdog is still running when the early 600-second boot-storm sleep executes at `Generalized-Core-Counter.cpp:392`; watchdog initialization is not until `:767`. |
| 3 | FAIL | Successful failsafe stage-3 evidence is not consumed, allowing another real reason-20 watchdog reset within 600 seconds to be classified as another power-up. A stale `0xA5` also has no intrinsic expiry. |
| 4 | PASS | WRTC/time seeding, oscillator latch/publication, payload, and fallback reset-cause contracts are internally consistent. |
| 5 | FAIL | Build and clean suite pass, but a 600→3600-second evidence-window mutation escaped the existing tests. |
| 6 | NOT VERIFIED | No hardware was available. Parent bench item 5 cannot collect/discriminate everything it requests using the production binary as written. |

## Findings

1. **High — armed external watchdog truncates the boot-storm holdoff**

   - Location: `src/Generalized-Core-Counter.cpp:383-393`, especially `:392`; watchdog initialization is `:767`.
   - Breaking path: v50 arms the chip watchdog → an early qualifying reset occurs → boot-storm count reaches six → the next boot enters a 600-second ULP holdoff before calling `ExternalWatchdog.init()` → the chip watchdog, still counting across reset, resets the MCU in at most 124 seconds, possibly sooner.
   - This is distinct from the accepted S3 shutdown-window residual. It is a pre-initialization sleep path introduced into the watchdog lifecycle only when the pieces are integrated.
   - Smallest fix: move the existing instance-2 `init/start` immediately after the RTC bind, then stop/start it around the boot-storm `System.sleep()` just like the S2 sleep paths. Remove the later duplicate initialization.
   - Owner: **S2**, interacting with the pre-existing boot-storm path.

2. **Medium — successful stage-3 evidence is reusable**

   - Locations: `src/power/ConnectivityFailsafe.cpp:323-327`; `src/time/HibernateWakeDiagnostics.h:169-173`; boot classification at `src/Generalized-Core-Counter.cpp:753-764`.
   - Breaking interleaving:
     1. Stage 3 persists `stage=3` and `lastConnectivityRecoveryAction=T`.
     2. POWER_OFF succeeds.
     3. The reason-20 boot correctly treats the recent stage-3 record as power-up evidence.
     4. Only the byte is cleared; stage/action remain.
     5. The external watchdog is rearmed at `:767`.
     6. A real watchdog reset before `T+600` again satisfies `failsafeRecent`, so it is reported as a power-up and its watchdog forensic record is suppressed.
   - Smallest fix: after a reason-20 classification actually consumes stage-3 evidence, durably consume that evidence before rearming the dog—for example, clear `lastConnectivityRecoveryAction` and flush while retaining stage 3 if the failsafe ladder must remain latched. Add a two-reason-20 regression case.
   - Owner: **S2’s evidence reader**, coordinated with **S3’s persisted record**.

3. **Low — `0xA5` has no intrinsic expiry**

   - Locations: writer `src/state/State_Error.cpp:178`; reader `src/Generalized-Core-Counter.cpp:758`; clear/flush `:763-764`.
   - A stale `0xA5` can mislabel a later real watchdog reset. It survives without a time bound if:
     - the write becomes durable but power/reset occurs before the requested power-off;
     - the post-return or boot clear cannot be flushed;
     - boots repeatedly reset before reaching `:763-764`; or
     - firmware rolls back to v49, which ignores the reserved byte, and later returns to v50.
   - Normally it survives only until the first v50 setup that reaches `:764`; across v49 or repeated pre-clear resets, calendar lifetime is unbounded. It can affect one reason-20 classification before a successful clear.
   - Smallest complete fix: bind the sentinel to recency using architect-approved rollback-compatible persistent evidence. At minimum, stop the external dog before writing/flushing the sentinel and make v49.1 clear the reserved byte during rollback.
   - Owner: **S2 evidence protocol / rollback floor**. This is separate from the already accepted manual-RESET residual.

4. **Medium, test net only — evidence duration is self-referential in tests**

   - Mutation: `kPowerOffEvidenceMaxAgeSec = 600` → `3600`.
   - Both `watchdog_inferred_classification_test.cpp` and `s2_alarm_watchdog_wake_structural_test.py` passed because the runtime test derives its boundary from the mutated constant and the structural test does not assert 600.
   - Smallest fix: independently assert the required literal/policy value and add the two-boot consumption scenario from finding 2.
   - Owner: **S2 tests**. This is not a separate code defect.

5. **Medium, bench hand-off — item 5 cannot prove the claimed wake source as written**

   - Parent item 5 requires the AM18x5 raw `STATUS` at boot, but the integrated production payload deliberately publishes `ab1805WakeReason:"N/A"` and does not expose raw status (`src/Generalized-Core-Counter.cpp:1293-1298`). Device OS may also service/alter the alarm state before application setup.
   - Reason 30 covers RTC and GPIO hibernate wakes, so reset reason plus timing alone cannot rule out a coincident BUTTON/GPIO wake.
   - Smallest fix: amend the bench to capture the I²C transaction/raw STATUS externally and monitor both WKP and BUTTON, or authorize a diagnostic image that reads the status before Device OS consumes it. Do not claim this evidence from the production CI image alone.
   - Owner: **parent bench plan / S2 hand-off**.

## 1. Chip ownership trace

- On an already-bound boot, Device OS owns the AB1805 before `setup()`; `Am18x5.status().bound()` causes the vendored pre-bind call to be skipped.
- On an unbound boot, `PreBindRtc::run()` at `Generalized-Core-Counter.cpp:294` is the only vendored owner. It runs on the application setup thread under one outer `Wire` lock (`PreBindRtc.h:5`), conditionally calls `Wire.begin()` (`:6`), reads/writes CONTROL1 and reads time (`:8-13`).
- The nested vendored calls are safe under that outer lock because Device OS 6.6.0 creates a recursive I²C mutex (`hal/src/nRF52840/i2c_hal.cpp:331-343,768-788`).
- `ExternalRtc::ensureBound()` at `Generalized-Core-Counter.cpp:295-296` then transfers ownership to Device OS. The external-RTC worker is created only during the bind (`hal/shared/am18x5.cpp:481-526`), after the pre-bind function and lock have returned.
- Because S1b already called `Wire.begin()`, Device OS skips `hal_i2c_reset()` at `am18x5.cpp:491-498`. That is the explicitly accepted S1b assumption; no conflicting transaction exists between the pre-bind call and bind.
- After binding, clock-source snapshot, external time read/write, oscillator event registration, alarm operations, and `ExternalWatchdog` all delegate to the Device OS `Am18x5` instance. Register accesses and the worker thread use the same recursive I²C lock (`am18x5.cpp:1331-1397,1407-1474`).
- `ExternalWatchdog` is instance 2 and delegates to this same driver; it is not a second raw owner.
- SensorManager’s raw TMP112 traffic explicitly locks `Wire` (`src/sensors/SensorManager.cpp:419-460`). Device OS FuelGauge and PMIC transactions likewise lock their I²C instance. These run after binding and may interleave only between complete locked transactions.
- The vendored object remains allocated for L, but no post-bind code calls it, so none of its cached state is consumed.

All literal `ab1805.`/`AB1805` matches under `src/`, including prose and enum names:

- `persist/RecoveryState.h:30,36`
- `device_pinout.h:68,69`
- `MyPersistentData.h:176`
- `Generalized-Core-Counter.cpp:47,127,217,293,772`
- `device_pinout.cpp:84,86`
- `BuildProfile.h:199,214`
- `sensors/SensorManager.cpp:418`
- `ResetCause.h:12`
- `power/ConnectivityFailsafe.h:4`
- `state/State_Error.cpp:27,85,173`
- `state/State_Common.h:225`
- `cloud/DeviceStatusPublisher.cpp:40`
- `state/StateMachine.h:4,29,31`
- `time/ClockTrust.h:27,61,138,139,158,219`
- `time/RtcSkewTest.h:11,12,107,114,133,134,355,362,374`
- `time/PreBindRtc.h:2`
- `time/Clock.cpp:79,82,83,97,172,178,282,292,308,341,346,353,375`
- `time/HibernateWakeDiagnostics.h:22,50`
- `time/ExternalRtc.h:2,50,56`

The executable vendored surface is only the include/object/extern at `Generalized-Core-Counter.cpp:47,127`, `StateMachine.h:4,31`, the call at `Generalized-Core-Counter.cpp:294`, and the generic `readRegister`/`writeRegister`/`getRtcAsTime` calls at `PreBindRtc.h:8-13`. They all execute before binding and only on the setup thread.

## 2. Watchdog lifecycle

- Initialization: `Generalized-Core-Counter.cpp:767`, instance 2, 124 seconds. Device OS `init()` detects a running chip dog, stops it, configures it, and `start()` rearms a full window (`hal/shared/am18x5.h:379-405`).
- Refresh: exactly one source, `Generalized-Core-Counter.cpp:1134`.
- Hibernate: stop `State_Sleep.cpp:706`; unexpected return abandons and restarts at `:735-736`.
- ULP/STOP cascade: stop once at `:773`; the watchdog remains stopped through ULP, GPIO STOP, and timer-only STOP. Total failure restarts before delay/reset at `:886`; every successful return converges on restart at `:914`.
- S3 POWER_OFF: stop/sleep/start at `ConnectivityFailsafe.cpp:139-141`.
- Returning S3 callers clear/downgrade/flush before resets:
  - ERROR: `State_Error.cpp:180-182`.
  - Failsafe: `ConnectivityFailsafe.cpp:327-329`.
- Successful hibernate and POWER_OFF both leave the chip dog stopped; the reboot’s `init/start` arms it.
- No ordinary S2/S3 path stops it permanently or double-starts it without an intervening stop. No additional post-init nRF-WDT gap was found beyond the already accepted S3 residual.
- Finding 1 is the uncovered pre-init boot-storm path.

**Rollback v50→v49 is bench-only, not source-bounded.** In `git show daec561:src/Generalized-Core-Counter.cpp`, v49 may delete the binding and reset at lines 303-308, and may enter a 600-second boot-storm sleep at lines 396-405 before its first vendored setup/rearm at line 811. Its first loop call is line 1176; the vendored refresh is `lib/AB1805_RK/src/AB1805_RK.cpp:65-70`. Therefore the source does not prove service within 124 seconds. Parent bench item 2 exercises the normal case, but not every retained boot-storm state.

## 3. Evidence lifecycle

Production writers/readers are:

- Raw field: `MyPersistentData.h:121`; only raw accessors are `MyPersistentData.cpp:1068-1069`.
- Fresh-record initialization zeroes all of SysData (`StorageHelperRK.cpp:151-156`).
- `0xA5` writer: `State_Error.cpp:178`.
- Byte readers: reason-20 classification at `Generalized-Core-Counter.cpp:758`.
- Byte clears: boot `:763`; returning ERROR `State_Error.cpp:181`; returning failsafe `ConnectivityFailsafe.cpp:328`.
- Stage/action writer: `persistConnectivityFailsafeState()` at `ConnectivityFailsafe.cpp:77-80`, used for stage 3 at `:323`, flushed at `:326`.
- Stage/action clears: initialization `MyPersistentData.cpp:150-151`; good connection `ConnectivityFailsafe.cpp:156-159`; returning stage 3 downgrades to 2 at `:328`.
- Other readers are the failsafe supervisor (`ConnectivityFailsafe.cpp:248,256`), clear helper (`:147-149`), and status telemetry (`Generalized-Core-Counter.cpp:1257,1259`).
- No production migration logic writes `structuresVersion`; the legacy name remains only in comments/tests. Legacy value 2 is not evidence.

Interleavings:

- Write then failed/incomplete flush: durable state may remain 0/2, causing a false inferred-watchdog result after actual power-off, or may contain A5 and behave as requested.
- Durable A5 then reset before POWER_OFF: a reason-20 reset is labelled power-up even though the rail never went off.
- POWER_OFF succeeds: A5 is consumed and immediately flushed on the next completed v50 setup.
- POWER_OFF returns: helper restarts the dog; caller clears evidence and flushes before reset. Causes 6 and 2 produce a USER reset with reset-data, not reason 20.
- Reset during/after a failed returning clear: stale evidence remains until a completed v50 boot clear.
- Hibernate with stale A5: reason 30 never reads it; the unconditional boot clear removes it.
- v49→v50 with legacy 2: exact comparison rejects it; boot clears it.
- v50→v49 with A5: v49 ignores it, so it may survive indefinitely; the next completed v50 setup clears it, but an earlier reason-20 reset can consume it incorrectly.
- Successful stage 3 followed by another reason-20 reset within 600 seconds produces finding 2.

## 4. Other contracts

- S1b leaves WRTC=1. Device OS masked CONTROL1 writes preserve unrelated bits, and the native relative alarm/hibernate path does not clear WRTC.
- Time is seeded before the reason-20 `timeValid` check: Device OS does it before setup on a bound boot; S1b does it at `PreBindRtc.h:12` on an unbound boot. Classification is much later at `Generalized-Core-Counter.cpp:751-760`.
- S4 uses one clock-source snapshot: `startupOscUsingRC` is assigned at `:658`, used for the latch at `:663`, and published unchanged at `DeviceStatusPublisher.cpp:312`.
- Payload contract is consistent: reason-20 records retain `ab1805WakeReason:"N/A"` and publish `watchdogSource` as `inferred`/`NONE`; `usingRC` is the same boot snapshot.
- S3 fallback reset data values are 2 and 6 (`ResetCause.h:17,21`), so boot forensics see USER plus data rather than entering the reason-20 inference gate.

## 5. Build, suite, symbols, mutations

- Device OS: **6.6.0**, Boron, fresh `BUILD_PATH_BASE=build-tmp/wo-2026-11-08-001-stage7-r1/bpb-clean`.
- Build: **text 151092 / data 1114 / bss 2256**, exactly matching the S4 PR reference; total 154462.
- `nm` and `objdump`: zero `deepPowerDown`, `clearAb1805StaleAlarmInterrupts`, `ab1805WakeReasonName`, `AB1805::loop`, `AB1805::stopWDT`, or `AB1805::resumeWDT`. Expected `PreBindRtc::run`, `AB1805::readRegister/writeRegister/getRtcAsTime`, and `powerOffForRecovery()` remain.
- Final discovery inside the worktree: **121/121** — 67 `tests/*.sh` via zsh and 54 `tests/*.py` via Python. Every helper exits zero bare.
- Thermal structural test: PASS in the source-copy-free worktree and PASS on a clean tracked export.
- The initial discovery attempts were intentionally not counted: the thermal test recursively found reviewer-created source copies under `build-tmp`. Those copies were archived, removed, and the clean discovery rerun.
- Mutations:
  - Removed S3 restart after a returning POWER_OFF: **caught** by `s3_timed_power_off_test.sh`.
  - Moved ERROR evidence clear/flush after fallback reset: **caught** by `s2_alarm_watchdog_wake_structural_test.py`.
  - Removed the boot-clear flush: **caught** by the S2 structural test.
  - Changed exact sentinel comparison to `!= 0`: **caught** by the runtime classification test.
  - Changed the stage-3 evidence window 600→3600 seconds: **not caught** by either relevant test; finding 4.

## 6. Bench hand-off

No hardware was available. All parent pre-ship bench items 1–5 remain Chip’s.

- Items 1, 3, and 4 can discriminate their stated normal-path outcomes if power/rail and reset data are recorded.
- Item 2 can discriminate the exercised normal rollback but cannot establish a universal source bound; include an adversarial retained boot-storm state if that is the claim.
- Item 5 needs the instrumentation amendment in finding 5.
- The two real nights, first-v50 boot, physical watchdog reset, post-POWER_OFF reason, power rail, modem behavior, and actual rollback remain unverified.

Model/reasoning actually run: **Codex · gpt-5.6-sol · xhigh**.  
Interpreters/toolchain: **zsh 5.9 (arm64-apple-darwin26.0); Python 3.14.6; GNU Arm Embedded 10.2.1; GNU Make 3.81**.  
Round **1 of at most 2**.