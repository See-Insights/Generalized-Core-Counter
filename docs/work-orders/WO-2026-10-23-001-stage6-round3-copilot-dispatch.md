AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: as in `WO-2026-10-23-001-stage6-round2-copilot-dispatch.md`. **Edit only** `tools/exerciser_baseline.py` and `tests/exerciser_baseline_test.py`; scratch `build-tmp/wo20261023-001-stage6r3/` (remove only it); no commits/pushes/checkout/stash/reset; no network. Budget: script ≤ 200 lines, tests ≤ about 125 lines (flag it).

# Stage 6, timezone rework (architect's instruction, 23 Oct): POSIX TZ handling

Round 2 parsed a fixed offset by hand. Replace it: **use the C library's POSIX TZ support**. In the script set `os.environ["TZ"]` to the device's TZ string, call `time.tzset()`, and convert every epoch with `time.localtime()` (and derive the local day and local hour from that). This covers `SGT-8` and `EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00` including DST. **Remove the `ZoneInfo` path and the hand-written offset parse.** An unparsable TZ string must not traceback: the C library silently falls back to UTC, so detect it (for a TZ string with no UTC offset recognized, e.g. compare `time.localtime(0).tm_gmtoff`/`tm_zone` with an obviously expected result is not possible in general; accept a minimal check: the string must match `^[A-Za-z]{3,}[+-]?\d{1,2}(:\d{2})?` and otherwise exit with a one-line error).

Keep unchanged: the timestamp-only phase (no `published_at` fallback for hook phase) and its test, the ignored-reports note, the model, the outputs.

**Tests (replace the round 2 timezone tests):**
1. `--tz SGT-8` on a day whose UTC date differs from its Singapore date (local midnight is 16:00Z the day before): the local-day assignment and the result.
2. The full rule string `EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00` on a date in **EDT** (October) and on one in **EST** (December), each with a session set near the close hour so the UTC-offset difference (4 h versus 5 h) changes which local day or close hour applies; work the expected values out by hand in a comment.
Because `tzset()` changes process state, the test runs each case in its own `subprocess` call of the script (or resets `TZ`).

Verify: tests pass; whole suite serially (baseline with the script 92); mutations in `git worktree add --detach` copies, each must FAIL a test: wrong sign for `SGT-8`; ignoring DST (treat the EST5EDT string as fixed UTC-5); the three WO mutations (`<` → `<=`, `published_at` for the phase, truncation note removed). Report (≤ 80 lines): line counts, hand answers, mutations, suite, deviations, model.
