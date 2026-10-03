AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/cloud/ConfigApply.cpp`, `src/MyPersistentData.cpp`, `src/Config.cpp` / `src/Config.h` (the shared rules check), `src/time/DailyBoundary.cpp` and `src/time/Clock.cpp`, only as this WO specifies; run `./bump_version.sh v36-HourRules "<one-line note>"`; add or update tests under `tests/`; run the host suite and a local ARM release build. Your scratch directory is **`build-tmp/wo20260924-004-stage6/`**: keep every temporary file in it and remove **only it** when done / Not authorized: commits, pushes, merges, branch changes, stash, reset, checkout, flashing, device settings, AWS or network access, any edit to `lib/`, `project.properties` or `docs/`, any change beyond this WO, and **deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`**.
**SIZE BUDGET: net negative `src/` code lines (nonblank, non-comment, including braces, declarations and includes). Don't compress code to meet it (`AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3). If it can't be net negative, STOP and report.**

# Stage 6 dispatch — WO-2026-09-24-004 (v36-HourRules)

**Goal, in plain language:** open and close hours always follow the three rules, enforced where settings are applied.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-24-004-hour-rules`, `HEAD` `6d8aaf9` (v35 on main). Do not switch branches. Files under `docs/` are records: don't touch them.
**Binding spec:** `docs/work-orders/WO-2026-09-24-004-retire-open-equals-close-convention.md`, including its Step 0, fact check, Stage 5 decisions and Acceptance. Where this dispatch and the WO differ, the WO wins; report the difference.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec is reported as a deviation. If it can't be done as approved within the budget, stop and report it. Leave an uncommitted working-tree diff.

## The rules

1. Always-open is exactly `openHour = 0`, `closeHour = 24`.
2. `closeHour > openHour`.
3. `0 ≤ openHour ≤ 12`.

Together: `openHour ∈ [0, 12]` and `closeHour ∈ (openHour, 24]`.

## What to implement (all file:line checked against `6d8aaf9`)

| # | Where | Change |
|---|---|---|
| 1 | `Config.h` / `Config.cpp` | **One shared check** of the three rules, for example `Config::hoursFollowRules(int openHour, int closeHour)`. It is the only place the rules are written. |
| 2 | `ConfigApply.cpp:279–300` (`Cloud::applyTimingConfig()`) | Replace the two independent 0–23 checks with **one pair check**: take the merged pair (an absent value falls back to the current one), check it, and apply **both or neither**. On rejection: keep the last valid hours, log the offending values and the rule broken, and set `success = false` (as `validateRange()` failures do). |
| 3 | `MyPersistentData.cpp:94–104` (`sysStatusData::validate()`) | Replace the two `> 23` checks with the shared check. |
| 4 | `Config.cpp:69–90` (`validateConfigFields()`) | Replace the two `> 23` blocks with the shared check, keeping the existing log/failure-reason style. |
| 5 | `DailyBoundary.cpp:37` | `close = (openHour == closeHour) ? 24 : closeHour;` becomes `close = closeHour`. Remove `openHour` there if it's then unused. |
| 6 | `Clock.cpp:14–24` (`isWithinOpenHoursForHour()`) | Keep only the daytime window (`hour >= openHour && hour < closeHour`). Remove the overnight branch (`:17–19`) and the `open == close` branch (`:20–22`). |
| 7 | `Clock.cpp:26–61` (`secondsUntilNextOpenForSeconds()`) | Remove the overnight branch (`:47–56`) and the `open == close` branch (`:57–60`), keeping the daytime logic. |
| 8 | Version | `./bump_version.sh v36-HourRules "<one-line note>"`, giving product 36. |

- **Don't change any other reader of the hours.** The WO's fact check confirms they handle 24 safely.
- **Persisted pairs at boot:** no special handling. Item 3 means a pair that breaks the rules makes the stored record invalid, as an out-of-range hour does today (Stage 5 decision 4).

## Tests (outside the budget)

1. **A rules table test,** against the real `Config` check and the real `applyTimingConfig()` where an existing harness pattern fits, otherwise the shared check plus a structural check. Say which.
   - **Accepted, and applied as a pair:** 6/22, 6/23, 0/24, 12/13.
   - **Rejected, with the previous hours kept:** 6/6, 22/22, 20/6, 13/22, 0/25, 13/24, 7/7, and any close ≤ open.
   - **Mutations:** removing or weakening each rule in turn (rule 1/24 handling, rule 2, rule 3) makes the test fail.
2. **Behavior unchanged for valid pairs:**
   - for every valid pair, `isWithinOpenHoursForHour()`, `secondsUntilNextOpenForSeconds()` and `DailyBoundary`'s close boundary agree before and after (a comparison against the old functions in your scratch copy is fine);
   - **0/24** is open at every hour, and its daily close is at midnight.
3. **Structural:** no `openHour == closeHour` (or equivalent) sentinel and no overnight branch remain in `src/`.
4. **Existing tests** (for example `clock_openness_test`, `connectivity_failsafe_open_hours_test` and the daily-cleanup tests) may pin the overnight or `open == close` behavior. Update one only if it pins behavior this WO removes, and report each change. `tests/publish_with_ack_structural_test.py`, `tests/sleep_config_ownership_structural_test.py` and `tests/ledger_no_retry_test.sh` must pass unchanged.

Run each mutation on a copy or restore byte-identically.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N`, before (v35: 61/61) and after.
2. Local ARM release build (boron, Device OS 6.4.1, after `make clean-user`, in your scratch directory or with outputs restored): text/data/bss against v35's 150684 / 1090 / 2180; `strings` shows `v36-HourRules`.
3. Net `src/` lines (must be negative), and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change per item, with line counts.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20260924-004-stage6/`.
- Deviations (or "none").
- The model and reasoning level actually used.
