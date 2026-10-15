# v42 fleet health check (read-only)

**Author:** architect, 2026-10-15 · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2 (Claude Code, read-only), §13 (h), (i), (j)
**When:** after 2b-5a's Stage 8. **Agent:** Claude Code itself (a read-only subsession can't reach S3 or the Particle API), Sonnet tier, medium reasoning.
**Budget:** 0 `src/` lines. Report ≤ 100 lines: `docs/work-orders/2026-10-15-v42-fleet-health-check-report.md` on a docs branch, committed and pushed; PR for Chip. Every number states its source query and window (§13 (j)).

## Plain goal

Confirm every fleet device runs v42 (`v42-ConfigWindowLatch`) cleanly since the 14 Oct rollout, and find anything v42 made worse.

## Window

Per device, from its first v42 boot (reset 70) to the time of the check. Compare against the same device's last 48 h on v40 where a comparison is asked for.

## Devices

The nine US devices (Court1–3, PCKL1–3, MAFC-1/2, Trail02). Dev-09 and Dev-14 for completeness only (locked, bench).

## Checks, per device (one table row each)

1. **Version and OTA time** (reset 70); list any device not yet on v42.
2. **Resets since the OTA:** reason and `resetReasonData`. Expected: none, or hibernate wakes (reason 30) on Trail02 only. Reset 140 data 2 (failsafe) or any watchdog is a finding — give `lastWatchdogBreadcrumb`.
3. **`watchdogResetCount`:** against its pre-OTA value.
4. **Reports:** hourly reports present in open hours; missing ones counted. Delivery rule: only a lost, never-retried report or a stuck device counts against v42.
5. **Heap:** `fh` range; a steady fall is a finding.
6. **Alerts:** list any. Known and not v42's: alert 40 at the open (queued fix), alert 44 after a boot clock step. Alert 41 is a finding.
7. **Battery and modes:** tier, `socTrust`, `connectionMode`, `sensorMode`; any downgrade judged from tier and modes. Note whether Court1 still shows the known downgrade gap (queued item 5).
8. **Failsafe:** `failsafeStage`, `failsafeCount`.
9. **v42-specific, indirect (US devices have no serial):**
   - **Standby latch:** compare `connecttime` per device, v42 window versus last 48 h on v40. A device whose connect times dropped from cold-attach levels (~10 s+) to warm levels (~1–2 s) after a daily close is evidence the daily clear works; say so per device, and say "not determinable" where the data can't show it.
   - **Config window:** confirm no device shows a change in report cadence or a missing first-after-open report attributable to the daily config wait.

## Grade

PASS or FINDING per device, with evidence for every finding. End with one line: v42 healthy on the fleet, or what needs action. No design or fix; findings go to the recovery plan only if Chip agrees.

## Out of scope

Device operations; gauge accuracy (WO 3 evidence); CONNECTED-mode items (no fleet device runs it).
