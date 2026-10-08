AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-08, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Edit `src/Generalized-Core-Counter.cpp` (the cadence rule in `connectivityFailsafeSupervisor()` only).
- Edit `src/state/State_Report.cpp` (the `already connected` branch only).
- Edit `src/cloud/ConfigApply.cpp` (the `reportingIntervalSec` range only).
- Add or update tests under `tests/`, including `tests/stubs/`.
- Run the host suite and a local ARM release build.
- Your scratch directory is **`build-tmp/wo20261008-001-stage6/`**. Keep every temporary file in it and remove **only it** when you finish.

**Not authorized:**
- Commits, pushes, merges, branch changes, stash, reset or checkout.
- Flashing, device settings, or AWS or network access beyond the Particle compile.
- Any edit to `lib/`, `project.properties`, `docs/` or Device OS, or to `src/diagnostics/ConnectivityFailsafeTest.cpp`.
- Running `bump_version.sh`.
- Any new state, timer, flag, persisted field, status field or alert change.
- Changing any log string.
- **Deleting anything you didn't create, including `build-tmp/` itself.**

**SIZE BUDGET:** WO total at most **+20 net `src/` code lines** (nonblank, non-comment); **about +10 expected**. Over +20 means STOP and report. Don't compress code (`AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3). If the two fixes don't both fit, implement item C (the cadence rule) and stop before item B.

# Stage 6 dispatch: WO-2026-10-08-001 (Step 6 WO 1b, the failsafe counts only overdue expected connections)

**Goal, in plain language:** the connectivity failsafe escalates only when a connection the device was expected to make is overdue, never just because time passed while no connection was due.

**Repository:** this worktree, branch `wo/2026-10-08-001-failsafe-overdue`, HEAD at the commit that adds this dispatch (on top of `f2dd392`, main after v39 merged in). Don't switch branches. Don't touch `docs/`.

**Binding spec:** `docs/work-orders/WO-2026-10-08-001-failsafe-overdue.md`, the architect's rulings 1–4. **Evidence and line citations:** `docs/work-orders/WO-2026-10-08-001-step0-report.md`, including its addendum. Where this dispatch and the WO differ, the WO wins; report the difference.

## What to implement

| Item | Where | Change |
|---|---|---|
| C | `Generalized-Core-Counter.cpp`, in `connectivityFailsafeSupervisor()`, right after the existing `connectionAgeSec < CONNECTIVITY_FAILSAFE_STALE_SEC` return (about `:2623-2626`) | **The cadence rule.** Under `#if !CONNECTIVITY_FAILSAFE_TEST_MODE` (ruling 1), read the effective cadence with `ReportingPolicyResolver::resolveRuntime(PowerManager::instance().soc(), now).effectiveIntervalSec` (`reporting/ReportingPolicy.h:33,59`; already included). When the cadence is ≥ `CONNECTIVITY_FAILSAFE_STALE_SEC` **and** `connectionAgeSec < STALE_SEC + cadence`, return. It applies in **every** mode. Add a short comment saying why it is compiled out of the test build. It sits after the cheap early returns, so it costs nothing on most loop passes; confirm that, and say how often `resolveRuntime()` would run once age ≥ 3 h. |
| B | `State_Report.cpp`, the `already connected` branch (about `:278-279`) | **Option (b).** Before `transitionTo(IDLE_STATE, "already connected")`, add `SystemConfig::set_lastConnection(Time.now());` with a one-line comment. An online device has, in fact, a working cloud connection. The alert-40 side effect is accepted (ruling 2). |
| T | `ConfigApply.cpp:265` | **The interval cap (ruling 3).** `validateRange(reportingInterval, 300, 86400, ...)` becomes `300, 65535`. 0 net lines. A configured value above 65535 is then rejected as invalid instead of wrapping. Say what a rejected value does today: does the apply fail as a whole (alert 41?) or skip only that field? Report it, but don't change it. |

## Tests (outside the budget)

1. **Cadence rule, host test** (new, for example `tests/failsafe_cadence_rule_test.cpp` + `.sh`; `tests/connectivity_failsafe_open_hours_test.*` is the existing failsafe harness to extend or follow):
   - **Cadence 1 h:** it escalates at age ≥ 3 h, unchanged from v32.
   - **Cadence 4 h** (for example CRITICAL ×4 on a 1 h interval): no escalation at 3 h, 5 h or 6 h 59 m; escalation at 7 h.
   - **Cadence exactly 3 h:** the threshold is 6 h.
   - **Mode-independent:** the same in INTERMITTENT, KEEP_ALIVE and CONNECTED.
   - **Test build:** with `CONNECTIVITY_FAILSAFE_TEST_MODE` set, the rule is absent and the threshold is plain `STALE_SEC`. A structural check is enough.
2. **CONNECTED-online, host test:**
   - A device with `Particle.connected()` true passing through Report's `already connected` branch gets `lastConnection == now`.
   - The failsafe age then stays below threshold across an open day of hourly reports; there is no stage-2 reset.
   - The offline branch is unchanged: `lastConnection` is still written only by Connect there.
3. **Interval cap:** 65535 is accepted; 65536 and 86400 are rejected and the stored interval is unchanged. Extend the existing ConfigApply or timing test if there is one.
4. **Mutations** (each must be caught by its targeted check, not by a compile failure):
   - drop the cadence rule;
   - make it ignore the `cadence >= STALE_SEC` condition;
   - remove the `#if !CONNECTIVITY_FAILSAFE_TEST_MODE` guard (the structural check);
   - remove the `already connected` refresh;
   - restore the 86400 upper bound.

   Run each one on a copy, or restore byte-identically by rewriting in place.
5. **Existing tests** that pin changed lines (for example `connectivity_failsafe_*`, `persisted_layout_preservation_test.py`, and the `State_Report` transition-count tests): update only what they pin, and report each change. `persisted_layout_preservation_test.py` must pass **unchanged**.

## Verification (run all; report results)

1. **Full host suite,** before and after, as `N/N (sh via zsh, py via python3)`. Main's baseline is 69/69. **Run it with no copy of `src/` anywhere under this worktree**, because `thermal_coupling_structural_test.py` scans the whole tree, including `build-tmp/`.
2. **Local ARM release build:** boron, Device OS 6.4.1, the §3 command, with a **fresh `BUILD_PATH_BASE`** (a directory that doesn't exist yet) in your scratch directory. Give text/data/bss against v39's 150956 / 1090 / 2196. Show with `nm` or disassembly that the cadence rule is present in the release build.
3. **A failsafe test-mode build** (`EXTRA_CFLAGS=-DCONNECTIVITY_FAILSAFE_TEST_MODE=1`, with its own fresh `BUILD_PATH_BASE`). Show that the rule is absent there, by size or disassembly.
4. **Size:** net `src/` code lines per item and in total, and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change per item, with net lines.
- How often `resolveRuntime()` runs under item C.
- What a rejected interval does under item T.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261008-001-stage6/`.
- Deviations (or "none").
- The model and reasoning level actually used.
