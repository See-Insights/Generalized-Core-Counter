AGENT: Copilot · MODEL: claude-sonnet-5.5 (probed 2026-10-20, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized (tests only):**
- **Edit:** `tests/firmware_update_wiring_structural_test.py` and `tests/firmware_update_dwell_test.sh` (and, only if needed, a stub or helper they use under `tests/`); report each change.
- **Run** the host suite (serially) and the two tests.
- **Scratch directory:** `build-tmp/wo20261020-003-followup/`. Remove **only it** when you finish.

**Not authorized:**
- **Any change under `src/`** (the `src/` tree is hashed before and after; one byte of difference fails the follow-up).
- Any other test, `docs/`, `lib/`, `project.properties`.
- Commits, pushes, merges, branch changes, stash, reset, checkout or restore. Device or network access. Deleting anything you didn't create.

**SIZE BUDGET:** `src/` 0. Tests: small (a few dozen lines); report the actual.

# Stage 6 follow-up dispatch: WO-2026-10-20-003 (Sleep S-1), Stage 7 finding F1

**Finding (Codex, `docs/work-orders/WO-2026-10-20-003-stage7-verdict.md`, F1):** the firmware-update ordering is no longer regression-pinned after S-1. The guard `if (!SleepTeardown::disconnectRequested() && firmwareUpdateInProgress)` (`State_Sleep.cpp:269`) must run **before the teardown is requested**. Before S-1 the teardown requests were inline in `handleSleepingState()`, so `firmware_update_wiring_structural_test.py` ordered the guard against the first `Connectivity::request*` call (its `TEARDOWN_CALLS`, `:41–45`, check at `:143–153`). They now live in `SleepTeardown::run()`, called at `State_Sleep.cpp:504`; the only direct requests left in `State_Sleep.cpp` come later (`:829`, `:845`). So a guard moved between `:269` and the later requests, **including below the `SleepTeardown::run(` call**, passes all 93 tests. Codex's mutation (move the guard below `run()`) survived. `firmware_update_dwell_test.sh` (`:149–170`) extracts the guard and wraps it in a synthetic function that places it first, so it does not preserve the real source order either.

**Ruling (architect, 20 Oct):** re-anchor the ordering check in `firmware_update_wiring_structural_test.py` on `SleepTeardown::run(`, and make the dwell test keep the real source order.

## Do

1. In `firmware_update_wiring_structural_test.py`, make the ordering boundary the earliest of `SleepTeardown::run(` and the direct `TEARDOWN_CALLS` in `handleSleepingState()`, and fail if the guard comes after it. Keep every existing assertion; the failure message names `SleepTeardown::run(`.
2. In `firmware_update_dwell_test.sh`, stop hard-coding the order: derive the order of the guard, the cloud-operations gate and the teardown call (`SleepTeardown::run(`) from the real `State_Sleep.cpp` text (their positions in the `handleSleepingState()` body) and build the synthetic `sleepGatePass()` in that order, so that moving the guard below `run()` changes the test's behaviour and fails it. Keep its runtime assertions unchanged.

## Verify (all required)

- **The mutation must fail:** in a scratch copy of the tree, move the guard block from above to just below the `SleepTeardown::run(` call (and, separately, to just above the later `Connectivity::request` calls) and show **both** tests fail (or at least the structural one, and the dwell test on the first); and that both pass on the real code. A scratch copy has no `.git`; these two tests do not need it. Report the failing messages.
- **Full suite**, `N/N (sh via zsh, py via python3)`, serial; baseline 93.
- `git diff --check` clean.
- **`src/` unchanged:** `find src -type f | LC_ALL=C sort | xargs shasum -a 256 | shasum -a 256` is `7fc57df7a302712ea3f26ff37758ce455aaa6203808a7092d11636f5d06d190a` before and after.

## Report (≤ 150 lines, final message)

Files changed with test lines added and removed; how each re-anchoring works; the mutation results with messages; the suite; the `src/` hashes; scratch removal; deviations (or "none"); the model used.
