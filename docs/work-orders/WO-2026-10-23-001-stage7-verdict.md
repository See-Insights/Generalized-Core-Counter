# Stage 7 verdict: WO-2026-10-23-001

**Overall: NOT VERIFIED.**

**Reviewer:** Codex · **Model:** `gpt-5.6-sol` · **Reasoning:** medium  
**Reviewed:** `git diff 283b28b f18e3e0` for `tools/exerciser_baseline.py` and `tests/exerciser_baseline_test.py`.

| Check | Result | Evidence |
|---|---|---|
| 1. Expected credit, boundary, windows, local days | **FAIL** | The formula and strict close boundary are correct at `tools/exerciser_baseline.py:70-75,88,102,118,123`; the fixture proves six 5.5-minute sessions and the exact-close exclusion. An independent EST5EDT probe crossing the UTC date boundary produced one 2.0-minute session on the correct local day. However, the required `--tz SGT-8` fails at line 80 with `ZoneInfoNotFoundError`, so local-day assignment does not support both named timezone forms. |
| 2. Payload phase and absent phase | **FAIL** | Normal reports use payload milliseconds (`tools/exerciser_baseline.py:29-36,98-102`), expected minutes are generated independently, and a day without `occupancy=1` is skipped with the required message. But lines 30 and 36 use `published_at` as T0 when the first occupied report lacks payload `timestamp`; an independent fixture returned a normal day instead of rejecting/skipping it. This violates “never `published_at`.” |
| 3. Fixture answers by hand | **PASS** | The arithmetic in `tests/exerciser_baseline_test.py:4-15` is correct: 330 s = 5.5 min; six sessions = 33 min; +60 s/session = +6 min; the missing report merges two credits; the reset removes one credit and truncation makes the day 27 vs 33. Changing the perfect expected measured value from 33 to 32 in a detached worktree failed the test. |
| 4. Local-only parsing, resets, gaps | **PASS** | Production imports are standard-library only (`tools/exerciser_baseline.py:6-8`), with no subprocess, socket, AWS, or network path. Data is `html.unescape` then `json.loads` at line 23. Reset reasons and `cyc` drops are handled at lines 82-83 and 109-114; missing reports use a gap greater than 1.5 times the median at lines 103-106. |
| 5. Firmware summaries and truncation note | **PASS** | Lines 39-40 and 140-148 compute means and sample standard deviations by `fw_version`, with day/window counts and explicitly excluded rows omitted. Lines 10-11 and 130 retain the truncation/quantization note. |
| 6. Budget, suite, hygiene | **PASS** | Script 166/200 lines; test 94/100 lines; `src/` diff 0; `git diff --check` clean. Full suite passed **92/92**, serially: 53 shell tests via `zsh`, then 39 bare Python tests via `python3`. |

## Findings

1. **High — observation:** `tools/exerciser_baseline.py:80` passes `SGT-8` directly to `ZoneInfo`. On the review host, `python3 tools/exerciser_baseline.py ... --tz SGT-8` exits 1 with `ZoneInfoNotFoundError: No time zone found with key 'SGT-8'`. `SGT-8` is explicitly required by check 1.
2. **High — observation:** `tools/exerciser_baseline.py:29-36` substitutes the envelope `published_at` when payload `timestamp` is absent, and `phase()` then returns that substituted value. A synthetic first `occupancy=1` report without payload `timestamp` was accepted and analyzed using `published_at`, contrary to check 2's “never `published_at`” rule.
3. **Medium — observation:** The checked-in fixture always supplies `timestamp` (`tests/exerciser_baseline_test.py:49-51`) and invokes the script with its UTC default (`tests/exerciser_baseline_test.py:55-58`). It therefore does not catch either failed requirement above.

## Mutation evidence

- Required WO mutants independently failed in detached worktrees: strict `<` changed to `<=`; phase changed to `published_at`; truncation note removed.
- Two additional implementation mutants independently failed: session credit changed from `(pulses - 1) * spacing` to `pulses * spacing`; reset reason 60 incorrectly added to the ignored set.
- Control candidate passed, and a deliberately wrong fixture answer failed.

## Confirmations

- Mutations were made only in `git worktree add --detach` copies registered from a local scratch clone; the review tree was not mutated.
- `build-tmp/wo20261023-001-stage7/` and all detached worktrees were removed.
- Before adding this verdict, `git status --short` was empty. This verdict is the only review-tree addition; no commit, push, stash, reset, checkout, restore, device, network, or AWS operation was performed.
