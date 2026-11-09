# WO-2026-11-05-007 (I21) Stage 6 round 2 report (Amendment 2)

**Model:** Claude Sonnet 5.5 (`claude-sonnet-5.5`). **Reasoning level:** dispatched as medium; the level actually run could not be observed from inside the session.
**Interpreter line:** `python3` 3.14.6, tests via `zsh`; Device OS 6.6.0 toolchain (gcc-arm 10.2.1, buildscripts 1.17.2). Everything is **uncommitted**, on top of HEAD `86e2c72`. Nothing about hardware is claimed.

## Measured baseline (before editing)
CI discovery in this worktree: first run **121/122** (`occupancy_session_restart_test.sh` failed once; it passed standalone and on the immediate rerun, so it looks intermittent, cause not investigated). Rerun: **122/122 (sh via zsh, py via python3)**.

## Net `src/` lines (`git diff --numstat 1d1d5af -- src`)
| file | + | - |
|---|---|---|
| `Generalized-Core-Counter.cpp` | 2 | 0 |
| `time/Clock.cpp` | 15 | 3 |
| `time/Clock.h` | 1 | 0 |
| `time/ClockTrust.h` | 7 | 0 |
| `time/HibernateWakeDiagnostics.h` | 2 | 0 |
| **total** | **27** | **3** = **+24 net** (budget +24, at the limit) |

## Design and the two intents
- Cache: `carryStampCache` (a `time_t` epoch stamp, `Clock.cpp:157`, 0 until evaluated). `evaluateClockCarry()` (`Clock.cpp:164`) is the **only** `ExternalRtc::isSet()` call: `cache = isSet() ? persisted lastTimeSync : 0`. Called once in `setup()` immediately after the verdict zeroing (`Generalized-Core-Counter.cpp:760`), which is after the latch seed (`:670`).
- `isClockTrusted()` (`Clock.cpp:~490`) = current-boot sync trust `|| carryEligible()`; no `isSet`.
- **Intent a (24 h age bound):** `carryEligible()` (`Clock.cpp:158-162`) runs `ClockTrust::isCarryEligible` per call on the cached stamp with `Time.now()`: `0 <= now - stamp < 24 h`, plus `Time.isValid()` and plausibility of now and of the stamp. Arithmetic only, no I2C.
- **Intent b (immediate latch withdrawal):** the same call ANDs a live relaxed load of `ExternalRtc::oscillatorFaultSeenSinceBind()` (`Clock.cpp:161`); the existing withdrawal branch of `checkClockResync()` (`:366-368`) also clears the cache (same line as `set_lastTimeSync(0)` and `flushNow()`).
- Recompute points: write-back stamp (`:~417`: `carryStampCache = carryStamp`, which is `now` on a clean small confirmed write and 0 on every revoke; no I2C needed since the chip was just written and read back); withdrawal branch; verdict zeroing (the one setup evaluation reads the already-zeroed persisted value, so a failed RTC arm leaves no carry).

## Tests and mutations (`tests/clock_trust_carry_test.sh`)
New harness `run_cache` compiles the **real** `carryEligible()`/`carryStampCache`/`evaluateClockCarry()` text extracted from `Clock.cpp` with stubs: untrusted before the evaluation call with a persisted stamp present; trusted after; `isSet()` called once across many calls; age `24 h - 1 s` trusted, `24 h` untrusted, with no sync; future stamp untrusted; the latch withdraws with no loop pass; invalid `Time`; zero stamp (failed arm/revoke) and invalid external clock leave no carry. Text checks: `isClockTrusted()` has no `isSet`/evaluation; `isSet()` appears once in `Clock.cpp`; the cache is touched only by the withdrawal and write-back; the evaluation call is unique, in `setup()`, after the latch seed and after the verdict line. Round-1 tests kept (predicate table, write-back, C2 regression, verdict arms); only text expectations changed as the code forced.
**Mutations (20 total, all caught):** the round-1 nine plus the two verdict mutations (round-1 set unchanged in meaning, two retargeted to the new text) and the nine new: `isSet()` back in `isClockTrusted()`; evaluation call removed; evaluation first in `setup()` (before the seed); evaluation before the verdict zeroing; age not rechecked per call; cache not cleared at a revoke; cache not cleared in the withdrawal; live latch AND removed; evaluation per call.
Deviation: mutations run on a scratch copy of `src/` inside `build-tmp/wo-2026-11-05-007-stage6-r2/` (removed at exit), not backup/mutate/cp-restore on the real files; the working tree is never mutated. "Cache not cleared at a revoke" and "sync renews" share one assignment, so one mutation covers both.

## Verification
- CI discovery after the change: **122/122 (sh via zsh, py via python3)**. `clock_trust_carry_test.sh` exits 0 bare. `thermal_coupling_structural_test.py` passes in the worktree (scratch copies removed) and on a clean export of tracked files (exit 0).
- Build (Device OS 6.6.0, Boron, fresh `BUILD_PATH_BASE`, `compile-user`): **151356 / 1114 / 2264** (round 1: 151316 / 1114 / 2256; bss +8 = the `time_t` cache and its padding, text +40). `nm`: `evaluateClockCarry()` 0x38 bytes (T) linked; `isClockTrusted()` 0xCC bytes calling `Time.isValid`, `Time.now`, `observedTimeSyncedLastMs`, and **no** `hal_rtc_time_is_valid`; `hal_rtc_time_is_valid` is called from `evaluateClockCarry()` only; `carryStampCache` is an 8-byte `b` symbol.

## Deviations and open questions
- `src/time/Clock.h` (not in the authorized list) gets one declaration line, forced because `setup()` must call the evaluation; the budget counts it.
- `evaluateClockCarry()` calls `isSet()` even when the stamp is 0 (once per boot, and nowhere else); simpler than a guard and it fits the budget.
- The cache is renewed at a sync without re-reading the chip (the write-back just wrote and verified it); a cloud-sync rewrite of the chip stays the bench `skew 61` question (Amendment 2 item 5).
- The cached stamp holds the persisted value even when it is implausible or in the future; `carryEligible()` rejects those per call.
- The `:378` boot-storm read before `SystemConfig::setup()` sees the cache as 0 (untrusted), accepted by Amendment 2 item 5.
- Baseline flake noted above.
