AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff for item B only; run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary harnesses under your scratch directory **`build-tmp/wo20261004-001-stage7r2/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 dispatch (narrow re-check, item B only) — WO-2026-10-04-001 (v37-PreStep6Fixes)

**Item B's goal, in plain language:** a restart in the middle of an occupancy session doesn't lose that session's minutes, and never credits more than one debounce period beyond the last moment the session is known to have been open.

**Why model and reasoning:** B is subtle time-ordering logic, so it gets `gpt-6-astra` at high reasoning (Chip, 2026-10-05).

**Two-round rule:** this is B's last round. If it isn't VERIFIED, B comes out of v37.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-04-001-pre-step6-fixes`, base `79abe84` (v36), with the uncommitted diff. Ignore the unrelated uncommitted edit to `docs/work-orders/WO-2026-09-24-004-retire-open-equals-close-convention.md` and the untracked `docs/` files.
**Binding spec:** `docs/work-orders/WO-2026-10-04-001-pre-step6-fixes.md`, including item B, Chip's B-timing and B-anchor decisions, the **Known limitation** paragraph with its fact check, and the approval record's narrow-edit entry.
**Your round-1 verdict:** `docs/work-orders/WO-2026-10-04-001-stage7-verdict.md`. Items A, C and D were VERIFIED there and are not re-checked; their source is unchanged since then.

**What changed since round 1 (Claude Code's narrow edit, applying your proposed fix):**
- `SessionState::occupancySessionBootAnchor` (`time_t`, RAM-only; 0 means no session was open across the restart) replaces `bool occupancySessionCrossedBoot` (`src/state/StateMachine.h`).
- `setup()` captures `max(occupancyStartTime, lastReport)` once, when `occupied` is true at boot (`src/Generalized-Core-Counter.cpp`).
- `closeOccupancySessionSafely()` uses that snapshot and clears it (`src/state/State_Common.h`).
- `tests/occupancy_session_restart_test.sh`: the pins and fake follow the rename, and there's a new case, `testReportAfterRestartBeforeTrustDoesNotMoveTheCap`.

Narrow review: check B against its goal and the WO's acceptance criteria only. Don't widen the fault model or propose new mechanisms. For any defect, give the smallest fix within budget as a proposed diff; don't apply it.

## Checks (PASS/FAIL with evidence for each)

1. **Your reproduction, against the production close:**
   - 300 s debounce; session start 1,000; last report and power-off at 5,000; 10-hour power-off; boot at 41,000; a report at 41,050 before the clock is trusted; then the trusted close;
   - **expected:** over-credit ≤ 300 s (credit 4,300 s);
   - also without the post-boot report (4,300 s).
2. **A restart mid-session credits up to the boot time, capped at the boot-time anchor plus one debounce,** at the first trusted close. Check both sides of the cap, and an anchor taken from `lastReport` later than the start.
3. **An untrusted clock at boot leaves the session open,** credits nothing, re-arms the debounce, and causes no repeated `REPORTING_STATE` transition or `OccAnom` line at any of the three callers (`State_Idle.cpp`, `State_Modes.cpp`, `State_Sleep.cpp`).
4. **The debounce no longer uses the previous boot's `millis()`.**
5. **Over-credit after a long power-off** (≥ 10 h, with and without reports after the restart) is ≤ one debounce period.
6. **No new persisted or `retained` field.** `occupancySessionBootAnchor` is in ordinary RAM (source and ELF section). A value of 0 can't be confused with a valid anchor, because an open session with start 0 is already invalid.
7. **The daily-boundary close** (`State_Report.cpp:59`) is still correct for a session that was open across a restart.
8. **The new test is meaningful:** reading `lastReport` at close time again (the round-1 code) fails it, and so does removing the cap. Run both mutations.
9. **The Known limitation's fact check is accurate** against source: the connectivity failsafe's 3-hour stage-2 reset needs a trusted clock, and the exceptions listed are complete. Report only; it's an accepted limitation.
10. **Suite and build:**
    - every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N` (Claude Code: 65/65);
    - a clean local boron 6.4.1 release build: text/data/bss against round 1's 150788 / 1090 / 2188; `strings` shows `v37-PreStep6Fixes`;
    - `nm` shows `closeOccupancySessionSafely` (or its inlined callers) and `setup` in the ELF;
    - Claude Code already ran the Particle cloud compile (succeeded; flash 152070 / RAM 3290).
11. **Budget:** B's net `src/` lines (nonblank, non-comment) ≤ 15 by your counting rule (Claude Code: +15); nothing compressed.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, for B overall and per check, with evidence. Include:
- binary sizes;
- B's budget versus actual;
- the model and reasoning level actually used.

Confirm that the working tree is byte-identical to how you found it, and that nothing outside `build-tmp/wo20261004-001-stage7r2/` was created or deleted.
