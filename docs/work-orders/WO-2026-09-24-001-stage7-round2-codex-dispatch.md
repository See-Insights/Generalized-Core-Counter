AGENT: Codex · MODEL: gpt-6-astra · REASONING: ultra
AUTHORIZATION SCOPE: review the uncommitted diff; temporarily mutate source/test files for the mutation checks, restoring each byte-identically; run builds and tests / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, or AWS access.

# Stage 7 round 2 (narrow) — WO-2026-09-24-001, Stage 5 decision 8

Per AI_DEVELOPMENT_WORKFLOW.md. Dispatched via `codex exec` (not `codex review --uncommitted`, which rejects custom instructions in CLI 0.154.0).

**Review target:** the uncommitted working-tree diff on `wo/2026-09-24-001-daily-cleanup-boundary-fix`. The decision-8 changes are the unstaged part of `src/state/State_Report.cpp`, `src/state/State_Common.h`, `src/state/StateMachine.h`, `src/Generalized-Core-Counter.cpp`, and `tests/daily_cleanup_boundary_test.py` (`git diff` without `--cached`); the staged part is the round-1 implementation already VERIFIED in `docs/work-orders/WO-2026-09-24-001-stage7-verdict.md`.

**Binding spec:** `docs/work-orders/WO-2026-09-24-001-daily-cleanup-boundary-fix.md`, Stage 5 decisions 1–8, the Fix section, Requirements, and acceptance criteria 8 and 9. Copilot's round-2 dispatch is `docs/work-orders/WO-2026-09-24-001-stage6-round2-copilot-dispatch.md`.

**Scope: decision 8 only.** Do not repeat the round-1 review, except that round-1 mutations (a)–(h) must still be caught (confirm with one run of the test, not a re-review).

Out of scope, do not review or modify: every file matching `docs/work-orders/WO-2026-09-25-*`, and `AI_DEVELOPMENT_WORKFLOW.md`.

Shell tests are zsh: run via shebang or `zsh <script>`, never bash. Report results as N/N (sh via zsh, py via python3).

## Verify

1. The in-block snapshot `publishData()` is gone: exactly one `publishData(` call in `handleReportingState()`.
2. The boundary test (trusted gate, `close` normalization, `localTodayAt`, the `now < boundary` adjustment, `due`) is evaluated before the publish and writes nothing.
3. When due: `wasOccupied` and the original session start are captured, and the session is closed at the boundary, before the publish.
4. The publish is `publishData(due ? boundary - 1 : 0)`, then (when due) `dailyCleanup()`, the always-open restart at `std::max(boundary, originalSessionStart)`, then `set_lastDailyCleanup(now)`, in that order.
5. `publishData(time_t stampOverride)` applies a nonzero override only to the occupancy payload's timestamp; the counting-mode payload is unchanged; the default argument appears exactly once, in a declaration every caller sees, and the build is clean.
6. Acceptance criteria 8 and 9: pass or fail, with file and line evidence.

## Mutations (each must make at least one test fail; restore byte-identically after each)

- (i) Restore the in-block publish: add a `publishData()` call inside the due block before `dailyCleanup()`.
- (ii) Move the report publish after the reset (after `dailyCleanup()`).
- (iii) Stamp at `now` instead of the boundary (for example `publishData(due ? now : 0)`).
- (iv) Apply `boundary - 1` to every report (for example `publishData(boundary - 1)`).

Report any surviving mutation as a finding.

## Builds

- Local Boron ARM build (README command). Reference after round 2, reported by Copilot: 150568 / 1090 / 2444.
- The cloud compile needs network, which your sandbox lacks; Claude Code runs it separately.

## Output

VERIFIED, VERIFIED WITH CONCERNS, or NOT VERIFIED, with file and line evidence, a mutation table, the suite result with interpreters, and the model and reasoning level actually used. Restore every mutated file byte-identically and remove temporary artifacts before finishing. Do not commit, push, stash, reset, or check out anything.
