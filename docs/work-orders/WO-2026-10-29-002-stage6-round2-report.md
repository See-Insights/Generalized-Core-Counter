# WO-2026-10-29-002 Stage 6 round 2 report (Copilot, claude-sonnet-5.5, medium)

_Verbatim final message, 29 Oct. Controller reproduction: suite 107/107 as one sequential loop; the mutations `drop start < boundary` and `<=` each fail the close-24 and `start == boundary` checks plus the structural `clamp:` check._

Round 2 is implemented and passes: the host suite is 107/107 and the local ARM release build succeeds. Nothing was committed, pushed or merged.

**`src/` lines (WO total +16 against the +20 limit)**

| File | Added | Removed | Net |
|---|---|---|---|
| `src/state/State_Common.h` | 11 | 0 | +11 |
| `src/state/State_Modes.cpp` | 6 | 1 | +5 |
| Total | 17 | 1 | +16 |

- **Clamp:** in `closeOccupancySessionSafely()` it now applies only when `start < boundary`.
- **Awake-path gate:** the extra log and the `detected`/`ignored` locals are gone. It is back to `if (SensorManager::instance().loop() && !occupancyIgnoredWhenClosed())`, so `loop()` still runs first. The wake-path log is untouched.

**Test changes**
1. The `close 24` case now advances the clock by the debounce expiry and expects credit of `300 + 301`, instead of 0.
2. New check: a session with `start == boundary` is not clamped (credit 300).
3. New check: a session with `start < boundary` is clamped to the boundary (credit 1).

I also updated the structural test's clamp check and the `GATE_AWAKE` text in `tests/sleep_wake_structural_test.py` for the new single condition.

**Mutations**
- **Drop the `start < boundary` condition:** caught by the `close 24` and `start == boundary` checks, and by the structural test.
- **`<=` instead of `<`:** caught by the same two checks, and by the structural test.
- **Earlier four:** all four are still caught. Gate removed, `!= Open` gate, and wake-gate removal are caught by name. The awake-gate-site mutation now targets the new condition. The clamp-dropped mutation now removes the `start < boundary` clamp line (`(void)boundary;`).

**Suite:** 107/107 (sh via zsh, py via python3), run as one sequential loop with output to `/dev/null`.

**Build:** local ARM release build using the `AI_DEVELOPMENT_WORKFLOW.md` recipe (Device OS 6.6.0, Boron, fresh `BUILD_PATH_BASE`) gives text/data/bss of 151596/1114/2200. I did not build a before-size from `HEAD`, so there is no delta. I removed the scratch directory afterwards.

**Deviations:** none. I found no "section 3 recipe" in the WO, so I used the build recipe in `AI_DEVELOPMENT_WORKFLOW.md`. The dispatch also names `docs/` as not authorized and the WO is spec-only, so I touched nothing there.

**Model:** Claude Sonnet 5.5 (`claude-sonnet-5.5`).
