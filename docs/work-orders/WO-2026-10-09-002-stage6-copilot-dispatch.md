AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-09, §5) · REASONING: high (gate timing)
AUTHORIZATION SCOPE:

**Authorized:**
- Edit `src/cloud/LedgerClient.cpp`: `Cloud::areLedgersSynced()` and its includes only.
- Edit `src/state/State_Sleep.cpp`: one line, `cloudSyncStartMs = 0;` in the CONNECTED+open abort at about `:406-409`.
- Add or update tests under `tests/`.
- Run the host suite and a local ARM release build.
- Your scratch directory is **`build-tmp/wo20261009-002-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any other `src/` change: the gate logic in `State_Sleep.cpp` beyond that one line, `ConnectivityPolicy.h` constants, `Cloud.cpp`, persistence.
- Any new persisted field, timer, schedule or constant.
- Any edit to `docs/`, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Flashing, device or network access beyond the Particle compile.
- Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:** at most **+20 net `src/` code lines** (Step 0 estimated about +17, comments included). Over +20 means STOP and report. Don't compress code.

# Stage 6 dispatch: WO-2026-10-09-002 (the once-a-day config window)

**Goal, in plain language:** a ledger config change reaches every sleeping device within a day. On one connection per day, and on the boot connection, the sleep gate holds briefly after the outbound syncs and ends early when an input `onSync` arrives. No other connection pays anything.

**Repository:** this worktree, branch `wo/2026-10-09-002-config-window`, HEAD at the commit that adds this dispatch. Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-09-002-config-window.md`, "The fix" and rulings 1–4. **Design and citations:** `docs/work-orders/WO-2026-10-09-002-step0-report.md` §§1–5. Where this dispatch and the WO differ, the WO wins; report the difference.

## What to implement (`LedgerClient.cpp`, `Cloud::areLedgersSynced()`, about `:100-230`)

| Item | Change |
|---|---|
| S | **Statics, RAM only,** beside the existing ones (about `:106-110`): `holdDoneEpoch` (`time_t`, 0 at boot), `configHold` (`bool`), `holdBaseSynced` (the same type as `lastSynced()`). Add `time/Clock.h` and `time/DailyBoundary.h` if needed. |
| D | **At the new-connection edge** (the `newConnectionObserved` block, about `:118-134`): `configHold = (holdDoneEpoch == 0) \|\| (Clock::isTrusted() && holdDoneEpoch < DailyBoundary::todayAt(SystemConfig::get_openTime()));`. **Ruling 4: the boot hold (`holdDoneEpoch == 0`) must not depend on clock trust;** only the first-after-open term is guarded. Snapshot `holdBaseSynced = max(defaultSync, deviceSync)`. |
| A | **Anchor:** while `configHold && hasPendingOutputLedgerSync()`, set `firstConnectedTime = nowMs`, so the 10 s window counts from the output ledgers clearing. |
| E | **End the hold:** if `configHold` and either `max(defaultSync, deviceSync) != holdBaseSynced` (an input `onSync` landed) or `nowMs - firstConnectedTime > LEDGER_SYNC_TIMEOUT_MS` (the existing constant; no new one), then set `configHold = false` and `holdDoneEpoch = currentConnectionEpoch`. Log one INFO line with the reason (`onSync` or timeout) and the elapsed ms. |
| R | **The early return** (`if (defaultSynced && deviceSynced)`, about `:138`) is taken only when `!configHold`. While the hold is active, the function returns false; let the existing window path decide. Make sure that during the hold it returns **false** until E ends it, and **true** right after. Check the window branches at about `:156-228`. If the existing window logic can't express "false until E", add the smallest explicit `if (configHold) return false;` after E. |
| C | **`State_Sleep.cpp`, the CONNECTED+open abort** (about `:406-409`): add `cloudSyncStartMs = 0;` before the transition, as the firmware-update and occupancy-pending exits do (ruling 1). |

**Must not change:**
- Every connection that isn't a boot connection or the first after today's open behaves **exactly** as today (the early return is unchanged).
- A hold cut short (teardown, an occupancy-pending exit, `GateFail`) does **not** set `holdDoneEpoch`, so the next connection holds again.

## Tests (outside the budget)

New host test (for example `tests/config_window_hold_test.{cpp,sh}`). Drive the **real** `areLedgersSynced()`, compiled or extracted byte-for-byte with a loud `COPY_MISMATCH` check, using stubs for the ledgers' `lastSynced()`, `millis()`, `SystemConfig::get_lastConnection()`, `Clock::isTrusted()`, `DailyBoundary::todayAt()` and `hasPendingOutputLedgerSync()`. Cases:
1. **Boot, trusted clock:** the first connection holds. It returns false until an input `lastSynced` advances, then true at once.
2. **Boot, untrusted clock (ruling 4):** it still holds.
3. **Boot, no change pending:** it holds, then returns true once 10 s have passed since the output clear.
4. **The anchor:** with the output pending for 7 s, the 10 s counts from the output clear, not from gate entry.
5. **Second connection the same day, after a completed hold:** no hold, with the early return as today.
6. **First connection after today's open, with a completed hold from yesterday:** holds. With an untrusted clock, no first-after-open hold.
7. **A hold cut short** (the connection changes before E): the next connection holds again.
8. **No regression of `4c2b734`:** a warm connection outside a hold returns true at once when both ledgers are synced, and the partial-sync branch behaves as before.

**Abort test:** structurally or behaviourally, the CONNECTED+open abort resets `cloudSyncStartMs`.

**Mutations** (each caught by its targeted check, not by a compile failure):
- drop `holdDoneEpoch == 0` (boot hold gated by trust);
- remove the anchor;
- end the hold on timeout only, ignoring `onSync`;
- set `holdDoneEpoch` when the hold is cut short;
- remove the `:407` reset.

Existing tests that pin changed lines: update only what they pin, and report each change.

## Verification

1. **Full host suite,** before and after, as `N/N (sh via zsh, py via python3)`. The baseline is 73/73 on this branch. No copy of `src/` under the worktree while it runs.
2. **Local ARM release build:** a fresh `BUILD_PATH_BASE`. Give text/data/bss against v40's 151012 / 1090 / 2196.
3. **Size:** net `src/` code lines, and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change per item, with net lines.
- How R is expressed: through the existing window path, or an explicit return.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261009-002-stage6/`.
- Deviations (or "none").
- The model and reasoning level used.
