**Overall: NOT VERIFIED** — one required regression-test invariant is incomplete. The current firmware passes the leak and OOM behavior checks.

**Finding — P2:** [sleep_config_ownership_structural_test.py:283](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/sleep_config_ownership_structural_test.py:283) detects only assignments spelled `= SystemSleepConfiguration(...)`. Inserting `ulpConfig = {};` after GPIO/network allocation passes both repository sleep checks. This invokes the same defective move assignment covered by the WO; the real-header allocator harness confirms it leaks.

Smallest fix: reject assignments to discovered configuration variables regardless of the right-hand syntax, and add this mutation. A temporary test-only correction passed the current source and caught the mutation. Nothing was applied permanently.

| Check | Verdict | Evidence |
|---|---|---|
| **1. Configuration ownership** | **FAIL — NOT VERIFIED** | Current source has five block-scope configurations, no global/static/extern objects, and no reassignment. Global, static, extern and explicit-constructor assignment mutations were caught. Brace assignment was missed, as described above. |
| **2. Leak check** | **PASS — VERIFIED** | Real, unmodified Device OS 6.4.1 header; **5,000 cycles per site**, including ULP standby off/on: **0 bytes, 0 blocks lost**. Old-pattern mutation loses **80 / 144 / 112 / 32 B/cycle** for hibernate / ULP standby / STOP GPIO / STOP timer. Preprocessing confirms `HAL_PLATFORM_RTL872X=0` for both GCC and Boron, selecting the same `new`/`delete` branches. |
| **3. Wake sources and sequence** | **PASS — VERIFIED** | Entire executable `State_Sleep.cpp` text matches `43b8a69` after normalizing only configuration ownership and receiver names. Modes, pins, edges, duration, standby guards, pre-sleep order, breadcrumbs, logging and `recordSleepReturn()` are preserved. |
| **4. Lifetimes** | **PASS — VERIFIED** | Every configuration encloses its sleep call and all references remain within its scope. Hibernate failure handling and fall-through are unchanged. |
| **5. Direct OOM reset** | **PASS — VERIFIED** | Extracted production block executes **log(size) → delay(100) → reset(7)** across all **256 reset-count values**, with no action for `outOfMemory=-1`. Structural mutations for missing, zero, reused and absent reset codes fail. |
| **6. Other behavior** | **PASS — VERIFIED** | Boot clear is byte-identical; entire `MyPersistentData.cpp` is unchanged. Removing case 14 is the only executable change in `State_Error.cpp`. Stored 14 reaches `default`, then Idle—explicitly accepted by the latest Stage 5 decision. |
| **7. Old OOM route mutation** | **PASS — VERIFIED** | Restored route compiles, then fails: **“issued 0 reset(s), expected exactly 1.”** The reset structural test also rejects it. |
| **8. Suite, build, linkage, budgets** | **PASS — VERIFIED WITH NOTES** | **60/60:** 31 shell tests with zsh, 29 Python tests with python3. WITH_ACK test unchanged and green. Breadcrumb regex retains the four-call count and ordering checks. Build/linkage details below. |

Fresh serial Boron release build, Device OS 6.4.1:

- **text 150,556 / data 1,090 / bss 2,180**; binary **151,650 bytes**.
- `strings`: **v34-SleepConfigLeak**; product section: **34** (`22 00`).
- DWARF identifies all five configuration objects as stack locals; none occupy `.bss`/`.data`.
- Five sleep calls and six destructor calls on sleep-handler paths.
- OOM disassembly: `movs r0, #7` immediately before `SystemClass::reset`.
- Fresh serial build succeeded with five warnings from unchanged files. The initial parallel attempt encountered a make dependency-ordering failure.

Counting nonblank, noncomment physical `src/` lines: **A −2** against about +12; **B −7** against about +5. Version changes are net zero.

**Integrity confirmed:** SHA-256 and entry-set comparison found **zero changed, added or removed pre-existing entries** across 125,163 entries. Git status is identical. Nothing was created or deleted outside `build-tmp/wo20261002-003-stage7/`; that directory was removed. `build-tmp/` and all **120,584 archive files** remain intact.

**Model/reasoning used:** `gpt-6-astra`, **high**.