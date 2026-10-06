AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff in this worktree (branch `wo/2026-10-06-001-alert19-clears`); run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary harnesses under your scratch directory **`build-tmp/wo20261006-001-stage7/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create, including `build-tmp/` itself.

# Stage 7 dispatch (narrow) — WO-2026-10-06-001 (v38-Alert19Clears)

**Goal, in plain language:** alert 19 is reported once, in the first report after a watchdog reboot, and then cleared, so it no longer hides other alerts.

**Repository:** this worktree of `Generalized-Core-Counter`, branch `wo/2026-10-06-001-alert19-clears`, base `7a2b026` (main: v36 + PR #61), with the uncommitted diff.
**Binding spec:** `docs/work-orders/WO-2026-10-06-001-alert19-clears.md`.

**Implementer:** Claude Code made a pre-authorized narrow edit, with no Copilot round. Its record is in the WO's approval record.

Narrow review: check the WO's acceptance criteria only. Don't widen the fault model or propose new mechanisms. For any defect, give the smallest fix as a proposed diff; don't apply it.

## Checks (PASS/FAIL with evidence for each)

1. **Reported once, then cleared:**
   - after a watchdog reboot, `raiseAlert(19)` (`Generalized-Core-Counter.cpp:~1307`) sets 19;
   - the first **queued** report carries `alerts: 19`;
   - the existing clear-after-report block (`:~2060`) then clears it, with `lastAlertTime` set to 0;
   - a later alert 40 is visible in the next report;
   - a report that isn't queued doesn't clear 19.
2. **Nothing else changes:**
   - the severity order (`MyPersistentData.cpp` `getAlertSeverity()`: 19 still tier 4);
   - the `watchdog` forensics (`watchdogResetCount`, `lastWatchdog*`, `publishWatchdogForensics()`);
   - the clear behavior of every other code.

   Say whether any other path could now clear 19 at an unintended time (for example a boot-time report queued before the reset is classified), and whether the `alert 17` boot-storm path or alert-40 escalation (which reads `lastAlertTime`) is affected.
3. **The comment** at the raise site no longer says 19 is sticky and accurately describes the new behavior.
4. **Tests:**
   - `tests/alert19_clears_after_report_test.sh` drives the **real** extracted functions, not copies that could drift;
   - removing `case 19:` fails it (run the mutation; restore byte-identically);
   - the updates to `tests/watchdog_alert_code_test.cpp`/`.sh` keep their intent, apart from the deliberate reversal of the sticky checks.
5. **Suite and build:**
   - every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N` (Claude Code: 63/63);
   - `publish_with_ack_structural_test.py`, `sleep_config_ownership_structural_test.py` and `ledger_no_retry_test` unchanged and green;
   - a clean local boron 6.4.1 release build: text/data/bss against v36's 150740 / 1090 / 2180; `strings` shows `v38-Alert19Clears`, product 38; `nm` shows `publishData` and `setup`.
   - Claude Code already ran the Particle cloud compile (succeeded; flash 151974 / RAM 3282).
6. **Budget:** net `src/` lines (nonblank, non-comment) against +1; nothing compressed.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence. Include:
- binary sizes;
- budget versus actual;
- the model and reasoning level actually used.

Confirm that the working tree is byte-identical to how you found it, and that nothing outside `build-tmp/wo20261006-001-stage7/` was created or deleted.
