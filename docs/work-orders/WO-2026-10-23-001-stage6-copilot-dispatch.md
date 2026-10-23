AGENT: Copilot · MODEL: claude-sonnet-5.5 (probed 2026-10-20, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:** create **`tools/exerciser_baseline.py`** and **`tests/exerciser_baseline_test.py`** only. Run the host suite (serially) and Python 3. Scratch directory `build-tmp/wo20261023-001-stage6/` (remove only it).
**Not authorized:** any `src/` change; any other file; network or AWS calls (the script must make none); commits, pushes, merges, branch changes, stash, reset, checkout or restore; deleting anything you didn't create.

**SIZE BUDGET:** `src/` 0 lines. Script **≤ 200 lines**, tests **≤ 100 lines including fixtures** (Step 0 estimate 170 and 95). Python 3, standard library only. Going over is a STOP: report, do not compress (§12.3).

# Stage 6 dispatch: WO-2026-10-23-001 (the exerciser baseline script)

**Goal:** a host script that reads one counter's S3 report-hook files for some days and compares the occupied minutes the counter reported with the minutes the exerciser pattern should produce, per day and per report window, and summarizes accuracy and precision per firmware version.

**Repository:** this worktree, branch `wo/2026-10-23-001-exerciser-baseline-script` (from main `6e4111a`). HEAD adds this dispatch. Don't switch branches. **Binding spec:** `docs/work-orders/WO-2026-10-23-001-exerciser-baseline-script.md` (pattern, inputs, outputs, tests, mutations) and `WO-2026-10-23-001-step0-report.md` (payload facts and the findings). Where this dispatch and the WO differ, the WO wins; report it.

## Facts (checked in Step 0)

- Hook files: `particle-events/<UTC day>/Ubidots-Sensor-Hook-v1/<id>/*.json` = `{"particle":{"event","data","coreid","published_at","fw_version",…},"parsed":{…}}`. `particle.data` is a **string with HTML-encoded quotes (`&quot;`)**: `html.unescape` it, then `json.loads`. Payload keys in order: `occupancy, dailyoccupancy, battery, vc, key1, temp, alerts, resets, connecttime, fh, lfb, cyc, slp, timestamp`. **`timestamp` is milliseconds**; `dailyoccupancy` is **whole minutes, truncated** (`totalOccupiedSeconds / 60`).
- Status files: `status/<id>/*.json`, same envelope; `data` carries `resetReason` (30 hibernate wake, 70 first boot after OTA, 0 unknown, 60 watchdog, 140 failsafe) and `watchdogResetCount`. A reset inside a window = a status event whose `resetReason` is not 30 (nor 70 right after an OTA), or a drop of the hook `cyc`.
- **The phase:** T0 of a day = the payload `timestamp` (not `published_at`) of that day's first report with `occupancy` = 1 (local day, `--tz`). It carries the connect delay (Step 0 finding 1), so add `--phase-offset-s` (default 0, added to T0) and print the uncertainty note. If a day has no `occupancy` = 1 report, print "no occupancy=1 report" for that day and skip it.

## Do

1. **Model:** session *k* (k = 0, 1, …) expected close = `T0 + k·cycle_s + (pulses−1)·spacing_s + debounce_s`; counted if the close is **strictly before the local close hour** that day; expected credit per session = `(pulses−1)·spacing_s + debounce_s` seconds. Defaults cycle 1200, pulses 10, spacing 30; `--debounce-ms` required (300000 production, 60000 bench).
2. **Per day:** measured = the last `dailyoccupancy` of the local day; expected minutes; error in minutes and %; sessions expected; flags (missing reports = a gap over 1.5× the median report spacing, resets from the status files, excluded windows).
3. **Per window** (between consecutive reports by `timestamp`): measured = `dailyoccupancy` difference (windows do not span the daily reset), expected = the sum of credits of sessions whose expected close falls in `(prev, ts]`, error; the output states that ±1 min per window is truncation.
4. **Summary per `fw_version`:** accuracy = mean daily error, precision = standard deviation of the daily error, the same for window errors, with the numbers of days and windows. Plain text tables and `--csv FILE`.
5. **Tests** (`tests/exerciser_baseline_test.py`, run as `python3`, picked up by the suite): **generate the hook JSON inside the test** (a short helper), not stored files. Known answers at 60 s debounce (330 s = 5.5 min per session; work them out by hand and put the hand arithmetic in a comment): a perfect counter; a counter crediting +60 s per session; one missing report (flagged); a reset in the middle of a session (flagged, shortfall visible).

## Verify (all required)

- The tests pass; the whole suite passes serially (`N/N`, sh via zsh, py via python3; baseline on main 91).
- **Mutations in `git worktree add --detach` copies, each must FAIL a test:** (1) off-by-one in the session count at the close hour (`<` → `<=`); (2) `published_at` instead of `timestamp` for the phase; (3) dropping the truncation note.
- Line counts: `wc -l` of both files against the budget; `git diff --check` clean.
- Run the script once on a synthetic day to show the output (no real data).

## Implementation Report (≤ 150 lines, your final message)

Line counts against budget; the fixtures' hand-worked answers; the mutations and which tests failed; the suite; deviations (or "none"); the model used.
