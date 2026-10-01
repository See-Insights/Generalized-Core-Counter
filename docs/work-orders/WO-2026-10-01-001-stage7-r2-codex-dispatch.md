AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff (item A only); run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary harnesses under your scratch directory **`build-tmp/wo20261001-stage7-r2/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 round 2 dispatch (narrow, item A only) — WO-2026-10-01-001

**Goal, in plain language:** a device doesn't go to sleep in the middle of a firmware download that's still making progress.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-01-001-connectivity-fixes`. **Compare against `9e10778` (v30).** `HEAD` is `74b0198`, a docs-only commit ("workflow: record budget vs actual per WO") on top of `9e10778`; it touches no `src/` or `tests/` file. The WO's work is the uncommitted working-tree diff.

**Binding spec:** `docs/work-orders/WO-2026-10-01-001-connectivity-fixes.md`, section **"A — round 2"**.
**Reference design:** Particle, "Wake publish sleep (cellular)", https://docs.particle.io/firmware/low-power/wake-publish-sleep-cellular/. A saved copy is at `build-tmp/connectivity-archive/particle-docs/firmware_low-power_wake-publish-sleep-cellular.txt`, lines 216–413.

**Context:**
- Stage 6 round 2 report: `docs/work-orders/WO-2026-10-01-001-stage6-r2-copilot-report.md`.
- Your round 1 verdict: `docs/work-orders/WO-2026-10-01-001-stage7-verdict.md`.

**Scope:** items B, C and D were VERIFIED in round 1 and are frozen. Claude Code checked that their `src/` lines are byte-identical to round 1. Re-check only that they are unchanged. This is round 2 of 2 (the two-round rule): report precisely; don't propose new mechanisms.

## Checks (PASS/FAIL with evidence for each)

1. **Your three round-1 reproductions, re-run against round 2.** Each should now pass by construction:
   - **P1:** a `begin` delivered between loop passes while the device is in `SLEEPING_STATE`'s gate never reaches a teardown request (cloud disconnect / radio off) or a sleep call.
   - **P2:** no re-entry into `FIRMWARE_UPDATE_STATE` after the download ends (complete or failed).
   - **P2:** a stale complete/failed from an earlier download doesn't cut short a later download's stay.
2. **Reference pattern:**
   - the handler sets `firmwareUpdateInProgress` on begin and clears it on complete/failed, records activity on begin/progress, and makes no transitions;
   - the sleep handler checks the flag before any teardown request, on every pass while no disconnect has been requested;
   - the update state exits to `SLEEPING_STATE` when the flag clears.
   Note any difference from the reference other than the three deliberate ones listed in the WO.
3. **Existing A checks:**
   - entry into the update state from Sleep (via the flag) and via `System.updatesPending()` (`State_Connect.cpp:651–652`);
   - progress keeps it there past 5 minutes in total;
   - 5 minutes without progress → `SLEEPING_STATE`;
   - the button still exits;
   - no other exit remains;
   - ThrashGuard: timeout 330 s and `markProgress` on new activity (a long download with progress causes no trip);
   - the webhook-ack hold is unchanged (3 lines).
4. **Mid-gate exit:** Copilot resets `cloudSyncStartMs` when leaving the sleep handler for the update state. Confirm that a later return to `SLEEPING_STATE` starts its gate cleanly, and that no other gate static carries stale state into that next cycle.
5. **Mutations** (each must fail a test): moving the sleep-handler check after the first teardown request; removing it; a fixed (non-restarting) 5-minute timer; removing `markProgress`; an update-state ThrashGuard timeout below 300.
6. **Suite:** every `tests/*.sh` with **zsh** (never bash), plus every bare `tests/*.py` with python3, as `N/N`. `tests/publish_with_ack_structural_test.py` unchanged and green. Copilot changed `tests/loop_stage_sleep_prep_exclusion_test.py` (`EXPECTED_TRANSITION_CALLS` 15 → 16). Confirm the change was needed only for item A's new `transitionTo` and preserves the test's intent.
7. **Linkage and build (mandatory):**
   - `firmwareUpdateHandler` is registered and present in the ELF;
   - the sleep-handler check is in the image;
   - a local boron release build after `make clean-user`, with text/data/bss (Copilot: 150420 / 1090 / 2204);
   - `strings`: `v31-ConnectivityFixes`, `firmware update in progress`.
8. **Size:** item A's net `src/` code lines against `9e10778`, by your round-1 rule (nonblank, non-comment physical lines, including braces, declarations and includes), against **about 30**. Copilot reports +27. Confirm B, C and D's counts are unchanged from round 1 (4 / 14 / 4).

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence; binary sizes; the model and reasoning level actually used. Confirm that the working tree is byte-identical to how you found it, and that nothing outside `build-tmp/wo20261001-stage7-r2/` was created or deleted.
