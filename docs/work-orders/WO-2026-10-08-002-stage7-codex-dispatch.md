AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-08, §5) · REASONING: medium (the architect's choice: two getter swaps and one validation limit)
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-08-002-sensor-type`, HEAD at the commit that adds this dispatch).
- Run the host suite in place and a local ARM build in a scratch copy.
- Write temporary files under **`build-tmp/wo20261008-002-stage7/`**, and remove **only that directory**.
- Mutate only in scratch copies.

**Not authorized:**
- Any change outside your scratch directory.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.
- Deleting anything you didn't create, including `build-tmp/` itself.

# Stage 7 dispatch: WO-2026-10-08-002 (Step 6 WO 1c, `sensor.type` reaches sensor creation)

**Goal, in plain language:** the `sensor.type` set in the ledgers is the value sensor creation uses, so a configured sensor type is never silently ignored.

**Binding spec:** `docs/work-orders/WO-2026-10-08-002-sensor-type.md`, fix A and rulings 1–4.

**Inputs:**
- Evidence: `docs/work-orders/WO-2026-10-08-002-step0-report.md`.
- Implementation: `docs/work-orders/WO-2026-10-08-002-stage6-copilot-dispatch.md`, and Copilot's report (the end of `build-tmp/WO-2026-10-08-002-stage6-copilot-output.log`).

Treat every claim as unverified.

**What changed** (+1 net `src/` lines claimed):
- `SensorManager.cpp:307` and `Generalized-Core-Counter.cpp:1127` now read `SystemConfig::SensorSettings::get_sensorType()`.
- `ConfigApply.cpp:351` now validates `1, 1`.
- Comments in `SystemConfig.h` and `SensorManager.h`.
- New tests: `sensor_type_source_test`, `sensor_type_validation_test`.

## Test-suite hygiene

`tests/thermal_coupling_structural_test.py` scans the whole worktree. Run the host suite before you create any copy of `src/`, or after removing it.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Linkage and binary (§3, §12.5).** In a fresh-path release build, `initializeFromConfig` and `setup` call `SystemConfig::SensorSettings::get_sensorType`, not the `sysStatus` getter. Give text/data/bss against main `23ccd81`'s 151028 / 1090 / 2196.
2. **Tests.** Report the full suite as `N/N (sh via zsh, py via python3)`.
3. **The goal.**
   - Confirm that every sensor-creation path (boot, idle-enable, post-wake retry) now reads the sensor store.
   - List every remaining caller of `SystemConfig::get_sensorType()`. Only `sensorISR` (dead, left for 2a) should remain in production.
   - Confirm that the persisted layout is unchanged (`persisted_layout_preservation_test.py`) and that the Step 5 distinct-fields test (`persistence_facade_behavior_test.cpp:236-243`) passes unchanged.
4. **Validation (ruling 1).**
   - `sensor.type` 1 is accepted; anything else is rejected, the stored value is unchanged, and the section fails.
   - Confirm the consequence matches WO 1b's accepted per-field apply: the whole apply reports failure, alert 41 is raised at connect, and the other fields still apply.
   - Say whether `docs/reference/alert-codes.md` needs an update. Judge whether alert 41 having a second over-range trigger in v40 is worth a line there.
5. **Migration and timing (ruling 2).**
   - Confirm a type change takes effect only when the sensor is next created: effectively the next reboot, since a ready PIR isn't re-created on wake.
   - Confirm that a device whose sensor store already holds a value other than 1 would build no sensor after a reboot. The fleet is all 1 today, per the Step 0 appendix.
   - Does the narrowed validation stop a new non-1 value reaching the store? Is there any other writer of the sensor store?
6. **Interactions.** No change to occupancy reporting (WO-004), the failsafe (WO 1b) or any payload. Only the config hash reads the sensor store, before and after.
7. **Tests and mutations.**
   - Do the new tests drive real code? Does extraction fail loudly?
   - Re-run Copilot's three mutations (revert A1, revert A2, restore `0, 255`), and confirm each is caught by its targeted check.
   - Add one: swap A1 only. A2 alone shouldn't satisfy the creation test.
8. **Budget.** Net `src/` lines against +20 (about +2 expected).

## Verdict (your final message; it is saved as the verdict file)

- **Header:**
  - one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED;
  - the model and reasoning level used;
  - the test interpreter line.
- **A table of checks 1–8.**
- **Findings,** each with file:line and severity. Separate observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you deleted only your scratch directory.

Limit: 150 lines.
