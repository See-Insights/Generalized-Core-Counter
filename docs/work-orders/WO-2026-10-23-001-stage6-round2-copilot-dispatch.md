AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: as in `WO-2026-10-23-001-stage6-copilot-dispatch.md`, narrowed. **Edit only** `tools/exerciser_baseline.py` and `tests/exerciser_baseline_test.py`; scratch `build-tmp/wo20261023-001-stage6r2/` (remove only it); no commits/pushes/checkout/stash/reset; no network. Budget: script ≤ 200 lines, tests: net ≤ +20 lines (the test file may reach about 115 lines; flag it).

# Stage 6 round 2: WO-2026-10-23-001, fix Stage 7 round 1 (`WO-2026-10-23-001-stage7-verdict.md`, NOT VERIFIED)

Checks 3, 4, 5, 6 passed. Fix the two real defects and cover them in the test:
1. **F1 (High): timezone `SGT-8`.** The projects' timezone strings are POSIX style (`SGT-8` on the Singapore bench, `EST5EDT` in the US). `ZoneInfo("SGT-8")` raises `ZoneInfoNotFoundError`. Accept both: try `ZoneInfo(tz)` first; if that fails, parse a fixed-offset POSIX string `NAME[+-]H[:MM]` (no DST part) into `timezone(timedelta(...))` with the **POSIX sign convention (SGT-8 = UTC+8, EST5 = UTC-5)**; anything else exits with a clear error message (not a traceback).
2. **F2 (High): never fall back to `published_at` for the phase.** When a hook's payload has no `timestamp`, do not substitute `published_at` (`tools/exerciser_baseline.py:29-36`): leave that report out of the analysis and say so in the output ("N reports without a payload timestamp ignored").
3. **F3 (Medium): tests.** Add (a) a run with `--tz SGT-8` on a day whose UTC date differs from its Singapore date (check the local-day assignment and the result), (b) a run with an `EST5EDT` day crossing the UTC date boundary, (c) a first `occupancy=1` report with no payload `timestamp` (it must be ignored, not used for T0, and the note printed).

Verify: tests pass; whole suite serially (baseline with the script 92); mutations in `git worktree add --detach` copies must each FAIL a test: restore the `published_at` fallback; make `SGT-8` use the wrong sign; the three WO mutations from round 1 still fail. Report (≤ 100 lines): line counts, the new fixtures' hand answers, mutation results, suite, deviations, model.
