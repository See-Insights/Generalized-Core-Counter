UNDER REVIEW: wo/2026-11-08-002-i21-clock-trust-carry @ 307274cbecef647d37bb69ba3ca610aba091b6e3

# NOT VERIFIED — Round 1 of at most 2

One source defect and one mutation-coverage defect go to the architect. No tracked files were changed.

| # | Judgment | Evidence |
|---|---|---|
| 1 | PASS | Cache starts at zero, is evaluated once at `Generalized-Core-Counter.cpp:760`, after S4 seed `:665-671` and verdict zeroing `:759`. `isClockTrusted()` is current-boot sync OR cached carry. `ExternalRtc::isSet()` occurs only in `evaluateClockCarry()`. Pre-evaluation callers are boot-storm trust at `:378`, the conditional invalid-storage initialization path `MyPersistentData.cpp:721-726 → :648`, and piece snapshot `Generalized-Core-Counter.cpp:506`; all see no carry. |
| 2 | PASS | Age is true at 24h−1s and false at 24h; future/implausible stamps are false. Forward jumps beyond the bound and backward jumps before the stamp withdraw trust. Returning into the valid interval re-enables it, matching Amendment 2’s literal stateless arithmetic and the existing test. The live relaxed S4 load withdraws current trust at the atomic-load linearization point. |
| 3 | **FAIL** | The failed-hibernate-arm zero is not flushed before potentially blocking work and other setup logic; see Finding 1. Other revokes use `set_lastTimeSync(0)` followed synchronously by `flushNow()`. A durable revoke survives a subsequent hibernate wake, and all zero consumers behave safely: `neverConfirmedSyncEver` forces connection, while status `lastSyncEpoch` and `ClockDiagnostics` report zero. |
| 4 | PASS | `carryStampCache` is read/written only by the application setup/loop thread. The Device OS `exrtc` worker writes only the atomic S4 latch. Cloud/ledger callbacks, publish-queue worker, ISRs, and watchdog callbacks do not call trust or access the cache. Plain `time_t` therefore conforms to the architect’s pre-ruling; no atomic cache is needed. |
| 5 | PASS | S1/S1b seed/bind precedes carry evaluation; S2 classification and evidence-byte structural blocks remain unchanged, with the boot-clear flush still at `:770`; S3 reason-20 power-off recovery can carry when the RTC retained time; S4 seed precedes evaluation. C1/C2 behavior is correct: an early boot straddler is untrusted, while a piece opened later under carried trust remains open if trust is withdrawn. |
| 6 | **FAIL** | Build and `122/122` host suite pass, but two of four independent mutations escaped the relevant tests; see Finding 2. |
| 7 | NOT VERIFIED | No hardware was available. The clean-restart, `skew 60/61`, physical fault, and clean-day bench steps were not run. The 60/61 step can discriminate the cloud-rewrite question only if the logged pre-write `correctionSec` and the following restart trust verdict are both checked; a near-zero correction is itself a bench failure. |

## Findings

1. **HIGH — failed-arm revocation can be lost across a reset**  
   `src/Generalized-Core-Counter.cpp:759-770`

   A `kRtcRead`, `kRtcBeforeZero`, or `kRtcOrder` verdict changes the in-memory stamp to zero at line 759. Line 760 then calls `evaluateClockCarry()`, including the external RTC validity operation, and the durable flush does not occur until line 770. If power loss, watchdog, or another reset happens in that interval, storage retains the old stamp. On the next boot the retained hibernate verdict can classify as `kResetReason` rather than an RTC arm, so the old stamp can be evaluated and carry can return.

   Smallest fix: make line 759 a one-line braced branch that calls both `set_lastTimeSync(0)` and `flushNow()` before `evaluateClockCarry()`. Keep the existing line-770 flush for S2’s evidence-byte consumption. This can preserve the exact +24-line budget.

   The current test at `tests/clock_trust_carry_test.sh:189-196` accepts any later flush within 1500 characters. My mutation inserting `System.reset()` after evaluation but before that flush passed the I21, S2, and watchdog-classification tests. Require the flush inside the failed-arm branch and before evaluation.

2. **MEDIUM — the correction producer is not protected by mutation tests**  
   `src/time/Clock.cpp:412-417`, `tests/clock_trust_carry_test.sh:134-197`

   The production expression is correct, but a mutation replacing every readable `rtcCorrectionSec` with zero passed both `clock_trust_carry_test` and `clock_resync_wiring_test`. The suite tests `carryStamp()` with ±threshold inputs but does not prove that `checkClockResync()` feeds it the measured `Time.now() - rtcPreWriteValue`.

   Smallest fix: add a source-driven harness or structural assertion for that producer expression, including a real large positive and negative correction reaching `carryStamp()`.

## Verification

- Independent mutations:
  - `< 24h` changed to `<= 24h`: **caught**.
  - Future-stamp guard removed: **caught**.
  - Readable correction forced to zero: **not caught**.
  - Reset inserted between failed-arm zero/evaluation and flush: **not caught**.
- Existing I21 helper: all 20 bundled mutations caught; bare helper exits 0.
- Host discovery: **122/122** — `.sh` via zsh, `.py` via python3. Neither known load-sensitive test failed.
- `thermal_coupling_structural_test.py`: PASS in the worktree and on a clean tracked-file export.
- Fresh Device OS 6.6.0 Boron build: **151356 text / 1114 data / 2264 bss**.
  - Versus integration head: **+272 / +0 / +8**.
  - Matches Stage 6 round 2 exactly.
- Source budget: `27 added − 3 removed = +24 net`, exactly at budget.
- `nm`: `isClockTrusted()` size `0xcc`; `evaluateClockCarry()` size `0x38`; cache is an 8-byte BSS symbol.
- `objdump`: `isClockTrusted()` contains no call to `hal_rtc_time_is_valid`; that call appears in `evaluateClockCarry()` only.
- Interpreter line: `/bin/zsh` 5.9; `/opt/homebrew/bin/python3` 3.14.6; ARM GCC 10.2.1; buildscripts 1.17.2.
- Model: `gpt-5.6-sol`; reasoning: `xhigh`.
- Hardware/flashing: not run and not authorized.