AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff (item C); run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary harnesses under your scratch directory **`build-tmp/wo20261002-stage7-r2/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 round 2 dispatch (narrow, item C only) — WO-2026-10-02-001

**Goal, in plain language (item C):** ConnSummary tells "modem off" from "searching", and nothing else changes: the label differs, and every timing and decision stays exactly as in v31.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-02-001-recovery-visibility`, base `71f955e`, with the uncommitted diff.
**Binding spec:** `docs/work-orders/WO-2026-10-02-001-recovery-visibility.md`: item C, and "Stage 5 decisions after Stage 7 round 1".
**Context:** your round 1 verdict (`WO-2026-10-02-001-stage7-verdict.md`, item C) and the round 2 report (`WO-2026-10-02-001-stage6-r2-copilot-report.md`).

**Scope:** A, B, D and E were VERIFIED in round 1 and are frozen. Claude Code hashed the 18 files outside `src/state/State_Connect.cpp` and `tests/conn_phase_modem_off_test.sh`, and they're unchanged. Re-check only that A, B, D and E's tests still pass. This is round 2 of 2: if C is NOT VERIFIED, C is dropped from v32, with no third round. So report precisely, and don't propose new mechanisms.

## Checks (PASS/FAIL with evidence for each)

1. **Your round-1 reproduction:** modem off at 0 s, powered but not ready at 50 s, cloud acquisition at 70 s. Recovery stage 1 fires at 70 s, as in v31. Run your own extraction of the real code, not only Copilot's test.
2. **v31 equivalence:** for sequences mixing `MODEM_OFF` and `CELLULAR_ACQUIRE` in any order before `CLOUD_ACQUIRE`, each cloud-recovery stage fires at the same moment, and `connPhaseCellMs`, `connPhaseNetMs` and `connPhaseCloudMs` are identical to a classifier that never returns `MODEM_OFF`. Trace every reader of the phase again: labels, accounting, the phase-change boundary, `phaseStartMs`, `cloudAcquireElapsedMs`, the recovery thresholds, and the success, timeout and exhausted summaries.
3. **The label:** `ConnSummary last=` and `ConnDiag phase=` show `MODEM_OFF` when the modem is off. Moving the `lastConnPhase` assignments changes nothing else, including the `ConnPhase:` trace log and anything that compares `lastConnPhase`.
4. **The mutation:** reverting the boundary to the raw comparison fails a test.
5. **Suite:** every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N` (round 2 reports 56/56). `tests/publish_with_ack_structural_test.py` is unchanged and green.
6. **Build:** a local boron release build after `make clean-user` (Copilot: 150628 / 1090 / 2204). `strings` finds `MODEM_OFF`.
7. **Size:** C's net `src/` code lines ≤ 8 (by your round-1 rule). The v32 total of A–E, against about 45.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, for C and overall for v32 (A, B, D and E carried from round 1), with evidence; binary sizes; the model and reasoning level actually used. Confirm that the working tree is byte-identical to how you found it, and that nothing outside `build-tmp/wo20261002-stage7-r2/` was created or deleted.
