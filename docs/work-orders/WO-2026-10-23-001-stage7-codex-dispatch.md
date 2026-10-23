AGENT: Codex · MODEL: gpt-5.6-sol · REASONING: medium
AUTHORIZATION SCOPE: review `git diff 283b28b f18e3e0` (HEAD adds only this dispatch): `tools/exerciser_baseline.py` and `tests/exerciser_baseline_test.py`. Run the host suite one test at a time; mutate only in `git worktree add --detach` copies; scratch `build-tmp/wo20261023-001-stage7/`. Write your verdict to `docs/work-orders/WO-2026-10-23-001-stage7-verdict.md` (the only file you may add). **Not authorized:** other changes; commits/pushes/stash/reset/checkout/restore in the review tree; device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-23-001 (exerciser baseline script)

**Spec:** `WO-2026-10-23-001-exerciser-baseline-script.md`, `…-step0-report.md`, `…-stage6-copilot-dispatch.md`. Treat Copilot's report as unverified. Claude Code's reproduction: script 166 lines (≤ 200), test 94 (≤ 100), `src/` 0; suite 92/92; the three WO mutations (`<`→`<=` at the close hour, `published_at` for the phase, dropping the truncation note) each fail the test; the control passes.

## Checks (PASS / FAIL / CONCERN with evidence; short)
1. **Expected-credit arithmetic and the close-hour boundary:** session *k* expected close = T0 + k·cycle + (pulses−1)·spacing + debounce, counted only if **strictly before** the local close hour; credit = (pulses−1)·spacing + debounce. Per-day and per-window bookkeeping (window = `(prev, ts]`; windows don't span the daily reset); local-day assignment by `--tz` (including a day boundary and DST-free zones used here: EST5EDT, SGT-8).
2. **The phase** comes from the payload `timestamp` (ms) of the first `occupancy` = 1 report, never `published_at`; expected minutes never come from the counter; no `occupancy` = 1 report → the day is skipped with a message.
3. **The fixtures' answers are right by hand** (60 s debounce, 330 s = 5.5 min per session): the perfect counter, +60 s per session, one missing report, a reset mid-session; verify the hand arithmetic in the test's docstring and that a deliberately wrong expected value fails.
4. **No network or AWS calls** (imports, `subprocess`, sockets); standard library only; `html.unescape` then `json.loads` on `data`; the reset detection (status `resetReason` not 30/70, or `cyc` drop) and the missing-report rule (gap over 1.5× the median spacing).
5. **Summary per `fw_version`:** mean and sample standard deviation of the daily and the window errors, counts, excluded days/windows left out; the truncation note present.
6. **Budget and suite:** script ≤ 200, tests ≤ 100 (flag, don't fail), `src/` 0, suite N/N serial, `git diff --check`.
Run **two mutations of your own** that the author did not list.

## Verdict (write to the file, then a short summary)
VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; model and reasoning; table of checks; findings with file:line and severity (observation vs inference); confirmations (tree unchanged, only your scratch removed).
