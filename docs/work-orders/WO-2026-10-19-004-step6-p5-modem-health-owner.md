# WO-2026-10-19-004: Step 6 P-5 — one owner for modem health and the standby latch

**Author:** architect, 2026-10-19 · **Status:** ready (citation re-check done 2026-10-19, below)
**Branch:** `wo/2026-10-19-004-step6-p5-modem-health`, from main **after the P-4 PR merges**
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-15-003-step0-report.md` Q1 group "(b) `power/Connectivity`" (Sleep), Q3 row "Connectivity/modem" ("all writers must route through the owner"), closing table WO 5. Cited at `71e346f`; re-cite everything on the new base.

## Plain goal

Today three files write the standby-latch state (`session.modemUnstable`, `modemUnstableReason`, `modemStandbySuppressed`, `modemConnectTimeoutCount`), and one of them duplicates another's logic with a magic number. Give that state **one owner**, so the planned latch fix (recovery plan, after measured teardown traces) has a single place to change. **No behavior change: the same decisions, the same log lines, character for character.**

## What exists today (architect, at `71e346f`; re-check)

- **`State_Sleep.cpp`:** the latch thresholds (`:31–32`) and `ModemUnstableReason` enum (`:34–38`); `markModemUnstable(reason, elapsedMs)` (`:103`) and `maybeClearModemUnstable(teardownMs)` (`:129`); plus the labels `connectionModeLabel` (`:40`), `batteryTierLabel` (`:55`) and `logTeardownContext` (`:79`).
- **`State_Connect.cpp`:** `markModemUnstableFromConnectTimeout()` (`:150–167`) re-implements `markModemUnstable(CONNECT_TIMEOUT)` with `session.modemUnstableReason = 2` and the same two log lines; threshold `MODEM_UNSTABLE_CONNECT_TIMEOUT_THRESHOLD = 2` (`:24`); the count reset on success (`:612`).
- **`State_Report.cpp`:** the daily-close clear (`:83–91`, v41's standby-latch fix), inline.

## Design (do not widen)

1. A new `power/ModemHealth.{h,cpp}` owns the thresholds, the enum and the four state writes. It moves the two Sleep functions **byte-for-byte**, and exposes:
   - `markModemUnstable(reason, elapsedMs)` and `maybeClearModemUnstable(teardownMs)`, as today;
   - `noteConnectTimeout()`: increment the count (saturating at 255) and, at the threshold, call `markModemUnstable(CONNECT_TIMEOUT, 0)`;
   - `noteConnectSuccess()`: reset the count (the `:612` write);
   - `clearAtDailyClose()`: today's `:83–91` block, with its log line unchanged.
2. `State_Connect.cpp` and `State_Report.cpp` call those instead of writing the fields. The Connect duplicate and its magic `2` go away.
3. **Proof of no behavior change:** every log line (`MODEM_HEALTH: unstable reason=…`, `MODEM_POLICY: standby temporarily disabled reason=unstable_modem`, `MODEM_HEALTH: recovered`, `MODEM_POLICY: standby restored`, `MODEM_POLICY: standby re-allowed at daily close; night modem-off decides`) is byte-identical, and each fires under exactly the same conditions as today. Show the old Connect path and the new `noteConnectTimeout()` path give the same state and the same log output for counts 0, 1, 2, 3 and 255, with the latch already set and not set.
4. The three labels and `logTeardownContext` move with the rest, unchanged. If the budget is tight, leave them in Sleep and say so.
5. `session` stays where it is (`StateMachine.h`); only the writers move.

## Budget

- **`src/`: ≤ +20 net non-move lines, hard** (Codex estimated 0 to +12; removing the Connect duplicate should make it smaller).
- Expected Sleep shrink about 88 lines; report `wc -l` for Sleep, Connect and Report.
- Tests: guideline +100 (+175 if a new harness is needed for the equivalence cases).

## Tests and checks

- **Structural:** no write to the four latch fields outside `ModemHealth.cpp`, by bare-name search across `src/`; a mutation that adds one in `State_Connect.cpp` must fail.
- **No `retained` in the new files** (negative checks for the `.cpp` and the `.h`).
- **Existing latch tests pass, re-pointed, not weakened:** `standby_latch_daily_clear_test`, `conn_phase_modem_off_test` and any others.
- **Mutations:** drop the daily-close clear; change the connect-timeout threshold to 3; change the recovery teardown limit from 5000 to 6000. Each must fail a test; if one passes at HEAD too, record it as a pre-existing gap.
- Golden payload test unchanged; `nm` retained check (narrowed rule); ELF delta explained by symbol; suite run serially.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **high**. It's not a pure move: three call sites change.
- **Controller (before Stage 7, §13 (g)):** reproduce the build, `nm`, the equivalence cases, the mutations and the suite.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. Checks: the log lines are byte-identical; each decision fires under the same conditions; the Connect duplicate is equivalent to what replaces it; no field write is left outside the owner.

## Bench

None in this WO. Ships in v45; the v45 soak counts `MODEM_HEALTH`/`MODEM_POLICY` lines on the bench against v44.

## Closing record

Budget versus actual (`src/`, moved lines, Sleep/Connect/Report `wc -l`, tests), ELF by symbol, retained outcome, the equivalence table.

## Claude Code's citation re-check (2026-10-19, main `db9e9e1`, P-0 to P-4 merged; `State_Sleep.cpp` is 1725 lines, `State_Connect.cpp` 754, `State_Report.cpp` 333)

**The Sleep sites** (old WO numbers hold): thresholds `MODEM_UNSTABLE_SLOW_TEARDOWN_MS` `:32` and `MODEM_UNSTABLE_RECOVERY_TEARDOWN_MS` `:33` (10000 and 5000 ms); the `ModemUnstableReason` enum `:35–39` (NONE 0, SLOW_TEARDOWN 1, CONNECT_TIMEOUT 2); `connectionModeLabel` `:41–54`; `batteryTierLabel` `:56–69`; `logTeardownContext` `:80–94`; `markModemUnstable` `:104–128` (25 lines); `maybeClearModemUnstable` `:130–149` (20 lines). The two latch functions are 45 lines; with the three labels and helper (43 lines) that is the WO's 88. Callers of the latch functions in Sleep: `markModemUnstable(SLOW_TEARDOWN, …)` at `:754`, `:800`, `:875`; `maybeClearModemUnstable` at `:802`.

**The Connect duplicate holds and is equivalent.** `State_Connect.cpp:24` (`MODEM_UNSTABLE_CONNECT_TIMEOUT_THRESHOLD = 2`); `markModemUnstableFromConnectTimeout()` `:150–167`; the success reset `:611–613` (`if (session.modemConnectTimeoutCount != 0) { … = 0; }`); the call at `:664`. Read line by line, the duplicate does exactly what `markModemUnstable(CONNECT_TIMEOUT, 0)` does once the count reaches the threshold: set `modemUnstable`, set `modemUnstableReason` to 2, log `MODEM_HEALTH: unstable reason=connect_timeout count=%u` only when the reason changed (using the current count), and log `MODEM_POLICY: standby temporarily disabled reason=unstable_modem` once when `modemStandbySuppressed` was false. The `elapsedMs` argument is unused for the CONNECT_TIMEOUT case.

**The daily-close block holds but is not `maybeClearModemUnstable`.** `State_Report.cpp:85–91`: if `modemUnstable`, clear `modemUnstable`, `modemStandbySuppressed`, `modemUnstableReason` and `modemConnectTimeoutCount`, then log `MODEM_POLICY: standby re-allowed at daily close; night modem-off decides`. Unlike `maybeClearModemUnstable` it has no connect-result or teardown-time condition, logs a different line, and does not log `MODEM_HEALTH: recovered`; `clearAtDailyClose()` must reproduce exactly this.

**Every write to the four fields** (all in the three files; `StateMachine.h:61–64` declares them; no other `src/` file reads or writes them): Sleep `:107, :108, :126, :141–143, :146`; Connect `:152, :157, :158, :165, :612`; Report `:86–89`. Reads that stay put: `State_Connect.cpp:444` (`modemStandbySuppressed`) and `State_Report.cpp:205` (`modemUnstable`, with its own `MODEM_UNSTABLE_RECONNECT_DEFER_MS` `:26`).

**Tests that name the latch:** only `standby_latch_daily_clear_test.{sh,cpp}` (the `.sh` extracts the daily-close block from `State_Report.cpp`, the `.cpp` mirrors it; both write the four fields directly in the mirror, which is fine). `conn_phase_modem_off_test.sh` extracts the connect-phase and cloud-recovery blocks from `State_Connect.cpp` and does not name the latch fields; removing the duplicate from the same anonymous namespace must not break its markers, so check it. No test covers the connect-timeout path or the recovery limit: the equivalence harness is new.

**Budget note:** a new `power/ModemHealth.{h,cpp}` (no such file exists; the `MODEM_HEALTH` tag is only a log string). Leave the three labels and `logTeardownContext` in Sleep: they are logging helpers the latch functions do not use, and moving them needs more declarations; say so in the report. The equivalence harness is a new host harness, so the test guideline is +175.

