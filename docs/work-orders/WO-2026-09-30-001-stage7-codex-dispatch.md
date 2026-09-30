AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff; run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary host harnesses under a gitignored scratch path; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access.

# Stage 7 dispatch (narrow) — WO-2026-09-30-001 (never sleep for the night with the close still due)

**Goal, in plain language:** the device never commits to night sleep while the daily close is due. If it is due, the device runs the report first, and that report does the close.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-30-001-close-before-night-sleep`, base `c2fb35c` (v27), with the Stage 6 change as an uncommitted working-tree diff. **Binding spec:** `docs/work-orders/WO-2026-09-30-001-close-before-night-sleep.md`, including its route table. Stage 6 report: `WO-2026-09-30-001-stage6-copilot-report.md`. Ignore the untracked `docs/work-orders/2026-09-29-*` and `WO-2026-09-29-002-*` files; they belong to another WO.

This is a narrow review: check against the WO's acceptance criteria only. Do not widen the fault model or propose new mechanisms.

## Checks (PASS/FAIL with evidence for each)

1. **Equivalence:** `DailyBoundary::check()` gives the same `due` and `boundary` as the old inline logic at `c2fb35c` (`src/state/State_Report.cpp:50–72`), over the existing test cases (the acceptance model in `tests/daily_cleanup_boundary_test.py`) and an untrusted clock. Show that the moved lines are unchanged in logic, and that `handleReportingState()` otherwise behaves exactly as at `c2fb35c` (same order: publish at `boundary - 1`, `dailyCleanup()`, `set_lastDailyCleanup(now)`; the same "Daily boundary reached" log; the item-A `else if (due)` trigger).
2. **Each traced route reaches the report before night sleep when the close is due:** a host check (a real harness or a faithful compiled extraction; say which) for each of:
   - connection timeout (`State_Connect.cpp` `connect-timeout`);
   - return-to-sleep after a report that finishes connecting after the boundary;
   - Idle's park-closed;
   - a motion wake while closed (`sleep-pir-return-to-sleep`);
   - occupied at close in intermittent (not keep-alive) mode.

   With a trusted clock, the park closed, and the close due, each must reach `REPORTING_STATE` ("close due before night sleep") before any night-sleep commitment (`onEnterSleep()`, `secondsUntilNextOpen()`, the HIBERNATE call).
3. **No loop:** after that report (which stamps `lastDailyCleanup = now`), the close isn't due and the next sleep pass commits to night sleep.
4. **Untrusted clock:** the close isn't due, and the device goes to night sleep (or the short-nap path for `Unknown` openness) with no report detour.
5. **Mutation:** removing the new `:971` condition makes at least one test fail; restore byte-identically.
6. **Suite:** every `tests/*.sh` with **zsh** (never bash) plus every bare `tests/*.py` with python3, `N/N`; `tests/publish_with_ack_structural_test.py` unchanged and green. Report any test changes beyond those the WO allows as findings.
7. **Identity and size:** `v28-CloseBeforeSleep`, product 28; the `src/` diff meets the budget (about 25 new lines, with the moved lines unchanged); a local ARM release build (boron, after `make clean-user`) with text/data/bss, and `strings` showing `v28-CloseBeforeSleep` and no `pdiag`.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence; binary sizes; the model and reasoning level actually used. Confirm the working tree is byte-identical to how you found it.
