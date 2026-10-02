AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff (items A and B); run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary harnesses under your scratch directory **`build-tmp/wo20261002-003-stage7/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 dispatch (narrow) — WO-2026-10-02-003 (v34-SleepConfigLeak)

**Goal, in plain language:** free memory stays level across wake cycles (item A), and if memory does run out, the device resets at once with a cause code that says so (item B).

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-02-003-sleep-config-leak`, base `43b8a69` (v33), with the uncommitted diff. Ignore the unrelated uncommitted edit to `docs/RECOVERY_PLAN_2026-09-26.md`.
**Binding spec:** `docs/work-orders/WO-2026-10-02-003-sleep-config-leak.md`, including all Stage 5 decisions in its approval record, in particular the alert-14 decisions.
**Stage 6 reports:** `WO-2026-10-02-003-stage6-copilot-report.md` (A) and `WO-2026-10-02-003-stage6-itemB-copilot-report.md` (B).
**The cause, for reference:** `docs/work-orders/2026-10-02-wake-heap-loss-codex-report.md` (your investigation).

Narrow review: check against the WO's acceptance criteria only. Don't widen the fault model or propose new mechanisms. For any defect, give the smallest fix within the approved design.

## Checks (PASS/FAIL with evidence for each)

### A. The per-wake leak

1. **No reused long-lived sleep configuration remains in `src/`:**
   - no global, static or `extern` `SystemSleepConfiguration`;
   - no assignment to an existing one;
   - every `System.sleep()` site passes a block-scope local;
   - the structural test fails on each of those mutations.
2. **Leak check:**
   - re-run or independently reproduce the ≥ 1,000-cycle check against Device OS 6.4.1's real `system_sleep_configuration.h` (Copilot compiled it on the host with `PLATFORM_ID=3` and a two-line shim, using a counting allocator). Confirm that the GCC-platform build of that header takes the same `new`/`delete` path as nRF52840 (not `HAL_PLATFORM_RTL872X`);
   - **0 bytes lost per cycle** with the new pattern;
   - **the mutation fails** (the old pattern loses bytes every cycle; Copilot reports 144 B per cycle at the ultra-low-power site with standby).
3. **Wake sources unchanged:** each of the four sites configures the same mode, pins and edges, RTC duration, and network standby (when `useNetworkStandby`, under `HAL_PLATFORM_CELLULAR`) as at `43b8a69`. The pre-sleep sequence, breadcrumbs, logging and `AwakeCycles::recordSleepReturn()` are unchanged.
4. **Object lifetimes:** each local configuration lives until after its `System.sleep()` returns (nothing reads a configuration after it's destroyed). The hibernate failure fall-through still works.

### B. The out-of-memory reset

5. **The direct reset:** with `outOfMemory >= 0`, the loop logs the size, `delay(100)`, and calls `System.reset(RESET_CAUSE_OUT_OF_MEMORY)` (7) directly, with no `ERROR_STATE` transition and no dependence on the reset count. Code 7 is in `ResetCause.h`, distinct and non-zero, and the structural test requires it.
6. **Nothing outside the out-of-memory path changed behavior:**
   - the boot-time clear of alert 14 (`Generalized-Core-Counter.cpp`, `clearOomAlertOnBoot`) and alert 14's severity rank (`MyPersistentData.cpp`) are unchanged;
   - removing `resolveErrorAction()` case 14 changes nothing for any other alert;
   - for a stored alert 14 that reaches `ERROR_STATE`, the result is `default` (return to Idle). Confirm that's what the WO accepts.
7. **Mutation:** restoring the `ERROR_STATE` route fails a test.

### Everywhere

8. **Suite and build:**
   - every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N` (Copilot: 60/60); `tests/publish_with_ack_structural_test.py` unchanged and green;
   - the existing test change (`sleep_breadcrumb_sequence_test.py`, a regex for the renamed `System.sleep(...)` argument) keeps its intent;
   - a local boron release build, clean (Copilot: 150556 / 1090 / 2180); `strings` shows `v34-SleepConfigLeak`, product 34;
   - linkage: no `SystemSleepConfiguration` object in `.bss`/`.data`, the destructor runs on the sleep path, and `System.reset(7)` is at the out-of-memory site;
   - net `src/` lines per item against its budget (A about 12, Copilot −2; B about 5, Copilot −7), by your counting rule.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence; binary sizes; the model and reasoning level actually used. Confirm that the working tree is byte-identical to how you found it, and that nothing outside `build-tmp/wo20261002-003-stage7/` was created or deleted.
