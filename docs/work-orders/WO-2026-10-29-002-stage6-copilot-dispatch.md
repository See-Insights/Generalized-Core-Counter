AGENT: Copilot · MODEL: claude-sonnet-5.5 (probed 2026-10-29, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:**
  - `src/state/State_Common.h`: add the helper `occupancyIgnoredWhenClosed()` next to `reportsOccupancyChangesNow()`, and the credit clamp inside `closeOccupancySessionSafely()`;
  - `src/state/State_Modes.cpp`: the two gate sites only (`handleOccupancyMode()` and the OCCUPANCY branch of `processSleepWakeEvent()`);
  - an include in those files if needed.
- **Tests:** add or extend host tests under `tests/` (the S-6 wake harness, `occupancy_session_restart_test.sh`, `close_before_night_sleep_test.py`, `sleep_wake_equivalence_test.*` are the nearest); adjust only what this change requires elsewhere, reporting each.
- **Run** the host suite and the local ARM release build.
- **Scratch directory:** **`build-tmp/wo20261029-002-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- `State_Report.cpp`'s close cleanup and its `close == 24` carry-over (`:79-82`): **unchanged**.
- `Clock::openness()` and any Clock file; `PIRSensor`; COUNTING branches; Unknown-openness behavior (item 19 stays separate).
- Any other `src/` change, `docs/`, `lib/`, Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Device or network access beyond the Particle compile.
- Deleting anything you didn't create.

**SIZE BUDGET:**
- **`src/`:** net **<= +20, HARD**: over means STOP and report (section 13 (b)). Expected +8 to +14. One commit.
- **Tests:** guideline +100 added lines; record the actual.

# Stage 6 dispatch: WO-2026-10-29-002 (occupancy ends at the close; recovery item 10; ships in v48)

**Goal, in plain language:** after the daily close, on a normal-hours site, no PIR detection starts or extends an occupancy session, and a session closing after the boundary is credited only up to the boundary. The next day starts at 0.

**Repository:** this worktree, branch `wo/2026-10-29-002-occupancy-ends-at-close` (from main `da6bef8` = v47). Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-29-002-occupancy-ends-at-close.md` and `docs/work-orders/WO-2026-10-29-002-step0-report.md` (citations at `da6bef8`). Where this dispatch and the WO differ, the WO wins; report the difference. **Architect rulings, 29 Oct:** (1) sites with `closeHour == 24` (e.g. 6-24) are gated with the same helper and the existing carry-over across midnight stays unchanged; (2) clamp the credit to the boundary.

## Design (do not widen)

1. **Helper** in `State_Common.h`: `inline bool occupancyIgnoredWhenClosed() { return Clock::openness() == Clock::Openness::Closed; }` with a short comment. `== Closed` only, never `!= Open`: `Unknown` (untrusted clock or invalid config) must behave as today, and for 0-24 `openness()` is never `Closed`.
2. **Awake path:** in `handleOccupancyMode()` (`State_Modes.cpp:63-65`), keep calling `SensorManager::instance().loop()` (it consumes the sensor flag) but act on a detection only when `!occupancyIgnoredWhenClosed()`. A short `Log.info` when a detection is ignored is welcome, but rate-limit nothing new and keep it to one line.
3. **Wake path:** in `processSleepWakeEvent()`, wrap the OCCUPANCY branch for `pirWake` (`State_Modes.cpp:206-270`) in the same test. The LED-timeout close at the top (`:166-186`) is unchanged.
4. **Credit clamp** in `closeOccupancySessionSafely()` (`State_Common.h:333-395`): when `occupancyIgnoredWhenClosed()` is true and the caller passed no explicit boundary (the default `closeAt = Time.now()`), clamp `closeAt` to the most recent close boundary so a debounce expiring after the boundary but before the close report credits only up to the boundary. Reuse an existing helper for the boundary (look at `DailyBoundary` and `Clock`); if none gives the last boundary without a new function of more than a few lines, STOP and report instead of inventing one. The daily-cleanup call, which passes `boundary` explicitly, must be unchanged.
5. **Not touched:** the cleanup, the `close == 24` carry-over, COUNTING.

## Tests

- **Host (nearest existing harness):**
  - a session open at the close is credited to the close and ended;
  - a detection after the close, awake and on a wake, starts nothing and adds nothing;
  - a debounce that expires after the boundary but before the cleanup credits only to the boundary;
  - the next day's first report shows `dailyoccupancy` 0;
  - a 0-24 site is unchanged;
  - an `Unknown` openness (untrusted clock) is unchanged;
  - a `closeHour == 24` site: detections 00:00-06:00 are ignored; the carried session ends on its debounce.
- **Structural:** the helper is used at both start/extend sites; no `!= Open` form.
- **Mutations** (in `$TMPDIR` copies, never in `src/`), each failing a named check: remove the gate; gate on `!= Open` instead of `== Closed`; leave the session open at the close (drop the explicit-boundary credit); drop the clamp.
- **Full suite** before and after: `N/N (sh via zsh, py via python3)`; record the baseline yourself. Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
- **Local ARM release build,** the section 3 recipe, fresh `BUILD_PATH_BASE`: text/data/bss before (from `git show HEAD:` sources) and after, and `nm -C -S` showing the helper's use (it may be inlined: say so).

## Implementation Report (required, as your final message)

- Files, net and gross `src/` lines with arithmetic; test lines added and removed.
- Design choices: how the boundary was obtained for the clamp, exact signatures.
- Test results: each case above, each mutation and its catching check, suite before and after.
- Builds: sizes and `nm`.
- Confirmations: you removed only your scratch directory.
- Deviations (or "none"), and the model used.
