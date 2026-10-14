AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-14, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Add the new test files: `tests/report_payload_golden_test.sh`, `tests/report_payload_golden_test.cpp`, `tests/fixtures/report_payload_golden.txt` (or similar names; report what you chose), and any fakes or stubs they need under `tests/`.
- Delete the stale payload mirror in `tests/watchdog_alert_code_test.cpp:100–135`, and adjust only what that deletion requires in the same file.
- Remove or re-point the superseded check in `tests/daily_cleanup_boundary_test.py:236–245`.
- Run the host suite.
- Your scratch directory is **`build-tmp/wo20261014-001-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- **Any `src/` change.** This WO is test-only: `git diff -- src lib` must be empty.
- Any other test's assertions, any `docs/` file, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Device or network access.
- Deleting anything you didn't create, including `build-tmp/` itself or the files already in it.

**SIZE BUDGET:** `src/` **0**. Tests about +300 / −40. **If the test grows past +400 lines, STOP and report.**

# Stage 6 dispatch: WO-2026-10-14-001 (Step 6 2b-1, golden payload test)

**Goal, in plain language:** pin today's webhook payload bytes, event name and publish flags with a host test that runs the **real** `publishData()` code through an observer of the actual queue call. Every later 2b WO can then prove its output is byte-for-byte unchanged.

**Repository:** this worktree, branch `wo/2026-10-14-001-step6-2b1-golden-payload` (from main `7a5f5e7`), HEAD at the commit that adds this dispatch. Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-14-001-step6-2b1-golden-payload.md`, including the architect's rulings:
- keep the `alerts`/`resets` swap exactly;
- pin MEASUREMENT's counting-format output;
- pin `temp` NaN/Inf rendering as today.

Where this dispatch and the WO differ, the WO wins; report the difference.

## The spec it refers to (Step 0 round 2, Q3, reproduced verbatim)

The WO cites `WO-2026-10-13-002-step0-round2-report.md`, which is not on main yet (it's in PR #96). Its Q3 section follows:

## Q3: The golden test (2b-1)
- **Boundary:** two literal markers, `static const char *batteryContext[7] = {` (`:1967`) through `bool queued = PublishQueuePosix::instance().publish(webhookName, data, PRIVATE | WITH_ACK);` (`:2058`), wrapped as `bool run(time_t stampOverride)`; `EXTRACT_FAILED` if either is missing or not unique (the `alert19_clears_after_report_test.sh:19–39` pattern). The markers stay valid across 2b-3; only the region shrinks. Post-queue code (`:2060+`) stays with the alert19 test.
- **Observer fake:** `publish(const char *name, const char *data, PublishFlags f1, PublishFlags f2 = PublishFlags())` (`lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:181`), recording call count (must be 1), name string and pointer (== the fake `getWebhookName()` return), data bytes and length, raw flags. Flag constants use the Device OS values mirrored in `lib/StorageHelperRK/automated-test/UnitTestLib/Particle.h:203–206` (PRIVATE 1, WITH_ACK 8): expect `f1 == 0x9`, `f2 == 0`. Do not reuse `tests/stubs/diag_overrides/PublishQueuePosixRK.h:10` (PRIVATE = 0 cannot detect a dropped PRIVATE).
- **Controlled inputs, each with a call counter asserted exactly once:**
  - `Time.now/minute/second`, `System.freeMemory()`, `HAL_Core_Runtime_Info`, `AwakeCycles::cycles/sleeps`, `millis()`;
  - `PowerManager::batteryState/soc`, `SensorManager::cachedBatteryVoltage` (bool and value);
  - `SystemConfig::get_sensorMode/get_lastConnectionDuration`, `RecoveryState::get_alertCode/get_resetCount`, `CurrentReadings` occupied, total, hourly, daily, internal temp;
  - `Cloud::getWebhookName`, `stampOverride`, queue return;
  - Stage 4's additions: locale pinned to C, a global `operator new` counter asserting no heap use in the region (protects `fh`).
- **Vectors (≈90):**
  - mode {0,1,2} × stampOverride {0, nonzero} × (min,sec) {(27,14),(0,0)};
  - alert {−128,−1,0,19,42,127} in both layouts (pins the swap `:2025–2026,2044–2045`);
  - SOC {NaN, ±Inf, −0.01, 0, 87.5, 100, 100.01} × battState {0,2,6,7,255};
  - vc {unavailable, 3.91, 4.20};
  - temp {23.4, −999.99, NaN, ±Inf, ±FLT_MAX} plus a 1e0..1e38 sweep that straddles 255/256 bytes (pins truncation: worst-case 237 B, FLT_MAX overflows 255);
  - all-max widths; name {"", configured, convention}; queue false.
- **Host caveats (INF):** host `unsigned long` is 64-bit, so keep every `%lu` input ≤ 4294967295 (the existing `report_payload_fields_test.py` uses 9999999999 as text only). NaN/Inf strings are host-libc; expected bytes are recorded from the unchanged code on the same host and committed, never regenerated.
- **Existing tests:** supersede `watchdog_alert_code_test.cpp:100–135` and `daily_cleanup_boundary_test.py:236–245`. Coexist with `alert19_clears_after_report_test.sh`, `occupancy_report_by_mode_test.sh` (only anchors `:235`), `report_payload_fields_test.py` (width and unquoted-number rule, re-pointed in 2b-3) and `clock_resync_wiring_test.py:635–652`.


## Facts checked at this HEAD (Claude Code, 2026-10-14)

- **Start marker:** `static const char *batteryContext[7] = {` is unique, at `src/Generalized-Core-Counter.cpp:1967`.
- **End marker:** use the **whole line**, `bool queued = PublishQueuePosix::instance().publish(webhookName, data, PRIVATE | WITH_ACK);` (unique, `:2058`). The bare `PublishQueuePosix::instance().publish(` appears 5 times in this file (`:2058`, `:2280`, `:2320`, `:2355`, `:2468`), so it is **not** a valid marker.
- **The stale mirror** is at `tests/watchdog_alert_code_test.cpp:100–135`; the superseded split is at `tests/daily_cleanup_boundary_test.py:236–245`.

## Requirements beyond the spec

1. **Record from unchanged code.** Build the expected fixture by running the harness against today's `src/`, on this host, and commit it. The test must never regenerate its fixture. State in the test which host produced it.
2. **Prove it bites.** Run each of these in a scratch copy of the extracted region (never in `src/`) and confirm the test fails:
   - change one format specifier (for example `%4.2f` → `%.2f` for `battery`);
   - rename one key;
   - undo the `alerts`/`resets` swap in one branch;
   - drop `PRIVATE` from the flags;
   - change the event name the fake returns, versus the one recorded.

   You can build the mutations into the `.sh` (as `tests/standby_latch_daily_clear_test.sh` does) or run them by hand; report which.
3. **Suite:** the full host suite before and after, as `N/N (sh via zsh, py via python3)`. **The baseline on this branch is 76/76.** Shell tests are zsh scripts: run them with `zsh`, not `bash`. No copy of `src/` under the worktree while the suite runs; put mutation copies under `$TMPDIR` (see `tests/standby_latch_daily_clear_test.sh:18`).
4. **Size:** test lines added and removed; `git diff --stat`; confirm `git diff -- src lib` is empty.

## Implementation Report (required, as your final message)

- **Files:** for each, lines added and removed.
- **The vector matrix:** what it covers and the total count.
- **Mutations:** each one, and the check that caught it.
- **Commands and results,** with interpreters.
- **Confirmations:** no `src/` diff, and only your scratch directory deleted.
- **Deviations** (or "none").
- **The model and reasoning level used.**
