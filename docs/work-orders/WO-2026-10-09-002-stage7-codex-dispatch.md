AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-09, §5) · REASONING: high (gate timing)
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-09-002-config-window`).
- Run the host suite in place and a local ARM build in a scratch copy.
- Write temporary files under **`build-tmp/wo20261009-002-stage7/`**, and remove **only that directory**.
- Mutate only in scratch copies.

**Not authorized:**
- Changes outside your scratch directory.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.
- Deleting anything you didn't create, including `build-tmp/` itself.

# Stage 7 dispatch: WO-2026-10-09-002 (the once-a-day config window)

**Goal, in plain language:** a ledger config change reaches every sleeping device within a day. On one connection per day, and on the boot connection, the sleep gate holds briefly after the outbound syncs and ends early when an input `onSync` arrives. No other connection pays anything.

**Binding spec:** `docs/work-orders/WO-2026-10-09-002-config-window.md`, "The fix" and rulings 1–4 (including the condition that the boot hold doesn't depend on clock trust).

**Inputs:**
- Evidence: `docs/work-orders/WO-2026-10-09-002-step0-report.md` and `docs/work-orders/WO-2026-10-07-005-ledger-sync-evidence.md`.
- Implementation: `docs/work-orders/WO-2026-10-09-002-stage6-copilot-dispatch.md`, and Copilot's report (the end of `build-tmp/WO-2026-10-09-002-stage6-copilot-output.log`).

Treat every claim as unverified.

**What changed:**
- `src/cloud/LedgerClient.cpp` (`Cloud::areLedgersSynced()`): the hold.
- `src/state/State_Sleep.cpp`: `cloudSyncStartMs = 0;` at the CONNECTED+open abort, with the variable's declaration moved above it (a deviation Copilot reported).
- New `tests/config_window_hold_test`.
- Claude Code counted +18 net code lines (comments excluded).

## Test-suite hygiene

`tests/thermal_coupling_structural_test.py` scans the whole worktree. Run the host suite before you create any copy of `src/`, or after removing it.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Linkage and build.** A fresh-path release build: text/data/bss against v40's 151012 / 1090 / 2196 (Copilot reports 151308 / 1090 / 2212). Confirm the hold logic and the `:407` reset in the ELF.
2. **Tests.** Report the full suite as `N/N (sh via zsh, py via python3)`.
3. **The hold's behaviour.** Trace `areLedgersSynced()` call by call. Confirm, on the boot connection and on the first connection after today's open:
   - it returns **false** while the outputs are pending and for up to `LEDGER_SYNC_TIMEOUT_MS` after they clear;
   - it returns **true** on the first call after either input ledger's `lastSynced` advances, or after the timeout;
   - it sets `holdDoneEpoch` only when the hold **completes**.
4. **No cost elsewhere.** Every other connection behaves exactly as at v40. In particular, confirm the `4c2b734` early return (both input ledgers synced means true at once) and the partial-sync branch.
5. **Ruling 4.** The boot hold (`holdDoneEpoch == 0`) holds with an untrusted clock, and the first-after-open hold doesn't.
   - What does `DailyBoundary::todayAt()` return with an untrusted or invalid clock?
   - Can the trusted term ever cause a hold on every connection?
6. **"Once a day" edges.**
   - A hold cut short (a connection change, an occupancy-pending exit, `GateFail`, the CONNECTED+open abort) is retried on the next connection.
   - A connection before open (for example a 03:00 boot) can hold twice that day. Accepted; confirm it's bounded.
   - What happens if `lastConnection` is 0 at the edge (Copilot deviation 3)?
7. **Gate interactions.** With the hold active, check:
   - the gate's ledger blocker and budget (30 s base, 70 s when the output is pending), including the accepted rare false alert 44;
   - WO-004's pending-flag exits;
   - 1b's failsafe (is `lastConnection` unchanged?);
   - the standby-latch teardown timing.
8. **The `:407` change.**
   - The CONNECTED+open abort now resets `cloudSyncStartMs`.
   - Moving the declaration upward is behaviour-neutral: it's the same function-local static, and no use comes before the old position.
   - It matches the firmware-update and occupancy-pending exits.
9. **Tests and mutations.**
   - Does the new test drive the real function, byte-checked?
   - Re-run Copilot's six mutations: the boot hold gated by trust; the anchor removed; timeout only; `holdDoneEpoch` set when cut short; the early return ungated; the `:407` reset removed.
   - Add one of your own: compare with `>` instead of `!=` for `inputLanded`. A clock step backwards must not suppress the early end. Check whether a case catches it, and report it if none does.
10. **Budget.** Net `src/` code lines (nonblank, non-comment) against +20.

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–10.**
- **Findings,** with file:line and severity. Separate observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you deleted only your scratch directory.

Limit: 150 lines.
