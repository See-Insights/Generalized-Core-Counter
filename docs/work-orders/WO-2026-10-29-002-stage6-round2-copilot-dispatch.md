AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:** edit `src/state/State_Common.h` (the clamp in `closeOccupancySessionSafely()` only), `src/state/State_Modes.cpp` (the awake-path gate only), and the tests the change requires; run the host suite and the local ARM release build. **Scratch directory:** `build-tmp/wo20261029-002-stage6r2/`; remove only it.
**Not authorized:** anything else in `src/`, `docs/`, `lib/`; commits, pushes, merges, branch changes, stash, reset or checkout; deleting anything you didn't create.
**SIZE BUDGET:** `src/` total for the WO (round 1 plus this round) **net <= +20, HARD**. The tree already stands at +20; this round must come out **at or below +20** (concern 3 removes lines, concern 2 adds about two).

# Stage 6 round 2: WO-2026-10-29-002, architect pre-rulings of 29 Oct on the Stage 7 verdict (`WO-2026-10-29-002-stage7-verdict.md`)

Same worktree and branch; the round-1 diff is uncommitted and is your starting point. Binding spec: the WO.

1. **Concern 2 (clamp):** clamp only when the session started **before** the boundary. In `closeOccupancySessionSafely()`: when `occupancyIgnoredWhenClosed()`, take `DailyBoundary::check(now).boundary` and set `closeAt = std::min(closeAt, boundary)` **only if `start < boundary`**. A `close == 24` session carried across midnight (start == boundary) keeps its normal credit up to its debounce close. A session that started before the boundary is clamped as now.
2. **Concern 3 (log):** remove the awake-path "OCCUPANCY detection ignored: closed" `Log.info` and the `detected`/`ignored` locals; restore a single condition, `if (SensorManager::instance().loop() && !occupancyIgnoredWhenClosed())`, so `loop()` still always runs first. Leave the wake-path log as is.
3. **Tests:** update the `close 24` case to expect the carried session's credit (debounce expiry minus the boundary start) instead of 0; add: a session with `start == boundary` is not clamped; a session with `start < boundary` is clamped. Mutations (copies in `$TMPDIR`, never `src/`): drop the `start < boundary` condition (the carried case must fail); `<=` instead of `<` (the `start == boundary` case must fail); keep the earlier four. Update the structural test's clamp check for the new condition.
4. **Suite:** run as one sequential loop with output to `/dev/null`, `N/N (sh via zsh, py via python3)`; the baseline is 107/107. Local ARM release build, section 3 recipe, fresh `BUILD_PATH_BASE`: sizes.

**Report (final message):** per file net and gross `src/` lines with the arithmetic and the total against +20; the three test changes and each mutation with its catching check; suite and build; deviations or "none"; the model used.
