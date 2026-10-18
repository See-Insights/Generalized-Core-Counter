# v43 fleet health check (read-only)

**Author:** architect, 2026-10-15 (v42 pattern), run 2026-10-18 · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2 (Claude Code, read-only), §13 (h), (i), (j)
**When:** about 24 h after v43 became the product default (about 12:29Z on 16 Oct). The scheduled run on 17 Oct 12:32Z stopped because the AWS SSO token had expired; it ran on 18 Oct about 02:05Z after Chip renewed it.
**Agent:** a Sonnet subagent, medium, read-only (S3 and the Particle API need the network). **Budget:** 0 `src/` lines. Report ≤ 100 lines.

## Plain goal

Confirm every fleet device runs v43 (`v43-SensorShape`) cleanly since the 16 Oct rollout, and find anything v43 made worse. v43 is meant to behave exactly like v42.

## Checks

The v42 health check's checks (`2026-10-15-v42-fleet-health-check.md`), with these v43 expectations: payload unchanged (same keys, order and formats), `cyc − slp = 1`, close stamped close − 1 s with the next report's daily totals 0, no breadcrumb-28 watchdog. Accepted: alert 31 (connect timeout), alert 44 after a GateFail; a timer slot inside a running Connect makes no report (recovery plan item 11); `dailyoccupancy` carried past a close is pre-existing (item 10). Dev-09 and Dev-14 are on v44 (the bench soak) and are out of scope.

## Result

See `2026-10-17-v43-fleet-health-check-report.md`: PASS Court3, PCKL1, PCKL2, Trail02; FINDING Court1 and Court2 (alert 21), PCKL3 (two reason-0 restarts in open hours), MAFC-1/2 (zero counts all of 17 Oct, cause unknown). v43 is healthy as firmware: no watchdog, no failsafe, no payload change, no heap decline, no lost report. The findings go to the architect; none is attributed to v43.
