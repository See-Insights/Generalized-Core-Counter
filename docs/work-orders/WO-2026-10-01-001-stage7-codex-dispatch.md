AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff; run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary host harnesses under a uniquely named scratch directory (`build-tmp/wo20261001-stage7/`) and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create (in particular `build-tmp/` itself and `build-tmp/connectivity-archive/`).

# Stage 7 dispatch (narrow) — WO-2026-10-01-001 (v31, connectivity and diagnosability fixes)

**Goal, in plain language:** a device doesn't go to sleep in the middle of a firmware download that's still making progress; its long connection attempt comes round when it should, even if the previous attempts failed; after any reset the firmware issues itself, the next status names the code that issued it; and the signal reads "not available" when Device OS has no reading, instead of 0/0.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-01-001-connectivity-fixes`, base `9e10778` (v30), with the Stage 6 change as an uncommitted working-tree diff. **Binding spec:** `docs/work-orders/WO-2026-10-01-001-connectivity-fixes.md`, including "Stage 5 decisions". Stage 6 report: `docs/work-orders/WO-2026-10-01-001-stage6-copilot-report.md`. Ignore the untracked `docs/work-orders/2026-10-01-connectivity-investigation-*` files except as background.

This is a narrow review: check each item against its own goal and the WO's acceptance criteria. Don't widen the fault model or propose new mechanisms. Where you find a defect, report it with the smallest fix that stays within the approved design.

## Checks (PASS/FAIL with evidence for each)

### A. Don't sleep while a firmware download is making progress

1. With a host check (a real harness or a faithful compiled extraction; say which):
   - `begin` enters `FIRMWARE_UPDATE_STATE` from Sleep, Idle and Connecting;
   - progress events keep it there past 5 minutes in total;
   - 5 minutes without progress exits to `SLEEPING_STATE`;
   - `complete` and `failed` exit to `IDLE_STATE`;
   - the button override still exits;
   - no other exit remains in the handler.
2. The handler (`firmwareUpdateHandler`) only records; it makes no transitions.
3. **No sleep transition while in the state.** ThrashGuard's update-state timeout is above 300 s and is refreshed on recorded progress.
4. **Mutations** (each must fail a test): restoring the fixed cap; restoring the `updatesPending()` exit; removing the progress `markProgress`; setting the update-state ThrashGuard timeout below 300.
5. **Questions to settle (not premises; rule each in or out with evidence):**
   - **(a) Ordering in `loop()`.** The loop-level `begin` check sits after the state dispatch (`switch (state)`). If a `begin` is recorded during a pass in which the device is in `SLEEPING_STATE`'s gate, can the next pass's sleep handler commit to sleep (radio off, or the sleep call) before the loop-level transition runs? If yes, is it reachable in practice (when are system events delivered relative to `loop()`), and what's the smallest in-design fix?
   - **(b) Stale record.** `firmwareUpdateLastEvent` is cleared only when acted on inside the state. Can a `complete`/`failed` recorded outside the state (for example after a no-progress exit) make a later entry via `System.updatesPending()` (`State_Connect.cpp:651–652`) exit immediately? Does it matter?
   - **(c) After `complete`.** The state exits to Idle. Can Idle or the sleep gate put the device to sleep before Device OS performs its update reset? Compare with v30's behavior on the same path.
6. **Webhook-ack pause (A5):** confirm it's within 3 lines and doesn't affect the ack path outside the state.

### B. Count failed attempts

7. A failed attempt increments `connectionAttemptCounter` with the same `< DEEP_ATTEMPT_COUNTER_THRESHOLD` guard. After 3 failures at charge ≤ 50%, the next attempt uses the 660 s budget. The success increment, the reset at deep start, the thresholds and the "above 50%" rule are unchanged. Mutation: removing the failure increment fails a test.

### C. Name the code behind each firmware-issued reset

8. Every `System.reset(` in `src/` passes a distinct, non-zero code from `src/ResetCause.h`. The structural test fails on a bare `System.reset()` or a duplicate code. The startup status payload still carries `resetReasonData` unchanged. Confirm against Device OS 6.4.1 that `System.reset(uint32_t)` gives `RESET_REASON_USER` and that the data is returned by `System.resetReasonData()` on the next boot. Note anything (for example reset flags, or Device OS overwriting the data) that would lose the code.

### D. Signal "not available"

9. With an invalid reading (−1), `valid` is false, −1 is kept, and the existing `sig=na` log branches are taken. With a valid reading, behavior is unchanged. Mutation: restoring the unconditional `valid = true` fails a test.

### Everywhere

10. **Suite:** every `tests/*.sh` with **zsh** (never bash), plus every bare `tests/*.py` with python3, as `N/N`. `tests/publish_with_ack_structural_test.py` unchanged and green. The two existing tests Copilot changed (`loop_stage_sleep_prep_exclusion_test.py`, `nightly_heap_guard_flush_test.sh`): confirm each change was required by text this WO changed and preserves the test's intent.
11. **Linkage (mandatory):** `firmwareUpdateHandler` has a production call site (the `System.on` registration) and is present in the linked ELF (`nm` on a symbolised local build). The loop-level transition is reachable.
12. **Local toolchain build (mandatory):** boron release, after `make clean-user`: text/data/bss (Copilot reports 150444 / 1090 / 2212 against v30's 150164 / 1090 / 2196); `strings` shows `v31-ConnectivityFixes` and `firmware update begin`.
13. **Identity and size:** `v31-ConnectivityFixes`, product 31. Count `src/` lines per item against its budget (A ≤ 20, B ≤ 5, C ≤ 25, D ≤ 12; total about 65), excluding comments and blanks, and state your counting rule. **Copilot reports item A at about 29 lines, over its 20-line budget, and continued instead of stopping.** Report A's count as a finding. Say whether anything in A could be removed without losing a WO requirement, but don't propose redesigns.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence; binary sizes; the model and reasoning level actually used. Confirm that the working tree is byte-identical to how you found it, and that nothing outside `build-tmp/wo20261001-stage7/` was created or deleted.
