AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-08, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the committed WO diff on branch `wo/2026-10-07-004-occupancy-reports`, HEAD `b3810d7`. The tree is clean. The WO code diff is `400ff48..b3810d7 -- src tests`; main (`2dfdca5`, docs only) was merged in at `d45b8c0`.
- Run the host suite in place.
- Run a local ARM build in a scratch copy.
- Write temporary harnesses and build copies under **`build-tmp/wo20261007-004-stage7r2/`**, and remove **only that directory** when done.
- Mutate only in scratch copies.

**Not authorized:**
- Any change to tracked files, commits, pushes, stash, reset or checkout.
- Flashing, device settings, network access or AWS access.
- Deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 round 2 dispatch: WO-2026-10-07-004 (occupancy changes report by mode)

**This is the last round under the two-round rule.** If it isn't VERIFIED, the WO stops. Any new edge case is reported with a severity, for the architect to accept or reject. It is not a design prompt.

**Goal, in plain language:** every occupancy change, start or end, is reported right away when the mode in use is KEEP_ALIVE or CONNECTED. In every other mode, changes wait for the scheduled report, as they do today.

**Binding spec:** `docs/work-orders/WO-2026-10-07-004-occupancy-reports-by-mode.md`, all sections, including "Stage 7 round 1, Stage 6 round 2 and the controller edit".

**Inputs:**
- Round 1's verdict: `docs/work-orders/WO-2026-10-07-004-stage7-verdict.md`.
- Round 2's dispatch: `docs/work-orders/WO-2026-10-07-004-stage6-round2-copilot-dispatch.md`.
- Copilot's report: the final section of `build-tmp/WO-2026-10-07-004-stage6r2-copilot-output.log`.

Treat every claim in them as unverified.

## Test-suite hygiene

`tests/thermal_coupling_structural_test.py` scans the whole repo, including `build-tmp/`. Run the host suite before you create any copy of `src/`, or after removing it. A failure caused by your own copy is not a finding.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Re-run round 1's thirteen checks** (`docs/work-orders/WO-2026-10-07-004-stage7-codex-dispatch.md`) against `b3810d7`. Give full treatment to checks 8–12 and to the checks below. Checks 1–7 and 13 can be confirmed briefly, but say what you re-ran. The base build for sizes is `400ff48`: 150964 / 1090 / 2196. Use a **fresh `BUILD_PATH_BASE`** for every build.
2. **Round 1's finding 1 (stale flag / no repeat).**
   - Report now takes the flag right after `publishData()`.
   - Confirm that **at most one report per occupancy change, never a repeat**, on every Report path: alert-40 escalation, config-invalid, service request, the offline occupancy branch, and already connected.
   - Confirm that no loop forms through Connect, Error or firmware-update state.
   - Confirm that the payload queued at `publishData()` already carries the change on every one of those paths.
3. **Round 1's finding 2 (teardown).** The sleep pending check now acts only while `!disconnectRequested`.
   - Confirm that it can't fire once the span has requested teardown.
   - Confirm that `disconnectRequested` is reset once per cycle on every path (`:459`, `:843`, `:890`, `:963` per Copilot), so the check is never permanently gated off.
   - Confirm that a flag latched mid-teardown is reported on the first pass after the wake.
   - Confirm that the check still runs before any standby decision, `System.sleep()` and the suppress decision.
4. **The controller edit (new).** The pending exit sets `cloudSyncStartMs = 0` before its transition.
   - Confirm that, without it, a stale start time makes the next sleep's gate fail at once (`elapsedMs = millis() - cloudSyncStartMs`), so teardown could proceed without draining the queued report.
   - Confirm that with it, the next sleep's gate starts with its full budget.
   - Confirm the store in the ELF. Claude Code saw `str r3, [r4]` at `c2b24`, with `r4` pointing at `handleSleepingState()::cloudSyncStartMs` (`0x2003dc2c`).
   - **Check every other exit from `handleSleepingState()`** that can leave during the gate wait (after `:504-505` starts the timer and before `:654`, `:692` or `:711` clears it). Does any of them also leave `cloudSyncStartMs` stale? List them. Any that predate this WO are a CONCERN to report, not a FAIL of this WO.
5. **Round 1's finding 3 (tests).**
   - Confirm that every extracted block in `tests/occupancy_report_by_mode_test.sh` is byte-identical to `src/` at `b3810d7`, and that a mismatch fails loudly (`COPY_MISMATCH`).
   - Confirm that the sleep and report passes follow source order, so moving the real sleep check below suppress fails a behaviour check (`soak:`).
   - Confirm that the gate model (`modelGate()`, placed at the gate's source position) behaves like the real gate in the respects the "midgate:" case relies on: it starts its timer on the first pass, waits under budget, and resets on timeout.
6. **Mutations.**
   - Re-run all eleven of the harness's mutations, and confirm that each is caught by its targeted check.
   - Add your own:
     - drop the `cloudSyncStartMs = 0;` line, which `midgate:` must catch;
     - move the flag take below the config-invalid exit, which `norepeat:` must catch.
   - Report any mutation that survives.
7. **Budget.**
   - WO total net `src/` code lines against **+20**; it is expected to be +15 (round 1 +14, round 2 0, controller edit +1).
   - Moved lines and replaced call-site lines.

## Verdict (your final message; it is saved as the verdict file)

- **Header:**
  - one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED;
  - the model and reasoning level actually used;
  - the test interpreter line.
- **A check table:** round 1's 1–13 plus checks 2–7 here.
- **The rule table as tested,** for the closing record.
- **The flag's fate on each Report path.**
- **The list of gate-wait exits** from check 4, with each one's `cloudSyncStartMs` handling.
- **Findings,** each with file:line and severity. Separate observation from inference.
- **Budget versus actual,** as rows for the closing record: | Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |. One row for round 1, one for round 2, one for the controller edit, and one for the WO total.
- **Confirmations:** `git status` is clean at the end, and you deleted only `build-tmp/wo20261007-004-stage7r2/`.
