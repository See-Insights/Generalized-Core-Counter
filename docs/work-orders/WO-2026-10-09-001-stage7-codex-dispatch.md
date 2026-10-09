AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-09, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-09-001-timediag-on-change`).
- Run the host suite in place and a local ARM build in a scratch copy.
- Write temporary files under **`build-tmp/wo20261009-001-stage7/`**, and remove **only that directory**.
- Mutate only in scratch copies.

**Not authorized:**
- Changes outside your scratch directory.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.
- Deleting anything you didn't create, including `build-tmp/` itself.

# Stage 7 dispatch: WO-2026-10-09-001 (Idle logs TimeDiag only on change)

**Goal, in plain language:** in Idle's CONNECTED branch, `logTimeDiag()` runs only when its content changes or at a state transition, never on every pass.

**Binding spec:** `docs/work-orders/WO-2026-10-09-001-timediag-on-change.md`: rulings 1–3, the constraints and the tests.

**Inputs:**
- Design: `docs/work-orders/WO-2026-10-09-001-step0-report.md`.
- Implementation: `docs/work-orders/WO-2026-10-09-001-stage6-copilot-dispatch.md`, and Copilot's report (the end of `build-tmp/WO-2026-10-09-001-stage6-copilot-output.log`).

Treat every claim as unverified.

**What changed:**
- `State_Idle.cpp`: an `enteredIdle` capture before `publishStateTransition()`, and the CONNECTED-branch key on `isOpen`, `openness`, `isClockTrusted()` and `Clock::isTimeValid()`, with a static last-key. Claimed +8 code lines.
- `docs/FIELD_MEANINGS_REFERENCE.md`: the TimeDiag entry.
- `tests/clock_trust_standard_structural_test.py`: a narrow allowlist entry, which is a deviation Copilot reported.
- New `tests/idle_timediag_on_change_test`.

## Test-suite hygiene

`tests/thermal_coupling_structural_test.py` scans the whole worktree. Run the host suite before you create any copy of `src/`, or after removing it.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Linkage and build.** A fresh-path release build: text/data/bss against v40's 151012 / 1090 / 2196. Show in the ELF that `handleIdleState` calls `logTimeDiag` only behind the key comparison.
2. **Tests.** Report the full suite as `N/N (sh via zsh, py via python3)`.
3. **The goal.**
   - With CONNECTED and Open, a stable key gives one TimeDiag on Idle entry and none on later passes.
   - Each flip of `isOpen`, `openness`, `trusted` or `valid` gives exactly one more.
   - Ticking values (`epoch`, `local`, `syncAgeMs`) add none.
   - Confirm the key's fields come from the **same sources** `logTimeDiag()` prints (`isOpen` argument, `Clock::openness()`, `isClockTrusted()`, `Time.isValid()`), and that the bit packing can't collide (`openness` has 3 values).
4. **The constraints.**
   - No gating on `Time.isValid()` or `isWithinOpenHours()`; they only feed the key.
   - `isOpen=` and `openness=` stay separate.
   - The Unknown ceiling (`:295-298`), `park closed`, and `State_Sleep.cpp:1002` are unchanged.
   - Confirm that capturing `enteredIdle` before `publishStateTransition()` changes no other behaviour in Idle (every use of the old inline test).
5. **The accepted entry-pass gap** (ruling 1). Identify the early returns before `:119` on the entry pass, and confirm the consequence is only a skipped entry log, recovered on the next key change.
6. **The allowlist deviation.** Copilot added a single-line allowlist entry to `tests/clock_trust_standard_structural_test.py` for `Clock::isTimeValid()` in `State_Idle.cpp`. Judge whether it weakens that test's protection: is it exact-line, file-scoped, and asserted present? Is the use telemetry only, never a decision?
7. **Tests and mutations.**
   - Does the new test drive the real Idle code, with a byte-checked extraction?
   - Re-run Copilot's three mutations: make the call unconditional, drop `trusted` from the key, move the entry test after `publishStateTransition()`.
   - Add one: drop `valid` from the key. A valid flip must then fail its check.
8. **Docs.** The `FIELD_MEANINGS_REFERENCE.md` entry matches ruling 3's wording and lists the four added fields.
9. **Budget.** Net `src/` code lines against +10.

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–9.**
- **Findings,** with file:line and severity. Separate observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you deleted only your scratch directory.

Limit: 120 lines.
