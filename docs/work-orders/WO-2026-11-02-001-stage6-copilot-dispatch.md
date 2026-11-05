AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:** edit `src/power/PmicFaultMonitor.cpp`, `src/power/PmicFaultMonitor.h`, the one call site in `src/sensors/SensorManager.cpp` only if the new input (battery voltage) must be passed in, and the tests the change requires (`tests/pmic_fault_monitor_test.cpp` and its `.sh`, or a new test file next to it); run the host suite and the local ARM release build. **Scratch directory:** `build-tmp/wo20261102-001-stage6/`; remove only it.
**Not authorized:** anything else in `src/`, `docs/`, `lib/`; commits, pushes, merges, branch changes, stash, reset or checkout; deleting anything you didn't create, including `build-tmp/` itself.
**SIZE BUDGET:** `src/` net **<= +20, HARD**.

# Stage 6: WO-2026-11-02-001, USB charge-latch recovery (recovery item 22, USB scope)

Binding spec: `docs/work-orders/WO-2026-11-02-001-usb-charge-latch-recovery.md` (read it first, and its Step 0 report `WO-2026-11-01-002-step0-report.md` sections 5, 6 and 7 for the code facts). Base: `origin/main` a7b69ce; branch `wo/2026-11-02-001-usb-charge-latch-recovery`; the tree is clean except the four WO records already copied into `docs/work-orders/` (leave them).

## What to build
In `PmicFaultMonitor::pollAndRemediate()` (it already reads REG08/REG09 and has `safeToCharge`, `powerGood`, `vbusStatus`, `thermalRegulation`, `chargeStatus`, the USB-backed tests, and the level-1 `advanceChargeCyclePhase()` machinery), add the **fault-free "Not Charging" recovery**:
- **Condition (all, continuously for 2 h, millis-based, boot-scoped `static` state, no `retained`, no persisted-layout change):** USB-backed input present, `powerGood`, `chargeStatus == 0` (Not Charging), no `kPmicChargeFaultMask` or `kPmicBatFaultMask` bit, battery voltage **< 4.05 V** (voltage only, no SoC condition), not `thermalRegulation`, `safeToCharge`. Any clause breaking resets the timer. `powerGood` false = no action.
- **Action at 2 h:** request the existing level-1 soft cycle (disable, wait, re-enable) through the smallest seam; raise **alert 21**; log **one** `Log.warn` line with REG01, REG08 and REG09 (read REG01 through the PMIC object; if the wrapper has no REG01 getter, use the existing getter for it and say which). **At most one attempt per 24 h**; if charging does not restore, alert 21 stays raised, no escalation, no loop.
- Reuse the existing cooldown and cycle machinery; **no new state machine, no `PMIC::reset()`, no fuel-gauge quick-start, no solar scope, no change to the fault-driven path.**
- Battery voltage: pass in what the call site already has (`vcell` is sampled in `SensorManager::batteryState()` before the `pollAndRemediate` call; check the order) and keep the signature change minimal.
- Put the condition in a **pure function** (inputs: elapsed ms since the condition began or a "condition holds" bool plus clock, last-attempt time) so a host test can drive it with no PMIC.

## Tests
Host tests for the condition: each clause breaking it resets the timer; no action before 2 h, one action at the 2 h boundary; the 24 h attempt limit; the 4.05 V exclusion (4.049 acts, 4.05 does not). Mutations (copies in `$TMPDIR`, never `src/`): `>=`/`>` at the 2 h boundary, `<`/`<=` at 4.05 V, drop the `powerGood` clause, drop the 24 h limit; each must be caught. Add a structural check if the repo's pattern for `src/` call sites calls for it. Suite: one sequential loop with output to `/dev/null`, `N/N (sh via zsh, py via python3)`; baseline from `main`: run it first and report the number. Local ARM release build per `AI_DEVELOPMENT_WORKFLOW.md` §3 recipe, fresh `BUILD_PATH_BASE`: report sizes.

## Report (final message)
Per file net and gross `src/` lines with the arithmetic and the total against +20; total test lines, fixture lines and suite count against `0c565fa`; each mutation and its catching check; the exact seam added; suite and build results; the model and reasoning level actually used; deviations or "none". If the plan cannot be done within +20 or needs a different architecture, stop and say so; do not improvise.
