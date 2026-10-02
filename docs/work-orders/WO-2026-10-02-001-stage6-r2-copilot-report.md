<!-- Copilot claude-opus-5 (medium) Stage 6 round 2 Implementation Report (item C), extracted verbatim from its --share transcript by Claude Code, 2026-10-02. Claude Code checked: the 18 frozen files (A, B, D, E) hash-identical to round 1; archive unchanged (120584). The docs/ change Copilot flagged was Claude Code's WO-2026-09-24-004 rewrite, on Chip's instruction. -->

# Implementation Report — WO-2026-10-02-001 item C, Stage 6 round 2

**Model/reasoning:** Claude Opus 5, medium.

## The change (`src/state/State_Connect.cpp`)
`MODEM_OFF` and `CELLULAR_ACQUIRE` are one timing phase; only the label differs.
- Added a one-line `timingPhase` normaliser and used it in the phase-change condition, so a `MODEM_OFF` ↔ `CELLULAR_ACQUIRE` change is no longer a boundary: no `addPhaseElapsed()`, no `phaseStartMs` reset.
- Moved the two existing `lastConnPhase = currentConnPhase;` assignments to one unconditional assignment after the block, so `ConnSummary last=` / `ConnDiag phase=` still show the raw `MODEM_OFF`.
- Classifier, `Cellular.isOn()` sampling, label and `connPhaseCellMs` accounting unchanged.

**Item C size:** 17 insertions / 5 deletions = 12 net, of which 5 are comment lines → **7 net `src/` code lines** (≤ 8, unchanged from round 1).

## Tests (`tests/conn_phase_modem_off_test.sh`)
Part 1 (classifier truth table) kept. New Part 1b is behavioural against the real source: the phase-accounting/phase-change block, the cloud-recovery block and the `CLOUD_RECOVER_STAGE*_MS` constants are extracted verbatim and driven tick by tick.
1. Reproduction (off 0 s → searching 50 s → cloud 70 s): **recovery stage 1 fires at 70 000 ms**, same as v31.
2. v31 equivalence over 5 mixed sequences: stage-1/stage-2 firing instants and `connPhaseCellMs`/`NetMs`/`CloudMs` identical to a run whose classifier never returns `MODEM_OFF` (v31's).
3. Label: after a `MODEM_OFF` interval the last phase is `MODEM_OFF`; it flips back to `CELLULAR_ACQUIRE` and back again, flushing nothing.
4. Mutation (new Part 3, automated): the boundary condition is reverted to `currentConnPhase != lastConnPhase` **on a copy** in `$TMPDIR` — the real file is never written, so byte-identity is by construction. Result: **caught** (stage 1 at −1 instead of 70 000, plus two v31-equivalence failures). Part 2's allowlist gained one exact pattern for the normalisation line.

## Commands and results
1. Host suite: every `tests/*.sh` with zsh + every bare `tests/*.py` with python3 → **56/56**. `tests/publish_with_ack_structural_test.py` unchanged and green.
2. ARM release (boron, 6.4.1, `clean-user` then `compile-user`): **COMPILED SUCCESSFULLY**, **150628 / 1090 / 2204** vs round 1's 150612 / 1090 / 2204 → **+16 text, +0 data, +0 bss**. `strings` on the `.bin` finds `MODEM_OFF` ×1. No new warnings from `State_Connect.cpp`.
3. `git diff --stat`: `src/state/State_Connect.cpp` 22 (14 in round 1); `tests/conn_phase_modem_off_test.sh` is untracked and was edited in place. No other file was touched by me.

## Files and deletions
Only the two permitted files were edited. Deleted only `build-tmp/wo20261002-stage6-r2/` (which I created); `build-tmp/`, `build-tmp/connectivity-archive/` and the round-1 transcript are untouched. No commit, push, branch or flash.

## Deviations
1. The mutation was applied to a copy of the source rather than to the file in place — strictly safer and byte-identity-preserving; it is now a permanent, automated part of the test.
2. **Not mine, flagged:** `docs/work-orders/WO-2026-09-24-004-retire-open-equals-close-convention.md` changed on disk at 10:01:39 during my ARM build (now 63/39 instead of round 1's 4/0). I never write to `docs/`; it appears to be a concurrent edit by Chip. Left untouched.

---

