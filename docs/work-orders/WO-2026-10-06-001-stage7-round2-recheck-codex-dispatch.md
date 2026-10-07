AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff in this worktree (branch `wo/2026-10-06-001-alert19-clears`, on top of round 1's commit `77abd4f`); run the host suite in place, and a local ARM build in a scratch copy; write temporary harnesses under your scratch directory **`build-tmp/wo20261006-001-recheck/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create (if `build-tmp/` didn't exist before, say so; Claude Code removes it if empty).

# Stage 7 re-check (narrow: the removal of alert 18 only) — WO-2026-10-06-001 (v38-AllAlertsClear)

**Context:** your round-2 verdict (`docs/work-orders/WO-2026-10-06-001-stage7-round2-verdict.md`) was NOT VERIFIED on alert 18 alone: a tier-3 thrash reset can lose it. Chip's decision: **drop 18, keep 42**. 18 stays sticky until the ThrashGuard/persistence work. This is not a new implementation round; it checks only that the removal is correct and complete.

**Binding spec:** `docs/work-orders/WO-2026-10-06-001-alert19-clears.md`, including the decision and the narrow-revert entry in its approval record.

## Checks (PASS/FAIL with evidence)

1. **18 is back to round-1 behavior:**
   - `case 18:` is absent from `isAutoClearAfterReportAlert()`;
   - your tier-3 reproduction (tier 2 raises 18 → a report → tier 3 raises 18 and resets) now carries 18 in the first report after the reset, as round 1 did;
   - nothing else in round 2 touches 18.
2. **42 is unchanged from what you verified:** the clear list is exactly 15, 19, 31, 41, 42, 43, 44 (in source and in the disassembled mask), and 42's behavior at its three raise sites is as verified in round 2.
3. **Tests:**
   - `testThrashStaysSetAfterReport` drives the real extracted code;
   - re-adding `case 18:` fails it;
   - removing `case 42:` and removing `case 19:` each fail their own test (run all three; restore byte-identically);
   - the copy of the list in `tests/watchdog_alert_code_test.cpp` matches the source.
4. **Suite and build:**
   - every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N` (Claude Code: 63/63);
   - a clean local boron 6.4.1 release build, with sizes; `strings` shows `v38-AllAlertsClear`.
5. **Budget:** v38's net `src/` against main is +2 (19 and 42).

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence. Include binary sizes and the model and reasoning level actually used. Confirm the working tree is byte-identical to how you found it.
