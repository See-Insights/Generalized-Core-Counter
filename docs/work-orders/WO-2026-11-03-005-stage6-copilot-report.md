# WO-2026-11-03-005 Stage 6 report (Copilot): flushAllBeforeReset()

**Model:** Claude Sonnet 5.5 (`claude-sonnet-5.5`). **Reasoning:** the dispatch asked for medium; the CLI does not expose the effective setting to the agent, so it is not confirmed here.
**Branch:** `wo/2026-11-03-005-item26-durability-flush`, changes **uncommitted**. Nothing committed, pushed, merged, flashed or released. The Dev-14 bench is Chip's later step and is not claimed.

## Budget: OVER by 1 (`src/` +21 net, limit +20)
`git diff --numstat -- src`: 22 added, 1 deleted.

| File | + | − |
|---|---|---|
| `MyPersistentData.cpp` (two facade definitions) | 2 | 0 |
| `persist/CurrentReadings.h` (decl + comment) | 1 | 0 |
| `persist/SystemConfig.h` (SensorSettings decl + comment) | 1 | 0 |
| `state/StateMachine.h` (declaration, blank line, one-line doc) | 3 | 0 |
| `state/State_Error.cpp` (definition 6 incl. blank, 2 call sites) | 9 | 0 |
| `ThrashGuard.cpp` | 1 | 0 |
| `state/State_Sleep.cpp` (heap guard, sleep exhausted) | 2 | 0 |
| `power/ConnectivityFailsafe.cpp` (2 sites, +1 defer-path flush, −1 flush in the persist helper) | 3 | 1 |

I did not trim style, per the dispatch. The one-line excess is the doc comment on the declaration in `StateMachine.h`; the architect can drop that comment, or accept +21. The defer-path `SystemConfig::flushNow()` (+1) keeps the existing behavior: `persistConnectivityFailsafeState()` is also called by the low-battery defer branch, which must not flush the queue and stores. Moving the flush out of the helper and into stage 2/3 as the new call is what Amendment 1.3 requires.

## Step 0 findings
- **Mismatch:** `docs/work-orders/WO-2026-11-03-002-step0-report.md` is **not in the tree** (nor at the head given). I worked from the WO text and the cited sites. Please supply the file if it carries requirements beyond the WO.
- Cited sites re-read on this base; the line numbers have drifted but each site exists as described: `ThrashGuard.cpp` tier 3, `State_Error.cpp` case 2 and case 3, `State_Sleep.cpp` `nightlyHeapGuard()` and the sleep-exhausted block, `ConnectivityFailsafe.cpp` stage 2 and 3.
- **Alert store confirmed:** `RecoveryState::set_alertCode` / `raiseAlert` forward to `current.*` (`MyPersistentData.cpp`, `currentStatusData::set_alertCode`), so alert codes are in `/usr/current.dat`, which `flushAllBeforeReset()` flushes. Thrash alert 18 is raised before the flush; the Error alert was raised when the state was entered.
- **Queue:** `PublishQueuePosix::instance()` is the app's singleton (`PublishQueuePosixRK.h:55`) and `writeQueueToFiles()` is public (`:234`), synchronous. The design fits (no library change).
- Declaration placed in `state/StateMachine.h`, which every site includes (thrash and failsafe do not include `StateHandlers.h`). Definition in `State_Error.cpp` per Amendment 1.1.
- OOM path (`Generalized-Core-Counter.cpp`) untouched. Persisted layout, `retained`, `lib/`, watchdog callbacks and HIBERNATE untouched.
- Failsafe order is: stage/action/count record, log line, `flushAllBeforeReset()`, reset / `deepPowerDown()`. `clearConnectivityFailsafeRecovery()` still has its own `flushNow()` (not a reset site).
- Order inside the function: current, sysStatus, sensor, then the queue.

## Tests
New: `tests/deliberate_reset_flush_test.py` (compiles the REAL function body against a fake backend, plus structural site checks on the real sources, with in-memory mutants). Checks (all pass):
- flushAllBeforeReset() is defined in State_Error.cpp and declared in StateMachine.h
- it flushes C, S, N, Q in that order (stores before queue)
- the body allocates nothing
- all seven sites call it immediately before the reset/power-down and after their record writes
- OOM structural exclusion (the out-of-memory block has no flush; `Generalized-Core-Counter.cpp` has no call)

Mutations (each detected): drop the C, S, N and Q flush (4); queue flushed before the stores; call removed at each of the 7 sites (7); flush added at the OOM site; failsafe stage 2 and stage 3 flush before their record (2); the persist helper regaining its own flush. Also: `oom_immediate_reset_test.sh` still passes unchanged (its extracted block would fail to compile if a flush call were added there).

Existing tests updated for the one intended addition:
- `connectivity_failsafe_unknown_openness_test.cpp`: fake `flushAllBeforeReset`; each stage 2/3 action now also requires the flush to have run after the stage record and before the action.
- `nightly_heap_guard_flush_test.sh`: order diag flush, `flushAllBeforeReset`, reset.
- `sleep_{cloud_gate,plan,preflight,wake,execution}_structural_test.py`: the call lines are mapped back before the byte compare against the pinned base (the existing OCC_NEW pattern), so everything else is still compared.
- `sleep_plan_equivalence_test.sh` (heap-guard mutation text) and `tests/stubs/sleep_teardown/SleepExecutionFakes.h` (an unrecorded no-op, so the frozen transcripts are unchanged).

**Host suite: 108/108 (sh via zsh, py via python3)**, run after the build copy was removed (baseline before the change was 98 + the then-failing ones fixed above; the new test is the 108th).

## Build and linkage
Local toolchain, Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE` (`build-tmp/wo-2026-11-03-005-stage6/obj-release`): built OK (rc 0, 3m54s, only pre-existing library warnings). `text 151692 / data 1114 / bss 2200`; `tree.bin` 152,810 B, sha256 `8245ea0ce7c38b0e31cb479505ff0973dc8cef2f4a9036ecd950704e2370c38d`.
`nm`/`objdump` on the ELF (`build-tmp/wo-2026-11-03-005-stage6/nm-evidence.txt`): `000c1b9c T flushAllBeforeReset()`; `bl flushAllBeforeReset()` in `ThrashGuard::loop` (1), `connectivityFailsafeSupervisor` (2), `handleErrorState` (2), `handleSleepingState` (2: heap guard inlined, sleep exhausted) = 7. No caller in `loop()` (OOM). Facades present: `CurrentReadings::flushNow`, `SystemConfig::SensorSettings::flushNow`.
The scratch tree and object dir were removed after the build; `build.log` and `nm-evidence.txt` remain.

## Open questions
1. Accept +21 or drop the `StateMachine.h` doc comment?
2. Supply `WO-2026-11-03-002-step0-report.md` (missing from the tree).
3. Flush durations are unmeasured (bench). The 250 ms / 2 s ceilings are not enforced; no timeout was added.
4. The "baseline 98" figure above is the count of passing tests at the first run, before fixing the 9 tests my change affected; the pre-change suite was not separately run as a clean count.
