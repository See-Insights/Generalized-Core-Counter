# v44 fleet health check (read-only)

**Author:** drafted by Claude Code on 21 Oct from §4 of `2026-10-16-v44-deviceos-6.6.0-release-and-soak.md` and Chip's instruction (21 Oct); the architect may amend. Modeled on `2026-10-15-v42-fleet-health-check.md`. **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §13 (h), (i), (j).
**When:** about 24 h after v44 became the product default (21 Oct 13:08:55Z), i.e. **22 Oct about 13:10Z or later.** **Agent:** a Sonnet subagent, medium, read-only (it needs S3 and the Particle API); report the model used.
**Budget:** 0 `src/` lines. Report ≤ 100 lines: `docs/work-orders/2026-10-22-v44-fleet-health-check-report.md` on a docs branch, committed and pushed; PR for Chip, not merged. Every number states its source query and window (§13 (j)).

## Plain goal

Confirm every fleet device runs v44 (`v44-DeviceOS660`, Device OS 6.6.0) cleanly since the default was set, and find anything v44 made worse. Court2 and PCKL1 have been on v44 since 19 Oct (their canary window is graded in `2026-10-21-v44-fleet-canaries-report.md`); Dev-09 and Dev-14 have been on v44 since 17 Oct.

## Window

Per device, from its first v44 boot (reset 70) to the time of the check. Compare each device against its own last 48 h on v43 (Court2, PCKL1: their canary days against their v43 days, as in the canary report).

## Devices

The nine US devices (Court1–3, PCKL1–3, MAFC-1/2, Trail02); Dev-09 and Dev-14 for completeness only (locked bench).

## Checks, per device (one table row each)

1. **Version and OTA time** (reset 70); list any device not yet on v44 (an OTA happens at a device's next connection).
2. **Resets since the OTA:** reason and `resetReasonData`. Expected: none, or hibernate wakes (reason 30) on Trail02 only. A reset 140 data 2 (failsafe) or any watchdog is a finding: give `lastWatchdogBreadcrumb`.
3. **`watchdogResetCount`:** against its pre-OTA value. **Breadcrumb-28 sleep watchdogs per 1,000 sleep returns** (sum of positive hook `cyc` deltas) against the device's own v43 rate; a higher rate is a finding. Any reset type not seen on v43 is a finding.
4. **Reports:** hourly reports present in open hours; missing ones counted. Only a lost, never-retried report or a stuck device counts against v44.
5. **Heap:** `fh` range and trend; a steady per-wake fall is a finding (v44's level is expected about 3.4 KB higher than v43's: not a fall).
6. **Alerts:** list any. Known and not v44's: alert 40 at the open or after a slow connect (queued fix, recovery plan item 7), alert 44 after a boot clock step. **Two or more alert 40 on one device in the window is a finding** (architect, 19 Oct); alert 41 is a finding; list every alert 15.
7. **Battery and modes:** tier, `socTrust`, `connectionMode`, `sensorMode` where the ledger shows them.
8. **Failsafe:** `failsafeStage`, `failsafeCount`.
9. **Connection timing and teardown:** `connecttime` median, p90 and the share at 8 s or more per device, v44 against v43; warm-to-cold steps; alert 15. (No serial on US devices: `slow_teardown` and `ConnSummary` are not determinable.)

## Items asked for by name (Chip, 21 Oct)

- **Court2's late 07:00 report:** on 21 Oct its 11:00Z (07:00 local) report arrived at 12:00:52Z, an hour late, and was the only fleet device late. **Does it repeat?** For each morning since (22 Oct 11:00Z and any other day in the window), give Court2's 11:00Z report delivery time (`published_at` minus the report `timestamp`) and its longest gap by `published_at`; say "repeated" or "not repeated".
- **PCKL1's connect-time median:** on its canary window the median was 2 s on v44 against 1 s on v43 (p90 7 s against 9 s; 7 of 79 reports at 8 s or more against 13 of 128). **Give its median, p90 and the share at 8 s or more for the whole v44 time to the check, and for 22 Oct alone,** against the v43 and canary figures, and say whether it is a shift or noise.

## Grade

PASS or FINDING per device, with evidence for every finding. End with one line: v44 healthy on the fleet, or what needs action. No design or fix; findings go to the recovery plan only if Chip agrees.

## Out of scope

Device operations; gauge accuracy; CONNECTED-mode items (no fleet device runs it).
