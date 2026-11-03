AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/ConfigApply.cpp`, `src/ResetCause.h` (and a call-site only if the WO requires it), tests under `tests/`, local build/test per the WO, scratch directory `build-tmp/wo-2026-11-02-004-stage6/` / NOT authorized: committing, pushing, merging, flashing, deleting anything you didn't create (including `build-tmp/` itself), changing the persisted layout or `retained` RAM, any live re-apply of the timezone, malformed-string handling.

# Stage 6 dispatch — WO-2026-11-02-004 (recovery item 25: planned reset on a changed timezone)

**Binding spec:** `docs/work-orders/WO-2026-11-02-004-timezone-applies-to-running-clock.md` (architect's design: a planned reset, forced flush first). Branch `wo/2026-11-02-004-timezone-planned-reset`, base `21804e3` (main, v48). Leave all changes uncommitted. **Budget: `src/` at most +10 net lines, hard.** Stop and report rather than trimming style.

## Do
Implement the WO's Change section: `RESET_CAUSE_TIMEZONE_CHANGE = 8` in `ResetCause.h`; a file-local flag set where `ConfigApply.cpp` stores a changed timezone; in the success branch of `applyConfigurationFromLedger` after `validateStoredData()`, `SystemConfig::flushNow()` then `System.reset(RESET_CAUSE_TIMEZONE_CHANGE)` (the failsafe pairing). Re-read the cited lines on this base first: the line numbers in the WO were taken on `a7b69ce` and may have moved. Also check: is `applyConfigurationFromLedger` ever reached from `setup()` before `LocalTime` is configured (WO Stage 7 check a)? If it would reset needlessly there, report it; do not redesign.

## Tests
Per the WO's Tests section: host tests for the order (flush before reset), no reset when unchanged, no reset on a failed apply, no flag when only other sections change; extend `tests/reset_cause_structural_test.py` for code 8. Add the listed mutations as a documented check (apply each, show the test fails, revert). Update `docs/reference/alert-codes.md` only if the WO's change touches an alert (it does not expect to).

## Verify
Host suite under zsh (`*.sh`) and python3 (`*.py`), reporting `N/N (sh via zsh, py via python3)`. Local toolchain build per `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 7 "local toolchain build" with a fresh `BUILD_PATH_BASE`; confirm with `nm` that the reset path is present in the symbolised ELF (linkage check). `git diff --stat -- src/` must show ≤ +10 net.

## Stop and report if
the design can't be done in +10, the flush API differs from the failsafe's usage, or the reset would fire from a path the WO didn't expect.

## Report (write to `docs/work-orders/WO-2026-11-02-004-stage6-copilot-report.md`)
Model and reasoning actually used; interpreter line; net `src/` lines per file; test names; mutation results; build and `nm` evidence; open questions.
