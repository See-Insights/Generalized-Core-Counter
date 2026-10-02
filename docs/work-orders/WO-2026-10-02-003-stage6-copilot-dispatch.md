AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/state/State_Sleep.cpp` (the four sleep sites), `src/Generalized-Core-Counter.cpp` (remove the global at `:131` only) and `src/state/StateMachine.h` (remove the `extern` at `:30` only); run `./bump_version.sh v34-SleepConfigLeak "<one-line note>"`; add or update tests under `tests/`; run the host suite and a local ARM release build. Your scratch directory is **`build-tmp/wo20261002-003-stage6/`**: keep every temporary file in it and remove **only it** when done / Not authorized: commits, pushes, merges, branch changes, stash, reset, checkout, flashing, device settings, AWS or network access, any edit to `lib/`, `project.properties`, `docs/` or Device OS, any change beyond this WO, and **deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`**.
**SIZE BUDGET: about 12 net `src/` code lines, likely net negative (nonblank, non-comment, including braces, declarations and includes). Don't compress code to meet it (`AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3). Going over means STOP and report.**

# Stage 6 dispatch — WO-2026-10-02-003 (v34-SleepConfigLeak)

**Goal, in plain language:** free memory stays level across wake cycles.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-02-003-sleep-config-leak`, `HEAD` `43b8a69` (v33 on main). Do not switch branches. Files under `docs/` are records: don't touch them, including the unrelated uncommitted `docs/RECOVERY_PLAN_2026-09-26.md` edit and the two untracked `docs/work-orders/2026-10-02-wake-heap-loss-*` files.
**Binding spec:** `docs/work-orders/WO-2026-10-02-003-sleep-config-leak.md`. Where this dispatch and the WO differ, the WO wins; report the difference.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec is reported as a deviation. If it can't be done as approved within the budget, stop and report it. Leave an uncommitted working-tree diff.

## What to implement

At each of the four sleep sites in `State_Sleep.cpp`, replace `config = SystemSleepConfiguration();` plus configuring the global with **a fresh local `SystemSleepConfiguration`**, configured identically and passed to `System.sleep()`:

| Site | Today |
|---|---|
| Hibernate | `:1116` reset, `:1117` configure, `:1149` sleep |
| Ultra-low-power | `:1202` reset (remove), `:1344–1360` configure (declare the local here), `:1402` sleep |
| STOP fallback | `:1443`, `:1444–1447`, `:1454` |
| STOP timer-only fallback | `:1460`, `:1461–1462`, `:1469` |

- **Never assign to an existing `SystemSleepConfiguration`.**
- **Remove the global** (`Generalized-Core-Counter.cpp:131`) **and its `extern`** (`StateMachine.h:30`).
- **Keep exactly as they are:** every wake source (pins, edges, the RTC duration, network standby when `useNetworkStandby`), modes, breadcrumbs, logging, `AwakeCycles::recordSleepReturn()`, and the order of the pre-sleep sequence.
- **Version:** `./bump_version.sh v34-SleepConfigLeak "<one-line note>"`, giving product 34.

## Tests

1. **Structural:** fails if `src/` declares a global or static `SystemSleepConfiguration`, or assigns to one (`= SystemSleepConfiguration(`). Mutation: restoring the global and an assignment-reset fails it.
2. **A leak check over many cycles:**
   - ≥ 1,000 simulated sleep cycles configuring each site's wake sources, run under LeakSanitizer or a counting allocator;
   - use Device OS 6.4.1's real `~/.particle/toolchains/deviceOS/6.4.1/system/inc/system_sleep_configuration.h` if it compiles on the host (with minimal shims), or a faithful copy of its allocation behavior if not: the constructor, the builder methods' `new`, the move assignment, the destructor. Say which;
   - **zero bytes lost per cycle with the new pattern;**
   - **mutation:** the old pattern (one long-lived object reset by move assignment each cycle) loses bytes every cycle, and the test fails.
3. **Wake sources unchanged:** each site configures the same pins and edges, RTC duration, and network standby as at `43b8a69`. A structural or behavioral comparison is fine.

Run each mutation on a copy or restore byte-identically. Update an existing test only if it pins text this WO changes, and report each one. `tests/publish_with_ack_structural_test.py` must pass unchanged.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N`, before (v33: 57/57) and after.
2. Local ARM release build (boron, Device OS 6.4.1, after `make clean-user`, in your scratch directory or with outputs restored): text/data/bss against v33's 150780 / 1090 / 2204; `strings` shows `v34-SleepConfigLeak`.
3. Linkage: `nm` shows no global `config` symbol for `SystemSleepConfiguration`, and the sleep path still calls `System.sleep`.
4. Net `src/` lines against about 12, and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change per site, with line counts.
- Tests and mutations, with results, and which leak-check approach you used.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261002-003-stage6/`.
- Deviations (or "none").
- The model and reasoning level actually used.
