AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/` (the S2 files already changed, plus the single call-site line after each `deepPowerDown()` in `src/state/State_Error.cpp` and `src/ConnectivityFailsafe.cpp`; the calls themselves unchanged), tests under `tests/` (S2 tests, the equivalence fixture and the four structural tests as F6 requires), and write the report; run host tests (zsh for .sh, python3 for .py), the local Device OS toolchain build and nm/objdump; scratch `build-tmp/wo-2026-11-05-004-stage6-r3/` (create it; use only that) / NOT authorized: committing, pushing, merging, flashing, creating PRs or tags, touching `lib/`, changing the `deepPowerDown()` calls themselves (S3), S1/S1b code, changing the persisted or `retained` layout, deleting anything you did not create (including `build-tmp/` itself).

# Stage 6 dispatch, round 3 (fixes after Stage 7 round 1): WO-2026-11-05-004 (v50 piece S2)
Binding spec: `docs/work-orders/WO-2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake.md` **including Amendments 1-4** (read **Amendment 4 first**), and Codex's verdict `docs/work-orders/WO-2026-11-05-004-stage7-r1-verdict.md` (findings F1-F6, with file:line on 86b11e0). Base: this branch HEAD (S2 code at 86b11e0, plus records); your changes go on top, uncommitted. **Budget: `src/` at most +20 net against 3b5cfb8** (`git diff --numstat 3b5cfb8 -- src`; state the number); stop and report rather than trim style. **Run at the dispatched reasoning level (medium); the report states the level actually run, or that it could not be observed.** This is sleep/watchdog/wake logic: be conservative.

## Do (Amendment 4)
1. F1: sentinel 0xA5 for the evidence byte, compared exactly (`== 0xA5`); name it as a constant; legacy value 2 and 0 and other values are "no record".
2. F2: flush right after the boot clear; after each `deepPowerDown()` returns, clear the evidence and flush; at the failsafe caller also downgrade stage 3 (one line each at the two call sites; the calls unchanged).
3. F3: reason 30 = "successful HIBERNATE wake" everywhere (comments, tests, names); restate the 1,800 s test's meaning; no timing arm on reason 30.
4. F4: stage-3 evidence with `Time` invalid is rejected.
5. F5: restore `ab1805WakeReason` in the status payload with the constant `"N/A"`.
6. F6: restore base comparison in the equivalence fixture and the four structural tests, normalising only the approved S2 substitutions; add cases for GPIO wake, legacy marker 2, forced consumption, and a returning `deepPowerDown()`. Add a mutation for each of F1, F2, F4, F5 (each must fail; back up the file, mutate, restore from the backup; never `git checkout`/`git restore` while your uncommitted work is present).
## Verify
Host suite N/N (sh via zsh, py via python3; count with `p=$((p+1))`; baseline 118/118 at 86b11e0; `thermal_coupling_structural_test.py` also on a clean export of tracked files); local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE` as a make argument), text/data/bss (86b11e0: 151468 / 1114 / 2240) and `nm` showing the call sites linked; numstat net as above; layouts untouched.
## Stop and report if
the budget cannot hold; the fix needs a layout change; S1/S1b behaviour would have to change.
## Report
`docs/work-orders/WO-2026-11-05-004-stage6-r3-copilot-report.md`: model and reasoning level actually used, interpreter line, net `src/` lines per file, tests and mutation results, build and `nm` evidence, deviations, open questions. Claim nothing about hardware.
