AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `.github/workflows/build.yml` (the one new step only), add `tests/release_flags_guard_test.sh`, optionally one note line in `docs/bench-validation.md`, and write the report; run host tests (zsh for .sh, python3 for .py); scratch `build-tmp/wo-2026-11-08-004-stage6/` (create it; use only that) / NOT authorized: any change under `src/` or `lib/`, any other workflow step, committing, pushing, merging, creating PRs or tags, deleting anything you did not create (including `build-tmp/` itself).

# Stage 6 dispatch: WO-2026-11-08-004 (F5, release-flag guard)
Binding spec: `docs/work-orders/WO-2026-11-08-004-f5-release-flag-guard.md` and `docs/RECOVERY_PLAN_2026-09-26.md` item 28. Base: this branch HEAD (`main` at e467364 plus the records). Leave changes uncommitted. **Run at the dispatched reasoning level (medium); the report states the level actually run, or that it could not be observed.**
## Do
Implement the WO's "The change" items 1-3 (item 4 only if it fits): the tag-only step before Compile; the extracted-script test with the six cases; the six mutations (back up the file, mutate, restore from the backup with cp; never `git checkout`/`git restore` while your work is uncommitted). The new test must exit 0 when run bare. Do not touch `src/`.
## Verify
CI's discovery inside this git worktree: `tests/*.sh` via zsh and `tests/*.py` via python3 (count `p=$((p+1))`; report `N/N (sh via zsh, py via python3)`; state the measured baseline before editing); `thermal_coupling_structural_test.py` also on a clean export of tracked files (remove scratch copies from `build-tmp/` first); `git diff --stat` shows nothing under `src/`.
## Stop and report if
the step cannot be tested without a `src/` change; the macros cannot be read standalone; the workflow needs more than +12 lines.
## Report
`docs/work-orders/WO-2026-11-08-004-stage6-copilot-report.md`: model and reasoning level actually used, interpreter line, measured baseline, the step as written, tests and mutation results, deviations, open questions. Claim nothing about a real tag build.
