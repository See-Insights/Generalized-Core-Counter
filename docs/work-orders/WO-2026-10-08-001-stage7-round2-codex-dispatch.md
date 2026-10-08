AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-08, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-08-001-failsafe-overdue`, HEAD at the commit that adds this dispatch).
- Run the host suite in place and local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261008-001-stage7r2/`**, and remove **only that directory**.
- Mutate only in scratch copies.

**Not authorized:**
- Any change to files outside your scratch directory.
- Commits, pushes, stash, reset or checkout.
- Flashing, device, network or AWS access.
- Deleting anything you didn't create, including `build-tmp/` itself.

# Stage 7 round 2 dispatch: WO-2026-10-08-001 (Step 6 WO 1b)

**This is the last round under the two-round rule.** If it isn't VERIFIED, the WO stops. Report any new edge case with a severity, for the architect to accept or reject. It is not a design prompt.

**Goal, in plain language:** the connectivity failsafe escalates only when a connection the device was expected to make is overdue, never just because time passed while no connection was due.

**Binding spec:** `docs/work-orders/WO-2026-10-08-001-failsafe-overdue.md`, all sections, especially "Stage 7 round 1 and the architect's rulings":
1. Finding 1 is **accepted** as existing behaviour. Pre-ruling condition 2 is narrowed to "no defaults, no half-applied field". Alert 41 on an over-range interval is an accepted behaviour change, with a CHANGELOG "Unreleased" line.
2. Finding 2 is fixed in the test only.
3. Finding 3 moves the rule below the stage and cooldown returns.

**Inputs:**
- Round 1's verdict: `docs/work-orders/WO-2026-10-08-001-stage7-verdict.md`.
- Round 2's dispatch: `docs/work-orders/WO-2026-10-08-001-stage6-round2-copilot-dispatch.md`.
- Copilot's report: the end of `build-tmp/WO-2026-10-08-001-stage6r2-copilot-output.log`.

## Test-suite hygiene

`tests/thermal_coupling_structural_test.py` scans the whole worktree, including `build-tmp/`. Run the host suite before you create any copy of `src/`, or after removing it.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Re-run round 1's ten checks** (`WO-2026-10-08-001-stage7-codex-dispatch.md`) against the current tree. Check 8 is now graded against the narrowed condition 2 (no defaults, no half-applied field). Say whether it holds, whether alert 41's reach is as the CHANGELOG line describes, and whether `docs/reference/alert-codes.md` needs an update.
2. **Finding 3, the move.**
   - Confirm the block is byte-identical to round 1's, apart from its position.
   - Confirm it now sits after the stage-3 and cooldown returns and before `activeConnectAttemptWithinBudget()`.
   - Confirm that nothing between the old and new positions has a side effect, so the behaviour is unchanged. Check `RecoveryState::get_*` and `connectivityFailsafeJitterSec()`; Copilot says the latter is a deterministic device-ID hash.
   - Confirm in the release disassembly that `resolveRuntime` comes after those checks.
   - Say how often it runs now.
3. **Finding 2, the test.**
   - Confirm `failsafe_cadence_rule_test` extracts the real cadence-rule block, checks it byte-for-byte (loud `COPY_MISMATCH`), and doesn't re-implement `STALE + cadence`.
   - Copilot says the supervisor's stale-return line is re-typed in the test, not extracted, and is only checked to exist in `src/`. Say whether that weakens anything.
   - Run the mutation `STALE_SEC + cadenceSec` → `cadenceSec`: the **4 h behaviour checks** must fail.
4. **Mutations.** Re-run all of round 1's and round 2's mutations, including the three round-1 mutations Copilot didn't plant separately this round:
   - the `#if` guard;
   - the `already connected` refresh;
   - the 86400 bound.

   Each must be caught by its targeted check. Add one of your own: move the block **above** the stale return. Report any survivor.
5. **Builds.** Fresh-path release build: text/data/bss against round 1's 151020 / 1090 / 2196. A fresh-path failsafe test-mode build: the rule is absent.
6. **Budget.** WO total net `src/` code lines against +20 (+9 expected); moved lines.

## Verdict (your final message; it is saved as the verdict file)

- **Header:**
  - one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED;
  - the model and reasoning level used;
  - the test interpreter line.
- **A check table.**
- **Findings,** each with file:line and severity. Separate observation from inference.
- **Budget versus actual,** as rows for the closing record: round 1, round 2, and the WO total.
- **Confirmations:** the `src/` and tests diff matches the start, and you deleted only your scratch directory.

Limit: 150 lines.
