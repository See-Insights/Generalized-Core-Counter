UNDER REVIEW: wo/2026-11-06-002-v50-s1b-pre-bind-rtc-access @ 89260c94544827da2b476944140158e268bb496c

NOT VERIFIED for S1b as scoped.

The normal v49 migration path works, but a valid RTC with WRTC=1 can still be overwritten with year 2000 after a failed binding attempt.

| # | Result | Evidence and judgment |
|---|---|---|
| 1 | **FAIL** | WRTC is correctly set before `enable()` at `src/time/PreBindRtc.h:9-10` and `src/Generalized-Core-Counter.cpp:297-298`. A real v49 chip with WRTC=0 is seeded. However, the time-overwrite defect is not closed for valid WRTC=1 states because the vendored read is gated at `lib/AB1805_RK/src/AB1805_RK.cpp:317-320`. |
| 2 | **FAIL** | `getRtcAsTime()` succeeds for the fleet’s v49 WRTC=0 state. With WRTC=1 it returns false, while the CONTROL1 write still runs. A cold WRTC=1 chip normally has no trustworthy time, which is acceptable. A valid WRTC=1 chip—or a second unbound boot after the first binding attempt failed—can instead reach Device OS binding with internal year 2000, which is copied to the chip at `exrtc_hal.cpp:236-248`. That residual destructive path is not acceptable. |
| 3 | **PASS** | The recursive Wire lock spans the complete operation at `src/time/PreBindRtc.h:5-10`; `Wire.begin()` is conditional at line 6. `TwoWire` initializes its recursive mutex in `spark_wiring_i2c.cpp:34-42`; nested library locks are safe (`i2c_hal.cpp:331-343,768-788`). Device OS skips its accepted reset because Wire is enabled (`am18x5.cpp:490-498`), and `enableInterfaceIfNeeded()` is compatible (`:1323-1328`). |
| 4 | **FAIL** | Gating and order are correct: one call, only when unbound, before `ensureBound`, and no reset (`Generalized-Core-Counter.cpp:297-298`). `objdump` showed `status` at `0xb5ee8`, `PreBindRtc::run` at `0xb5efa`, and `enable` at `0xb5f56`. But a failed calendar read is discarded at `PreBindRtc.h:8`; if CONTROL1 succeeds, `run()` returns true and no failure is logged. |
| 5 | **PASS** | Seeding requires a successful read, `Clock::isPlausibleEpoch()` and invalid system time (`PreBindRtc.h:7-8`; plausibility is 2024–2035 at `Clock.cpp:549-552`). `Time.setTime()` updates the internal clock and emits the ordinary asynchronous manual-time-change event (`spark_wiring_time.cpp:284-295`). Trust remains false because `isClockTrusted()` also requires a nonzero confirmed cloud-sync timestamp (`Clock.cpp:475-477`, `ClockTrust.h:289-298`). |
| 6 | **FAIL** | The six allowlist edits are exact-path additions and do not weaken the structural checks (`clock_owner_structural_test.py:153`; five sleep tests at lines 206, 139, 212, 248 and 179). The comments are not all correct: Amendment 2’s requested `Generalized-Core-Counter.cpp:657` comment was left unchanged while line 667 was edited instead, and the comments at `MyPersistentData.h:631`, `CurrentReadings.h:47`, and `Generalized-Core-Counter.cpp:667` overstate seeding for WRTC=1/read-failure paths. |
| 7 | **PASS** | Source budget is **+17/−3 = +14 net**, including the new 12-line `PreBindRtc.h`. Host suite: **117/117**—66 shell tests via zsh and 51 Python tests via python3. The clean-export thermal test passed. All 17 supplied mutants were caught; independently, removing plausibility and changing the CONTROL1 read/write `&&` to `||` were both caught. Device OS 6.6.0 build passed at **153052 / 1114 / 2240** text/data/bss. |
| 8 | **PASS** | No other boot, time, layout, retained-state, or payload behavior changed in the diff. The vendored time read and CONTROL1 access are localized at `PreBindRtc.h:8-10`, with the L replacement explicitly documented at line 2, so conversion to raw I²C remains straightforward. |

## Findings

1. **High — valid WRTC=1 time can still be destroyed.**  
   **Evidence:** `src/time/PreBindRtc.h:8`; `lib/AB1805_RK/src/AB1805_RK.cpp:317-320`; Device OS `rtc_hal.cpp:37-38` and `exrtc_hal.cpp:236-248`.  
   **What breaks:** After S1b sets WRTC=1, a failed bind followed by another unbound boot prevents reseeding. If the next bind succeeds, Device OS writes internal year 2000 over the still-good chip time.  
   **Smallest fix:** Under the existing lock, read CONTROL1 first; if WRTC is set, temporarily clear only WRTC, call the vendored time reader, then always write the saved normalized `(c1 | 0x01) & ~0x40`. Propagate every transaction result. This requires an architect-approved adjustment to Amendment 2’s operation order but can be compacted into a 13-line header, taking the total to exactly **+15 net**. Add a WRTC=1 valid-time/retry test.

2. **Medium — calendar-read failure is not logged.**  
   **Evidence:** `src/time/PreBindRtc.h:8-10`, `src/Generalized-Core-Counter.cpp:297`.  
   **What breaks:** A failed time-register read followed by a successful CONTROL1 RMW returns success, violating the failure-log contract and hiding why no seed occurred.  
   **Smallest fix:** Retain the time-read result and return false after completing the mandatory CONTROL1 write when that read failed. This fits within the High fix’s **+15** implementation.

3. **Low — required comments remain inaccurate.**  
   **Evidence:** `src/Generalized-Core-Counter.cpp:657,667`; `src/MyPersistentData.h:631`; `src/persist/CurrentReadings.h:47`.  
   **What breaks:** Documentation promises first-boot seeding more broadly than the implementation provides and attributes it to `ensureBound()` rather than the pre-bind seed.  
   **Smallest fix:** Make the three required comments conditional on a successful plausible pre-bind read and correct the attribution. Line-neutral; no budget cost.

## Verification environment

AGENT: Codex · MODEL: `gpt-5.6-sol` · REASONING: `xhigh`

Interpreters: `zsh 5.9 (arm64-apple-darwin26.0)` for `.sh`; `Python 3.14.6` for `.py`.

Build: Device OS 6.6.0, Boron, gcc-arm 10.2.1, buildscripts 1.17.2, fresh `BUILD_PATH_BASE` passed as a make argument. Binary: **153052 text / 1114 data / 2240 bss**.

I could not run flashing or hardware tests. In particular, the Dev-14 `settime <epoch> hal`, first-v50-boot, and rollback benches remain Chip’s. The reviewed checkout remains clean at the exact SHA.