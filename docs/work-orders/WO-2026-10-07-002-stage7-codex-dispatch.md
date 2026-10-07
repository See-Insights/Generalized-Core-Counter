AGENT: Codex · MODEL: gpt-6-astra · REASONING: high (top model, chosen by Chip for this WO: the reader classification and the lowBatteryMode interactions need to be exact)
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-07-002-config-downgrade`, HEAD `73f752e`).
- Run the host suite in place.
- Run a local ARM build in a scratch copy.
- Write temporary harnesses and build copies under your scratch directory **`build-tmp/wo20261007-002-stage7/`**, and remove **only that directory** when done.
- Temporarily mutate files for checks, restoring each one byte-identically by rewriting it in place (never `mv` a backup over the source).

**Not authorized:**
- Lasting edits, commits, pushes, stash, reset or checkout.
- Flashing, device settings, network access or AWS access.
- Deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 dispatch: WO-2026-10-07-002 (Step 6 WO 1a, config/downgrade overwrite)

**Goal, in plain language:** the configured connection mode is never changed by a downgrade. The device works out the mode it actually uses from the configured mode plus any active downgrade, so when the downgrade ends, it goes back to exactly what the ledger says.

**Binding spec:** `docs/work-orders/WO-2026-10-07-002-config-downgrade.md`, including "Architect's ruling and amendments" and "Architect's decisions after dispatch". Read it first.

**Evidence:**
- `docs/work-orders/WO-2026-10-07-002-step0-report.md`: citations, writers and readers, the migration check, and the bench plan.
- `docs/work-orders/WO-2026-10-07-002-stage6-copilot-dispatch.md`: the Stage 6 dispatch.
- `build-tmp/WO-2026-10-07-002-stage6-copilot-output.log`: Copilot's Implementation Report is the final section.

You are the independent verifier. Treat every claim in those reports as unverified.

**Controller edit after Stage 6 (Chip pre-authorized; not a round):** Claude Code deleted `BatteryAuthority::clearLowBatteryMode()`, which had become uncalled: its definition in `BatteryAuthorityCommand.cpp` and its declaration and doc comment in `BatteryAuthority.h`. It also deleted the now-unused `#include "power/BatteryAuthority.h"` in `ConfigApply.cpp`, and updated three comments that named the function. That is −5 net code lines. Claude Code's checks afterwards:
- a fresh-path ARM build: text/data/bss 150940 / 1090 / 2196;
- `nm`: `clearLowBatteryMode` 0, `get_connectionMode` 0, `PowerManager::effectiveConnectionMode() const` present;
- the host suite: 68/68 (sh via zsh, py via python3).

Verify the edit like the rest of the diff.

## Important: test-suite hygiene

Several structural tests scan the whole repository. A copy of `src/` anywhere under the repo, including `build-tmp/`, makes `tests/thermal_coupling_structural_test.py` fail. **Run the full host suite before you create any build copy, or after removing it.** A failure caused by your own copy is not a finding.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

### A. Mandatory (workflow §3, Stage 7)

1. **Linkage.**
   - `PowerManager::effectiveConnectionMode()` has production call sites outside its own files, and it appears in the linked ELF (`nm` on a symbolised build).
   - Show the chain for one connect path: reader → `effectiveConnectionMode()` → `get_configuredConnectionMode()` + `get_lowBatteryMode()`.
2. **Local toolchain build.**
   - Boron, Device OS 6.4.1, in your scratch copy, using the §3 command.
   - Start from a **fresh `BUILD_PATH_BASE`** (the directory must not exist), or run `make clean` in `deviceOS/6.4.1/main` with the same `PLATFORM`, `APPDIR` and `BUILD_PATH_BASE` arguments.
   - `make clean-user` is a Workbench buildscripts target, not a Device OS one, and it cleans only the default build path.
   - Record text/data/bss.
3. **Tests.** Report the full suite as `N/N (sh via zsh, py via python3)`. Run `.sh` files with zsh, never bash.
4. **Verify the binary, not the source (§12.5).** Confirm in the ELF (symbols or disassembly) that `BatteryAuthority::commit` no longer stores to the persisted connection mode. No call to `set_connectionMode` should be reachable from it.
5. **Retirement check.**
   - Show that no write to the configured mode remains outside `ConfigApply.cpp` and the defaults in `MyPersistentData.cpp` (grep `set_connectionMode`, including any indirect writer).
   - No `get_connectionMode` remains in `src/`.
   - Every system requirement the retired writes satisfied is still met: the downgrade engages, recovery restores KEEP_ALIVE behaviour, and an operator override ends the downgrade.
6. **Alerts.** Confirm that no alert is raised, ranked or cleared differently. Otherwise the WO needed `docs/reference/alert-codes.md` updated, which is a FAIL.

### B. The WO's specific checks

7. **Reader classification.**
   - Independently list every caller of `effectiveConnectionMode()` and `get_configuredConnectionMode()` in `src/`.
   - Check each against the WO's rule: configured only where the code reports or compares configuration; the mode in use everywhere that connects, sleeps, or sets standby or reporting cadence.
   - Flag any misclassification and any reader the rename missed (for example through a macro, a cast or a cached copy).
8. **Branch equivalence in `BatteryAuthorityCommand::commit`.**
   - Build a case table over sensorMode {OCCUPANCY, COUNTING}, the configured mode {0, 1, 2, 3}, the flag {0, 1} and the tier {HEALTHY, CONSERVING, worse}.
   - For each case, compare old code (stored mode, as at `73f752e`) with new code: flag after commit, mode in use after commit, and log lines.
   - Every difference must be either intended by the WO (the overwrite and the flip-flop end) or a FAIL.
   - **Include Chip's case explicitly:** the operator sets the ledger to INTERMITTENT, or to any non-KEEP_ALIVE value, while downgraded. Old path: config apply's clear ended the downgrade at once. New path: the mode in use changes at once, and the next `commit()` clears the flag through the "configured != KEEP_ALIVE" branch, with a different log line. Confirm the end state is identical.
9. **The flag-clearing gap: a bound or a finding.** In the case above, the flag stays set from the config apply until the next `BatteryAuthority::commit()`. While it is set, the failsafe's hard stages are blocked (`Generalized-Core-Counter.cpp` around `:2664`, and `ConnectivityFailsafeTest.cpp` around `:186`).
   - Enumerate every `commit()` call site: setup, `State_Report.cpp`, and the two in `State_Sleep.cpp`.
   - Determine the worst case: is a `commit()` **guaranteed within one wake cycle** after a config apply, on every path, including night sleep, hibernate, error and reset loops, firmware-update state, and DISCONNECTED or CONNECTED modes?
   - Chip's rule: if it is bounded within one wake cycle, record it as an accepted, known path difference. If any path can carry the flag across a sleep without a `commit()`, that is a FAIL and becomes the round-2 fix. Name the path.
10. **Persistence.** Confirm that `lowBatteryMode` is persisted (`sysStatus`) and survives a reset. So a device that is downgraded at reset still runs INTERMITTENT after reboot, before any `commit()`, as it did when INTERMITTENT was stored. Check the order at boot: is any reader of the mode in use executed before persistent data is loaded?
11. **Side effects of the old write at `:83`.** List everything that happened because the stored mode changed at `:83`: a disconnect, a state transition, a log line, a publish, a config-hash change, a status-ledger write, and any code that compares the previous and current stored mode. Confirm that each one still happens exactly once when the flag is set now, or is intentionally gone (the hash). In particular, check whether anything listened to the stored-mode change (for example a status publish triggered by a config-hash change) that now goes silent.
12. **The config hash.** Confirm that a downgrade no longer changes the configuration hash (`DeviceStatusPublisher.cpp` around `:113`), and that a ledger change of `connectionMode` still does.
13. **Readers of `lowBatteryMode`.** List every reader in `src/`, including the status ledger's `battery.lowBatteryMode`, the failsafe and setup. Confirm that none relied on config apply clearing the flag, beyond the gap judged in check 9.
14. **Migration.** A device that was downgraded under the old firmware has stored mode 1 and flag 1, and its ledger says 3. Confirm that the first config apply after the OTA writes 3. Confirm that until then it behaves as it did before the OTA. Confirm that config apply runs on every successful cloud connection (Step 0 report §8).
15. **Tests and mutations.**
    - Review `tests/connection_mode_downgrade_test.cpp` and `tests/connection_mode_downgrade_structural_test.py`. Does the behaviour test exercise the real `commit()`, the real PowerManager getter and the real ConfigApply block, or a copy? If it extracts ConfigApply code, can the extraction drift from the source without the test noticing?
    - Re-run Copilot's four mutations and confirm that each is caught **by the check aimed at it**, not incidentally:
      - restore the `:83` write;
      - drop the OCCUPANCY term from the derivation;
      - restore `|| get_lowBatteryMode()` in ConfigApply;
      - point `State_Sleep.cpp:1638` at the configured getter.
    - Add these mutations:
      - drop the `!lowBatteryDowngradeActive` guard on the downgrade branch (the flag re-fires every commit);
      - change the stale-clear branch back to `!= INTERMITTENT`.
    - Report any mutation that survives.
    - Review the test edits to the existing tests and stubs: did any change weaken an assertion?
16. **Budget.** Net `src/` code lines (nonblank, non-comment) for the whole diff, including the controller edit, against the +35 cap. Also moved lines (`git diff --color-moved=zebra`) and replaced call-site lines. Copilot reported +8 net / 0 moved / 29 + 4 replaced, before the controller's −5.

## Verdict (your final message; it is saved as the verdict file)

- **Header:**
  - one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED;
  - the model and reasoning level actually used;
  - the test interpreter line.
- **A table of checks 1–16:** result and one-line evidence each.
- **The check 8 case table.**
- **The check 9 worst-case bound:** the path and its timing, and the accepted or FAIL ruling under Chip's rule.
- **Findings,** each with file:line, severity, and whether it needs a round 2.
- **Budget versus actual,** as one row for the closing record: | Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |.
- **Confirmations:** every mutated file was restored byte-identically (show `git diff --stat` matching the start), and you deleted only `build-tmp/wo20261007-002-stage7/`.
- **Observation versus inference:** keep them separate throughout.
