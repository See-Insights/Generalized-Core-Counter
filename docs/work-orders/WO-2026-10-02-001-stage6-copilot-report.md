<!-- Copilot claude-opus-5 (medium) Stage 6 Implementation Report, extracted verbatim from its --share transcript by Claude Code, 2026-10-02. Claude Code checked: archive file count unchanged (120584); only build-tmp/wo20261002-stage6/ created and removed. -->

# Implementation Report — WO-2026-10-02-001 (v32-RecoveryVisibility), Stage 6

Uncommitted working-tree diff left in place on `wo/2026-10-02-001-recovery-visibility`.

## Files and net `src/` code lines per item
(nonblank, non-comment; includes braces, declarations, includes)

| Item | Files | Net | Budget |
|---|---|---|---|
| **A** | `Generalized-Core-Counter.cpp` +3, `ConnectivityFailsafeTest.cpp` +4, `DailyBoundary.h` +1, `DailyBoundary.cpp` −2, `ConnectivityPolicy.h` 1/1 | **+6** | ≤ 8 |
| **B** | `Generalized-Core-Counter.cpp` −13 (stage-1 block −10, breadcrumb enum −1, label case −2); `nextStage` 1/1 in app and test-mode file | **−13** | net negative |
| **B** (test-build fix, counted separately) | `ConnectivityFailsafeTest.cpp` `"Config.h"`→`"../Config.h"`, `ConnectivityFailsafeTest.h` +`cloud/BatteryBackoffPolicy.h` | 2 lines | 2 |
| **C** | `State_Connect.cpp` | **+7** | ≤ 8 |
| **D** | `Generalized-Core-Counter.cpp` | **+5** | ≤ 10 |
| **E** | `observability/AwakeCycleCounters.h` (new, +4), `Generalized-Core-Counter.cpp` +3, `State_Sleep.cpp` +5 | **+12** | ≤ 12 |
| **Total** | | **+18** | ~45 |

`git diff --numstat` (src only): app 37/23, FailsafeTest.cpp 8/4, FailsafeTest.h 1/0, ConnectivityPolicy.h 1/1, State_Connect.cpp 12/2, State_Sleep.cpp 5/0, DailyBoundary.cpp 3/7, DailyBoundary.h 5/0, plus `AwakeCycleCounters.h` (new) and the four `bump_version.sh` files.

**Implementation detail (A):** the open-hours gate is folded into the existing guard — `if (Clock::openness() != Clock::Openness::Open || now <= ageBase) return;` — to stay inside budget; semantics are identical (both return). `ageBase = max(lastConnection, DailyBoundary::todayAt(openTime))`, `connectionAgeSec = now − ageBase`.

## Tests (4 new, 1 updated)

| Test | Kind | Covers |
|---|---|---|
| `connectivity_failsafe_open_hours_test.sh/.cpp` | behavioural host mirror + real-source fidelity | A. The three timings are parsed from `ConnectivityPolicy.h`, and the open gate / open base / first stage are parsed from the real supervisor body and injected as `-D` flags, so a `src/` mutation changes what Part 1 asserts. 14 behavioural checks + 6 fidelity checks. |
| `connectivity_failsafe_stage1_retired_test.py` | structural (asserting an absence) | B. Supervisor body extracted verbatim: no radio reset, no `nextStage == 1`, no `stage=1`; the real `nextStage` expression is evaluated for stage 0/1/2 → 2/2/3; breadcrumb 9 gone from `src/`; the 2-line test-build fix. |
| `conn_phase_modem_off_test.sh` | behavioural against real source + structural | C. The real enum, label and classifier are extracted verbatim and compiled on the host (6-row truth table); then: no non-diagnostic reference to `MODEM_OFF`, its time counts into `connPhaseCellMs`, only `CLOUD_ACQUIRE` drives a decision, `isOn()` sampled next to `ready()`. |
| `awake_cycle_counters_test.sh` | behavioural against the real header + structural | E. Real `AwakeCycleCounters.h` compiled and driven: cyc starts 1, slp 0, `cyc ≥ slp` after 1003 calls, a failed sleep moves cyc not slp. Then all 4 `System.sleep()` sites must be followed by `recordSleepReturn(<that call>.error() == SYSTEM_ERROR_NONE)`. |
| `report_payload_fields_test.py` | behavioural on the serialized bytes | D+E. Both real format strings rendered with per-field worst-case values and `json.loads`-parsed: `fh`/`lfb`/`cyc`/`slp` are unquoted numbers, `key1` is still the only string-valued key, max size vs `char data[256]`. |
| `daily_cleanup_boundary_test.py` | **updated** | It pinned `localTodayAt`, the exact name this WO renames. Changed to `DailyBoundary::todayAt`, and the header check now also requires the export. 9/7 numstat. This is the only existing test touched. |

### Mutations (each run, then restored byte-identically — verified by `shasum`)

| Mutation | Result |
|---|---|
| A1 restore 12 h stale | **caught** — "3 h of open hours fires the full reset" → stage 0 |
| A2 remove the open-hours base | **caught** — "06:00 wake after a 22:00 close does not reset" → stage 2 |
| A3 remove the open-hours gate | caught |
| B1 `nextStage = currentStage + 1` | caught |
| C1 classifier never returns `MODEM_OFF` | caught |
| D1 quote `fh` in both formats | caught |
| E1 drop one sleep-site counter | caught |
| E2 `slp` counts failed sleeps | caught |

## Commands and results

1. **Host suite** — `zsh` for every `tests/*.sh`, `python3` for every bare `tests/*.py`: **51/51 before**, **56/56 after** (5 new files, 4 of them tests + 1 `.cpp` harness). `tests/publish_with_ack_structural_test.py` passes **unchanged** (no diff).
2. **ARM builds** (boron, Device OS 6.4.1, `make clean-user` then `compile-user`):
   - **release: 150612 / 1090 / 2204** vs v31's 150420 / 1090 / 2204 → **+192 text, +0 data, +0 bss**. `strings` finds `v32-RecoveryVisibility` (1), `MODEM_OFF` (1), and **no** `radio-reset` or `CONN_FAILSAFE`. Rebuilt at the end, so `target/` holds the release image.
   - **failsafe test mode** (`EXTRA_CFLAGS=-DCONNECTIVITY_FAILSAFE_TEST_MODE=1`, after `clean-user`): **compiles**, 152180 / 1090 / 2204.
3. **Linkage** (`arm-none-eabi-nm -C -S`): release ELF has `HAL_Core_Runtime_Info` (T), `AwakeCycles::recordSleepReturn(bool)` (W), `AwakeCycles::cycles` and `::sleeps` (V, distinct addresses), `DailyBoundary::todayAt(unsigned char)` (T). The user object `Generalized-Core-Counter.o` shows `U HAL_Core_Runtime_Info`, `U DailyBoundary::todayAt`, `U Clock::openness()` — the new paths are really called, not merely present.
4. **Maximum payload** (worst-case per field type, incl. NUL):

| Format | Before | After | Buffer |
|---|---|---|---|
| occupancy | 169 | **228** | 256 |
| counting | 158 | **217** | 256 |

## Deletions

Deleted only `build-tmp/wo20261002-stage6/`. `build-tmp/`, `build-tmp/connectivity-archive/` and `build-tmp/WO-2026-10-01-001-stage6-copilot-transcript.md` are untouched.

## Deviations

1. **Alert 45 (`CONNECTIVITY_FAILSAFE_ALERT`) was kept.** B says remove it "if nothing else uses them". After stage 1 is gone it can never be *raised*, but `clearConnectivityFailsafeRecovery()` still reads it to clear a stage-1 alert persisted by pre-v32 firmware. Removing it would strand that alert on an upgraded device, so the condition is not met. `BREADCRUMB_CONNECTIVITY_FAILSAFE` (9) *was* removed — it had no other reader; its number is left reserved in a comment, and value 9 reported by an older image now renders as `?`.
2. **`if (nextStage >= 2)` is now always true** in two places (the jitter addition and `lowBatteryHardActionBlocked`). The expressions remain correct, so they were left as-is rather than simplified; simplifying would add diff noise beyond "remove what becomes unused".
3. **Edge case recorded, not designed around:** for a 24-hour site (`openTime == closeTime != 0`), `todayAt(openTime)` can be in the future during the pre-opening part of the local day; `now <= ageBase` then defers. This is the literal consequence of the WO's "later of lastConnection and the start of today's open period" and was not altered.
4. **One existing test updated**, as permitted: `tests/daily_cleanup_boundary_test.py` pinned the name `localTodayAt`, which item A renames to `DailyBoundary::todayAt`.
5. **`std::max` avoided** in favour of a ternary, to not add an `<algorithm>` include against A's budget.

## Model

`claude-opus-5`, reasoning **medium** — as dispatched.

---

