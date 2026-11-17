UNDER REVIEW: wo/2026-11-16-001-watchdog-feed-register @ 99ece4e8cda66afb88d90481b0b7f92d070e8fc9
1 Medium (tests)

# VERIFIED WITH FIXES

The implementation is correct and builds, but a required placement mutation survives the regression tests.

| # | Result | Evidence |
|---|---|---|
| 1 | PASS | Only the pin removal and rc logging change `src/`: `src/Generalized-Core-Counter.cpp:295-300,1133-1137`. `WATCHDOG_DONE_PIN` remains at `src/device_pinout.h:69` and `src/device_pinout.cpp:44`. Device OS uses register writes with no pin and preserves `watchdogValue_`: `hal/shared/am18x5.cpp:900-946`. `begin()`/`end()` handle absent pins safely: `:481-536`; watchdog init/start/stop/refresh remain valid: `hal/shared/am18x5.h:379-427`. |
| 2 | PASS | The latch logs the first nonzero rc and recovery to zero, without steady-failure spam: `src/Generalized-Core-Counter.cpp:1133-1137`. The refresh remains after all state handlers and before `checkClockResync()`. Function-static RAM safely persists across returning ULP sleeps; boot-storm sleep occurs in `setup()` before the loop uses the latch. `SYSTEM_ERROR_NONE` is in scope and the ARM build succeeds. |
| 3 | PASS | D5-bound units compare unequal and rebind once: `src/time/ExternalRtc.h:40-59`, covered at `tests/external_rtc_test.cpp:87-103`. Device OS persists the replacement binding and resynchronizes the chip from internal RTC time after rebind: `hal/shared/exrtc_hal.cpp:267-279,236-248`. v49 remains on the existing unbound path through `PreBindRtc`: `src/time/PreBindRtc.h:4-13`, `src/Generalized-Core-Counter.cpp:293-300`. |
| 4 | FAIL | Pin restoration, refresh removal/reordering, discarded rc, and removed logging are caught. However, moving the refresh block into only `IDLE_STATE` passes all three relevant structural tests. See finding below. |
| 5 | PASS | Fresh Boron/Device OS 6.6.0 build succeeded: text/data/bss `151124/1114/2264`. Against the reported base `151084/1114/2256`, delta is `+40/0/+8`. `nm` found `loop::lastRefreshRc`, `WatchdogClass::refresh()` at `0x000ce306`, and `Logger::warn()` at `0x000b513a`. Disassembly proves `loop` calls refresh, compares the rc, conditionally calls `warn`, updates the latch, then calls `checkClockResync()`. |
| 6 | PASS | `122/122 (sh via zsh, py via python3)`. `src/`: 6 added/3 removed, **+3 net**, within +4. Tests: 122 added/6 removed, +116 net. `docs/reference/alert-codes.md` is unchanged and needs no update because no alert is added, ranked, or cleared. |
| 7 | PASS | A no-refresh 5097 variant still starts the 124 s watchdog but has no recurring feed. The rc log cannot feed it independently, and Device OS does not automatically refresh watchdog instance 2. Nothing in this change should prevent the expected approximately 124 s reset. Hardware execution remains an acceptance step. |

## Finding

**Medium — refresh placement is not mutation-protected.**  
`tests/s2_alarm_watchdog_wake_structural_test.py:97-112`

Path that breaks: move the refresh/latch/log block into `case IDLE_STATE`. The count remains one and it still appears textually before `checkClockResync()`, so all relevant tests pass, but CONNECTING, REPORTING, SLEEPING, and other states no longer feed the chip and can reset after about 124 seconds.

Smallest fix: require the refresh position to satisfy:

```text
thrashGuard.loop(state, millis()) < ExternalWatchdog.refresh() < checkClockResync()
```

and add a probe that moves the block into a state-handler case. No production-source fix is needed.

## Mutation results

| Mutation | Class | Feed test | RTC test | S2 test | Outcome |
|---|---|---:|---:|---:|---|
| Pin restored | Accidental regression | Caught | Caught | Pass | Caught |
| Refresh removed | Accidental regression | Caught | Pass | Caught | Caught |
| Refresh after `checkClockResync()` | Accidental reorder | Pass | Pass | Caught | Caught |
| Refresh inside `IDLE_STATE` | Accidental placement | Pass | Pass | Pass | **Survived** |
| Refresh rc discarded | Accidental regression | Caught | Pass | Pass | Caught |
| Rc log removed | Accidental regression | Caught | Pass | Pass | Caught |
| Rc logged every loop | Accidental regression | Caught | — | — | Caught |
| Config chain on one line | Accidental formatting | Pass | Pass | Pass | Expected survivor |
| Pin set through config variable using `.watchdogPin()` | Constructed placement | Pass | Caught | Pass | Caught overall |

## Documented limits

- The 124 s reset cause is confirmed by timing, not a captured reason.
- D5-to-WDI wiring remains unverified.
- The feed test models Device OS routing; it does not run the real HAL.
- Rc logging is change-based, not once per wake.
- Deliberately indirect setters such as generic pin APIs can evade text-based structural checks; this is a constructed-tooling limit, not an accidental-regression finding.
- `nm` proves the symbols are linked; disassembly proves the call edges. Neither proves runtime I²C success or physical watchdog behavior.

## Known gaps

- The fixed-build 30-minute soak and 5097 no-refresh bench proof were not run under this authorization.
- Only the current image was built. The base-side size values come from Copilot’s report; the independently built current values match that report exactly.
- One preliminary `make` invocation stopped before compilation with “No sources found” after a sandbox-blocked copy step. The completed build used a new, fresh object path.

Model/reasoning: `gpt-5.6-sol`, high.  
Interpreter: Python 3.15.2 via `python3`; shell tests via zsh 5.9; host C++ compiler where invoked.  
Cleanup: the authorized scratch directory was removed; tracked worktree remains unchanged.