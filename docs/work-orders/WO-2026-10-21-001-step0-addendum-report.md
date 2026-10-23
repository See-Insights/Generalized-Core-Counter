# WO-2026-10-21-001 Step 0 addendum (option D prototype)

**Author:** Claude Code (controller), read-only, 2026-10-21 · **Base:** branch `wo/2026-10-21-001-sleep-s2-execution` at `1e6d34f` (`src/` identical to `0cb5622`) · No build, no device.

## Result: PROCEED. Option D prototypes at +17 net `src/` lines (≤ +20): Stage 6 dispatched without returning to the architect.

**Prototype (uncompiled; the diff is `WO-2026-10-21-001-optionD-prototype.diff`, a non-binding reference).** In `State_Sleep.cpp`'s anonymous namespace, before its closing `} // namespace` (`:121`): `struct WakeContext`, one file-static struct `standbyReturn` holding the three fields (always defined, so S-1's by-reference arguments need no non-cellular dummies), `effectiveStandby(requested)` (`:181–201`), `hibernateAttempt(overnightFallbackSleep, wakeInSeconds)` (`:639–705`) and `sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)` (`:879–1111`; returns `true` when all attempts failed, else fills the four-field `WakeContext`). The handler calls them where the blocks were, unpacks the four locals, and passes `standbyReturn.forceModemOff, standbyReturn.error` to `SleepTeardown::run(...)` unchanged. `SleepTeardown.*`, `drainSerialBeforeSleep` and every include are untouched.

| Count (same method as the first prototype) | All lines | Non-blank |
|---|---:|---:|
| `State_Sleep.cpp` 1,331 → 1,348 | **+17** (non-move added 40, removed 23) | +13 |

Moved lines are byte-identical except the renames of the three fields (`preserveStandbyAfterNormalReturn` → `standbyReturn.preserveAfterNormalReturn`, `forceModemOffAfterStandbyFailure` → `standbyReturn.forceModemOff`, `standbySleepFailureError` → `standbyReturn.error`, 13 lines, net 0) and one `return;` → `return true;`. **Headroom is 3 lines**: any forward declaration or extra helper breaks the budget, which is a STOP.
**Compile-order check:** none of the 57 identifiers called in the moved blocks is defined in `State_Sleep.cpp` after `:121`; the globals they use (`measure`, `session`, `thrashGuard`, `ab1805`, `userSwitchISR`, the pins, the build flags) come from headers; the only handler locals the blocks read are `overnightFallbackSleep`, `wakeInSeconds`, `useNetworkStandby`, `sleepReason` and `useNetworkStandbyRequested`.

## Ruling-8 table, re-run on D

I overlaid the prototype on a git-backed scratch tree and ran the suite serially: **90/93**. Three tests fail:
| Test | Why | Re-anchor |
|---|---|---|
| `occupancy_report_by_mode_test.sh` | extracts `bool pirWake = sensorCanWake && (wakePin == sensorWake.pin);` by marker from the handler; the line now sits in `sleepAndResume`, and the model orders the sleep call against the handler's decisions | extract from `sleepAndResume`; order the handler's call `sleepAndResume(` against the sleep-or-suppress decisions; mutation: move the occupancy check below that call |
| `sensor_wake_source_test.sh` | `bool pirWake =[^;]*;` now matches twice (the callee and the handler's unpack line) | extract from `sleepAndResume`; mutation: change `wakePin == sensorWake.pin` |
| `loop_stage_sleep_prep_exclusion_test.py` | counts `transitionTo(` in `handleSleepingState()`: 16 expected, 15 found (the dead `sleep-all-attempts-failed` line is now in `sleepAndResume`) | count 15 in the handler and exactly 1 in `sleepAndResume`; mutation: add a transition to either |

**Mutation-checked, still bind on D (each mutation applied to the prototype fails the named test):** drop breadcrumb 28 at the ULP site → `sleep_breadcrumb_sequence_test.py` ("expected 4 … found 3"); make `ulpConfig` static → `sleep_config_ownership_structural_test.py`; drop the standby network flag → `sleep_gate_modem_off_test.sh`; drop `recordSleepReturn` after the ULP call → `awake_cycle_counters_test.sh`; drop the `sleep-attempts-failed-reset` log line → `modem_teardown_confirmation_logging_test.py`; neutralize the STOP-with-GPIO call → `sleep_config_leak_test.sh` and `sleep_breadcrumb_sequence_test.py`; a direct `Clock::isPlausibleEpoch(` in the moved ULP code → `clock_owner_structural_test.py`.
**Widened (architect, test-only):** `clock_owner_structural_test.py` scans all of `src/` for the direct `isPlausibleEpoch(` call, not only `State_Sleep.cpp`. `clock_hibernate_helpers_move_structural_test.py` keeps binding (hibernate stays in `State_Sleep.cpp`, so its include stays). `modem_health_owner_structural_test.py` and `hibernate_cycle_single_owner_structural_test.py` already scan all of `src/`.
**All other tests in the first report's group C still bind** (their ranges do not move and no code leaves the file). `firmware_update_dwell_test.sh` is not touched, so the C1 comment fix does not apply.

## Stage 6 inputs

New: a structural test (the three fields in one struct; only `effectiveStandby`, `sleepAndResume` and the teardown call arguments touch them; no `retained`); the S-1 harness extended with scenarios and a fixture `tests/fixtures/sleep_execution_transcript.txt` recorded from the base, which extracts the three functions verbatim by marker with `COPY_MISMATCH` on any drift (§13 (g)); the three re-anchors above; the widened `clock_owner` test.
