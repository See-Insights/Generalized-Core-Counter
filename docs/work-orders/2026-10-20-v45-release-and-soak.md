# v45 release and bench soak (Step 6 power half: P-0 to P-6)

**Author:** architect, 2026-10-20 · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §13 (g), (h), (i), (j)
**Agents:** release PR, Claude Code itself (no model dispatch); soak grading, a **Sonnet subagent, medium**. Report the model used.
**Budget:** 0 `src/` lines beyond the version string.
**Start:** only after #108 (v44, Device OS 6.6.0) is merged and v44 is the product default. **Cut the branch before any sleep WO (S-1 onward) merges.**

## Plain goal

Ship the Step 6 power half (P-0 #110, P-1 #112, P-2 #114, P-3 #116, P-4 #118, P-5 #123, P-6 #124) as v45 on Device OS 6.6.0. **The device should behave exactly like v44**, except for P-0's one-time retained reset on the first boot after the update.

## 1. Release PR (Claude Code)

- Branch `release/v45` from main after #108. Confirm the `src/` diff from v44's commit contains only P-0 to P-6 and the version string; list anything else and stop.
- `bump_version.sh` to `v45-PowerSplit` (or the nearest under the convention); product version 45. Final before the soak.
- CHANGELOG, plain language:
  - no intended behavior change; the power, failsafe, report, clock and modem-health code moved to its own modules (main file 2,720 → about 1,490 lines); duplicated power constants and dead code removed.
  - **First boot after this update:** the boot-storm and breadcrumb values reset once (P-0 pins their memory layout), so the first status shows breadcrumb 0. That is expected.
  - **The ThrashGuard diagnostic counters read wrong once** on the first boot after this update (report-only, P-0).
- Fresh `BUILD_PATH_BASE` build on 6.6.0; `strings` shows the version; record the `.bin` size and sha256 against v44's; suite `N/N`, serial. Open the PR; **don't merge until the soak passes.**

## 2. Bench soak: Dev-09 and Dev-14, 24 h. Chip does the device steps

Chip uploads the PR's `.bin` as product v45 (Particle's size and sha256 must match the PR), OTAs it to Dev-09 and Dev-14 only, both locked, KEEP_ALIVE, no config changes. Each is graded over 24 h from its first v45 boot, against its own v44 days.

**Checks, per device:**
1. The v43 and v44 soak checks:
   - the payload unchanged;
   - PIR and timer wakes;
   - the daily reset;
   - standard checks (resets, `watchdogResetCount`, `fh`, alerts, GateFail).

   "Missing" means a report v44 would have sent.
2. **P-0's one-time reset:**
   - on the first boot after the OTA, the status shows `appBreadcrumb` 0 and **no boot-storm alert and no boot-storm sleep**;
   - on the second boot (the nightly hibernate wake is enough), the values carry over normally.
3. `connecttime`, `slow_teardown` trips, alert 15, and the `MODEM_HEALTH`/`MODEM_POLICY` lines against v44. The lines must be identical in wording (P-5).
4. **Evidence for the later sleep simplification (counts only, no pass/fail):** how often each of these fires on the bench, from serial:
   - the STOP fallbacks (both);
   - the hibernate bypass;
   - the nightly heap guard;
   - the all-modes-failed reset.

Any breadcrumb-28 watchdog, a payload difference, a missing report, or a boot-storm alert after the update is a FINDING and holds the rollout.

## 3. Default (if both pass)

Set v45 as the product default (recheck size and sha256); merge the release PR; fleet health check about 24 h later. Dev-09 and Dev-14 stay locked for v46 (the sleep split).

## Result

(to be filled in by the grader)
