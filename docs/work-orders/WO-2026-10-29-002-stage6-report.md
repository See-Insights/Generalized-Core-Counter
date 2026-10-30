# WO-2026-10-29-002 Stage 6 Implementation Report (Copilot, claude-sonnet-5.5, medium)

_Verbatim final message of the Copilot run, 29 Oct. Controller reproduction below._

Implemented, not committed. Suite: 106/106 before, 107/107 after. ARM release build passes.

**`src/` size:** +21 added, −1 removed, net **+20**. That is exactly the hard limit, not over it. State_Common.h is +10 and State_Modes.cpp is +10 (11 added, 1 removed).
**Tests:** +305 added, 1 removed, against a guideline of +100. 219 of that is in modified files and 86 is the new structural test.

**Design**
- **Helper:** `occupancyIgnoredWhenClosed()` returns `Clock::openness() == Clock::Openness::Closed`.
- **Awake path:** `handleOccupancyMode()` still calls `loop()` so the sensor flag is consumed, and ignores the detection with one log line when closed.
- **Wake path:** the OCCUPANCY `pirWake` branch logs and returns `false` right after `thrashGuard.markProgress`.
- **Clamp:** inside `closeOccupancySessionSafely()`, when the helper is true, `closeAt = std::min(closeAt, DailyBoundary::check(now).boundary)`.
  - **Boundary source:** `DailyBoundary::check()` already gives the last boundary, so no new function was needed.
  - **Explicit boundary:** the daily-cleanup call passes that same boundary, so it is unchanged. I clamp with `min` and don't try to tell a default `closeAt` from an explicit one.
  - **`close == 24` site:** a carried session therefore ends at 0 credit, because its start is the boundary.

**Tests**
- **Added** to `occupancy_report_by_mode_test`, as `testEndsAtClose`, run for all 5 modes:
  - a session open at the close is credited to the close and ended;
  - an awake detection after the close starts nothing and doesn't extend an open session;
  - a PIR wake after the close does the same;
  - a debounce expiring after the boundary credits only to the boundary, awake and on a wake;
  - a night of detections leaves the total at 0;
  - the `close == 24` carried session ignores detections and ends on its debounce;
  - Open (the 0-24 case) and Unknown are unchanged.
- **Structural:** new `occupancy_ends_at_close_structural_test.py` checks the helper is used at exactly the two sites, has no `!= Open` form, and that the cleanup close and the `close == 24` carry-over are intact.
- **Mutations**, each caught by a named check:
  - gate removed: `close: an awake detection after` and `close: a PIR wake after`;
  - `!= Open`: `close: unknown:`;
  - awake site gate removed: `close: an awake detection after`;
  - wake site gate removed: `close: a PIR wake after`;
  - clamp dropped: `close: … debounce expiring after the boundary`;
  - session left open at the close: the structural check `cleanup:`;
  - the structural test also catches the `close == 24` carry-over being changed.

**Existing tests adjusted:**
- `occupancy_session_restart_test.sh`: added fakes for Openness (Open) and `DailyBoundary`, plus the helper extract.
- `occupancy_report_by_mode_test`: `.sh` and `.cpp` get the helper extract and a settable `DailyBoundary` fake.
- `sleep_wake_equivalence_test.cpp`: the gate is stubbed to `false`, so the frozen S-6 transcript still holds.
- `sleep_wake_structural_test.py`: the two gates are mapped back to base text before the S-6 comparison.

**Build** (text/data/bss):

| | text | data | bss |
|---|---|---|---|
| Before (`git show HEAD:` sources) | 151492 | 1114 | 2200 |
| After | 151644 | 1114 | 2200 |

The helper is inlined, so `nm -C -S` shows no symbol for it. `handleOccupancyMode` grew 0x11c→0x130, `processSleepWakeEvent` 0x250→0x268, and `closeOccupancySessionSafely` 0x1bc→0x1dc.

I removed only `build-tmp/wo20261029-002-stage6/`, and `build-tmp/` with it since I had created it. A few test and build-script edits went through bash/python instead of the edit tool.

**Deviations:** none from the dispatch. Model: Claude Sonnet 5.5 (`claude-sonnet-5.5`).

## Controller reproduction (Claude Code, 29 Oct)

- `src/` diff read: `git diff --stat -- src` = 21 added, 1 removed, **net +20 (at the hard limit, not over)**.
- Suite: a single sequential loop gave 102/107 with five sleep tests failing (`sleep_{execution,plan,preflight,wake}_equivalence_test.sh`, `sleep_cloud_gate_structural_test.py`); **each passes when run alone** on this tree and on the v47 baseline. The cause of the loop failures was not found; recorded, not explained. Treat the suite as 107/107 by individual runs (sh via zsh, py via python3).
- Mutations reproduced in copies outside the repo: gate always false and `!= Open` and clamp dropped each fail named checks (`close: an awake detection after the close starts nothing`, `close: unknown: ...`, `close: ... debounce expiring after the boundary ...`, plus the structural `helper:` and `clamp:` checks).
- `DailyBoundary::check(now).boundary` is the most recent boundary (`DailyBoundary.cpp:30-35`: `if (now < boundary) boundary -= 86400`), so the clamp is right for `close == 24` too.
- Overrun: tests +305 against a +100 guideline (about 3x; the dispatch said not to stop below about 2x unless noted). Flagged for the architect.
