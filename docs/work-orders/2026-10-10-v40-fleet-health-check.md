# v40 fleet health check (read-only)

**Author:** architect, 2026-10-10 · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2 (Claude Code, read-only), §13 (h) and (i)
**Agent and model:** Claude Code, Sonnet tier, medium reasoning, after a probe. *Why:* reading and tabulating Particle and AWS data; no design.
**Budget:** 0 `src/` lines. Report ≤ 80 lines, saved as `docs/work-orders/2026-10-10-v40-fleet-health-check-report.md` on a docs branch, committed and pushed; PR for Chip.

## Plain goal

Confirm every fleet device runs v40 cleanly after the 10 Oct rollout, and catch anything v40 changed for the worse before v41.

## When

Two passes:
1. **Now:** each device's first v40 status event.
2. **24 h after the last device took v40:** the full check below. One report, written after pass 2; pass 1 is a short chat note only.

## Devices

The nine US devices (Court1–3, PCKL1–3, MAFC-1/2, Trail02). Dev-09 and Dev-14 are already graded (v40 soak, 10 Oct); list them for completeness only.

## Checks, per device (table, one row each)

1. **Version:** `v40-FailsafeAndSensorType`, and the OTA time (reset reason 70).
2. **Resets after the OTA:** list any, with reason and `resetReasonData`. Expected: none, or hibernate wakes (reason 30) on Trail02 only. Reset 140 with data 2 (failsafe) is a finding.
3. **`watchdogResetCount`:** compare with its last pre-OTA value. Any rise is a finding; give `lastWatchdogBreadcrumb`.
4. **Reports:** hourly reports present in open hours; count missing ones. A delivery counts against v40 only if a report was lost, never retried, or the device got stuck.
5. **Heap:** `fh` range across reports; a steady fall is a finding.
6. **Alerts:** list any. Expected: possibly one alert 44 right after the OTA boot (known post-boot clock-step case). Alert 41 is a finding (v40 rejects `reportingIntervalSec` > 65535 and `sensor.type` ≠ 1, so it would mean a ledger problem).
7. **Battery and modes:** tier, `socTrust`, `connectionMode`, `sensorMode`. Judge any downgrade from tier and modes, not the flag alone.
8. **Failsafe:** `failsafeStage` and `failsafeCount` from status. Non-zero is a finding.

## Grade

PASS or FINDING per device, with the evidence (event time and fields) for every finding. End with one line: fleet ready for v41, or what blocks it.

## Out of scope

Any design or fix; device operations; the standby latch (not observable without serial); battery gauge accuracy (WO 3 evidence, already logged).
