# v40 fleet health check: report (pass 2)

**Run:** Claude Code (Opus), read-only, 2026-10-11 about 13:20Z; code cited at main `ee76747`. **Model deviation:** the WO names a headless Sonnet session, whose read-only tools can't reach S3 or the Particle API, so Claude Code ran it directly (allowed by the scheduled task).
- **Sources:** S3 raw events for 10–11 Oct (`status`, `watchdog`, `Ubidots-Sensor-Hook-v1`, `hibernate_wake`), and Particle ledgers per instance (`device-status`, `device-data`, `device-settings`, `default-settings` 42131). The US devices have no serial logs.
- **Window:** each device's OTA to 2026-10-11 10:03Z (the last OTA was Court3 at 10:02:56Z on 10 Oct).

## Plain goal

Confirm every fleet device runs v40 cleanly after the 10 Oct rollout, and catch anything v40 changed for the worse before v41.

## Per device

Columns: resets from `device-status` boot epoch and reason plus S3 `status` events; watchdog count pre-OTA → now; missing = hourly reports missing in open hours (06–22 local, Trail02 to 23); fh range across reports; failsafe stage/count from the OTA-boot status (the last available). All devices run OCCUPANCY (`sensorMode` 1) in KEEP_ALIVE (`connectionMode` 3) via `default-settings`, which has `sensor.type` 1.

| Device | OTA (reset 70) | Resets after OTA | Watchdog | Reports / missing | fh (B) | Alerts | Tier / socTrust / flag | Failsafe | Grade |
|---|---|---|---|---|---|---|---|---|---|
| Court1 | 10 Oct 10:01:32Z | none | 20 → 20 | 18 / 0 | 77312–81768 | 40 at the OTA boot only | CONSERVING / Suspect / **false** | 0/0 | FINDING (pre-existing) |
| Court2 | 10:02:16Z | none | 2 → 2 | 24 / 0 | 76824–81760 | 40 at the OTA boot only | HEALTHY / Trusted / false | 0/0 | PASS |
| Court3 | 10:02:56Z | none | 77 → 77 | 34 / 0 | 77160–81712 | 40 at the OTA boot only | CONSERVING / Suspect / true | 0/0 | PASS |
| PCKL1 | 10:02:13Z | none | 27 → 27 | 23 / 0 | 77256–81752 | 40 at the OTA boot only | HEALTHY / Trusted / false | 0/0 | PASS |
| PCKL2 | 10:01:59Z | none | 7 → 7 | 33 / 0 | 76840–81784 | 40 at the OTA boot only | HEALTHY / Trusted / false | 0/0 | PASS |
| PCKL3 | 10:01:40Z | none in window | 6 → 6 | 39 / 0 | 76408–81832 | 40 at the OTA boot only | HEALTHY / Trusted / false | 0/0 | PASS |
| MAFC-1 | 10:02:46Z | none | 81 → 81 | 17 / 0 | 77096–81616 | 40 at the OTA boot only | HEALTHY / Trusted / false | 0/0 | PASS |
| MAFC-2 | 10:02:20Z | none | 35 → 35 | 20 / 0 | 76656–81728 | 40 at the OTA boot only | HEALTHY / Trusted / false | 0/0 | PASS |
| Trail02 | 10:01:13Z | reset 30 at 10:00:35Z on 11 Oct (hibernate wake) | 37 → 37 | 19 / 0 | 74040–77424 | none | HEALTHY / Untrusted / false | 0/0 | PASS |
| Dev-09 | soak 9 Oct | reset 30 (hibernate) | 46 | listed only | — | — | HEALTHY / Trusted | — | graded 10 Oct |
| Dev-14 | soak 9 Oct | reset 30 (hibernate) | 126 | listed only | — | — | HEALTHY / Trusted | — | graded 10 Oct |

**Checks that held across the fleet**
- **No resets after the OTA:** none with reason 140, no `watchdog` events, and no alert 41 anywhere. Every non-hibernating device's `device-status` still shows `startup.reason: update` with the OTA boot epoch.
- **Every report has cyc − slp = 1.**
- **Closes stamped correctly:** at 01:59:59Z, and 02:59:59Z for Trail02.
- **Heap is flat:** about 77.3 KB while awake, 81.7 KB right after a boot or wake, and no steady fall.

**Notes (not findings)**
- **PCKL2 and MAFC-1:** their 10Z open report came before the OTA (v38), so the hour is covered.
- **PCKL1 at 16Z on 10 Oct:** covered by an occupancy-end report stamped 15:59:49 and delivered at 16:00:03. The next hourly report is at 17:00. No count was lost.
- **Court1 and Court3 deliver late:** for example, Court3's reports stamped 18:58, 19:00 and 19:16 were delivered at 20:00:19Z. That is CONSERVING's 7200 s interval (`adjustmentReason: low-battery`). They were delivered, not lost.
- **PCKL3 rebooted with reason 0** at 12:02:26Z on 11 Oct, after the window. Its watchdog count stayed at 6. That matches its known v38 pattern of reason-0 restarts (seven such status events on 9–10 Oct before the OTA).
- **Trail02:** SOC 20.9% (Untrusted) at 4.12 V, charging fast, tier HEALTHY. The gauge is still converging (WO 3 evidence, out of scope).

## Finding: Court1 is CONSERVING but not downgraded (pre-existing, not a v40 regression)

- **What `device-status` shows (11 Oct 10:00Z):** Court1 and Court3 are identical: tier CONSERVING, vcell 3.97 V (resting estimate about 64%), gauge SOC 85.5% and 87.2% marked Suspect, `chargeState` NOT. The tier follows voltage, as designed.
- **The difference:** Court3 has `lowBatteryMode: true`, so it's downgraded to INTERMITTENT. Court1 has `false`, so it's still in KEEP_ALIVE. Court1 has been on the 7200 s interval since about 23:00Z on 10 Oct.
- **Why (code fact):** the flag is set only by `BatteryAuthority::commit()` (`BatteryAuthorityCommand.cpp:80-83`).
  - In Report it's committed only when the device is not cloud-connected (`State_Report.cpp:168`, `:184`).
  - In Sleep it's committed only when the flag is already set (recovery).
  - So a KEEP_ALIVE device that stays connected may never commit a downgrade.
  - **Inference:** Court3 hit a disconnected report and Court1 hasn't. There's no serial to confirm this.
- **Not caused by v40:** the `!Particle.connected()` condition on that commit is unchanged from v38 (`41ad238`) to v40 (`2d7b770`).

## Alert 40 at the open (the architect's addition)

**Answer: yes, on every non-hibernating device, at every ordinary open, briefly.** Here's the evidence and its limits.
- **The code path is deterministic.**
  - The last webhook response comes around the close (about 02:00Z).
  - At the first open report (10:00Z) that's about 8 h, more than the 3 h `WEBHOOK_LONGTERM_ALERT40_SEC` (`ConnectivityPolicy.h:223`).
  - `Clock::openness()` is Open, so `RecoveryState::raiseAlert(40)` runs (`State_Report.cpp:108-121`).
  - The only suppression is for an overnight hibernate wake at the open hour (`Generalized-Core-Counter.cpp:1478-1485`). Its comment calls 8+ hours without a webhook "expected, not an error". That is why Trail02 shows 0 on both mornings.
- **Observed when it was caught.** It has been seen whenever a status was published between the raise and the webhook response:
  - all eight non-hibernating devices at the 10 Oct OTA boot, each with `lastAlert` equal to its 06:00 open-report timestamp;
  - Court2's v38 OTA boot on 7 Oct (alert 40).

  The raise doesn't depend on the OTA. The OTA reboot only captured it.
- **Can't be seen on an ordinary morning.**
  - The report payload records the alert code before the raise (`State_Report.cpp:69`).
  - The first webhook response clears it (`Generalized-Core-Counter.cpp:2412-2416`).
  - `device-status` has no alert field, and `status` events are published only at boot.

  So 11 Oct shows nothing: every report's `alerts` was 0. This is the limit of the evidence.
- **Cleared:** no report after any OTA boot carried 40, and no device reset at 10:00Z on 11 Oct.
- **Side effects today:** the alert sets `lastAlertTime = now` (`MyPersistentData.cpp:913`). That 3 h cooldown is the only thing stopping the 6 h ERROR_STATE escalation (`State_Report.cpp:136-147`, `ConnectivityPolicy.h:225`, `:227`) on the same pass.
- **Logged** in the recovery plan's queued WOs as a small fix.

## Grade

**8 PASS** (Court2, Court3, PCKL1–3, MAFC-1, MAFC-2, Trail02); **1 FINDING, pre-existing** (Court1 is CONSERVING in KEEP_ALIVE without a downgrade); **no v40 regression.**

**Fleet ready for v41.** Nothing blocks it. Two items are for the architect: Court1's downgrade-commit gap (WO 3 or 3a scope) and the alert-40 open fix, now queued.
