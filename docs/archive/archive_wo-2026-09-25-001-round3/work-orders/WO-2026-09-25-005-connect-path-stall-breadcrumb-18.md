# WO-2026-09-25-005: connect-path stall at breadcrumb 18

**Status:** Stub. Stage 4 (investigation) pending.

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`.

## Problem

Devices are reset by the watchdog while trying to connect. The watchdog event carries `bc=18` (`BREADCRUMB_PUBLISH_QUEUE_EXIT`, set in the main loop after `PublishQueuePosix::instance().loop()` returns), `state=4` (`CONNECTING_STATE`), and `stage=connectivity` (set in `handleConnectingState()`, `src/state/State_Connect.cpp:213`). The connect path writes no further breadcrumb until the cloud connects, so the main loop stalls for about 60 s inside the connect state (`AWAKE_WATCHDOG_TIMEOUT_MS = 60000`) and the watchdog resets the device.

## Evidence

Read-only query of fleet `watchdog` events in S3, 2026-09-05 through 2026-09-25 (126 watchdog events in total): **31 have this exact signature, on 6 devices**, on both production firmware 21 and dev firmware 24.

| Device | Firmware | Occurrences |
|---|---|---:|
| Boron-Dev-09 (`…963935`) | 24 | 8 |
| `…984295` | 21 | 6 |
| Boron-Dev-14 (`…3ac4fb`) | 24 | 5 |
| `…0a014e` | 21 | 4 |
| `…45e380` | 21 | 4 |
| `…ccc884` | 21 | 4 |

First occurrence in the window: 2026-09-05 02:00Z (Dev-14). Most recent: Dev-14, 2026-09-25 08:18Z, on the WO-2026-09-24-001 decisions 1–7 build (the first build to run the daily cleanup at close; not the decision-8 commit `599038e`): an 08:15:13Z occupancy report completed (its webhook was delivered after the reboot), the device moved to connecting, and the watchdog fired at uptime 896.4 s (`queue=2`, `connAge=485088`). The code changed by WO-2026-09-24-001 is not on this path, and the signature predates it.

## Related

- WO-2026-09-03-004 (MAFC-1 sleep-path watchdog stalls; the `bc=28 stage=sleep` class).
- WO-2026-09-15-002 (cellular-acquire registration stall, no escalation).

## Approval record

- [ ] Investigation (Stage 4)
- [ ] Chip approval (Stage 5)
- [ ] Implementation (Stage 6)
- [ ] Codex verification (Stage 7)
- [ ] Chip final gate / commit (Stage 8)
