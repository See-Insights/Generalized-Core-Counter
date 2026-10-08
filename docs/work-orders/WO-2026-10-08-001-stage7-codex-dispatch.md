AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-08, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-08-001-failsafe-overdue`, HEAD `9f91aa4`).
- Run the host suite in place.
- Run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261008-001-stage7/`**, and remove **only that directory** when done.
- Mutate only in scratch copies.

**Not authorized:**
- Any change to tracked or untracked files outside your scratch directory.
- Commits, pushes, stash, reset or checkout.
- Flashing, device settings, network access or AWS access.
- Deleting anything you didn't create, including `build-tmp/` itself.

# Stage 7 dispatch: WO-2026-10-08-001 (Step 6 WO 1b, the failsafe counts only overdue expected connections)

**Goal, in plain language:** the connectivity failsafe escalates only when a connection the device was expected to make is overdue, never just because time passed while no connection was due.

**Binding spec:** `docs/work-orders/WO-2026-10-08-001-failsafe-overdue.md`, the architect's rulings 1–4.

**Inputs:**
- Evidence: `docs/work-orders/WO-2026-10-08-001-step0-report.md`, including its addendum.
- Implementation: `docs/work-orders/WO-2026-10-08-001-stage6-copilot-dispatch.md`, and Copilot's report (the final section of `build-tmp/WO-2026-10-08-001-stage6-copilot-output.log`).

Treat every claim in them as unverified.

**What changed** (three `src/` files, +9 net code lines claimed):
- **C:** the cadence rule in `connectivityFailsafeSupervisor()`, under `#if !CONNECTIVITY_FAILSAFE_TEST_MODE`.
- **B:** `SystemConfig::set_lastConnection(Time.now())` in Report's `already connected` branch.
- **T:** the `reportingIntervalSec` validation upper bound is 65535.

New tests: `failsafe_cadence_rule_test`, `connected_online_last_connection_test` and `reporting_interval_cap_test`.

## Test-suite hygiene

`tests/thermal_coupling_structural_test.py` scans the whole worktree, including `build-tmp/`. Run the host suite before you create any copy of `src/`, or after removing it. A failure caused by your own copy is not a finding.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

### Mandatory (workflow §3, Stage 7)

1. **Linkage.** Each change is in the linked release ELF. Show the `resolveRuntime` call in `connectivityFailsafeSupervisor`, and the `lastConnection` store in `handleReportingState`'s connected branch.
2. **Local toolchain builds**, each with a **fresh `BUILD_PATH_BASE`**:
   - the release build: give text/data/bss against v39's 150956 / 1090 / 2196;
   - a failsafe test-mode build (`EXTRA_CFLAGS=-DCONNECTIVITY_FAILSAFE_TEST_MODE=1`): show the cadence rule is **absent** there (ruling 1).
3. **Tests.** Report the full suite as `N/N (sh via zsh, py via python3)`.
4. **Verify the binary (§12.5),** as in check 1, by disassembly or symbols.
5. **Alerts.** Does any change alter how an alert is raised, ranked or cleared? Two to look at specifically:
   - ruling 2's accepted alert-40 side effect, which needs no doc change if it's only a reach change;
   - check 8 below (alert 41).

   Say whether `docs/reference/alert-codes.md` needs an update.

### WO-specific checks

6. **The cadence rule.**
   - Confirm the threshold is `max(STALE, STALE + cadence)` exactly as ruled: cadence ≥ 3 h gives cadence + 3 h; otherwise 3 h. It must apply in every mode.
   - Confirm that a genuinely failing device on a short cadence still escalates on the v32 schedule (about 3 open hours).
   - Confirm that a failing device on a 4 h cadence escalates at 7 h.
   - Confirm that `effectiveIntervalSec` here is the same value the cadence logic uses to decide when the device connects (`ReportingPolicy`, `State_Report.cpp`). If they could differ (a different SoC sample, the clock, a tier change between calls), say how far, and whether the failsafe could then fire before a due connection.
   - **Cost:** it runs `resolveRuntime()` on every loop pass once age ≥ 3 h, including during the stage-3 cooldown and in the stage-0 low-battery case (recovery plan, "Failsafe flash churn"). Is that acceptable for the loop's 100 ms budget? Does `resolveRuntime()` have side effects: logs, persisted writes, sampling?
7. **The CONNECTED fix (option b).**
   - Confirm that `lastConnection` is now refreshed only while `Particle.connected()` is true in Report.
   - List every reader of `get_lastConnection()` (Step 0 addendum §2) and confirm each one's change matches what the addendum predicted:
     - the alert-40 "connected recently" test (accepted);
     - the alert-40 force-connect;
     - `LedgerClient.cpp:114`'s input window;
     - the status age.
   - Confirm that KEEP_ALIVE and INTERMITTENT behave exactly as before. They should never reach the `already connected` branch after a wake, because the sleep gate waits for the cloud to drop. If they can, say how.
8. **The interval cap (ruling 3) and what a rejected value does.** Copilot reports that a value above 65535 makes `applyTimingConfig` return false, so `applyConfigurationFromLedger` returns false. That skips `validateStoredData` and the status publish, and leaves sections already applied in place.
   - Trace it fully: does it raise **alert 41** (`State_Connect.cpp` around `:559-563`) on every connection while the ledger holds such a value? Does a deferred apply (`Cloud::loop`) do the same?
   - Is this a behaviour change from today? Today 86400 is accepted and wrapped.
   - Claude Code checked the Particle API on 2026-10-08: `default-settings.timing.reportingIntervalSec` is 3600, and no `device-settings` instance sets it, so no device is currently affected.
   - Report it as a FAIL or a CONCERN for the architect, with severity. Don't design a fix.
9. **Tests and mutations.**
   - Review the three new tests. Do they exercise real code (the real `resolveRuntime`, the real Report branch, the real `validateRange`) or copies? If they copy, are the copies checked against `src/`?
   - Re-run Copilot's five mutations and confirm each is caught by its targeted check:
     - rule dropped;
     - `cadence >= STALE` ignored;
     - the `#if` guard removed;
     - the refresh removed;
     - the 86400 bound restored.
   - Add your own: change `STALE + cadence` to `cadence`. A 4 h cadence would then reset at 4 h, so the 4 h-cadence checks must fail.
10. **Budget.** WO total net `src/` code lines against +20 (about +10 expected). Report moved lines too.

## Verdict (your final message; it is saved as the verdict file)

- **Header:**
  - one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED;
  - the model and reasoning level actually used;
  - the test interpreter line.
- **A table of checks 1–10.**
- **Findings,** each with file:line and severity, and whether it needs a round 2. Separate observation from inference.
- **Budget versus actual,** as one row: | Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |.
- **Confirmations:** `git status` and `git diff --stat` match the start, and you deleted only `build-tmp/wo20261008-001-stage7/`.

Limit: 150 lines.
