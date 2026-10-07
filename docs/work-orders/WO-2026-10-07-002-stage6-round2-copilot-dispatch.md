AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Edit `src/power/PowerManager.h`/`.cpp` (one new getter, plus reusing it inside `effectiveConnectionMode()` if that keeps the logic in one place).
- Edit `src/Generalized-Core-Counter.cpp` (`:2664` only), `src/diagnostics/ConnectivityFailsafeTest.cpp` (`:186` only) and `src/cloud/DeviceStatusPublisher.cpp` (`:255` only).
- Add or update tests under `tests/`, including `tests/stubs/`.
- Run the host suite and a local ARM release build.
- Your scratch directory is **`build-tmp/wo20261007-002-stage6r2/`**. Keep every temporary file in it and remove **only it** when you finish.

**Not authorized:**
- Commits, pushes, merges, branch changes, stash, reset or checkout.
- Flashing, device settings, or AWS or network access beyond the Particle compile.
- Any edit to `lib/`, `project.properties`, `docs/` or Device OS, and running `bump_version.sh`.
- Any other `src/` change, including:
  - no new battery checks or `BatteryAuthority::commit()` call sites;
  - no change to `BatteryAuthorityCommand.cpp` or `ConfigApply.cpp`;
  - no migration code;
  - no change to the setup gate (`Generalized-Core-Counter.cpp:1574`), the post-wake gates (`State_Sleep.cpp:1614`, `:1683`) or the `SensorManager.cpp:816` log. Those keep reading the raw flag.
- New persisted, status or alert fields.
- **Deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.**

**SIZE BUDGET:** at most **+10 net `src/` code lines** for this round (nonblank, non-comment, including braces, declarations and includes). Over +10 means STOP and report. Don't compress code to meet it (`AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3).

# Stage 6 round 2 dispatch: WO-2026-10-07-002 (config/downgrade overwrite)

**Goal, in plain language:** the configured connection mode is never changed by a downgrade. The device works out the mode it actually uses from the configured mode plus any active downgrade, so when the downgrade ends, it goes back to exactly what the ledger says.

**Repository:** branch `wo/2026-10-07-002-config-downgrade`, HEAD `0e94f81` (round 1, committed). Don't switch branches. Files under `docs/` are records, including the untracked Stage 7 files; don't touch them.

**Binding spec:** `docs/work-orders/WO-2026-10-07-002-config-downgrade.md`, especially its last section, "Stage 7 round 1 and the architect's decisions". The reason for this round is Finding 1 in `docs/work-orders/WO-2026-10-07-002-stage7-verdict.md` (check 9).

**Why this round exists:** after an operator moves a downgraded device off KEEP_ALIVE, the mode in use changes at once, but `lowBatteryMode` stays set until the next `BatteryAuthority::commit()`. That can be after a sleep, or never while connected. Code that reads the raw flag then wrongly sees a downgrade. The fix is that code which acts on "a downgrade is active" reads a value derived the same way as the mode in use, so a stale flag has no effect. The flag itself and its set and clear rules don't change.

## What to implement

| Item | Where | Change |
|---|---|---|
| G | `power/PowerManager.h`/`.cpp` | Add a const getter (for example `downgradeActive()`), true exactly when `SystemConfig::get_sensorMode() == OCCUPANCY` **and** `get_configuredConnectionMode() == INTERMITTENT_KEEP_ALIVE` **and** `PowerConfig::get_lowBatteryMode()`. Make `effectiveConnectionMode()` use it, so the condition exists once: `return downgradeActive() ? INTERMITTENT : configured;`. Document it in the style of the existing getter. |
| F | `Generalized-Core-Counter.cpp:2664` | In `lowBatteryHardActionBlocked`, replace `PowerConfig::get_lowBatteryMode()` with the new getter. `\|\| tier == TIER_SURVIVAL` is unchanged. |
| D | `diagnostics/ConnectivityFailsafeTest.cpp:186` | The same replacement, so the diagnostic mirror matches F. |
| S | `cloud/DeviceStatusPublisher.cpp:255` | The status ledger's `lowBatteryMode` value comes from the new getter. The key name stays `lowBatteryMode`, so the status schema is unchanged. Update the nearby comment (`:250-253`) so it doesn't claim the raw persisted value is published. |

## Tests (outside the budget)

1. **Extend `tests/connection_mode_downgrade_test.cpp`** (or add a sibling) with the gap case from Stage 7 check 9:
   - Start downgraded (OCCUPANCY, configured 3, flag 1, CONSERVING). Apply a ledger value of 1 through the real extracted ConfigApply block, with **no** `commit()` afterwards.
   - Assert that the flag is still 1 (raw), the new getter is false, and the mode in use is 1.
   - Repeat for ledger values 0 and 2, and for sensorMode changed to COUNTING with the flag still set.
   - Also assert the getter is true in the ordinary downgraded state, and false after recovery.
2. **Failsafe and status wiring:** a structural test (extend `tests/connection_mode_downgrade_structural_test.py`) checking that:
   - `lowBatteryHardActionBlocked` in both files and the status `lowBatteryMode` value use the new getter, not `PowerConfig::get_lowBatteryMode()`;
   - the gates at `Generalized-Core-Counter.cpp:1574` and `State_Sleep.cpp:1614` and `:1683` still read the raw flag.

   If a host harness can drive the real failsafe decision cheaply with the existing failsafe test stubs (`tests/connectivity_failsafe_open_hours_test.*`), prefer a behaviour check there; otherwise structural is enough.
3. **Mutations** (each must be caught at run time or by the targeted structural check, not by a compile failure):
   - point F back at the raw flag;
   - drop the configured-mode term from G;
   - drop the OCCUPANCY term from G;
   - point S back at the raw flag.

   Run each one on a copy, or restore it byte-identically by rewriting in place.
4. **Existing tests** that pin the changed lines: update only what they pin, and report each change.

## Verification (run all; report results)

1. **Full host suite,** before and after, as `N/N (sh via zsh, py via python3)`. Round 1's baseline is 68/68. **Run it with no copy of `src/` anywhere under the repo:** `thermal_coupling_structural_test.py` scans the whole repo, including `build-tmp/`.
2. **Local ARM release build:** boron, Device OS 6.4.1, with the §3 command and a **fresh `BUILD_PATH_BASE`** (a directory that doesn't exist yet) under your scratch directory. Give text/data/bss against round 1's 150940 / 1090 / 2196. Show with `nm` whether the new getter is a symbol or inlined.
3. **Size:** net `src/` code lines for this round, and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change per item, with net lines against +10.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261007-002-stage6r2/`.
- Deviations (or "none").
- The model and reasoning level actually used.
