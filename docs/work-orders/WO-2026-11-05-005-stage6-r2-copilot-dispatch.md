AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/power/ConnectivityFailsafe.cpp` (the helper only), tests under `tests/` (the two structural tests and the S3 test and its fakes), and write the report; run host tests, the local Device OS toolchain build and nm; scratch `build-tmp/wo-2026-11-05-005-stage6-r2/` (create it; use only that) / NOT authorized: committing, pushing, merging, flashing, creating PRs or tags, touching `lib/`, S1/S1b/S2 behaviour, the callers' logic or log lines, the off time (30 s stays, Amendment 4), the evidence/persisted logic, deleting anything you did not create (including `build-tmp/` itself).

# Stage 6 dispatch, round 2 (fixes after Stage 7 round 1): WO-2026-11-05-005 (S3)
Binding spec: `docs/work-orders/WO-2026-11-05-005-v50-s3-timed-power-off.md` **Amendments 3, 4 and 5 (read Amendment 5 first)** and Codex's verdict `docs/work-orders/WO-2026-11-05-005-stage7-r1-verdict.md` (findings F1-F3). Base: this branch HEAD (S3 code at 1a5bd12); your changes go on top, uncommitted. **Budget: `src/` at most +20 net against 62e3dbc** (reported +13; state the number). **Run at the dispatched reasoning level (medium); the report states the level actually run, or that it could not be observed.**
## Do
1. F1: stop `ExternalWatchdog` immediately before `System.sleep()` in `powerOffForRecovery()` and restart it on every return (use the same stop/start calls S2 uses around HIBERNATE in `State_Sleep.cpp`; cite file:line).
2. F2 and F3 as in Amendment 5 (tests only). Add a mutation for F1 (stop removed; restart removed); each mutation must fail (back up, mutate, restore from the backup; never `git checkout`/`git restore` while your work is uncommitted).
## Verify (workflow §10, binding)
CI's discovery inside this git worktree: `tests/*.sh` via zsh and `tests/*.py` via python3 (count with `p=$((p+1))`; report `N/N (sh via zsh, py via python3)`; S3 round 1: 121/121; any new helper under `tests/` exits 0 bare); `thermal_coupling_structural_test.py` also on a clean export of tracked files; local toolchain build (fresh `BUILD_PATH_BASE` as a make argument; round 1: 150908 / 1114 / 2240); `nm` shows the helper linked; numstat net as above.
## Stop and report if
the budget cannot hold; the fix needs a change outside the helper or the allowed tests.
## Report
`docs/work-orders/WO-2026-11-05-005-stage6-r2-copilot-report.md`: model and reasoning level actually used, interpreter line, net `src/` lines, tests and mutation results, build and `nm` evidence, deviations. Claim nothing about hardware.
