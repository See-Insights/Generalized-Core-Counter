AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-14, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted changes in this worktree (branch `wo/2026-10-14-001-step6-2b1-golden-payload`).
- Run the host suite and the new test.
- Write temporary files under **`build-tmp/wo20261014-001-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies.

**Not authorized:**
- Changes outside your scratch locations.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-14-001 (Step 6 2b-1, golden payload test)

**Goal, in plain language:** pin today's webhook payload bytes, event name and publish flags with a host test that runs the real `publishData()` code through an observer of the actual queue call.

**Binding spec:** `docs/work-orders/WO-2026-10-14-001-step6-2b1-golden-payload.md`; the test spec is the Q3 text reproduced in `docs/work-orders/WO-2026-10-14-001-stage6-copilot-dispatch.md`.

**Inputs:** the Stage 6 dispatch, and Copilot's report (the end of `build-tmp/WO-2026-10-14-001-stage6-copilot-output.log`). Treat every claim as unverified.

**What changed** (uncommitted):
- **New:** `tests/report_payload_golden_test.{sh,cpp}` and `tests/fixtures/report_payload_golden.txt` (129 vectors).
- **Edited:** `tests/watchdog_alert_code_test.cpp` (+1/−66: the stale payload mirror and the two tests that used it) and `tests/daily_cleanup_boundary_test.py` (+1/−8: the stamp-override branch checks).
- **Claimed:** 77/77, and no `src/` or `lib/` diff.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **No `src/` change.** `git diff -- src lib` is empty.
2. **It runs the real code.** The extraction boundary is `static const char *batteryContext[7] = {` through the whole line `bool queued = PublishQueuePosix::instance().publish(webhookName, data, PRIVATE | WITH_ACK);`. It must fail loudly if either marker is missing or not unique. Nothing between the markers may be rewritten before compiling, apart from what the harness documents.
3. **Passes on unchanged code, and the fixture is honest.** The test passes on this tree. Show the fixture was produced by today's `src/`: re-record it yourself into scratch, from a clean copy of HEAD's `src/`, and diff it byte for byte against the committed fixture. The test must not write or regenerate the fixture.
4. **The WO's mutations fail it.** Re-run each in a scratch copy and confirm the test fails, naming the check that caught it:
   - any single format specifier (try at least `battery`, `vc`, `temp`, `timestamp`, and one `%lu`);
   - any key, in each layout;
   - the `alerts`/`resets` swap, in each layout;
   - the flags (`PRIVATE` dropped, and `WITH_ACK` dropped);
   - the event name.

   Add at least one mutation of your own that you think the test might miss, and report the result.
5. **Determinism.** Every input listed in the Q3 spec is controlled and counted (one call each), including locale, `millis()`, heap allocation, and 32-bit `%lu` inputs. Run the test twice and compare. Note anything host-dependent (libc NaN/Inf text, `unsigned long` width) and whether the fixture header records the host.
6. **The removed checks are really superseded.** For each assertion deleted from `watchdog_alert_code_test.cpp` and `daily_cleanup_boundary_test.py`, show the golden vectors that now pin the same behaviour: alert codes in both layouts, the stamp override used only by occupancy, and counting's `endOfPrevHourStampSec`. Flag any coverage lost.
7. **The architect's rulings are pinned:** the `alerts`/`resets` swap, MEASUREMENT's counting format, and `temp` NaN/Inf rendering.
8. **Suite:** the full host suite, `N/N (sh via zsh, py via python3)`. Before the change it was 76/76 (Claude Code's run on this branch).
9. **Budget:** test lines added and removed, against "about +300 / −40; stop past +400".

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–9.**
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.

Limit: 120 lines.
