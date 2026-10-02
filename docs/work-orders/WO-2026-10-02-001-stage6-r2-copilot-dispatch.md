AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: item C only. Edit `src/state/State_Connect.cpp` (item C's code) and `tests/conn_phase_modem_off_test.sh`; run the host suite and a local ARM release build. Your scratch directory is **`build-tmp/wo20261002-stage6-r2/`**: keep every temporary file in it and remove **only it** when done / Not authorized: commits, pushes, merges, branch changes, stash, reset, checkout, flashing, device settings, AWS or network access, any edit to `lib/`, `project.properties` or `docs/`, **any change to items A, B, D or E or their tests (frozen, byte-identical)**, and **deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`**.
**SIZE BUDGET: item C stays ≤ 8 net `src/` code lines in total (it's +7 now; Codex's fix is expected to keep it at +7). Going over means STOP and report. This is round 2 of 2: there's no round 3.**

# Stage 6 round 2 dispatch — WO-2026-10-02-001 item C (MODEM_OFF, diagnostics only)

**Goal, in plain language (item C):** ConnSummary tells "modem off" from "searching", **and nothing else changes**: the label differs, and every timing and decision stays exactly as in v31.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-02-001-recovery-visibility`, base `71f955e`. The working tree holds round 1's uncommitted diff. **A, B, D and E were VERIFIED and are frozen.** Claude Code has hashed every file other than `src/state/State_Connect.cpp` and `tests/conn_phase_modem_off_test.sh`, and will check they're unchanged.
**Binding spec:** `docs/work-orders/WO-2026-10-02-001-recovery-visibility.md`: item C, and the approval record's "Stage 5 decisions after Stage 7 round 1".
**The defect:** `docs/work-orders/WO-2026-10-02-001-stage7-verdict.md`, item C.

## The defect

A change between `MODEM_OFF` and `CELLULAR_ACQUIRE` makes `handleConnectingState()`'s phase-change block (`State_Connect.cpp`, `else if (currentConnPhase != lastConnPhase)`, about `:368`) run `addPhaseElapsed()` and reset `phaseStartMs`. `phaseElapsedMs` then feeds the cloud-recovery decisions (about `:407`).

Codex's compiled reproduction (the identical input sequence):
- modem off at 0 s;
- powered but not ready at 50 s;
- cloud acquisition at 70 s.

**v31: recovery stage 1 fires at 70 s. Round 1: it doesn't, because the elapsed value is only 20 s.** Phase accounting is unaffected (70,000 ms in `connPhaseCellMs` either way).

## The fix (Codex's smallest fix; Chip approved)

`MODEM_OFF` and `CELLULAR_ACQUIRE` count as **the same phase for timing** and differ **only in the label**:
- When the phase-change block decides whether a boundary has happened, treat `MODEM_OFF` as `CELLULAR_ACQUIRE`. A change between the two is **not** a boundary: no `addPhaseElapsed()` and no `phaseStartMs` reset.
- **Update the raw value separately,** so `ConnSummary last=` and `ConnDiag phase=` still show `MODEM_OFF` when the modem is off. Codex verified that moving the existing `lastConnPhase` assignments keeps C at +7.
- Nothing else changes: the classifier, `Cellular.isOn()` sampling, the label, and the `connPhaseCellMs` accounting all stay.

## Test (`tests/conn_phase_modem_off_test.sh`)

Keep the existing checks, and add:
1. **Codex's reproduction:** modem off at 0 s → powered, not ready at 50 s → cloud acquisition at 70 s. With the fix, recovery stage 1 fires at 70 s, exactly as in v31.
2. **v31 equivalence for timing:** over sequences that mix `MODEM_OFF` and `CELLULAR_ACQUIRE` in any order before `CLOUD_ACQUIRE`, the times at which each cloud-recovery stage fires, and `connPhaseCellMs` and `connPhaseCloudMs`, are identical to a classifier that never returns `MODEM_OFF` (v31's).
3. **The label still distinguishes them:** after a `MODEM_OFF` interval, the reported last phase or ConnDiag phase shows `MODEM_OFF`.
4. **Mutation:** making a `MODEM_OFF` ↔ `CELLULAR_ACQUIRE` change a timing boundary again (round 1's behavior) must fail test 1 or 2. Restore byte-identically afterwards.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N` (round 1: 56/56). `tests/publish_with_ack_structural_test.py` unchanged and green.
2. Local ARM release build (boron, Device OS 6.4.1, after `make clean-user`, in your scratch directory or with outputs restored): text/data/bss against round 1's 150612 / 1090 / 2204; `strings` still finds `MODEM_OFF`.
3. Item C's net `src/` lines (≤ 8).
4. `git diff --stat` showing that only `src/state/State_Connect.cpp` and `tests/conn_phase_modem_off_test.sh` changed in this round.

## Implementation Report (required, as your final message)

- The change, and C's line count.
- The test additions and the mutation result.
- Commands and results.
- Confirmation that only the two permitted files changed and that you deleted only your scratch directory.
- Deviations (or "none").
- The model and reasoning level actually used.
