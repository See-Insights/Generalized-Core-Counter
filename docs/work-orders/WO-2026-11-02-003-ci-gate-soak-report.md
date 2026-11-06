# WO-2026-11-02-003: CI-built v48 24 h gate on Dev-14 — grading (read-only S3, 5 Nov 2026)

**Window:** 2026-11-03T23:49:11Z (Dev-14's first status after the USB flash of `dev09-ci-built-v48-0c8b5a2.bin`, built by CI from `main` `0c8b5a2`, SHA-256 `83218db3…c3667b3`) to 2026-11-04T23:49Z. **Comparison:** Dev-09, the console-uploaded v48, same bench (Singapore, Singtel BRN404X), same exerciser, same window. Graded by a Sonnet subagent from S3 (`status`, `Ubidots-Sensor-Hook-v1`, `hibernate_wake`, `serialLog`; no `watchdog` events on either device), checked by the controller. Both report `v48-OccupancyEndsAtClose`, `fw_version` 48. The binary identity of Dev-14 rests on the file hash and the flash step (the status payload carries no build hash).

## Result: Dev-14 CLEAN, the same as Dev-09 on every pass/fail check
| Check | Dev-14 | Dev-09 |
|---|---|---|
| 1 Restarts | 1 planned (22:00:47Z, reason 30, hibernate wake ok, err 1 s); none unplanned | 1 planned (22:00:38Z, reason 30, err 1 s) |
| `watchdogResetCount` / `failsafeCount` / alert / `pmicAnomalyCount` | 127→127 / 0 / 0 / 0 | 46→46 / 0 / 0 / 0 |
| 2 (a) Reports lost | none; 113 events (111 unique, 2 re-deliveries); no gap over 1,250 s | none; 113 events, no duplicates |
| 3 (b) cyc − slp = 1 | 113/113 | 113/113 |
| 4 (c) Close | data 13:59:59Z, `dailyoccupancy` 419; next morning 22:00:29Z = 0 | data 13:59:59Z, 459; next morning 22:00:18Z = 0 |
| 5 Sessions (partial serial sample) | closes 569-586 s; `total=` matches `dailyoccupancy` | 570-586 s |
| 6 Publish-lag trend | none (first/middle/last third medians 12/16/12 s) | none (17/24/11 s) |
| 7 Heap | status 89,568→89,568; hook sawtooth 75-89 k, no decline | 89,584→89,584; sawtooth, no decline |

## Observations (not failures)
- Cadence is every ~10 min through open hours (a :00 report plus 10-minute reports), not hourly, on both devices.
- Dev-14 was noisier on connectivity, the known profile of this bench (Dev-14 BRN404X/Singtel): 15 cloud-recover stage-2 network resets against 3 on Dev-09 (neither went past stage 2); 2 webhook timeouts (alert 40 at 03:19Z and 03:59Z) and alerts 31 (cyc 63) and 44 in hooks; 2 duplicate deliveries (cyc 95, cyc 108); a late-delivery cluster 01:28-04:30Z (17 deliveries over 100 s; three reports with distinct data timestamps but the same `cyc` 63 arrived together at 02:09:54-59Z, connect times 190/660/596 s). Nothing generated was left undelivered. This is inference, not proven: both devices share one bench, so the difference is not shown to be radio-side only.
- The serial `Occ:` lines are sparse (25 in the window on Dev-14, 41 on Dev-09), so a full session reconciliation was not possible; the `total=` seconds on close lines match the hook `dailyoccupancy`.
- The same-`cyc`-63 triple is unexplained; it did not lose a report.

## Disposition
The release rule from WO-2026-11-02-003 is met: CI-built binary, parity accepted (product version 48 in both, +168 bytes of compiler inlining), 24 h clean against Dev-09. **CI is the only release build; the Particle cloud-compile gate (#156) is retired** (`AI_DEVELOPMENT_WORKFLOW.md`, "Release gate: the CI build").
