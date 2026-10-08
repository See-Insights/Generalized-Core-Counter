AGENT: Claude Code (resumed headless session `2291df78-38b0-4fdd-aa90-0e53505f8e1b`, the same one that wrote the Step 0 report) · MODEL: claude-sonnet-5-5 · REASONING: high (`--effort high`)
AUTHORIZATION SCOPE: the same as Step 0, read-only. Your final message is the addendum; Claude Code appends it to `docs/work-orders/WO-2026-10-08-001-step0-report.md`, then commits and pushes it. No file writes, no state-changing git, no builds or tests, no device or network access.

# WO-2026-10-08-001 Step 0 addendum: the architect's 1b rulings (2026-10-08)

**Base:** the checkout is now main at `8f82e1b` (after #76, the v39 release). Its `src/` is identical to your Step 0 base `2d77c2a`, except `src/FirmwareVersion.h`, so your earlier line numbers still hold. Cite file:line in the current tree.

**The architect's rulings on your report:**
1. **The cadence rule is approved without the INTERMITTENT condition:** whenever the effective cadence is ≥ 3 h, the threshold is cadence + 3 h.
2. **The CONNECTED-online case (your "possible larger gap") is in scope** under the same plain goal.
3. **The whole WO must fit in ≤ +20 net `src/` lines.** If both fixes don't fit, the CONNECTED fix takes priority.
4. **Check whether the `effectiveIntervalSec` the fix reads is `uint16_t`-truncated.** If it is, report it before anything is implemented.

## Questions

1. **The CONNECTED-online case, end to end.** Trace a CONNECTED-mode device that stays on the cloud through open hours, with file:line at each step:
   - Report sees `Particle.connected()` true and takes `already connected`.
   - `lastConnection` is written only at `State_Connect.cpp:513`.
   - The failsafe age grows past 3 open hours.
   - The stage escalates: which stage, what action, and the 6 h cooldown.
   - Every guard in `connectivityFailsafeSupervisor()` that could stop it. Say whether any does.

   Also cover a background cloud reconnect (Device OS reconnecting without the app entering CONNECTING): does anything refresh `lastConnection`? Then say, with code, **whether KEEP_ALIVE is affected**: does each KEEP_ALIVE wake go through CONNECTING (`Connect: start`), or can it report while `Particle.connected()` is already true? The 2026-10-08 bench log `~/Downloads/2026-10-08 09-05-50 Boron CDC Mode #1.log` shows `Sleep: … standby=1/1` followed by `Report->Connect` after the wake; use it as evidence where it helps. Mark OBS and INF.
2. **The smallest CONNECTED fix, one or two lines.** Compare:
   - **(a)** the failsafe treats `Particle.connected()` as nothing overdue, for example an early return near the other defers;
   - **(b)** Report's `already connected` branch refreshes `SystemConfig::set_lastConnection(Time.now())`.

   **List every reader of `get_lastConnection()`** in `src/` (for example the alert-40 "connected recently" test in `State_Report.cpp`, status payloads and the failsafe) and say how each option changes what that reader sees. Then:
   - say which option covers more cases, including a CONNECTED device that is online but has no report due for hours;
   - say which carries less risk, for example hiding a genuine failure where the cloud session is up but webhooks fail;
   - pick one, with its exact file:line and net lines.
3. **The cadence rule without the INTERMITTENT condition.**
   - Re-estimate it.
   - Say what it now changes for KEEP_ALIVE and CONNECTED, which connect on every report. Can their effective cadence reach 3 h, through a configured interval or the battery multiplier? If so, what does a cadence + 3 h threshold mean for them?
   - Confirm the test-mode mirror (`ConnectivityFailsafeTest.cpp`) still needs the change, and size it.
4. **Truncation.**
   - Trace `effectiveIntervalSec` from the configured value (`Config.h:69`, `ConfigApply.cpp:265`) through `ReportingPolicy` / `ReportingPolicyResolver::resolveRuntime()`. Give the type at each step.
   - Is the value the fix would read truncated to `uint16_t` anywhere: a configured interval above 65535 s, or interval × multiplier overflowing?
   - If it is on the fix's path, say so plainly: the architect wants that reported before implementation.
   - List any truncation sites **outside** the fix's path separately; they go to the recovery plan.
5. **Flash churn.** Restate your earlier point precisely, with file:line: when the low-battery block fires at stage 0, does `persistConnectivityFailsafeState(...)` with `flushNow` run on every loop pass? What bounds it? It goes in the recovery plan as written.
6. **Budget and verdict.**
   - A table: cadence rule (both files), CONNECTED fix, and total, against +20.
   - If both don't fit, say what the CONNECTED fix alone costs, and what would be dropped or split.
   - End with **PROCEED** or **STOP** and the reason. STOP if truncation is on the fix's path, if the total is over +20 even after dropping to the CONNECTED fix, or if anything needs a new state, timer or persisted field.

## Format (your final message: Markdown only, **≤ 70 lines**, nothing before or after it)

Start with the heading `## Addendum (2026-10-08): the architect's rulings`, then sections 1–6 using tables, then "Open points". Re-open every `src/` file:line you cite before finishing.
