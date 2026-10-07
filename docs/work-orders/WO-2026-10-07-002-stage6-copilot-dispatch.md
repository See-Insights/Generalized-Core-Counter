AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Edit `src/persist/SystemConfig.h` and `src/MyPersistentData.cpp`/`.h` (the getter rename only).
- Edit `src/power/PowerManager.h`/`.cpp` (the new getter for the mode in use).
- Edit `src/power/BatteryAuthorityCommand.cpp`.
- Edit `src/cloud/ConfigApply.cpp` (the `connectionMode` block only).
- At every reader the rename breaks, change only the getter call.
- Add or update tests under `tests/`, including `tests/stubs/`.
- Run the host suite and a local ARM release build.
- Your scratch directory is **`build-tmp/wo20261007-002-stage6/`**. Keep every temporary file in it and remove **only it** when you finish.

**Not authorized:**
- Commits, pushes, merges, branch changes, stash, reset or checkout.
- Flashing, device settings, or AWS or network access beyond the Particle compile.
- Any edit to `lib/`, `project.properties`, `docs/` or Device OS.
- Running `bump_version.sh` (the WO names no release).
- Any new persisted field, status field, or alert change.
- Changing any log string the Step 0 bench plan relies on.
- Any change beyond this WO.
- **Deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.**

**SIZE BUDGET:** at most +35 net `src/` code lines (nonblank, non-comment, including braces, declarations and includes). Report three numbers: **net**, **moved** (`git diff --color-moved=zebra`), and **replaced call-site lines** (the one-for-one getter swaps). Over +35 net means STOP and report. Don't compress code to meet the budget (`AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3).

# Stage 6 dispatch: WO-2026-10-07-002 (Step 6 WO 1a, config/downgrade overwrite)

**Goal, in plain language:** the configured connection mode is never changed by a downgrade. The device works out the mode it actually uses from the configured mode plus any active downgrade, so when the downgrade ends, it goes back to exactly what the ledger says.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-07-002-config-downgrade`, HEAD `73f752e` (`src/`, `lib/` and `tests/` are identical to `41ad238`). Do not switch branches. Files under `docs/` are records: don't touch them, including the untracked WO and dispatch files there.

**Binding spec:** `docs/work-orders/WO-2026-10-07-002-config-downgrade.md`, including the architect's ruling and amendments. Evidence and line citations are in `docs/work-orders/WO-2026-10-07-002-step0-report.md`. Where this dispatch and the WO differ, the WO wins; report the difference.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO, and report any correction to the spec as a deviation. If the change needs an architectural decision this dispatch doesn't settle, stop and report it; don't improvise. Leave an uncommitted working-tree diff.

## What to implement (file:line checked at 73f752e)

| Item | Where | Change |
|---|---|---|
| R | `persist/SystemConfig.h:157`, `MyPersistentData.cpp:1050` (and the `.h` if declared there) | Rename `SystemConfig::get_connectionMode()` to `get_configuredConnectionMode()`. Same body, same store (`sysStatus.connectionMode`). `set_connectionMode` keeps its name. Persisted layout is unchanged. |
| P | `power/PowerManager.h`/`.cpp` | Add a const getter (for example `effectiveConnectionMode()`) returning the mode in use. It is **INTERMITTENT** when `SystemConfig::get_sensorMode() == OCCUPANCY` **and** `get_configuredConnectionMode() == INTERMITTENT_KEEP_ALIVE` **and** `PowerConfig::get_lowBatteryMode()` is true. Otherwise it is the configured mode. This reproduces the downgrade state from `BatteryAuthorityCommand.cpp:80-84`, including its stickiness: the flag is set when the downgrade fires and cleared only at recovery. Follow how other code reaches PowerManager. |
| B | `power/BatteryAuthorityCommand.cpp:76-97` | Delete the writes at `:83` and `:90`. The battery code keeps sole ownership of `lowBatteryMode` and now reads the **configured** mode. Keep the flag's transitions the same as today: set once on entering CONSERVING-or-worse while configured KEEP_ALIVE (it must not re-fire on every commit now that the configured mode stays KEEP_ALIVE); cleared at HEALTHY; cleared when the configured mode is no longer KEEP_ALIVE or sensorMode leaves OCCUPANCY. Keep the existing `Log.info` strings unchanged, because the bench plan greps them. In your report, give a before/after table of the branch conditions. |
| C | `cloud/ConfigApply.cpp:445-460` | Compare the ledger value with `get_configuredConnectionMode()` only. Drop `\|\| PowerConfig::get_lowBatteryMode()`, and drop any `lowBatteryMode` clear or BatteryAuthority call made from this block (the flag belongs to battery). Report exactly what you removed. |
| T | The ~29 readers the rename breaks (Step 0 report §2b) | Each reader gets exactly one getter. A reader gets **configured** only if it reports or compares configuration: the config hash at `DeviceStatusPublisher.cpp:113` and the comparison in `ConfigApply.cpp:448`. Everything that decides connecting, sleeping, standby, reporting cadence or connection-related behaviour gets the **mode in use**. For labels and log lines (`State_Sleep.cpp:88`, `:869`; `State_Idle.cpp:318`; `ConnectivityFailsafeTest.cpp:254`), use the mode in use and say so. The `BatteryAuthorityCommand.cpp:77` reader is covered by item B. |

**Reader table (required in your report):** one row per reader with file:line, the getter it now uses, and a one-clause reason. Let the compiler drive this: after R, build and fix every error, so none is missed.

## Tests (outside the budget)

1. **Host behaviour test**, new (for example `tests/connection_mode_downgrade_test.cpp` + `.sh`, matching the existing `*_test.cpp`/`*_test.sh` pairs). Run it against the real `BatteryAuthorityCommand` decision, the real PowerManager getter and the real ConfigApply comparison, using stubs as the existing power tests do. Cover:
   - **Downgrade:** OCCUPANCY, configured 3, tier CONSERVING. The configured mode stays 3, the mode in use is INTERMITTENT, and the flag is set once (a second commit at CONSERVING doesn't re-log or re-set).
   - **Recovery:** tier HEALTHY. The flag clears, the mode in use is 3 again, and the configured mode was never written.
   - **Config re-apply while downgraded (today's flip-flop):** the ledger still says 3, so nothing is written, the flag stays set, and the next commit doesn't flip.
   - **Ledger change while downgraded:** the ledger says 1. The configured mode becomes 1, the mode in use is 1, and the flag clears at the next commit.
   - **Restart:** persisted configured=3 and flag=true. After reload, the mode in use is INTERMITTENT.
   - **COUNTING mode:** no downgrade at any tier.
   - **Migration:** stored configured=1 (old overwrite), flag=true, ledger 3. The first apply writes 3.
2. **Structural test:** no `SystemConfig::set_connectionMode` call in `src/` outside `ConfigApply.cpp` and the defaults in `MyPersistentData.cpp`, and no `get_connectionMode(` remaining in `src/`.
3. **Mutations** (each must make a test fail): restore the `:83` write; drop the OCCUPANCY term from P; restore the `|| get_lowBatteryMode()` clause in C; point one connect-path reader (for example `State_Sleep.cpp:1638`) at the configured getter. A mutation that only causes a compile failure doesn't count as caught. Run each one on a copy, or restore byte-identically by rewriting in place (never `mv` a backup over the source).
4. **Existing tests that pin the old name or behaviour:** `tests/clock_trust_standard_structural_test.py`, `tests/occupancy_session_restart_test.sh`, `tests/stubs/power_source_override_overrides/{MyPersistentData.h,persist/SystemConfig.h}`, and the `battery_authority_*_structural_test.py` files. Update only what they pin, and report each change. `persisted_layout_preservation_test.py` must pass **unchanged**.

## Verification (run all; report results)

1. **Full host suite:** run every `tests/*.sh` with **zsh** and every bare `tests/*.py` with python3. Report `N/N (sh via zsh, py via python3)` before and after.
2. **Local ARM release build:** boron, Device OS 6.4.1, after `make clean-user`, in your scratch directory (command in `AI_DEVELOPMENT_WORKFLOW.md` §3, "Mandatory: local toolchain build"). Give text/data/bss before and after. Use `nm` on the ELF to show that the new PowerManager getter is present (or inlined: say which) and that `get_connectionMode` is absent.
3. **Size:** net / moved / replaced call-site lines, plus `git diff --stat`.

## Implementation Report (required, as your final message)

- The change per item, with the three line counts against the +35 budget.
- The reader table.
- The before/after branch table for `BatteryAuthorityCommand`.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261007-002-stage6/`.
- Deviations (or "none").
- The model and reasoning level actually used.
