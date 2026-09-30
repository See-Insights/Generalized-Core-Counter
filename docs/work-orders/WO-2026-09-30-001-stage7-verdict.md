**Overall: NOT VERIFIED — size-budget finding only.** The behavioral checks, mutation check, host suite, and ARM build passed.

| Check | Verdict | Evidence |
|---|---|---|
| 1. Equivalence | **PASS — VERIFIED WITH NOTES** | All **8 acceptance-model calls** matched compiled v27 and new logic for `due` and `boundary`, including untrusted time. `localTodayAt()` and the due calculation are byte-identical. The report tail is unchanged: publish at `boundary - 1` → `dailyCleanup()` → `set_lastDailyCleanup(now)`. The log text and item-A `else if (due)` trigger are preserved. The log adds one read of the existing, side-effect-free getter. |
| 2. Five routes | **PASS — VERIFIED** | Compiled extraction passed each route listed below. |
| 3. No loop | **PASS — VERIFIED** | For every route, the report stamped `lastDailyCleanup = now`; the next check returned not-due and the next sleep pass reached HIBERNATE. |
| 4. Untrusted clock | **PASS — VERIFIED** | Returned not-due with `boundary = 0`; `Unknown` openness selected the short-nap path without a report detour. |
| 5. Mutation | **PASS — VERIFIED** | Removing the new guard made `close_before_night_sleep_test.py` fail with exit 1. Restored in place, SHA-256 matched, and the test passed again. |
| 6. Suite | **PASS — VERIFIED** | **45/45:** 22 shell tests using **zsh**, 23 Python tests using **python3**. `publish_with_ack_structural_test.py` is byte-identical to base and green. |
| 7. Identity/build/size | **FAIL — NOT VERIFIED** | Identity and ARM build passed; the source-size accounting does not support the claimed budget compliance. |

The route check used a **faithful compiled extraction**, with hardware calls stubbed and preceding sleep gates assumed released—not a full firmware host harness. It compiled the route branches, shared sleep decision through HIBERNATE, and report publish/cleanup/stamp sequence. Each reached `REPORTING_STATE` with `"close due before night sleep"` before any `onEnterSleep()`, `secondsUntilNextOpen()`, or HIBERNATE call:

- Connection timeout: **PASS**
- Pre-boundary report finishing its connection after close: **PASS**
- Idle park-closed: **PASS**
- Closed-hours motion wake, `sleep-pir-return-to-sleep`: **PASS**
- Occupied at close in intermittent mode: **PASS**

**Size finding:** excluding blank lines, comments, and the three identity/version lines, the physical source delta is **+35 code lines**:

| File | Net code lines |
|---|---:|
| `State_Report.cpp` | −22 |
| `State_Sleep.cpp` | +5 |
| `DailyBoundary.cpp` | +39 |
| `DailyBoundary.h` | +13 |
| **Total** | **+35** |

Stage 6’s **26** is reproducible only by matching additions against the entire old report file, crediting nine lines as moved although their originals remain. That undercounts additions against the WO’s approximately 25-line budget. The actual due logic moved unchanged.

The other modified tests are permitted adaptations: clock-trust owner path, sleep-branch nesting check, and transition count **14 → 15**. The acceptance model is unchanged; no unauthorized test changes were found.

The scratch **Boron / Device OS 6.4.1 release build**, after successful Workbench `make clean-user`, produced:

| text | data | bss |
|---:|---:|---:|
| 150,308 | 1,090 | 2,196 |

The binary contains **`v28-CloseBeforeSleep`**, product version **28**, and **no `pdiag`**. ELF disassembly confirms both production handlers call `DailyBoundary::check()`.

**Restoration confirmed:** all **1,323 original files** matched their initial hashes after scratch cleanup; Git diff, status, branch, and HEAD remained identical.

**Model / reasoning used:** `gpt-6-astra`, **high**.