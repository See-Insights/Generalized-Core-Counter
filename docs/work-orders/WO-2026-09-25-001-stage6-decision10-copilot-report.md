_Copied verbatim from the run output by Claude Code. Agent: Copilot CLI 1.0.88, `claude-opus-5`, reasoning medium._

## Implementation Report — WO-2026-09-25-001, Stage 5 decision 10

**Model/reasoning:** claude-opus-5, reasoning medium. Branch `wo/2026-09-25-001-publish-with-ack` @ `599038e`, no commits; uncommitted diff left in place.

**Files changed (src/):**
| File | +/- |
|---|---|
| `src/Generalized-Core-Counter.cpp` | 5 / 5 |
| `src/power/PowerDiagnostics.cpp` | 1 / 1 |
| `src/state/State_Idle.cpp` | 2 / 10 |
| **Total** | **8 added / 16 removed = 24 diff lines; 14 distinct source lines touched** |

**Budget note:** 14 distinct `src/` lines touched (6 publish lines + 8 Idle lines) — within "about 15". Counted as `--numstat` added+removed it is 24, because each modified line counts twice and the Idle removal is 8 lines for 1 replacement. No line was changed beyond decision 10; nothing was expanded to reach or exceed the budget. Flagging the accounting ambiguity rather than silently choosing the favourable number.

**The six sites (final flags), confirmed by search as the only `PublishQueuePosix::instance().publish(` calls in `src/`:**
- `Generalized-Core-Counter.cpp:2061` report → `PRIVATE | WITH_ACK`
- `:2283` `status` → `PRIVATE | WITH_ACK`
- `:2323` `watchdog` → `PRIVATE | WITH_ACK`
- `:2358` `hibernate_wake` → `PRIVATE | WITH_ACK`
- `:2471` `publishDiagnosticSafe()` → `flags | WITH_ACK` (as v3.12)
- `power/PowerDiagnostics.cpp:439` `pdiag` → `PRIVATE | WITH_ACK`

No further publish sites found. `lib/` untouched.

**Idle change:** `State_Idle.cpp` now reads `if (!updatesPending) {`. The `canSleepGate` local, its `Particle.connected()` guard, and the three comment lines describing the queue-drain requirement are gone (the remaining two comment lines still apply). `State_Sleep.cpp` untouched; the queue wait is now Sleep's bounded 30–120 s gate → alert 43 → disconnect.

**Test:** new `tests/publish_with_ack_structural_test.py` (147 lines), repository structural-test pattern, comment-stripped, line-accurate. Invariants: (1) every queue publish under `src/` passes `WITH_ACK`; (2) ≥6 such sites exist (non-vacuity); (3) Idle enters sleep on `if (!updatesPending) {` and `canSleepGate` is absent. Mutation-verified: removing `WITH_ACK` from each of the six sites individually fails with the correct file:line (2061, 2283, 2323, 2358, 2471, PowerDiagnostics 439); reinstating the `canSleepGate` term also fails.

**Verification:**
1. Host suite: **44/44 (22/22 sh via zsh, 22/22 py via python3)**.
2. Local ARM build (boron, README command): text/data/bss **150624 / 1090 / 2444** vs `599038e` 150576 / 1090 / 2444 → **+48 / 0 / 0**.
3. Cloud `particle compile boron . --target 6.4.1`: **Flash 151810, RAM 3538** vs 151754 / 3530 → **+56 / +8**. Binary deleted.
4. `git diff --numstat -- src/` as tabled above. Temp logs were under gitignored `build-tmp/wo-2026-09-25-001-decision10-verification/`, now removed (directory gone).

**Idle-ceiling finding (reported, not changed):** the ceiling at `State_Idle.cpp:270–301` computes `noMeaningfulWorkRemains = !updatesPending && queueCanSleep` and only applies when that is true. With `WITH_ACK` restored, a never-acknowledged event holds `getCanSleep()` false, which makes `shouldApplyIdleCeiling` false and **disables** the ceiling. In `CONNECTED` connection mode the low-power block above is skipped entirely, so the ceiling is the only bound there and a stuck queue removes it — the device can stay awake with the modem powered. Outside decision 10; left untouched.

**Deviations (2, both test-harness, no `src/` impact):**
1. `tests/stubs/diag_overrides/PublishQueuePosixRK.h` — added `constexpr int WITH_ACK = 8;`. Without it `power_diagnostics_batch_test` and `power_diagnostics_log_guard_test` fail to compile (`use of undeclared identifier 'WITH_ACK'`).
2. `tests/hibernate_wake_diagnostics_test.sh:90` — fidelity pattern updated from `..., payload, PRIVATE))` to `..., payload, PRIVATE | WITH_ACK))`; it pins the exact publish text and failed otherwise.

Both were required to keep the suite green and are outside the dispatch's literal authorization ("the `src/` lines named below and add one test file"), hence reported rather than assumed. Reverting them re-breaks those three tests.

