# Triage of Grok's review of the v49.1 progress pack (6 Nov 2026)

**Rulings: Chip (6 Nov).** Grok's full response is `grok-response.md` in this folder (**not yet saved: the text was not in the session; Chip to paste it**). This file records only the rulings and the checks Claude Code made. F-numbers are Grok's; this triage does not restate the findings beyond Chip's one-line labels.

| # | Ruling | Where it lands |
|---|---|---|
| F1 | **Covered.** C1/C2 ship in v50.1 (merged to `main` 6 Nov: [#185](https://github.com/See-Insights/Generalized-Core-Counter/pull/185), [#184](https://github.com/See-Insights/Generalized-Core-Counter/pull/184)); I21 (clock trust survives a restart) is candidate C in v50.2 | `WO-2026-11-05-007-i21-clock-trust-carry.md`; recovery plan items 19, 21 |
| F2 | **Partly wrong.** The session start is persisted (in `current.dat`); HIBERNATE is after the close. Claude Code's check is below. | Check below |
| F3 | **Covered** by the pre-ship bench items 1-3 plus the S2/S3 rulings | `WO-2026-11-05-002-v50-ab1805-switch.md` (pre-ship bench; rulings of 5 Nov) |
| F4 | **Cleanup:** put the last failsafe stage and its time in the status payload | Recovery plan item 17 (cleanup series) |
| F5 | **Accept now:** a CI step on `v*` tags that fails the build if `DEV_BUILD` or `CONNECTIVITY_FAILSAFE_TEST_MODE` is not 0. Small WO, before RC1; Codex high | Recovery plan item 28 |
| Cuts | The PMIC retained fields declared in `SensorManager.h` go to cleanup. Library removal (L) stays as planned (v50.2) | Recovery plan item 17; `WO-2026-11-05-002` (L) |

## F2 check (read-only, `origin/main` @ daec561, v49.1 + C1 + C2)
**Confirmed:**
- **The session start is persisted.** `occupancyStartTime` is a field of the current-readings store, whose file is `/usr/current.dat` (`MyPersistentData.cpp:613`). It is written through `CurrentReadings::set_occupancyStartTime` (cleared at close, `State_Common.h:~455`; set at `State_Modes.cpp:71,233`), validated at boot (`MyPersistentData.cpp:684-686`: occupied with a zero start is forced unoccupied) and clamped if in the future (`:712-717`). The open session survives a reset.
- **The nightly HIBERNATE comes after the close.** The hibernate attempt is the overnight fallback path (`hibernateAttempt(overnightFallbackSleep, ...)`, `State_Sleep.cpp:689`), logs `Sleep: HIBERNATE reason=closed` (`:723`) and is reached only from the closed-hours sleep plan.

**Not confirmed as stated ("close and daily totals are flushed before the nightly hibernate"):**
- There is **no explicit flush at the hibernate site**: `State_Sleep.cpp:705-737` (config, `armForSleep`, `System.sleep(hibernateConfig)` at `:737`) calls neither `flushAllBeforeReset()` nor any `flushNow()`. `flushAllBeforeReset()` is called in `State_Sleep.cpp` only at `:648` (the nightly heap guard) and `:910` (sleep exhausted). The v49 notes record HIBERNATE as not flushed by design.
- What protects the totals is the **deferred save**: setters call `saveOrDefer()` (`lib/StorageHelperRK/src/StorageHelperRK.cpp:35-41`), the stores use a save delay of 100 ms (`sysStatus`, `MyPersistentData.cpp:79`) and 250 ms (`current`, `:634`), and the main loop's `CurrentReadings::loop()` / `SystemConfig::loop()` (`Generalized-Core-Counter.cpp:1180-1182`, which call `flush(false)` at `MyPersistentData.cpp:85` and `:638-639`) save once the delay has passed (`StorageHelperRK.cpp:25-31`). So the close credit, `dailyCount` and `lastReport` writes reach flash within about 250 ms plus one loop transit **if the loop runs after the last write**.
- **Residual gap (INF, not tested):** a write made within about 250 ms of the `System.sleep(hibernateConfig)` call (for example one made in sleep preparation) has no loop transit after it, and HIBERNATE loses RAM. In the normal path the close report and its response come seconds to minutes earlier, so the totals are saved long before; but this rests on timing, not on a flush. Fleet evidence: the close and next-morning zero have been correct in every soak (v44-v49.1), including Dev-09 and Dev-14's 22:00Z hibernates.
- **Option for Chip (not decided):** a `CurrentReadings::flushNow()` and `SystemConfig::flushNow()` pair immediately before the hibernate `System.sleep` would close the gap for a few `src/` lines. It would go through the S3/error-state family (item 26) rather than a separate change.
