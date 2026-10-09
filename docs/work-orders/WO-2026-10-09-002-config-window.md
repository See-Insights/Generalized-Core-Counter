# WO-2026-10-09-002: the once-a-day config window

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3, §12.1, §13 (b) and (c).
**Base:** main `83df4a3` (v40 plus docs), branch `wo/2026-10-09-002-config-window`.
**Recorded by:** Claude Code, from the architect's WO and rulings (2026-10-09).

## Plain goal

A ledger config change reaches every sleeping device within a day. On one connection per day, and on the boot connection, the sleep gate holds briefly after the outbound syncs and ends early when an input `onSync` arrives. No other connection pays anything.

## Why

Device OS 6.4.1 exposes no inbound-pending signal (`WO-2026-10-07-005-ledger-sync-evidence.md`). The gate's inbound test returns at once when both input ledgers have ever synced, so a fresh inbound fetch is cut off by teardown. Field case: Dev-09 (v40, 2026-10-09). `connectionMode: 0` did not land through five connections (`-1001` at 14:00:44 and 14:10:49, about 3 s after `GateRelease`). It landed only on the boot connection after a pin reset. Chip's 7 Oct decision: one config wait per day plus the boot connection, and no per-connection dwell.

## Step 0 (done)

`docs/work-orders/WO-2026-10-09-002-step0-report.md`: **PROCEED**, about +17. A separate read-only session ran on `claude-sonnet-5-5` at `--effort high`.

## The fix (from Step 0)

In `Cloud::areLedgersSynced()` (`src/cloud/LedgerClient.cpp`):
- **Statics** (RAM only): `holdDoneEpoch`, `configHold` and `holdBaseSynced`.
- **At the new-connection edge,** decide `configHold` and snapshot `holdBaseSynced = max(default, device lastSynced)`.
- **While the output ledgers are pending,** re-anchor `firstConnectedTime`, so the 10 s counts from the outbound clear.
- **End the hold** when `max(lastSynced)` differs from the snapshot (the input `onSync`) or the window `LEDGER_SYNC_TIMEOUT_MS` has passed. Then set `holdDoneEpoch` to the current connection epoch.
- **The "both synced" early return** is skipped while `configHold` is set.
- **One log line** at the end of the hold, with the reason.

The same mechanism applies to every mode that sleeps.

## Architect's rulings (2026-10-09)

1. **Include `cloudSyncStartMs = 0;` at `State_Sleep.cpp:407`** (the CONNECTED+open abort). This WO increases that path's exposure.
2. **The bound is 10 s, via the existing `LEDGER_SYNC_TIMEOUT_MS`.** No new constant.
3. **Accept both smaller edges with no extra code:**
   - No extra second for a second input ledger; it can land a day later.
   - The rare false alert 44 when the output ledger clears within 10 s of the 70 s budget is recorded, not coded around.
4. **Condition: the boot hold doesn't depend on clock trust.** `holdDoneEpoch == 0` always holds. The trusted-clock guard applies only to the first-after-open decision (`holdDoneEpoch < DailyBoundary::todayAt(openTime)`). Test it: a boot with an untrusted clock still holds.

## Size budget

At most **+20** net `src/` lines. Over budget means stop and report.

## Agents and models

- **Implementation:** Copilot, Sonnet tier, **high** (gate timing).
- **Verification:** Codex, `gpt-5.6-sol`, **high**.
- Every model is probed first (§5). The two-round rule applies.

## Bench: in the v41 soak

1. **Config change:** a Dev-09 `device-settings` change lands at boot or at the first connection after open, **without a pin reset** (`LedgerCallback: kind=input`, then the `Config:` line).
2. **No-change day:** exactly **one** extra wait (outbound clear plus up to 10 s) per device-day, and no change on any other connection.

## Release

**v41**, with WO-2026-10-09-001 (TimeDiag on change) and the standby-latch WO.

## Closing record (to be completed)
