AGENT: Codex · MODEL: gpt-5.6-sol · REASONING: high (standard model: the architect judged the narrow round-2 scope doesn't need the top model)
MODEL NOTE (Claude Code, 2026-10-07): the first send named `gpt-6.1-sol` and the API rejected it before any work ("not supported when using Codex with a ChatGPT account"; `gpt-6-sol` and `gpt-6-luna` were rejected the same way). Re-sent unchanged except for this header, on `gpt-5.6-sol`, the standard model in `docs/notes/vendor-model-notes-2026-09.md`. The run log of the failed send is `build-tmp/WO-2026-10-07-002-stage7r2-codex-run-rejected-gpt-6.1-sol.log`.
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted round-2 diff in this worktree (branch `wo/2026-10-07-002-config-downgrade`, on top of round 1's commit `0e94f81`), and the whole WO diff `73f752e` → worktree.
- Run the host suite in place.
- Run a local ARM build in a scratch copy.
- Write temporary harnesses and build copies under your scratch directory **`build-tmp/wo20261007-002-stage7r2/`**, and remove **only that directory** when done.
- Temporarily mutate files for checks, restoring each one byte-identically by rewriting it in place.

**Not authorized:**
- Lasting edits, commits, pushes, stash, reset or checkout.
- Flashing, device settings, network access or AWS access.
- Deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 round 2 dispatch: WO-2026-10-07-002 (config/downgrade overwrite)

**Goal, in plain language:** the configured connection mode is never changed by a downgrade. The device works out the mode it actually uses from the configured mode plus any active downgrade, so when the downgrade ends, it goes back to exactly what the ledger says.

**Binding spec:** `docs/work-orders/WO-2026-10-07-002-config-downgrade.md`, especially its last section, "Stage 7 round 1 and the architect's decisions".

**Inputs:**
- Round 1's verdict: `docs/work-orders/WO-2026-10-07-002-stage7-verdict.md`. It was NOT VERIFIED, failing check 9 (the flag gap) and check 14 (migration).
- Round 2's dispatch: `docs/work-orders/WO-2026-10-07-002-stage6-round2-copilot-dispatch.md`.
- Copilot's report: the final section of `build-tmp/WO-2026-10-07-002-stage6r2-copilot-output.log`.

Treat every claim in them as unverified.

**Round 2 in short:** a new `PowerManager::downgradeActive()` (OCCUPANCY && configured KEEP_ALIVE && `lowBatteryMode`), which `effectiveConnectionMode()` now uses. Three readers are re-pointed to it: the failsafe's hard-stage block (`Generalized-Core-Counter.cpp` around `:2664`), its diagnostic mirror (`ConnectivityFailsafeTest.cpp` around `:186`) and the status ledger's `lowBatteryMode` (`DeviceStatusPublisher.cpp` around `:257`). The gates at `Generalized-Core-Counter.cpp:1574` and `State_Sleep.cpp:1614` and `:1683` deliberately keep reading the raw flag.

**Architect's rules for this round:**
- Round 2 is the last round under the two-round rule. If it doesn't reach VERIFIED, the WO stops.
- Any new edge case you find is reported with a severity, for the architect and Chip to write down and accept or reject. Don't treat it as a design prompt.
- **Check 14 (migration) is accepted, with no code.** A device downgraded at OTA time runs KEEP_ALIVE until a battery check taken while disconnected re-downgrades it. Chip's pre-rollout fleet check covers it. Record it as ACCEPTED; don't fail it.

## Test-suite hygiene

`tests/thermal_coupling_structural_test.py` scans the whole repo, including `build-tmp/`. Run the host suite before you create any build copy, or after removing it. A failure caused by your own copy is not a finding.

## Checks (each one: PASS / FAIL / CONCERN / ACCEPTED, with evidence)

Re-run all sixteen checks from round 1's dispatch (`docs/work-orders/WO-2026-10-07-002-stage7-codex-dispatch.md`) against the current worktree. Checks 1–8 and 10–13 can be confirmed briefly where round 2 doesn't touch them, but say what you re-ran. Give these full treatment:

- **2. Local build.** Boron, Device OS 6.4.1, the §3 command, with a **fresh `BUILD_PATH_BASE`** (a directory that doesn't exist yet). Record text/data/bss against round 1's 150940 / 1090 / 2196.
- **9. The flag gap, re-run.** From round 1's starting state (downgraded, then an operator override to 0, 1 or 2, or sensorMode moved to COUNTING), follow every path round 1 enumerated: normal daytime sleep, night sleep and hibernate, CONNECTED mode, DISCONNECTED mode, firmware-update state, and error and reset loops.
  - On each path, until the next `commit()` clears the raw flag, confirm that the stale flag now has **no behavioural effect**:
    - the failsafe hard stage is not blocked by it;
    - the status ledger reports `lowBatteryMode` false;
    - the mode in use is the configured mode.
  - Name any remaining reader of the raw flag that changes behaviour during the gap. The three intended raw gates only trigger a `commit()`, which clears the flag; confirm that's all they do. `SensorManager.cpp` around `:816` is a log line.
  - PASS if the stale flag is inert on every path.
- **13. Readers of `lowBatteryMode`.** Re-list every reader. Classify each as derived (`downgradeActive()`), an intended raw gate, or a log line, and flag anything else.
- **14. Migration.** Record it as ACCEPTED, per the architect. Also say whether round 2 changes the round-1 analysis in any way, for better or worse. For example, the status ledger and failsafe now report a migrating device as not downgraded once setup clears the flag.
- **15. Tests and mutations.**
  - Review `testStaleFlagBeforeCommit` and the structural additions.
  - Re-run Copilot's mutations and confirm each is caught by its targeted check:
    - F, D and S pointed back at the raw flag;
    - drop the configured-mode term from `downgradeActive()`;
    - drop the OCCUPANCY term.
  - Re-run round 1's six mutations, adapted to the new code layout.
  - Report any mutation that survives.
- **16. Budget.**
  - Round 2's net `src/` code lines (nonblank, non-comment) against **+10**.
  - The WO total (`73f752e` → worktree) against **+35**.
  - Moved lines and replaced call-site lines.
  - Round 1 measured +3 for the whole WO.

## Verdict (your final message; it is saved as the verdict file)

- **Header:**
  - one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED;
  - the model and reasoning level actually used;
  - the test interpreter line.
- **A table of checks 1–16:** result and one-line evidence each, with check 14 as ACCEPTED.
- **Check 9's path table:** for each path, whether the stale flag has any effect before the clearing `commit()`.
- **Findings,** each with file:line and severity. Separate observation from inference.
- **Budget versus actual,** as rows for the closing record: | Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |. One row for round 1 including the controller edit, one for round 2, and one for the WO total.
- **Confirmations:** every mutated file was restored byte-identically (show `git status` and `git diff --stat` matching the start), and you deleted only `build-tmp/wo20261007-002-stage7r2/`.
