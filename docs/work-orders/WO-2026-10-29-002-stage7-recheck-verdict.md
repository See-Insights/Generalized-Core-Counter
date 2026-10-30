# WO-2026-10-29-002 Stage 7 recheck verdict (round 2 only)

_Codex gpt-5.6-sol, high, read-only; final message saved verbatim (absolute paths shortened by the reader)._

VERIFIED WITH CONCERNS — **Low**

1. **Clamp:** `occupancyIgnoredWhenClosed()` and `start < boundary` jointly guard the clamp ([State_Common.h:359](src/state/State_Common.h:359)).

   - `start < boundary`: credits `boundary - start`.
   - `start == boundary`: no clamp; the close-24 carry starts at `max(boundary, originalStart)` ([State_Report.cpp:79](src/state/State_Report.cpp:79)) and retains credit through its normal debounce close.
   - `start > boundary`: no clamp; an ordinary close credits `closeAt - start`. The explicit daily-cleanup call remains `closeAt = boundary` ([State_Report.cpp:60](src/state/State_Report.cpp:60)) and safely credits zero because future effective starts are folded to `closeAt` ([State_Common.h:385](src/state/State_Common.h:385)).

2. **Low concern:** after an Unknown-clock detection followed by trust becoming Closed, `start > boundary` can credit closed-hours time on an ordinary debounce close. It is bounded and arithmetic-safe ([State_Common.h:396](src/state/State_Common.h:396)), but may be semantically wrong against “nothing after close is counted.”

3. **Awake path:** left operand `SensorManager::instance().loop()` executes first under `&&`; the single condition is present ([State_Modes.cpp:66](src/state/State_Modes.cpp:66)). Awake locals and log are gone; the wake-path log remains at line 211. No other awake-path behavior changed. `sleep_wake_structural_test.py` passes.

4. **Size:** `git diff --numstat -- src` reports `State_Common.h +11/−0`, `State_Modes.cpp +6/−1`: **+17/−1, net +16**.

5. **Tests/mutations:** close-24 credit is asserted at [occupancy_report_by_mode_test.cpp:744](tests/occupancy_report_by_mode_test.cpp:744); equality and before-boundary cases at lines 751–759. Both unconditional-clamp and `<=` mutants are defined at [occupancy_report_by_mode_test.sh:532](tests/occupancy_report_by_mode_test.sh:532) and are killed by close-24/equality expectations. The before-boundary case appropriately still passes those two mutants and instead pins clamp presence.

6. The structural test pins the exact `<` condition ([occupancy_ends_at_close_structural_test.py:15](tests/occupancy_ends_at_close_structural_test.py:15)); its in-memory mutations pass. Gap: no behavioral `start > boundary` case.

**Model:** gpt-5.6-sol · **Reasoning:** high