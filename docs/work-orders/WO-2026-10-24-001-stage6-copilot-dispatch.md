AGENT: Copilot · MODEL: claude-sonnet-5.5 (probed 2026-10-20, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit in `src/`:** `src/power/ConnectivityFailsafe.cpp` (one line, `:228`) and `src/diagnostics/ConnectivityFailsafeTest.cpp` (one line, `:159`) **only**. **Tests:** re-anchor `tests/connectivity_failsafe_open_hours_test.{sh,cpp}`; add a new harness `tests/connectivity_failsafe_unknown_openness_test.{cpp,sh}` on the real extracted supervisor body; report each change.
- **Run** the host suite (serially) and the local ARM release build (Boron 6.4.1) plus one with `EXTRA_CFLAGS=-DCONNECTIVITY_FAILSAFE_TEST_MODE=1`. Argon is not built.
- **Scratch directory:** `build-tmp/wo20261024-001-stage6/`. Remove **only it** when you finish.

**Not authorized:** any other `src/` change (not the stage timing, cooldown, jitter, the low-battery suppression, the age base, or any other file); docs, `lib/`, `project.properties`; commits, pushes, merges, branch changes, stash, reset, checkout or restore; device or network access beyond the Particle compile; deleting anything you didn't create.

**SIZE BUDGET:** `src/`: net **0** expected (two one-line edits; prototype `WO-2026-10-24-001-prototype.diff`), **hard limit +20**, counted over all lines. Tests: guideline +100; expect +150 to +220 (flag it); record ruling 9's numbers against main `0c565fa` (baseline 28,893 / 834 / 91 / 22,306; this tree's baseline: suite **104**).

# Stage 6 dispatch: WO-2026-10-24-001 (the connectivity failsafe must act when openness is Unknown; recovery item 18)

**Goal:** after the failsafe's own stage-2 reset the clock is untrusted (openness Unknown) and the supervisor returned unless openness was exactly Open, so stage 3 could never fire and the device stayed dark (Dev-09, 23–24 Oct). Treat Unknown as Open in the supervisor; Closed still skips. **Nothing else changes.**

**Repository:** this clone, branch `wo/2026-10-24-001-failsafe-unknown-openness` (from main `da62202`; HEAD adds this dispatch). Don't switch branches. Compute the `src/` hash first (`find src -type f | LC_ALL=C sort | xargs shasum -a 256 | shasum -a 256`); it should be `87715dba33ea1d75ef6c689da75ec79d1b6d97307eb8301750d97b29d9c2f56b`.

**Binding spec:** `WO-2026-10-24-001-failsafe-unknown-openness.md` and `WO-2026-10-24-001-step0-report.md` (the two sites, the night-case analysis, the ruling-8 finding). Where this dispatch and the WO differ, the WO wins; report it.

## Do

1. `ConnectivityFailsafe.cpp:228`: `if (Clock::openness() != Clock::Openness::Open || now <= ageBase) return;` → `if (Clock::openness() == Clock::Openness::Closed || now <= ageBase) return;`. Keep the existing comment block; add at most **two comment lines** saying Unknown is treated as Open here (recovery item 18, WO-2026-10-24-001) and why (the circular case at `:190–205`).
2. `ConnectivityFailsafeTest.cpp:159`: `Clock::openness() == Clock::Openness::Open && now > ageBase` → `Clock::openness() != Clock::Openness::Closed && now > ageBase` (the TEST_MODE prediction must follow the real rule).
3. **Re-anchor `connectivity_failsafe_open_hours_test.{sh,cpp}`** (ruling 8; it is the one failing test on the prototype): Part 2's grep for the old text (`:89`, `:136`), the host mirror `failsafeAction(bool open, ...)` (give it three-valued openness; `REAL_OPEN_GATE` becomes the real gate mode, parsed from the supervisor body), and the prediction-file needle (`:169–176`: it must now require `!= Clock::Openness::Closed`). Mutations: restoring `!= Open` in **either** source file must fail this test.
4. **New harness** `connectivity_failsafe_unknown_openness_test.{cpp,sh}` on the **real supervisor body**: extract `connectivityFailsafeSupervisor()` verbatim from `src/power/ConnectivityFailsafe.cpp` by marker (the existing `extract_supervisor` in the open-hours test is the model) and **fail with `COPY_MISMATCH` if the compiled copy differs**; compile it against small fakes (`Clock`, `RecoveryState`, `SystemConfig`, `PowerManager`, `DailyBoundary`, `ConnectivityPolicy`, `ab1805`, `System.reset`, `Log`, `activeConnectAttemptWithinBudget`, battery tier). Cases (each sets openness, a last connection older than `CONNECTIVITY_FAILSAFE_STALE_SEC`, the stage and `lastAction`, then asserts which action ran): (1) **Unknown, stage 0: stage 2 acts** (`System.reset`); (2) **Unknown, stage 2, past the cooldown and jitter: stage 3 acts** (`deepPowerDown`) **the trap case**; also Unknown at stage 2 inside the cooldown does **nothing**, and at stage 3 does nothing (no loop); (3) **Closed: no action**; (4) **Open: unchanged** (stage 2 at stage 0, stage 3 past the cooldown, nothing inside it). Also Unknown with `now <= ageBase` (00:00–06:00 local) does nothing.

## Verify (all required)

- **Mutations (each in a `git worktree add --detach` copy):** restore `!= Open` at `:228` (case 1 and 2 of the new harness and the re-anchored test must fail); restore `== Open` at `ConnectivityFailsafeTest.cpp:159` (the re-anchored test must fail); make Unknown skip the cooldown (the "inside the cooldown does nothing" case must fail); tamper the extracted copy (`COPY_MISMATCH`).
- **Full suite** before and after, `N/N (sh via zsh, py via python3)`, serial; baseline **104**; golden payload unchanged.
- **ARM (Boron 6.4.1, fresh `BUILD_PATH_BASE`, `-frandom-seed=p6`; "before" from `git archive HEAD`):** release and the TEST_MODE build: text/data/bss before and after (prototype: release `151340/1114/2212` unchanged); the whole `.backup` section before and after (outcome A); explain any change over ±64 B by symbol.
- `git diff --check` clean on new lines.

## Implementation Report (≤ 150 lines, your final message)

Files changed with net and gross `src/` lines against +20; test lines and ruling 9's numbers; the re-anchor and its mutations; the harness (cases, `COPY_MISMATCH` shown); the suite before and after; the ARM builds and the `.backup` outcome; the `src/` hash before and after; scratch removal; deviations (or "none"); the model used.
