# WO-2026-11-05-007 (I21) Stage 6 report

**Model:** Claude Sonnet 5.5 (`claude-sonnet-5.5`). **Reasoning level:** dispatched as medium; the level actually run could not be observed from inside the session.
**Interpreter line:** `python3` 3.14.6 (`/opt/homebrew/bin/python3`), tests via `/bin/zsh`; Device OS 6.6.0 toolchain (gcc-arm 10.2.1, buildscripts 1.17.2).
**Base:** this branch HEAD (`fa74680`, source identical to `1d1d5af` plus records). Everything is **uncommitted**. Nothing about hardware is claimed.

## Step 0 (read-only, on this head)

1. **Re-cite / latch.** `isClockTrusted()` is `time/Clock.cpp` (was `:477-480`, now about `:488`); write-back is `checkClockResync()` (`Clock.cpp:~366-455`; stamp was at `:413`); `isPlausibleEpoch` `Clock.cpp:~560`; RTC helpers `time/ExternalRtc.h:8-27` (`get`, `set` with read-back verify, `setFromSystem`, `isSet` = `hal_rtc_time_is_valid(EXTERNAL)`). The latch is `ExternalRtc::oscillatorFaultSeenSinceBind()` (`ExternalRtc.h:35`, function-local `static std::atomic<bool>`, readable from any TU, no init-order hazard). It is set by `onOscillatorEvent` (`:36-38`) and seeded at `Generalized-Core-Counter.cpp:670` (RC fallback), i.e. **after** two early trust reads (item 3).
2. **`lastTimeSync` consumers:** writers: `Clock.cpp` write-back only (before this change), `MyPersistentData.cpp:271,956`. Readers: `ClockDiagnostics.cpp:45` (logged), `DeviceStatusPublisher.cpp:291` (`lastSyncEpoch`; a revoke shows 0), `Generalized-Core-Counter.cpp:951` (`neverConfirmedSyncEver = get_lastTimeSync()==0` forces CONNECTING). **Effect to know:** a revoke (large correction, fault, failed write, failed RTC arm) makes the next boot, and a failed-arm wake in the same boot (the zeroing sits before `:951`), connect to the cloud as "no confirmed sync ever". That is the safe direction and is a behaviour change on a revoked unit only. `SystemConfig::setup()` is `:496`, before the piece snapshot (`:506`) and every later `isClockTrusted()`.
3. **Boot order.** `Time.isValid()` and the external clock are set before `setup()` (Device OS bind, S1b). First `isClockTrusted()` callers in `setup()`: boot-storm window (`Generalized-Core-Counter.cpp:378`, **before** `SystemConfig::setup()` at `:496`: the persisted stamp may not be loaded yet, so it reads no carry or the zero default; I could not confirm which without `lib/`, both are the old behaviour), and the piece snapshot (`:506`, after the load). **Both precede the latch seed (`:670`) and `classifyWake` (`:~758`).** So on a boot where the RC fallback is already reported, or a hibernate wake whose RTC arm failed, the `:506` snapshot can see carried trust; a piece opened there is a trusted piece, and the later withdrawal lands in C2's B2 rule (kept open, nothing credited, re-armed), the safe outcome. I did not move the latch seed or `classifyWake` (S4/S2 territory). **Open question for the architect.**
4. **Bad but plausible RTC at restart:** no input catches it before the first sync; exposure = restart to first completed sync (minutes connected; up to the next scheduled connection for a sleeping unit), bounded by 24 h. Unchanged from Part A.
5. **C2:** `pieceUntrusted` is set at piece start (`State_Modes.cpp:73,235`, `State_Report.cpp:82`, `:506`); `closeOccupancySessionSafely` (`State_Common.h:356`) keeps a trusted piece open when trust is gone. Regression written from that code (below).
6. **Ruling 7.** "Out of window" = the gate failed on an RTC arm: `kRtcRead`, `kRtcBeforeZero`, `kRtcOrder`. This is expressible from the existing verdict with **no new bound and no new stored field**: the verdict is applied once, right after `classifyWake()` (`Generalized-Core-Counter.cpp:~758`), by zeroing the persisted `lastTimeSync` (flushed by the existing `flushNow()` at `:~768`). Zeroing reuses the persisted value, so a later non-hibernate restart the same day also has no carry until a sync (stricter than "carry stays allowed", and safe). `sleepErrorSec` is **not** used (no bound exists; sizing one from `hibernate_wake` events was not possible here). `kResetReason`/`kRequestedZero` are not RTC arms and do not withdraw.
7. **Count:** see below; budget holds.

No Step 0 answer broke the plan, so I proceeded (item 3 is the exposure to rule on).

## Measured baseline (before editing)
`121/121 (sh via zsh, py via python3)`; integration head size 151084 / 1114 / 2256 (from the dispatch; not rebuilt).

## Net `src/` lines (`git diff --numstat 1d1d5af -- src`)
| file | + | - |
|---|---|---|
| `Generalized-Core-Counter.cpp` | 1 | 0 |
| `time/Clock.cpp` | 13 | 3 |
| `time/ClockTrust.h` | 7 | 0 |
| `time/HibernateWakeDiagnostics.h` | 2 | 0 |
| **total** | **23** | **3** = **+20 net** |

To fit I dropped doc comments/blank lines I had added (rationale lives in one-line trailing comments). Design: `ClockTrust::isCarryEligible` / `carryStamp` (pure), `carryEligible()` in `Clock.cpp` (the I2C `isSet()` runs only when a stamp exists), `isClockTrusted()` = current-boot trust `|| carryEligible()`, a stamp via `carryStamp(rtcUpdated, rtcPreWriteReadOk, correction, now)` before the `if (rtcUpdated)` block (revoke flushed synchronously), the latch withdrawal at the top of `checkClockResync()` (flushed), `HibernateWakeDiagnostics::isRtcArm`.

## Tests and mutations
New `tests/clock_trust_carry_test.sh` + `.cpp`: predicate table (valid/invalid external, zero/future/stale/implausible stamp, age `N-1/N/N+1`, latch), write-back `T-1/T/T+1/-T-1`, unreadable pre-write value, failed write, clean renewal, composition (carry-only, sync-only, neither, latch mid-boot, revoke leaves the boot trusted), each RTC arm, the C2 regression (real `closeOccupancySessionSafely` extracted from `State_Common.h`: piece opened trusted, trust withdrawn, kept open with nothing credited, closes on the wall clock when trust returns), exact-text checks of `Clock.cpp` and the boot site (flush follows the revoke, epoch not `millis()`). **Mutations, all caught:** latch ignored; age ignored; age on `millis()`; `abs()` dropped; `-1` compared with T; no flush after revoke; revoke that returns early (current boot left untrusted); no plausibility (pure and call site); verdict input ignored; plus a dropped verdict arm (11 in total).
**Deviation:** mutations run on a scratch copy of `src/` (removed at exit), not by backing up and restoring the real files, so the working tree is never mutated.
Existing tests changed as forced: `clock_resync_wiring_test.py` (the stamp is now `carryStamp(rtcUpdated, ...)` before the `if (rtcUpdated)` guard; a failed write stamps 0). My verdict line sits directly after `classifyWake()` so the `watchdog_inferred_classification_test.sh` and `s2_alarm_watchdog_wake_structural_test.py` blocks are unchanged.

## Verification
- CI discovery in this worktree: **`122/122 (sh via zsh, py via python3)`** (121 + the new test).
- New helper `tests/clock_trust_carry_test.sh` exits 0 bare; `thermal_coupling_structural_test.py` passes in the worktree (scratch copies removed) and on a clean export of tracked files (`git ls-files` copy, exit 0).
- Build (Device OS 6.6.0, Boron, fresh `BUILD_PATH_BASE` make argument, `compile-user`): **151316 / 1114 / 2256** (text +232 vs 151084; data and bss unchanged). `nm`: `isClockTrusted()` 0xF8 bytes; `objdump` shows it calling `get_lastTimeSync` and `hal_rtc_time_is_valid` (the predicate is inlined, so there is no separate symbol).

## Deviations and open questions
- I applied the first `Clock.cpp` edit with a one-off Python rewrite instead of the edit tool (content identical to the diff).
- **Item 3 exposure** (latch seed and verdict after the `:378`/`:506` reads): rule whether to accept (C2 B2 makes it safe) or authorize moving the seed.
- If Device OS itself rewrites the chip on a cloud sync before `checkClockResync()` pre-reads it, the measured correction would read ~0 and T would not trigger; bench item 2 (`skew 61`) is the check. I could not determine this from source.
- Ruling 6 fleet read not done (not a Stage 6 task).
