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

## Stage 6 and Stage 7 round 1 (2026-10-09)

- **Stage 6** (Copilot, `claude-sonnet-5.5`, high): +18 net code lines.
  - **Code:** the hold in `areLedgersSynced()`, and `cloudSyncStartMs = 0;` at the CONNECTED+open abort (its declaration moved up, behaviour-neutral).
  - **Test:** new `config_window_hold_test` (real function, byte-checked, 12 cases; 6 mutations caught).
  - **Results:** suite 74/74 (sh via zsh, py via python3); ARM 151308 / 1090 / 2212.
- **Stage 7 round 1** (Codex, `gpt-5.6-sol`, high): **NOT VERIFIED** (`WO-2026-10-09-002-stage7-verdict.md`).
  - **Passes:** the warm-path hold, no cost elsewhere (the `4c2b734` return intact), ruling 4, gate interactions, the `:407` change, and the budget.
  - **F1 (P2):** `inputLanded` compares only `max(defaultSync, deviceSync)` with one saved maximum. One ledger's `onSync` can be masked by the other, for example after a backward clock step. The hold then runs to the timeout instead of ending early.
  - **F2 (P2):** the hold stamps `holdDoneEpoch = currentConnectionEpoch`. If `lastConnection` is 0, the zero sentinel stays, and every later connection pays the boot hold.
  - **F3 (P3):** a `!=` → `>` mutation survives every committed case; the test suite has no unequal-baseline backward-clock case.

## Round 2 (approved by the architect, 2026-10-09; the last under the two-round rule)

- **F1:** keep a separate snapshot for each input ledger, and end the hold when **either** ledger's `lastSynced` differs from its snapshot.
- **F2:** `holdDoneEpoch = currentConnectionEpoch ? currentConnectionEpoch : 1;`, on the same line, so "done" is never stored as 0.
- **F3:** add a different-baselines, backward-clock test case, so the `!=` → `>` mutation fails.
- **Cap:** +20 for the whole WO.
- **Pre-ruled fallback:** if the total exceeds +20, implement F1 and F3 only, and record F2 as **accepted**. It fails toward extra holds, and extra holds deliver config.
- **Agents:** Copilot on Sonnet, high; Codex on `gpt-5.6-sol`, high.

## Stage 6 round 2 and Stage 7 round 2 (2026-10-09)

- **Stage 6 round 2** (Copilot, `claude-sonnet-5.5`, high):
  - **Code:** F1, per-ledger snapshots ending on either change (+2 net); F2, `holdDoneEpoch = currentConnectionEpoch ? currentConnectionEpoch : 1;` (0 net).
  - **Tests:** F3 (an unequal-baseline backward-clock case) and a zero-epoch case. 9 of 9 mutations caught.
  - **Totals:** WO **+20 code lines, exactly at the cap**; the fallback was not needed. Suite 74/74 (sh via zsh, py via python3); ARM 151316 / 1090 / 2220.
- **Stage 7 round 2** (Codex, `gpt-5.6-sol`, high): **VERIFIED WITH CONCERNS** (`WO-2026-10-09-002-stage7-round2-verdict.md`).
  - **Passes:** every round-1 and round-2 check. F1 is confirmed for both equal- and unequal-baseline backward steps, and output writes can't move the input snapshots. F3 catches `>`. Codex's own "snapshot only the default ledger" mutation was caught. No P1 or P2 findings.
  - **P3 concern, for the architect:** the stored marker `1` (from F2) can allow **one** extra first-after-open hold once the clock becomes trusted. It is reachable if the first successful connection stamped `lastConnection = 0` before trust arrived. It is bounded to once, because the next connection writes a real epoch, and it fails toward one extra delivery-safe hold. This edge was not explicitly pre-accepted.

## Closing record (2026-10-09)

**Result:** **VERIFIED WITH CONCERNS** at Stage 7 round 2 (`WO-2026-10-09-002-stage7-round2-verdict.md`), within the two-round rule. **Stage 8 approved** by the architect. **The P3 is accepted:** at most one extra first-after-open hold, and only if the first connection recorded `lastConnection = 0`; it fails toward delivering config. **Release: held for v41**, with WO-2026-10-09-001 (TimeDiag on change) and the standby-latch WO.

| Stage | Agent / model | Result |
|---|---|---|
| Step 0 | Claude Code, separate session, `claude-sonnet-5-5`, `--effort high` | PROCEED, est. +17 |
| Stage 6 round 1 | Copilot, `claude-sonnet-5.5`, high | +18; 74/74 |
| Stage 7 round 1 | Codex, `gpt-5.6-sol`, high | NOT VERIFIED: F1 max-based snapshot, F2 zero marker, F3 test gap |
| Stage 6 round 2 | Copilot, `claude-sonnet-5.5`, high | +2 (F1 per-ledger snapshots; F2 `? : 1`; F3 test) |
| Stage 7 round 2 | Codex, `gpt-5.6-sol`, high | VERIFIED WITH CONCERNS; one bounded P3, accepted |

Every model was probed first (§5).

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| Round 1 | +20 WO cap | — | +18 | 74/74 |
| Round 2 | — | — | +2 | 74/74; 9 of 9 mutations caught |
| **WO total** | **+20** | — | **+20** (`LedgerClient.cpp` +19, `State_Sleep.cpp` +1) | **74/74 (sh via zsh, py via python3)**; new `config_window_hold_test` (14 cases) |

- **ARM build:** 151316 / 1090 / 2220 (+304 text against v40).
- **What changed:**
  - **`LedgerClient.cpp`, `areLedgersSynced()`:** on the boot connection (regardless of clock trust) and on the first connection after today's open (clock trusted), the gate holds after the outbound ledgers clear. The hold ends when either input ledger's `lastSynced` changes (`onSync`) or after `LEDGER_SYNC_TIMEOUT_MS` (10 s cellular). Only a completed hold counts toward "once a day". All state is RAM-only, with no persisted field.
  - **`State_Sleep.cpp`:** the CONNECTED+open abort now resets `cloudSyncStartMs`.
- **Accepted:**
  - a second input ledger may land a day later;
  - a rare false alert 44 when the output ledger clears within 10 s of the 70 s budget;
  - the bounded P3 above.
- **Bench: in the v41 soak.**
  - A Dev-09 `device-settings` change lands at boot or on the first connection after open, **without a pin reset** (`ConfigHold: ended reason=onSync`, `LedgerCallback: kind=input`, then the `Config:` line).
  - Exactly one extra wait per device-day when nothing changed.
