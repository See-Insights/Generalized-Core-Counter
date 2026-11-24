AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `tests/default_settings_coverage_test.py`; create `docs/config/README.md`; write the report; run host tests; scratch `build-tmp/wo-2026-11-11-002-stage6-r2/` (create it; use only that) / NOT authorized: ANY change under `src/` or `lib/`, any change to `docs/config/default-settings-42131.json` (its content is exact), committing, pushing, merging, flashing, creating PRs or tags, deleting anything you did not create, `git checkout` or `git restore` while your work is uncommitted

# Stage 6 round 2 dispatch: WO-2026-11-11-002 Amendment 1 (every default file is checked; README)
Binding spec: `docs/work-orders/WO-2026-11-11-002-default-settings-coverage.md`, **Amendment 1** at the end (read it), on top of round 1 (committed on this branch; report `docs/work-orders/WO-2026-11-11-002-stage6-copilot-report.md`). **Budget: `src/` 0 lines.** Run at the dispatched level (medium); the report states the level actually run, or that you could not observe it.

## Do
1. **Baseline first:** CI's discovery inside this git worktree; report `N/N (sh via zsh, py via python3)` before editing (expect 119/119 on this head).
2. **Tests first:** change the test to check **every** `docs/config/default-settings-*.json` and to fail when there are none; same rule per file (no missing keys, no dead keys, at least 25 keys); print the lists per file. Show it fails for: a scratch directory with no default file; a scratch second file with a key removed; a scratch second file with a dead key added. (The test needs a way to point at a scratch config directory, for example an optional argument or an environment variable, with the repo's `docs/config/` as the default and the bare run exiting 0.) State the new test line count.
3. **`docs/config/README.md`** (about 20 lines) as Amendment 1 says. Re-cite the `sensor.setting1`-`setting4` meanings from `src/cloud/ConfigApply.cpp` and the sensor code (find where `setting1` is consumed, for example the PIR debounce); put the `file:line` evidence in the report. If the code disagrees with Chip's table, **stop and report**.
4. **Mutations** (back up, mutate, restore with `cp`; never `git checkout`/`git restore`): re-run round 1's four (delete `modes.enableHibernateSleep`; add `messaging.disconnectedMode`; a scratch `getMergedIntValue(... "newKey" ...)` in a copy of `ConfigApply.cpp`; a regex that matches nothing) plus the three above; each must fail. Report each.
5. **Verify:** the full suite `N/N (sh via zsh, py via python3)` after editing; the test exits 0 bare; `git diff --numstat -- src` is empty.

## Report
`docs/work-orders/WO-2026-11-11-002-stage6-r2-copilot-report.md` (at most 150 lines): model and level, interpreter line, baseline, failing-before evidence, test size, the README evidence (`file:line`), mutation table, suite result, deviations, open questions. Leave your changes **uncommitted**.
