# WO-2026-11-02-004 Stage 7 verdict — round 1

**Verdict: NOT VERIFIED**

**Model actually used:** `gpt-5.6-sol`  
**Reasoning actually used:** `xhigh`  
**Interpreter line:** shell tests with `zsh tests/*.sh`; Python tests with `python3 tests/*.py`; extracted host harness with `clang++ -std=c++17`; ARM build with gcc-arm 10.2.1 and Device OS 6.6.0.

Branch `wo/2026-11-02-004-timezone-planned-reset`, base/HEAD `21804e3`. Review was read-only except this verdict and authorized scratch output under `build-tmp/wo-2026-11-02-004-stage7/`. The production diff is 13 insertions/3 deletions, exactly **+10 net `src/`**, at the binding limit (`docs/work-orders/WO-2026-11-02-004-timezone-applies-to-running-clock.md:4-7`; `src/ResetCause.h:23`; `src/cloud/ConfigApply.cpp:4,11,147-155,264`). `git diff --check` passed.

## Findings

### B1 — blocking: daily occupancy/open-session state is not made reset-safe, and a close-boundary reset can leave the two stores inconsistent

The reset path flushes only `/usr/sysStatus.dat` (`src/cloud/ConfigApply.cpp:150-155`; `src/MyPersistentData.cpp:914-921`). Occupancy, `dailyoccupancy`, open-session state and totals live in `/usr/current.dat`, whose writes are delayed 250 ms and whose facade exposes only deferred `loop()`, not a forced flush (`src/MyPersistentData.cpp:613-640,862-890,1103-1129`; `src/persist/CurrentReadings.h:20-28`).

This is a concrete boundary race, not just a bench unknown. The state handler can close the session, queue the old-day report, clear daily state, and stamp `lastDailyCleanup` (`src/state/State_Report.cpp:37-91,306-328`; `src/sensors/PIRSensor.cpp:14-29`). The same loop then services the delayed current/system stores before `Cloud::loop()` (`src/Generalized-Core-Counter.cpp:1128-1177`). If a deferred timezone apply resets there, `SystemConfig::flushNow()` persists `lastDailyCleanup` but the <250 ms current-record changes need not have been saved. On reboot, `DailyBoundary::check()` sees the persisted cleanup stamp and will not repeat the boundary (`src/time/DailyBoundary.cpp:26-45`), while the old current record can resurrect the open session/old total or lose the close credit. That can drop or double credit and can carry the prior day's `dailyoccupancy` into the next day.

The failsafe comparison does not prove survival: stage 2 likewise flushes only `SystemConfig` and then resets (`src/power/ConnectivityFailsafe.cpp:77-87,301-307`). It shares the same gap.

### B2 — blocking: the publish queue is not guaranteed to survive `System.reset(8)`

Production configures only the file-queue depth, leaving the RAM queue at its default of two (`src/Generalized-Core-Counter.cpp:647-653`; `lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:57-71,399-406`). While connected and the file queue is empty, up to two events remain only in RAM (`lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:69-90`). The library explicitly says non-software-update resets lose that queue and requires RAM size zero when loss is unacceptable (`lib/PublishQueuePosixRK/README.md:32-49`). This includes the close report queued just before a deferred timezone reset (`src/state/State_Report.cpp:54-70`).

The registered disconnect/reset handler does not close the proof: it writes RAM entries only when its application-thread callback executes (`lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:42-60,402-406`). Device OS performs a connected reset by synchronously moving reset work to the system thread (`~/.particle/toolchains/deviceOS/6.6.0/system/src/system_mode.cpp:165-179`), while ordinary system-event delivery is queued to the application thread (`~/.particle/toolchains/deviceOS/6.6.0/system/src/system_event.cpp:103-110,214-228`). The graceful disconnect raises `cloud_status_disconnecting` from that reset work (`~/.particle/toolchains/deviceOS/6.6.0/system/src/system_task.cpp:725-744`), while the calling application thread is blocked in `System.reset`; therefore the handler is not a synchronous persistence guarantee.

### B3 — blocking: simultaneous sensor-ledger changes are not flushed before the planned reset

`applySensorConfig()` runs before timing and mutates the separate `/usr/sensor.dat` store (`src/cloud/ConfigApply.cpp:135-142,341-416`). That store also has a 250 ms delayed write (`src/MyPersistentData.cpp:508-539`), and `SensorSettings` exposes validation/deferred loop but no `flushNow()` (`src/MyPersistentData.cpp:1010-1033`; `src/persist/SystemConfig.h:223-253`). The timezone success path validates it but flushes only the system store before resetting (`src/cloud/ConfigApply.cpp:147-155`). A ledger that changes timezone and sensor settings can therefore reboot with the old sensor record, contrary to the design rationale that the reset re-reads all boot-only settings (`docs/work-orders/WO-2026-11-02-004-timezone-applies-to-running-clock.md:19-22`).

### B4 — blocking: first-ledger validity finalization is skipped until the next connect, changing closed-hours behavior

`loadConfigurationFromCloud()` calls `finalizeLedgerAppliedConfig()` only after the apply returns; finalization is what sets `hasValidLedgerConfig`, selects ledger source and force-flushes it (`src/cloud/Cloud.cpp:597-644`; `src/Config.cpp:108-129,154-160`). The reset does not return, so a first-connect `UTC0` to `EST5EDT` migration reboots before this state is recorded. On that boot, setup marks factory defaults active when the persisted ledger-valid bit is false (`src/Generalized-Core-Counter.cpp:493-532`), and `Config::isValid()` remains false until the second connect.

This window is behavioral: `isWithinOpenHours()` fails open on invalid config (`src/time/Clock.cpp:41-60`), setup can initialize the sensor while actually closed (`src/Generalized-Core-Counter.cpp:1033-1048`), `Clock::openness()` returns `Unknown` (`src/time/Clock.cpp:567-583`), and the occupancy gate ignores only `Closed` (`src/state/State_Common.h:61-63`; `src/state/State_Modes.cpp:63-71`). Detections can therefore start/extend a session after close until ledger reconnect/finalization, violating the no-after-close rule (`docs/work-orders/WO-2026-10-29-002-occupancy-ends-at-close.md:8-17`). The failsafe also deliberately acts unless openness is `Closed` (`src/power/ConnectivityFailsafe.cpp:219-230`), and reporting forces another connect while config is invalid (`src/state/State_Report.cpp:166-169`). Devices that already have a persisted valid-ledger bit do not enter this window; the binding first-connect migration does.

### B5 — blocking test-contract gap; N1 — note on worktree suite discovery

The new behavior test does compile the currently extracted flag, timezone block and whole apply function (`tests/timezone_planned_reset_test.sh:28-59`; `tests/timezone_planned_reset_test.cpp:60-88`). All six behavior mutations and both cause-code mutations were independently observed to fail their intended checks, not compilation (`tests/timezone_planned_reset_test.sh:76-143`). However, the extraction has no byte-for-byte `COPY_MISMATCH` check required for every extracted block (`AI_DEVELOPMENT_WORKFLOW.md:783-786`), and its seven one-process cases do not exercise failed-apply then later-success state, rollback to the live zone, or repeated device-override flips (`tests/timezone_planned_reset_test.sh:56-68`; `tests/timezone_planned_reset_test.cpp:101-148`).

The worktree suite result was **107/108**, not the expected 108/108. The only failure was `tests/thermal_coupling_structural_test.py`, because it scans every `.cpp`/`.h` under the repository and mistakes the pre-existing Stage 6 copied source under `build-tmp/` for outside references (`tests/thermal_coupling_structural_test.py:249-274`). I was not authorized to delete that prior artifact. In the byte-identical Stage 7 source copy excluding prior `build-tmp`, 62/62 shell plus 46/46 Python tests passed: **108/108**. This is a test-discovery note, not a production regression.

### N2 — note: the file-local flag does not loop, but stale state can cause one avoidable delayed reset

Device override wins over defaults (`src/cloud/ConfigApply.cpp:74-95`), the stored string is compared before writing (`src/cloud/ConfigApply.cpp:257-266`), and reset is inside the all-sections-success branch (`src/cloud/ConfigApply.cpp:142-155`). Thus `UTC0` to `EST5EDT` produces one reset; the reboot clears BSS state and equal stored/ledger values produce none; equal values produce none; alternating override values produce one reset per successful real stored change; a failed section does not reset.

The flag is never cleared (`src/cloud/ConfigApply.cpp:11,153-155,176-184`). If timing stores a zone and a later section fails, the next successful apply resets even if that apply has no timezone change. That normally applies the already-stored zone and does not loop. But if a later ledger update restores the currently running zone before success, the stale flag still causes an avoidable reset. The current test checks only the failed call in its own process (`tests/timezone_planned_reset_test.cpp:121-132`), so it does not cover this sequence.

### N3 — note: flush is synchronous in control flow but does not report write durability failures

`set_timeZoneStr()` updates the in-memory record/hash and schedules persistence (`src/MyPersistentData.cpp:191-203`; `lib/StorageHelperRK/src/StorageHelperRK.cpp:59-75,102-110`). `flushNow()` calls `flush(true)`, which immediately calls `save()` when dirty (`src/MyPersistentData.cpp:914-918`; `lib/StorageHelperRK/src/StorageHelperRK.cpp:25-41`); the filesystem save opens/truncates, writes the whole record and closes before returning (`lib/StorageHelperRK/src/StorageHelperRK.cpp:258-270`; `lib/StorageHelperRK/src/StorageHelperRK.h:461-475`). That is synchronous for a normal software reset. The implementation ignores short-write/close results and has no explicit `fsync`, so it is not a power-loss/disk-error durability proof (`lib/StorageHelperRK/src/StorageHelperRK.cpp:258-268`).

### N4 — note: reset/status/alert side effects

The apply is not reached from setup. `Cloud::setup()` only registers ledgers/callbacks, whose async handlers set flags (`src/cloud/LedgerClient.cpp:29-69`); production apply sites are the application-thread connect load, firmware-update load and deferred `Cloud::loop()` (`src/state/State_Connect.cpp:524-545,691-697`; `src/cloud/Cloud.cpp:675-689`). Device OS explicitly supports `System.reset(data)` from this context and records OS reason USER with the supplied data (`~/.particle/toolchains/deviceOS/6.6.0/wiring/src/spark_wiring_system.cpp:47-55`; `~/.particle/toolchains/deviceOS/6.6.0/system/src/system_mode.cpp:144-180`). The reset call point itself is valid; B2 is the queue-preservation defect.

Because reset precedes `pendingStatusPublish`, the changed-zone apply never schedules its ConfigApply status (`src/cloud/ConfigApply.cpp:153-175`). On reboot, the pre-apply ConnectState status has the new persisted timezone generation, but reporting policy is built before validity finalization (`src/state/State_Connect.cpp:524-536`; `src/cloud/DeviceStatusPublisher.cpp:96-123,189-232`). Successful changed-zone apply also never returns to the Alert 41 branch; the equal-value second apply finalizes and returns success, while a genuinely failed apply still raises 41 (`src/state/State_Connect.cpp:536-548`). Alert 41 remains report-auto-clear (`src/cloud/ReportPublisher.cpp:25-38,140-151`).

No new alert is raised or ranked by the change, and `docs/reference/alert-codes.md` is unchanged. But “no alert is cleared” is not literally true: Device OS reports the planned reset as reason USER, and boot clears a pre-existing legacy alert 14 for every USER reset (`src/Generalized-Core-Counter.cpp:586-607`); the alert reference already documents that rule (`docs/reference/alert-codes.md:27-31`). The USER reset also increments `resetCount` (`src/Generalized-Core-Counter.cpp:609-620`). Cause 8 itself is documented in `ResetCause.h` (`src/ResetCause.h:5-23`); code switches inspect the OS reason, not the data, so no new `case 8` is required (`src/Generalized-Core-Counter.cpp:337-354,589-634`; `src/observability/StartupSnapshot.cpp:18-53`). Data 8 is surfaced in the startup status and device-data ledger (`src/Generalized-Core-Counter.cpp:1280-1396`; `src/cloud/DeviceStatusPublisher.cpp:525-529`; `docs/contracts/ledger-contracts.md:203,220`).

## Time/boundary and linkage checks

After reboot, setup loads persistence and configures `LocalTime` from the stored timezone before its first open-hours decision and before sensor initialization (`src/Generalized-Core-Counter.cpp:493-505,924-938,1006-1013,1033-1048`). `DailyBoundary` reads that configured converter (`src/time/DailyBoundary.cpp:9-23`), and the local-time cache includes the timezone signature (`src/time/LocalTimeCache.cpp:39-75`). Thus there is no stale-converter read after reboot. The boundary accounting still fails verification because of B1 and B4, not converter ordering.

Fresh Boron build succeeded from Device OS 6.6.0 with `BUILD_PATH_BASE=build-tmp/wo-2026-11-02-004-stage7/build-base`; ELF size was text 151636, data 1114, bss 2200, total 154950. `nm -C -S` on `app/localbuild/app.elf` found:

- `000baf78 0000017c T Cloud::applyConfigurationFromLedger(...)`
- `000b6cac 00000012 T SystemConfig::flushNow()`
- `2003da84 00000001 b (anonymous namespace)::timezoneStored`
- `000cc474 00000018 T SystemClass::reset(unsigned long, ...)`

Linked disassembly at `0x0bb042-0x0bb04a` calls `flushNow`, loads immediate `8`, then calls `SystemClass::reset`. Linked production callers at `0x0b8f2a` and `0x0b8f88` are `Cloud::loadConfigurationFromCloud()` and `Cloud::loop()`, matching source (`src/cloud/Cloud.cpp:634-644,675-689`); the state-machine call sites are `src/state/State_Connect.cpp:536,695`. Build and linkage pass, but the five blocking findings prevent Stage 7 approval.
