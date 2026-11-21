# WO-2026-11-08-001 Stage 6 report (Copilot)

Uncommitted; nothing committed, pushed, merged or flashed. Nothing here claims anything about hardware.

- **Model:** Claude Sonnet 5.5 (`claude-sonnet-5.5`). **Reasoning level:** dispatched as medium; the level actually run could not be observed from inside the session.
- **Interpreter:** Python 3.14.6 (`python3`); `.sh` via zsh; clang++ for host tests.
- **Measured baseline before editing (HEAD, src == ae2619b):** 121/121.

## Source changes (`git diff --numstat ae2619b -- src`)

| file | + | - |
|---|---|---|
| `src/Generalized-Core-Counter.cpp` | 8 | 9 |
| `src/power/ConnectivityFailsafe.cpp` | 2 | 0 |
| `src/time/HibernateWakeDiagnostics.h` | 4 | 11 |
| **total** | **14** | **20** |

Net **-6** (budget: at most +6).

- F1: `ExternalWatchdog.init/start` now sits right after `ExternalRtc::ensureBound` (`setup()`, line ~299). The boot-storm `System.sleep(bootStormSleep)` is bracketed by `ExternalWatchdog.stop()`/`start()` (~397-399). The later duplicate init is removed.
- F2: the stage-3 path writes `set_powerOffRequested(kPowerOffSentinel)` before `flushAllBeforeReset()` (the after-return clear and downgrade block is untouched). `isInferredWatchdog(uint8_t)` takes the byte only; `kPowerOffEvidenceMaxAgeSec` and the stage/time/`timeValid` parameters and call-site arguments are gone. `lastConnectivityRecoveryAction`, `powerOffForRecovery()` and all layouts are untouched.

## F1 checks (what I looked at)

- **No dependency on later setup:** init uses only `WatchdogConfiguration`, the `ExternalWatchdog` global and the AB1805 bind config (`WATCHDOG_DONE_PIN` is passed to the bind, `Generalized-Core-Counter.cpp:295`). Nothing between the bind and the new init point is needed by it. Built ELF: `WatchdogClass::init`/`start` are the first watchdog calls in `setup` (0xb5fd0/0xb5fdc).
- **Longest block before the first `refresh()` (:1133):**
  - bounded serial wait 30 s (`:569-573`; it calls `serviceAwakeWatchdog`, which refreshes the other instance, not this one)
  - 500 ms debug-serial settle (`:273`)
  - DEV-only `waitFor(Serial.isConnected, 10000)` + `delay(1000)` (`:405-406`, compiled only with `ALLOW_BLOCKING_SERIAL_WAITS`)
  - the boot-storm sleep itself is now stopped/restarted
  - These total well under 124 s.
- **Not proven:** I did not audit every callee in the remaining ~700 lines of `setup()` (publish/status/config calls) for hidden blocking. I read the direct waits above and grepped `waitFor`/`delay`/`while`/`waitUntil` in this file only.
- **Open:** the storm path calls `Connectivity::requestFullDisconnectAndRadioOff()` and `Particle.process()` *before* `stop()`, with the watchdog now running. If Device OS blocks there for a long time it is not covered by the bracket (the spec bracketed only the sleep). I did not find `SYSTEM_THREAD(` in `src`, so threading mode is unknown to me.

## Tests

Result: **121/121 (sh via zsh, py via python3)**; no new helper files under `tests/`. `thermal_coupling_structural_test.py` also passes (exit 0) on a clean export of tracked files; the scratch export was removed afterwards.

- `watchdog_inferred_classification_test.{cpp,sh}`: **rewritten.** The old table (stage-3 arm, 600 s recency, future timestamp, invalid-Time rejection, `any_stage`/`no_recency`/`invalid_time_accepted` mutants) tested removed behaviour and is gone. New:
  - (b) byte-only reader: 0xA5 is a power-up; 0/1/2/0x5A/0xA4/0xA6/0xFF are inferred. The stage-3 state alone is not an input any more, by signature.
  - (c) two-boot consumption: runs the REAL boot block extracted from `setup()`. Boot 1 reason 20 with 0xA5 gives power-up, the byte is cleared and flushed with it clear. Boot 2 reason 20 gives inferred watchdog. A non-PIN_RESET boot still consumes the byte.
  - `.sh` mutants: always_wake, never_wake, sentinel_nonzero, sentinel_legacy_two, boot_clear_removed, boot_flush_removed (all caught).
- `s3_timed_power_off_test.cpp`: (a) stage-3 expectation now includes `byte=0xA5` between the log and the flush (before the sleep).
- `s2_alarm_watchdog_wake_structural_test.py`: reader-byte-only; stage-3 order (sentinel, flush, power-off); init after the bind and before the storm sleep; storm sleep bracketed by stop/start; init once and start twice in the file (setup + storm). Existing probes updated; added 7 probes (stage-3 write removed, reader takes stage, storm stop removed, storm start removed, init moved back, duplicate init, evidence-from-retained updated). 20 probes in total.
- `connectivity_failsafe_unknown_openness_test.cpp`: forced change. It extracts the real supervisor body, which now references `HibernateWakeDiagnostics::kPowerOffSentinel`, so it needed a one-line constant stub.

## Mutations on the real `src/` (back up, mutate, `cp` back; the final `diff -r` of `src/` against the backup copy was identical)

| mutation | caught by |
|---|---|
| stage-3 sentinel write removed | s3 test, s2 structural |
| reader accepts any non-zero byte | watchdog_inferred |
| boot clear removed | watchdog_inferred, s2 structural |
| boot flush removed | watchdog_inferred, s2 structural |
| storm-sleep `stop()` removed | s2 structural |
| storm-sleep `start()` removed | s2 structural |
| init moved back after the storm block | s2 structural |
| duplicate init re-added | s2 structural |

All 8 failed as required. The script is `build-tmp/wo-2026-11-08-001-stage6/mut.py`.

## Build and nm (Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE` passed to make, tracked files copied to a scratch APPDIR)

- Exit 0. **text/data/bss = 151084 / 1114 / 2256** (ae2619b: 151092 / 1114 / 2256; -8 text).
- `nm -C`: `T connectivityFailsafeSupervisor()` (0xbe0bc), `T powerOffForRecovery()` (0xbdfa4), `T setup` (0xb5efc) are linked.
- `objdump` of `setup`: `WatchdogClass::init`, then `start` at 0xb5fd0/0xb5fdc (right after the bind). The storm path is `WatchdogClass::stop` (0xb6164), `SystemClass::sleep` (0xb616c), `WatchdogClass::start` (0xb617e). The other `init`/`onExpired`/`start` sequence at 0xb6322+ is the separate awake-watchdog instance, not the external one.

## Deviations and open questions

- **Scratch not cleaned:** `build-tmp/wo-2026-11-08-001-stage6/` still holds my build tree and `BUILD_PATH_BASE` (~700 MB: `tree/`, `bpb/`) plus `mut.py`, `build.log`, `bak.tmp`. My `rm -rf` was refused in this session. Remove it by hand when convenient. Nothing outside that directory was created, apart from the regenerable `tests/__pycache__` (gitignored).
- `s2` structural test's reason-20 `block` slice now ends 25 characters after the first `SystemConfig::flushNow();` (comments are stripped before slicing).
- Open: the pre-`stop()` radio-off blocking question above.
