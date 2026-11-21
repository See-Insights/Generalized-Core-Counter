UNDER REVIEW: wo/2026-11-08-001-v50-integrated-fixes @ cf09b8037fd17a72d1193bf15d09edbc1b5b2fbf

PASS for the integrated head with fixes.

`git rev-parse HEAD` matched the full SHA. Tracked `src/` and `tests/` remained untouched and clean.

| # | Result | Judgment |
|---|---|---|
| 1 | PASS | External watchdog initialization is immediately after RTC binding, exactly once; boot-storm sleep is stop/start bracketed. No intended pre-refresh path approaches 124 seconds. `src/Generalized-Core-Counter.cpp:293-301,388-400,1133`. |
| 2 | PASS | Stage 3 writes `0xA5` before flushing; classification is byte-only; every boot clears and flushes the byte; returning POWER_OFF clears, downgrades to stage 2, flushes, then resets. `src/power/ConnectivityFailsafe.cpp:323-331`; `src/time/HibernateWakeDiagnostics.h:163-167`; `src/Generalized-Core-Counter.cpp:760-768`. |
| 3 | PASS | The two-boot test executes the extracted real boot block, and structural/behavioral tests meaningfully cover ordering and lifecycle. Five independent mutations were all caught. |
| 4 | PASS | The complete source delta is limited to the three intended files, net −6 lines. Payloads, persistent layouts, forensics fields, reset-cause values, and other piece behavior are untouched. |
| 5 | PASS | Fresh Device OS 6.6.0 Boron build: **151084 text / 1114 data / 2256 bss**. Full discovery: **121/121**—67 shell tests via zsh and 54 Python tests via python3. |
| 6 | PASS | No additional boot, sleep, wake, power-off, or bench-hand-off regression was found. Hardware bench execution remains Chip’s. |

### 1. Watchdog lifecycle

The bind is at `Generalized-Core-Counter.cpp:294-296`; external instance-2 `init/start` follows at `:299`. Source counts are exactly one `ExternalWatchdog.init`, two `start` calls—the initial start and storm restart—and one refresh.

Device OS 6.6.0 handles a watchdog still running after reset as follows: `Am18x5Watchdog::init()` detects it, stops it, installs the new configuration, and `start()` arms the full window (`hal/shared/am18x5.h:379-405`). Built disassembly confirms:

- Bind, then external init/start: `0xb5fd0 / 0xb5fdc`.
- Storm teardown, then stop/sleep/start: `0xb60d6-0xb60fc / 0xb6164 / 0xb616c / 0xb617e`.
- The later init at `0xb6328` is the separate internal awake-watchdog instance.

The boot-storm teardown does not perform a synchronous 120-second modem shutdown. `Particle.disconnect()` sets the cloud-disconnect flag; Boron’s `Cellular.disconnect()` and `Cellular.off()` enqueue system-thread work, and `Particle.process()` handles at most one application-queue item. Thus no normal teardown path approaches 124 seconds before `stop()`. A wedged system thread remains a watchdog-covered hang, not an intentional teardown duration, so the pre-ruled refresh fix is not needed.

The longest designed wait before the first refresh is approximately 32 seconds in the release configuration: the persisted-serial 30.5-second bound plus at most 1.4 seconds of fuel-gauge stabilization. Even a DEV build’s extra 11-second serial wait remains around 43 seconds. Cloud registration is local, publishes are file-queue operations, sensor initialization is pin/interrupt setup, and first-loop connection acquisition is non-blocking.

On a storm-sleep return, control unconditionally reaches `ExternalWatchdog.start()` at `:399`. If sleep resets instead of returning, the new boot repeats init/start. No normal control-flow path leaves it stopped.

### 2. Stage-3 interleavings

- **Flush succeeds; POWER_OFF succeeds:** stage 3 and `0xA5` are durable. The reason-20 boot reads the byte as power-up evidence, then clears and flushes it at `Generalized-Core-Counter.cpp:767-768`.
- **Flush fails:** the next power-up may see no durable byte and be conservatively classified as inferred watchdog, or may see a durable `0xA5` and follow the normal consumption path. A partially durable stale byte is within the architect-closed stale-sentinel residual.
- **POWER_OFF returns:** `powerOffForRecovery()` restarts the watchdog. The caller clears the byte, downgrades stage 3 to stage 2, flushes, and calls `System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE)` at `ConnectivityFailsafe.cpp:329-331`.
- **Fallback reset:** Device OS reports `RESET_REASON_USER` with reset data 2, not reason 20. ERROR fallback remains data 6. These values are unchanged in `src/ResetCause.h:15-23`.
- **Second real reason-20 after the completed power-up boot:** it sees the durably cleared byte and is classified as inferred watchdog.
- **Second reset before that boot reaches the clear/flush:** it can still consume the old sentinel, but that is exactly the closed stale-sentinel residual—at most one classification, cleared by the next completed v50 boot.

Plainly: **no second real reason-20 reset is misread as a power-up wake other than through the closed stale-sentinel residual.**

Neither `lastConnectivityRecoveryAction` nor the recovery stage remains reason-20 evidence. Their remaining reads are for the failsafe ladder and status payload only. Cooldown logic remains unchanged at `ConnectivityFailsafe.cpp:267-272`, with the production six-hour cooldown and 30-minute jitter at `ConnectivityPolicy.h:115-116`.

### 3. Test quality and mutations

The runtime test extracts and runs the real boot classification block (`tests/watchdog_inferred_classification_test.sh:11-32`). It verifies exact-byte classification, durable first-boot consumption, a second reason-20 classification, and consumption on non-PIN_RESET boots (`watchdog_inferred_classification_test.cpp:34-50`).

Structural checks cover byte-only input, stage-3 ordering, one init, init placement, storm bracketing, and boot-clear durability (`s2_alarm_watchdog_wake_structural_test.py:62-109`). The stage-3 behavior test checks the real supervisor’s ordered event trace (`s3_timed_power_off_test.cpp:197-209`).

Independent scratch-copy mutations:

| Mutation | Result |
|---|---|
| Move stage-3 sentinel write after its flush | Caught by the real stage-3 behavior test and ordering rule |
| Restore stage-3-only evidence in the reader | Caught by byte-only structural rule |
| Remove storm-sleep restart | Caught by start-count and stop/sleep/start rules |
| Move initialization after the storm block | Caught by bind/init/storm ordering rule |
| Remove boot-clear flush | Caught by structural rule and the real two-boot runtime assertion |

**Not caught: none.**

### 4. Complete source-diff inventory

- `src/Generalized-Core-Counter.cpp` — **+8/−9**
  - `:298-302`: add the five-line external-watchdog init/start block after binding.
  - `:397`: add storm-sleep stop.
  - `:399`: add storm-sleep restart.
  - `:763`: replace the former four-line stage/time/recency reader call with one byte-only call.
  - Remove the former five-line late init/start block.
- `src/power/ConnectivityFailsafe.cpp` — **+2/−0**
  - `:12`: include the sentinel owner header.
  - `:327`: write `0xA5` before `flushAllBeforeReset()`.
- `src/time/HibernateWakeDiagnostics.h` — **+4/−11**
  - `:163-165`: update documentation to the byte-only design.
  - Remove `kPowerOffEvidenceMaxAgeSec`.
  - `:167`: replace the five-argument stage/time reader with the one-byte exact-sentinel predicate.

Total: **14 additions / 20 deletions, net −6**.

The status payload at `Generalized-Core-Counter.cpp:1292-1353`, watchdog forensics at `:1370-1394`, persistent-layout declarations, and `ResetCause.h` have no diff. No retired source or ELF symbol remains for `deepPowerDown`, stale-alarm clearing, wake-reason naming, `AB1805::loop`, `stopWDT`, or `resumeWDT`.

### 5. Build, symbols, and suite

Fresh build used Device OS 6.6.0, Boron, the actual worktree as `APPDIR`, and a previously nonexistent `BUILD_PATH_BASE` passed to make:

- **text:** 151084
- **data:** 1114
- **bss:** 2256
- **total:** 154454

This exactly matches Stage 6 and is −8 text versus ae2619b’s 151092/1114/2256.

`nm -C` found:

- `setup` at `0xb5efc`
- `powerOffForRecovery()` at `0xbdfa4`
- `connectivityFailsafeSupervisor()` at `0xbe0bc`

The full in-worktree suite passed **121/121**. `thermal_coupling_structural_test.py` passed as part of that discovery. I did not run a second clean-export copy because authorization expressly required tests inside this worktree, not an export; the supplied Stage 6 report records that separate clean-export pass.

No hardware was available. All bench items—including real nights, physical watchdog reset, rail-off timing, modem behavior, actual reason-20/30 results, and rollback—remain Chip’s.

Model/reasoning actually run: **Codex · gpt-5.6-sol · xhigh**.  
Interpreters/toolchain: **zsh 5.9; Python 3.14.6; Apple clang 14.0.3; GNU Arm Embedded 10.2.1; GNU nm/objdump 2.35.1; GNU Make 3.81**.  
Findings: **none**.