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

## Pre-ruling for Stage 7 (architect, 2026-10-08)

**Alert 41 on an over-range `reportingIntervalSec` is accepted**, provided Codex confirms both of these:
1. other out-of-range values already reject the whole config apply in the same way;
2. the device keeps its last good configuration, rather than defaults or a partial apply.

If either fails, it is a round-2 finding. Otherwise the closing record lists it as an accepted behaviour change, with a CHANGELOG line for v40.

*Claude Code's note before the verdict:* `applyConfigurationFromLedger()` (`ConfigApply.cpp:133-140`) runs all six sections before combining their results. So within one ledger update, valid fields are applied and only the rejected field keeps its previous value. Condition 2 needs Codex's judgement on whether that counts as a "partial apply". It is how every field behaves today.

## Stage 7 round 1 and the architect's rulings (2026-10-08)

**Stage 7 round 1** (Codex, `gpt-5.6-sol`, high): **NOT VERIFIED** (`WO-2026-10-08-001-stage7-verdict.md`). Two medium findings and one low.

The rulings for round 2, the last under the two-round rule:
1. **Finding 1, per-field config apply: option (a).** The per-field apply is accepted as existing behaviour. Pre-ruling condition 2 is narrowed to "no defaults, no half-applied field", which holds. **Alert 41 on an over-range `reportingIntervalSec` is an accepted behaviour change** (CHANGELOG, "Unreleased", for v40). "All-or-nothing config apply" goes in the recovery plan as a possible later WO.
2. **Finding 2, the cadence test:** fix the test so it drives the real supervisor block (or a byte-checked copy), so the 4 h checks fail when the formula is mutated. No `src/` change.
3. **Finding 3, cost:** move the cadence rule below the stage and cooldown returns. 0 net lines, same behaviour.

Agents: Copilot, Sonnet tier, medium; Codex, `gpt-5.6-sol`, high.

## Stage 6 round 2 and Stage 7 round 2 (2026-10-08)

- **Stage 6 round 2** (Copilot, `claude-sonnet-5.5`, medium):
  - **F3:** the cadence-rule block moved below the stage-3 and cooldown returns. It is byte-identical to round 1, at 0 net lines.
  - **F2:** the test extracts the real block with a `COPY_MISMATCH` check.
  - **Results:** tests 72/72 (sh via zsh, py via python3); release build 151028 / 1090 / 2196.
- **Stage 7 round 2** (Codex, `gpt-5.6-sol`, high): **NOT VERIFIED** (`WO-2026-10-08-001-stage7-round2-verdict.md`). Every check, test and mutation passes; there are no survivors. Check 8 passes under the narrowed condition 2, and `alert-codes.md` needs no change. The two medium findings are edges for the architect to accept or reject:
  1. **Per-pass `String` allocation.** The move crosses `connectivityFailsafeJitterSec()` (`Generalized-Core-Counter.cpp:386-400`, called at `:2648`). It builds a `String` from `System.deviceID()` on every call, so long-cadence devices between 3 h and their extended threshold allocate and free a device-ID string on every loop pass. *Claude Code's note:* main already does this on every pass for any device past 3 h in open hours (before it resets), so it is not new relative to main, only relative to round 1.
  2. **Cadence shortening.** The threshold uses the current cadence only. If the battery recovers and the cadence shortens (for example from 12× to 1×) after age has passed 3 h, the failsafe can reset before Report gets its first chance on the new schedule. *Claude Code's note:* main resets at 3 h in that case anyway, so this is a residual gap against the plain goal, not a regression.
- **Low:** the test retypes the stale-return line rather than extracting it; it is checked by location and order.
- **Two-round rule:** this was the last round. The WO stops unless the architect accepts both edges.

## Closing record (to be completed)
