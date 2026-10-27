VERIFIED WITH CONCERNS  
High: none. Medium: none.  
Low: backward-clock latency edge; two cited evidence reports are absent; test growth is near the guideline’s flag threshold.

### 1. Change scope — PASS

Command: `git diff -- src/`, `rg -n 'sleepPlan\\(|signalLED...' src`, and line-by-line inspection.

- Production changes are limited to `src/state/State_Sleep.cpp:40` and the occupied OCCUPANCY branch at `:607-620`.
- COUNTING remains unchanged at `:592-605`, including its wake close at `:1057-1060`.
- `updateOccupancyState()` and `processSleepWakeEvent()` remain unchanged at `src/state/State_Modes.cpp:127-184`.
- `closeOccupancySessionSafely()` is unchanged at `src/state/State_Common.h:333-380`.
- `signalLED*` and retained `ledOffTime` are unchanged at `src/device_pinout.cpp:95-143`.
- No persisted/retained field or other log text changed.
- The only production caller is `handleSleepingState()`: it obtains the plan at `State_Sleep.cpp:1240-1244`, then passes the value to hibernate/heap/sleep processing at `:1246-1278`. No caller depends specifically on the old full-debounce value.
- Source size: `+7/-3`, net `+4`; `State_Sleep.cpp` 1275→1279 lines.
- `git diff --check` passed.

### 2. Wake after deadline — PASS

Commands: source trace with `nl`; Device OS search with `rg`; ELF disassembly with `arm-none-eabi-objdump -d -C`.

- `signalLED()` sets `ledOffTime = Time.now() + durationMs/1000` at `device_pinout.cpp:97-105`; non-multiples truncate downward. For example, 300999 ms produces a 300-second deadline, never a later one.
- Remaining time is the integer difference `ledOffTime - Time.now()` at `:124-139`.
- `State_Sleep.cpp:613-619` requests `remaining+1`; after that timer elapses, `Time.now()` is beyond the stored integer-second deadline and remaining is zero.
- Software leaves the LED on: the open-hours occupied branch contains no `signalLED(false)`. Timeout-driven clearing occurs only from Idle at `State_Idle.cpp:42-43`, after the sleeping-state wake processing.
- `processSleepWakeEvent()` therefore sees remaining zero and LED high, and closes at `State_Modes.cpp:165-184`.
- ULP and both STOP paths receive `wakeInSeconds*1000` at `State_Sleep.cpp:780`, `:867`, and `:882`.
- Device OS 6.6.0 stores the supplied milliseconds unchanged at `system/inc/system_sleep_configuration.h:310-329`. Boron ULP and STOP share `enterStopBasedSleep()` at `hal/src/nRF52840/sleep_hal.cpp:1523-1527`; RTC2 uses `ms/125` ticks at `:458-480`. Whole-second app durations are exact multiples of 125 ms, so no rounding is lost.
- The 75 ms serial drain at `State_Sleep.cpp:43-48` occurs before `System.sleep()` and only makes the actual wake later. A ULP failure followed by STOP likewise cannot make the STOP wake earlier.
- A timer wake with one second still remaining does not close; the next plan requests two seconds. It converges with at most one extra wake, two seconds of requested sleep, and its normal awake/teardown overhead.
- The +1 margin is sufficient for both ULP and STOP’s software rounding. No bench timing claim was made.

### 3. Fallback cases — PASS

Command: `nl -ba src/state/State_Sleep.cpp`, `device_pinout.cpp`, `Config.cpp`, and `Config.h`.

- Invalid clock, expired deadline, and `ledOffTime==0` all return zero at `device_pinout.cpp:124-138`.
- Zero remaining takes the exact old fallback: `min(debounceSeconds,wakeInSeconds)` at `State_Sleep.cpp:617-618`.
- A valid but untrusted clock can still provide a relative LED deadline because this helper intentionally uses `Time.isValid()`, not `Clock::isTrusted()`. Following that existing deadline is more accurate unless the clock steps; see check 4.
- With `setting1==0`, `Config::occupancyDebounceMsForRuntime()` returns the current 60000 ms default because zero makes the config invalid (`Config.cpp:94-103,145-151`; `Config.h:34`). Thus the LED and plan both use 60 seconds today. Following elapsed remaining time is more correct; the intentional +1 margin may make the initial request 61 rather than 60 seconds.
- `setting1` values 1–999 have a pre-existing zero-second sleep-plan fallback, although normal awake arming substitutes 60 seconds at `State_Modes.cpp:73-82`. With valid time, the new remaining-time path improves that case; with an invalid clock it remains no worse than today.

### 4. Backward-clock edge — PASS WITH LOW CONCERN

Commands: source trace plus `rg -n 'handleOccupancyMode|updateOccupancyState' src`.

- A backward `Time.now()` step can inflate `remaining` beyond the configured debounce.
- `min(remaining+1,wakeInSeconds)` still bounds sleep by the next reporting boundary (`State_Sleep.cpp:564-589,613-619`).
- If that report is after the real debounce, the same loop reaches centralized `handleOccupancyMode()` at `Generalized-Core-Counter.cpp:1224-1234`; its `millis()`-based `updateOccupancyState()` closes the session at `State_Modes.cpp:127-162`. If the first report is legitimately before the debounce, closure occurs at the first later awake/report opportunity.
- Severity: Low. A rare backward step can defer closure and over-credit up to the boundary, but it cannot create an unbounded sleep.
- No cap is needed for this WO. A cap would conflict with the approved rule and add unnecessary wakes; stricter clock-step handling should be a separate WO.

### 5. Short sleeps — PASS

Command: `nl`/`rg` over `State_Sleep.cpp`, `SleepTeardown.cpp`, `HibernateEligibility.cpp`, `ThrashGuard.cpp`, and `ConnectivityPolicy.h`.

- Teardown completes before the plan is calculated (`State_Sleep.cpp:1234-1242`), so remaining time is sampled after teardown rather than becoming stale during it.
- Short occupied sleeps occur only in open/unknown-clock handling. Closed handling turns the LED off at `:505-518`, so the new deadline path does not select hibernate.
- Hibernate requires overnight fallback and at least 900 seconds (`State_Sleep.cpp:689-756`; `HibernateEligibility.cpp:54-73`).
- `nightlyHeapGuard()` acts only on overnight fallback (`State_Sleep.cpp:630-658`).
- ULP and STOP accept any nonzero timer duration; there is no minimum-sleep or “skip below N” rule.
- Watchdogs are paused immediately before sleep and restored immediately after wake (`:782-785`, `:923-928`). `SLEEP_ATTEMPT` and `WAKE_FROM_SLEEP` mark ThrashGuard progress at `:814` and `:927-929`.
- Debug serial can add 500 ms plus an optional bounded 30-second wait only when enabled (`:940-958`; `ConnectivityPolicy.h:255-271`).
- A deadline wake normally closes the session, so it does not create a self-sustaining 1–6 second loop. Repeated short wakes require external PIR rearming or an external early-wake fault. There is no new modem-teardown storm, watchdog/reset path, or sustained power problem; the bounded extra wake does have ordinary wake/setup energy cost.

### 6. `Wake:` / `led=` consumers — PASS

Command: `rg -n 'Wake:.*led|led=|parse.*led' src tools tests docs release`, excluding generated HTML/fixtures where appropriate.

- `src/state/State_Sleep.cpp:35-40` is the producer.
- No source, tool, alert logic, or fleet parser consumes this field.
- `tools/exerciser_baseline.py:27-57,101-143` reads report-hook JSON fields, not serial `Wake:` lines.
- Tests contain fixtures and the new direct test, but no production consumer depends on zero.
- There is no `release/` directory in this checkout.
- Strictly, the old expression was zero for ordinary sub-1000-second debounce values; for longer values it was still wrong by a factor of 1000 rather than always zero.

### 7. Fixture diff — PASS

Commands: `git diff -- tests/fixtures/sleep_plan_transcript.txt` plus an independent Python scenario parser comparing HEAD and working tree.

- Old/new scenario totals: 88→98.
- After removing exactly one added `CALL signalLEDTimeRemaining` from each S55–S67, every S1–S80 scenario was byte-identical.
- No scenario was removed; only release S81–S90 were appended.
- Header counts and body SHA were updated at `tests/fixtures/sleep_plan_transcript.txt:1-10`.
- Hand-checked outputs:
  - 300: boundary-nearer 300, farther 301.
  - 120: 120, 121.
  - 5: 5, 6.
  - 1: 1, 2.
  - 0: boundary 100, fallback debounce 300.
- These match `min(remaining+1,boundary)`, or `min(300,boundary)` at zero.

### 8. Test authenticity and independent mutations — PASS

Commands: source inspection; two `git worktree add --detach` worktrees; each run with `SLEEP_PLAN_SRC_ROOT=<mutant>/src zsh tests/sleep_plan_equivalence_test.sh`.

- The plan is marker-extracted at `sleep_plan_equivalence_test.sh:37-62`; a fresh re-extraction must match the compiled copy or emits `COPY_MISMATCH` at `:161-168`.
- Expected wakes are independently computed at `sleep_plan_equivalence_test.cpp:205-216`; mismatch handling is at `:138-140`.
- `wake_summary_led_test.sh:15-40` extracts and byte-compares the real function; `wake_summary_led_test.cpp:27-43` drives 150, 0, and 3600 seconds. The old division is behaviorally rejected at `wake_summary_led_test.sh:42-56`.
- The three structural edits require the exact new text once and map only that text back to the base: `sleep_plan_structural_test.py:43-55,140-150`; `sleep_preflight_structural_test.py:275-291`; `sleep_wake_structural_test.py:187-197`.
- Mutation 1, `remaining - 1`: caught behaviorally with wakes 299/119 and `EXPECT_MISMATCH`; exit 1.
- Mutation 2, replacing min with max: caught behaviorally with wakes 301/1300/1120 and `EXPECT_MISMATCH`; exit 1.
- The original repository’s Git metadata was read-only, so the detached worktrees were created from a scratch Git repository initialized from `git archive HEAD`. Both were genuine `git worktree add --detach` worktrees with `.git` linkage, not plain copies.

### 9. Host suite — PASS

Command: serial loop over `tests/*_test.sh` via `zsh`, then `tests/*_test.py` via `python3`.

Result: **105/105** — 60 shell and 45 Python tests; no failures.

### 10. ARM build and linkage — PASS

Command: the workflow’s Device OS 6.6.0 Boron `make -s` command with separate fresh `BUILD_PATH_BASE` directories and literal `EXTRA_CFLAGS=-frandom-seed=$$@`; before tree from `git archive HEAD`.

| Build | text | data | bss | `.backup` |
|---|---:|---:|---:|---:|
| Before | 151572 | 1114 | 2200 | 152 |
| Candidate | 151580 | 1114 | 2200 | 152 |

Commands: `arm-none-eabi-size`, `size -A`, `nm -C -S --size-sort`, and `objdump -d -C`.

- `logWakeSummary(char const*)`: 72→64 bytes.
- `handleSleepingState()`: 4768→4784 bytes.
- One compiler-local switch symbol renumbered with unchanged 64-byte size; no other symbol-size delta.
- `signalLEDTimeRemaining()` remains linked at 60 bytes.
- `sleepPlan()` is inlined and has no ELF symbol.
- Before ELF has two `handleSleepingState()` calls to `signalLEDTimeRemaining()`; candidate has three. The new occupied call is followed in disassembly by `adds r0,#1`, comparison against planned wake, and conditional minimum selection.
- This proves the production chain `handleSleepingState` → inlined occupied plan → `signalLEDTimeRemaining` → sleep duration is linked.
- `.backup` unchanged proves no retained-memory addition.

### 11. Alerts — PASS

Commands: `git diff -- src`, `rg -n 'raiseAlert|Alert 15|Alert 16'`, and inspection of `docs/reference/alert-codes.md:25-56`.

The patch adds/removes/changes no alert raise, severity, or clear path. `docs/reference/alert-codes.md` needs no update.

### 12. Ship notes

- The v47 soak remains the hardware acceptance test: hour-straddling sessions should return to about 570 seconds as specified at `WO-2026-10-25-001-debounce-sleep-uses-remaining-time.md:46`.
- Non-fixture tests grew by 189 lines against the +100 guideline—about 1.9× and therefore a Low budget concern, though the growth is traceable to independent expectations, copy checks, structural normalization, and the new diagnostic test.
- No source edit is required. The backward-clock cap should not be added in Stage 6 for this WO.

### Concerns and resolution

- **Low — backward valid-clock step:** close may wait until a reporting/awake opportunity. Resolve only through a separate clock-step WO if stricter latency is required; no cap recommended here.
- **Low — missing evidence documents:** `docs/work-orders/2026-10-25-item-20-hour-straddler-step0-report.md` and `docs/work-orders/2026-10-24-v45-baseline-day1-result.md` are absent. Restore them or correct the dispatch paths before the closing record.
- **Low — test budget:** +189 non-fixture lines is near twice the +100 guideline. Resolve by explicit acceptance in the closing record or test-only consolidation; it does not weaken verification.

### Could not verify

- The two missing field-evidence reports above.
- Particle cloud compilation, bench timing/current, and fleet behavior: excluded by this dispatch; the controller runs cloud compilation separately and the v47 soak supplies hardware evidence.

`src/` hash before: `1c5659d45f8544bee15dcc5665dc64f0811b6270aca3a0730a4d88d02e186dc8`  
`src/` hash after: `1c5659d45f8544bee15dcc5665dc64f0811b6270aca3a0730a4d88d02e186dc8`  
`build-tmp/wo20261025-001-stage7/` was removed.