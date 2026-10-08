# WO-2026-10-08-001: the failsafe counts only overdue expected connections (Step 6 WO 1b)

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §3, §12.1–§12.4.
**Base:** main after #75 (`2d77c2a`) for Step 0. The branch `wo/2026-10-08-001-failsafe-overdue` has main after #76 (v39, `8f82e1b`) merged in at `f2dd392`; `src/` is unchanged apart from the version file.
**Recorded by:** Claude Code, from the architect's WO and rulings (2026-10-08).

## Plain goal

The connectivity failsafe escalates only when a connection the device was expected to make is overdue, never just because time passed while no connection was due.

## Step 0 (done)

Records: `docs/work-orders/WO-2026-10-08-001-step0-report.md` (the report and its addendum) and both dispatches. A separate read-only session ran on `claude-sonnet-5-5` at `--effort high`. Findings:

- **The cadence scenario is real.** With an effective cadence of 3 h or more (battery multiplier, or a long configured interval), the fixed 3 h threshold resets the device before its next connection is due.
- **The CONNECTED-online gap is real in the code.** `lastConnection` is written only in Connect, and Report's `already connected` branch never goes through Connect, so an online CONNECTED device would reset about every 3 open hours. Not seen in the fleet: Court3 is KEEP_ALIVE, which reconnects through Connect on every wake, and its resets ended real silences.
- **`reportingIntervalSec` wraps.** The store is `uint16_t`, but `ConfigApply.cpp:265` accepts up to 86400.

## Architect's rulings (2026-10-08)

1. **Cadence rule:** whenever the effective cadence (`ReportingPolicyResolver::resolveRuntime(...).effectiveIntervalSec`) is ≥ 3 h, the stale threshold is cadence + 3 h. It applies in every mode (no INTERMITTENT condition), and it is **compiled out of the failsafe test build** (`CONNECTIVITY_FAILSAFE_TEST_MODE`). Test mode's 5-minute threshold would otherwise stretch the bench reset to about 65 minutes. The test-mode mirror (`ConnectivityFailsafeTest.cpp`) therefore needs no change.
2. **CONNECTED fix, option (b):** Report's `already connected` branch refreshes `SystemConfig::set_lastConnection(Time.now())`. The alert-40 side effect is accepted: a CONNECTED device failing webhooks for 6 h should escalate.
3. **Interval cap:** cap `reportingIntervalSec` at 65535 in ConfigApply's validation (0 net lines), inside this WO. The backlog gets "widen interval storage if a daily cadence is ever needed".
4. **Scope and priority:** both fixes are in scope (Chip, 2026-10-08). If they don't both fit in +20, the cadence rule goes first and the CONNECTED fix waits.

## Size budget

WO total ≤ **+20** net `src/` lines; **+10 expected**. Going over means stop and report. No compressed code (§12.3).

## Agents and models

- **Implementation:** Copilot, Sonnet tier, medium reasoning (small, well-specified changes).
- **Verification:** Codex, `gpt-5.6-sol`, high.
- Every model is confirmed with a one-line probe (§5).
- The two-round rule applies.

## Closing record (to be completed)
