AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff; run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary harnesses under your scratch directory **`build-tmp/wo20261002-002-stage7/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 dispatch (narrow) — WO-2026-10-02-002 (v33-HourlyWhileOccupied)

**Goal, in plain language:** the device makes its scheduled report every hour whether or not the site is occupied. That also means v32's "3 hours without a successful connection" only happens when the device actually can't connect.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-02-002-hourly-while-occupied`, stacked on v32 at `e8e33a7`, with the uncommitted diff. Ignore the uncommitted edit to `docs/work-orders/WO-2026-10-02-001-recovery-visibility.md` (Claude Code's v32 bench results).
**Binding spec:** `docs/work-orders/WO-2026-10-02-002-hourly-while-occupied.md`, including the Stage 5 decision (the clock-hour due rule) and "The same cadence as unoccupied reports".
**Stage 6 report:** `docs/work-orders/WO-2026-10-02-002-stage6-copilot-report.md`.

Narrow review: check against the WO's acceptance criteria only. Don't widen the fault model or propose new mechanisms. For any defect, give the smallest fix within the approved design.

## Checks (PASS/FAIL with evidence for each)

1. **Hourly while occupied:** with a host check (a real harness or a faithful compiled extraction; say which), a timer wake at the report time while occupied → `REPORTING_STATE` with occupancy 1. The session continues: it isn't closed or restarted, `occupancyStartTime` is unchanged, and accumulation continues through the report path (`State_Report.cpp`).
2. **No more than due:**
   - after a report in the current interval, debounce timer wakes and PIR wakes in the same interval don't report;
   - 4 h of continuous occupancy, with PIR wakes every few seconds and debounce wakes every 300 s, gives exactly one scheduled report per hour boundary (plus any occupancy-change reports);
   - an occupancy-change report made after the boundary counts as that interval's report.
3. **The v32 failsafe:** with continuous occupancy for 4 h and a working connection, it never resets. Each report's successful connection refreshes `lastConnection` (`State_Connect.cpp`, the `set_lastConnection` path).
4. **Same cadence as unoccupied:**
   - the due test uses `Config::reportingIntervalSecForRuntime()` and `Time.now()`, the same interval function and epoch base as the unoccupied wake alignment (`State_Sleep.cpp:1039–1069`);
   - for 3600 s and 1800 s intervals, occupied reports fall on exactly the boundaries an unoccupied device reports on.
5. **Nothing else changes:**
   - unoccupied behavior at all three sites;
   - occupancy-change reports (`sleep-pir-occupancy-report`, `sleep-occupancy-debounce-report`);
   - the daily close and close-before-sleep;
   - v32's items (failsafe, `MODEM_OFF`, payload fields).
6. **Mutations:** restoring the suppression at site 1, and at site 2, each fail a test.
7. **Existing test change:** `tests/loop_stage_sleep_prep_exclusion_test.py` 16 → 17 transitions. Confirm it's required by site 2's new exit and preserves the test's intent.
8. **Code quality within budget:** Copilot put two declarations on one line in `reportDueThisInterval()` to land on 8 lines. Report whether that's a readability concern only, or anything more (types: `time_t` vs `uint16_t`, division by zero, negative or untrusted `Time.now()`). No redesign.
9. **Suite and build:**
   - every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N` (Copilot: 57/57); `tests/publish_with_ack_structural_test.py` unchanged and green;
   - a local boron release build after `make clean-user` (Copilot: 150780 / 1090 / 2204); `strings` shows `v33-HourlyWhileOccupied`, product 33;
   - linkage: the shared due test is reached from all three sites in the ELF;
   - net `src/` lines against about 8, by your counting rule.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence; binary sizes; the model and reasoning level actually used. Confirm that the working tree is byte-identical to how you found it, and that nothing outside `build-tmp/wo20261002-002-stage7/` was created or deleted.
