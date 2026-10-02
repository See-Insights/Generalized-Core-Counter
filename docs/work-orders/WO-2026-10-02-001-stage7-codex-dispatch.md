AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff; run the host suite and local ARM builds in a scratch copy (or with outputs restored); write temporary harnesses under your scratch directory **`build-tmp/wo20261002-stage7/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 dispatch (narrow) — WO-2026-10-02-001 (v32-RecoveryVisibility)

**Goal, in plain language:** a device that can't reach the cloud gets back within about 3 hours instead of 18, and every report shows enough about memory and sleep to diagnose the next problem.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-02-001-recovery-visibility`, base `71f955e` (v31), with the Stage 6 change as an uncommitted working-tree diff.
**Binding spec:** `docs/work-orders/WO-2026-10-02-001-recovery-visibility.md`, including "Stage 5 decisions" and the "Ubidots payload rule".
**Stage 6 report:** `docs/work-orders/WO-2026-10-02-001-stage6-copilot-report.md`.

Narrow review: check each item against its own goal and the WO's acceptance criteria. Don't widen the fault model or propose new mechanisms. For any defect, give the smallest fix that stays within the approved design.

## Checks (PASS/FAIL with evidence for each)

### A. Recover within about 3 hours, counting open hours only

With a host check (a real harness or a faithful compiled extraction; say which):
- during open hours with no successful connection, the full reset (code 2) fires once `now − max(lastConnection, todayAt(openTime))` reaches 3 h, and **not before**;
- **a 06:00 wake after a 22:00 close does not reset.** Check it for Trail02's 23:00 close too;
- closed hours never act, and an untrusted or Unknown clock never acts;
- a successful connection clears the state (`State_Connect.cpp`, the `set_lastConnection` and `clearConnectivityFailsafeRecovery("cloud-ok")` path, unchanged);
- stage 3 stays `COOLDOWN` + jitter after stage 2, and the WO's recorded consequence holds: a late stage 2 pushes stage 3 into the next open period;
- **mutations:** restoring 12 h fails a test; removing the open-hours base fails the morning-wake test.

Also confirm that the test-mode timings are unchanged (5 min / 15 min / 0 / 60 s) and that `ConnectivityFailsafeTest.cpp`'s predictions match the new rule. Rule on Copilot's deviation 3 (24-hour sites, where `openTime == closeTime`): is it reachable with any current configuration, and what does it cost? Report only.

### B. Stage 1 retired

- No radio-reset path remains in the supervisor (structural).
- The first action is stage 2; a persisted stage 1 progresses to 2; `failsafeStage` never newly takes 1.
- Breadcrumb 9 is removed with its number reserved.
- Rule on Copilot's deviation 1: is keeping alert 45 for the clear path correct?
- The failsafe test-mode build compiles locally.

### C. MODEM_OFF

- With `!Cellular.ready()` and `!Cellular.isOn()` → `MODEM_OFF`; with `isOn()` true → `CELLULAR_ACQUIRE`.
- Its time counts into `connPhaseCellMs`.
- **No decision path reads `MODEM_OFF`.** Trace every reader of the phase, including phase-change resets of `phaseStartMs` and anything that keys off a phase change.

### D and E. Report fields

- In both payload formats, **render the real format strings** with worst-case values and parse the result:
  - `fh`, `lfb`, `cyc` and `slp` are **unquoted JSON numbers**;
  - `key1` remains the only string-valued key;
  - the maximum size is under 256 (Copilot reports 228 / 217).
- `lfb` uses `HAL_Core_Runtime_Info` with `.size` set correctly.
- `cyc` starts at 1, increments after every return from all four `System.sleep()` sites, and is not retained.
- `slp` increments only on success.
- `cyc ≥ slp` always.

### Everywhere

- **Suite:** every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N`. `tests/publish_with_ack_structural_test.py` is unchanged and green. The one updated existing test (`daily_cleanup_boundary_test.py`) changed only for the rename and keeps its intent.
- **Linkage and local toolchain build (mandatory):**
  - the release build (Copilot: 150612 / 1090 / 2204) and the failsafe test-mode build;
  - `nm` shows the new paths called from user code.
- **Identity and size:** `v32-RecoveryVisibility`, product 32. Net `src/` code lines per item against its budget (A ≤ 8, B net negative plus the 2-line fix, C ≤ 8, D ≤ 10, E ≤ 12, total about 45), using your round-1 counting rule.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence; binary sizes; the model and reasoning level actually used. Confirm that the working tree is byte-identical to how you found it, and that nothing outside `build-tmp/wo20261002-stage7/` was created or deleted.
