UNDER REVIEW: wo/2026-11-06-002-v50-s1b-pre-bind-rtc-access @ c0e4b27d8804f3c00d36506be47084652d3c3769

**NOT VERIFIED for S1b as scoped.**

Runtime behavior passes, including the round-1 High and Medium fixes. Two required persistence comments remain factually incorrect.

| # | Result | Evidence and judgment |
|---|---|---|
| 1 | **PASS** | **Finding 1 High is closed.** Under the Wire lock, CONTROL1 is read, WRTC alone is conditionally cleared, the vendored time is read, plausible time seeds only when `!Time.isValid()`, and saved CONTROL1 is always normalized to `(c1 \| 0x01) & ~0x40` when its initial read succeeded (`src/time/PreBindRtc.h:5-13`). The WRTC=1 valid-time and failed-bind retry cases pass. |
| 2 | **PASS** | **Finding 2 Medium is closed.** A failed time read still reaches the final CONTROL1 write and returns false (`PreBindRtc.h:11-13`); the caller logs failure and continues to bind (`Generalized-Core-Counter.cpp:297-299`). |
| 3 | **FAIL** | `Generalized-Core-Counter.cpp:657,667` are true, but `MyPersistentData.h:630-631` and `persist/CurrentReadings.h:46-47` incorrectly say `.load()` occurs “long before” Device OS/S1b seeds Time. Pre-bind seeding runs at `Generalized-Core-Counter.cpp:297`; `CurrentReadings::setup()` is later at `:494`, reaching `.load()` at `MyPersistentData.cpp:631-635`. |
| 4 | **PASS** | Wire lock/begin, unbound-only gating and order are unchanged. Objdump shows `status()` at `0xb5f14`, `PreBindRtc::run` at `0xb5f26`, and `enable()` at `0xb5f82`. Plausibility and `!Time.isValid()` remain at `PreBindRtc.h:12`; clock trust remains cloud-sync-recency-gated. Round-1 allowlists are unchanged. |
| 5 | **PASS** | Budget is **+21/−4 = +17 net**. Host suite is **117/117**: 66/66 shell via zsh and 51/51 Python via python3. The live-tree thermal scanner saw pre-existing round-1 scratch exports; its required tracked-only clean-export run passed. Device OS 6.6.0 Boron build passed at **153100 / 1114 / 2240 text/data/bss**. |
| 6 | **PASS** | No additional boot or L-handoff defect found. `PreBindRtc.h:2` identifies L’s raw-I²C replacement; L must preserve the exact lock, transaction order, saved CONTROL1 normalization and error aggregation. |

Failure branches were handled correctly:

- CONTROL1 read failure: no guessed final write; CONTROL1 is untouched and `run()` returns false. The vendored read may still succeed and safely seed, but failure is still propagated.
- WRTC-clear failure: the time read and saved final normalization are still attempted; return is false.
- Time-read failure: final normalization is attempted; return is false.
- Final-write failure: return is false, the warning is emitted, and bind is still called. WRTC may remain clear because the physical write failed; Device OS does not repair it, but there is no path that silently skips the mandated attempt or subsequent bind.
- The bitwise `&` is acceptable here: its operands are already-computed booleans except for the explicitly parenthesized `read && final-write`, so the write is unconditional with respect to `ok` and `timeOk`, without problematic side-effect ordering.

## Finding

**Low — two required comments state the boot order backwards.**

- **Files:** `src/MyPersistentData.h:630-631`; `src/persist/CurrentReadings.h:46-47`
- **What breaks:** The documentation says persistence loading precedes RTC seeding, although both bound Device OS seeding and S1b’s successful pre-bind seed happen before `CurrentReadings::setup()` and its `.load()`.
- **Smallest fix:** Rewrite those two two-line passages to say `.load()` runs after the pre-bind seed opportunity and only receives valid Time when Device OS or a successful plausible S1b read supplied it.
- **Budget:** Line-neutral; remains exactly **+17 net**.
- **Architect’s last-round ruling required:** either order this line-neutral correction before merge, or explicitly waive the two known-false temporal statements. Without that ruling, the binding specification’s comment requirement does not pass.

Mutations caught included the requested Copilot cases: WRTC clear removed, final write skipped after time-read failure, saved value not normalized, and final-write result ignored. Two fresh mutations were also caught: toggling WRTC with XOR instead of setting it, and moving final normalization before the vendored time read.

**Execution environment:** AGENT Codex · MODEL `gpt-5.6-sol` · REASONING `xhigh`. Interpreters: `zsh 5.9 (arm64-apple-darwin26.0)` for `.sh`; `Python 3.14.6` for `.py`. Build: Device OS 6.6.0, Boron, gcc-arm 10.2.1, fresh scratch build.

No hardware tests or flashing were performed. The Dev-14 `settime`, first-v50-boot, failed-bind retry and rollback benches remain Chip’s. The tracked checkout is clean at the reviewed SHA.