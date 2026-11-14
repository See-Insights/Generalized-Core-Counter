# Fleet health check: the procedure, for handover to fleet-ops

**Written 12 Nov 2026 by Claude Code (the Generalized-Core-Counter workflow controller), at Chip's request, so the fleet-ops agent can take health checks over.** It records what the checks have been on 7, 11 and 12 Nov (read-only; one Sonnet subagent per run, verified by me before reporting), and what each check is looking for. Facts below come from those runs and from `docs/reference/alert-codes.md` at `f00aa04`. Judgment calls are marked **(judgment)**. All times UTC.

## 0. What a health check is
A read-only look, from the cloud side only, at every active unit of Particle product 42131 over a window (usually the last 2-3 UTC days, or since the unit's own firmware change). It answers four questions:
1. Is every unit on the firmware we expect, and did any unit restart, reset or alarm in the window?
2. Did every report that should have been made arrive, and arrive in order?
3. Is anything trending the wrong way (heap, resets, reason-0 restarts, alerts)?
4. Do the **watch list** items (section 6) still hold, and has an **escalation trigger** fired?

It makes **no claim about hardware** and does not diagnose causes. It reports FINDINGs and NOTEs with evidence, and says what was not measured.

## 1. Sources (read-only)
- **S3 raw archive** `s3://infrastack-rawparticlelogsbucket4adba400-7yquehl7iulz/particle-events/<UTC day>/<event>/<deviceId>/<ISO time>.json`, AWS profile `particle-admin` (SSO; Chip renews it; never run `aws sso login` yourself).
  - Events used: `Ubidots-Sensor-Hook-v1` (the report), `status` (boot and config snapshot), `watchdog`, `hibernate_wake`, `serialLog` (**Dev-09 and Dev-14 only**).
  - The archive holds the **Particle event**, with `published_at`. The `data` field is HTML-escaped JSON (`&quot;`); decode before parsing. The report's own device timestamp is `timestamp` (epoch ms) inside `data`.
  - `aws s3 ls` times are in the **local zone of the machine**, not UTC. Use the key name (`2026-11-10T17-01-21-551Z`) as the UTC publish time.
  - It does **not** contain the legacy alert hook (`Ubidots_Alert_Hook`), the counting webhook or Slack posts. Alerts are read from the `alerts` field of the hooks and the `alert` field of `status` events.
- **Particle API (GET only):** `/v1/products/42131/devices` for the device list, `online`, `last_heard`, `firmware_version`. Token: `~/.particle/particle.config.json` (read in code, never print). The legacy occupancy fleet is most likely product 14220 (an inference; not part of this check).
- **Not available to agents:** the Ubidots console and the Particle console (Chip reads those). Device-settings ledgers were not read in the runs below, so each unit's open hours were **inferred from its report times**; if the ledger read is allowed, use it.

## 2. The fleet and per-device context
Product 42131, "GCC" firmware. On 12 Nov: 27 devices listed, **21 active**, all on fw 4901 (`v49.1-DurableResets`). Open 06:00 / close 22:00 local, except Trail02 (close 23:00). The US units are Eastern: close report stamped ~02:59:59Z (Trail02 03:59:59Z), open ~11:00Z in winter.

| Group | Units | Notes |
|---|---|---|
| Singapore bench | Dev-09, Dev-14 | BRN404X on Singtel. **Stale cloud with a healthy serial log is expected.** CloudRecover stage-2 episodes and 15-20 s wake attach are normal there. They are not representative of the US fleet. Their daily 22:00Z hibernate wake (reset 30/0) is expected. |
| US test and pilot | Court1, Court2, Court3, PCKL1, PCKL2, PCKL3, MAFC-1, MAFC-2, Trail02, ChurchSt-1a, ChurchSt-2, LAB-01 (OCCUPANCY-LAB-01), WAKEFOREST-PC01 | Trail02: hibernate enabled, **2-hourly reports by design**, wake gate shows `fail` / wakeReason UNKNOWN every morning but wakes on time (known, not a finding). |
| Vaughan | Vaughan-001 to -006 (active); -007, -008 silent | 001-006 took the 4901 OTA on 10 Nov 11:04-18:00Z from legacy firmware. |
| Silent / inactive | Vaughan-007 (last heard 16 Sep), Vaughan-008 (4 Oct), ChurchSt-3 (28 May) | Listed every time; on the site-visit list. **DEVSAM01-03 are inactive (kept for dev or demo): list as inactive, never as findings.** |

## 3. The checks, in order
For each active unit, over the window:

**3.1 Firmware.** `fw_version`/`version` in hooks, `status`, and the API list. Flag any unit off the majority version. (Expect all 4901 until the next release; Chip announces releases.)

**3.2 Restarts and resets.** From `status` events: `resetReason` and `resetReasonData`. What each means (from the runs and recovery plan):
- **70** = OTA restart; **30 data 0** = hibernate wake (Dev-09/14/Trail02 daily). Both expected; list every 70 with device and time (needed for the escalation trigger).
- **140** = `RESET_REASON_USER`: the firmware called `System.reset(data)`. GCC puts its own cause in `resetReasonData` (table below): data **2 = stage-2 connectivity failsafe**, data **4 = nightly heap guard**. Any 140 is a FINDING (read `resetReasonData` first; the data codes mean different things). 140 with data 0 came from outside GCC's reset sites (AB1805 library fallback, a cloud reset, or Device OS).
- **60** = watchdog (check the `watchdog` event: `bc` breadcrumb, `stage`, `queue`, `elapsed`). **40** = `POWER_DOWN`, **not** brownout: on Gen 3 (Boron) Device OS cannot tell power-down from brownout, so a real brownout also arrives as 40; `POWER_BROWNOUT` (**50**) is not reported on this hardware. **20** = pin reset; seen on Dev-14 on v48, report it. **140 data 8** = binding-deleted: none has occurred since the fix; any is a FINDING.
- **0** (none/unknown) startups: known pattern on PCKL3 and Court1 (recovery plan item 29, **not an RC1 blocker**, cause not diagnosed). Report each as a NOTE with the count; flag if it appears on another unit.
**Reset reason codes** (`resetReason`; source: [Particle reset reason reference](https://docs.particle.io/reference/device-os/api/system-calls/reset-reason/), checked 14 Nov 2026; names are `RESET_REASON_*`):

| Code | Name | Meaning |
|---|---|---|
| 0 | NONE | No information available |
| 10 | UNKNOWN | Cause not specified |
| 20 | PIN_RESET | Reset button or reset pin |
| 30 | POWER_MANAGEMENT | Low-power management reset (our hibernate wake is 30 / data 0) |
| 40 | POWER_DOWN | Power-down reset. On Gen 3 also covers brownout |
| 50 | POWER_BROWNOUT | Brownout reset. Not reported by Gen 3 (Boron); it arrives as 40 |
| 60 | WATCHDOG | Device OS's own watchdog reset (see the external-watchdog note below) |
| 70 | UPDATE | Firmware update completed (OTA restart) |
| 80 | UPDATE_ERROR | Deprecated |
| 90 | UPDATE_TIMEOUT | Firmware update timed out |
| 100 | FACTORY_RESET | Factory reset requested |
| 110 | SAFE_MODE | Safe mode requested |
| 120 | DFU_MODE | DFU mode requested |
| 130 | PANIC | System panic; `resetReasonData` is the panic code (1 HardFault, 2 NMI, 3 MemManage, 4 BusFault, 5 UsageFault, 6 InvalidLength, 7 Exit, 8 OutOfHeap, 9 SPIOverRun, 10 AssertionFailure, 11 InvalidCase, 12 PureVirtualCall, 13 StackOverflow, 14 HeapError, 15 SecureFault) |
| 140 | USER | `System.reset(data)` by the application; data is ours (below) |

Particle: on devices with an external hardware watchdog (Tracker One, Monitor One, Tracker SoM, Muon), watchdog resets show as pin reset or power management. Source: the same [Particle reset reason reference](https://docs.particle.io/reference/device-os/api/system-calls/reset-reason/). 60 is Device OS's own watchdog.

Our boards: the AB1805 (watchdog on D5) resets the Boron through its reset pin, so an AB1805 watchdog reset arrives as PIN_RESET (20). See `docs/reference/alert-codes.md` row 19 and GCC's wake classification. So 20 is not "button only".

**GCC `resetReasonData` for reason 140** (`src/ResetCause.h`): 1 app watchdog handler; 2 connectivity failsafe stage 2; 3 ThrashGuard tier 3; 4 nightly heap guard before overnight sleep; 5 all sleep attempts failed; 6 ERROR_STATE soft reset; 7 out of memory; 8 stored Device OS RTC binding deleted at setup(); 0 = not from a GCC site.

- `watchdogResetCount` (in `status`): report the delta over the window per unit; "no status event" means unverified, not zero. `failsafeCount`/`failsafeStage` are in the same event.
- **Item 17 count:** stage-2 resets (140 data 2) **per device over the last 7 days; 3 or more triggers the stage-2 ceiling Step 0.** Today: ChurchSt-1a 1 (10 Nov 14:00:53Z); LAB-01 had 1 on 1 Nov.

**3.3 Alerts.** Read `alerts` in every report and `alert` in every `status`. Codes (full table: `docs/reference/alert-codes.md`): 19 watchdog reset; 20 PMIC thermal; 21 charge timeout/stuck; 22 USB charge latch; 23 battery fault; 31 failed to connect; 40 repeated webhook failures; 43 publish queue not drained before sleep; 15/16/17/18 etc. as in the table. **Do not decode a code that is not in that table; say "not decoded".** Report every 17-23 and 40 with device and time. Notes:
- One alert at a time is held (the most severe); it shows in the next report, and report-once codes then clear. A code seen once and gone is normal.
- **Alert 40 at open** (every unit raised it at its 06:00 open after the 6 Nov OTA) was the known "alert-40-at-open" case (WO 6); it did **not** recur on 9-11 Nov. Flag it if it returns on several units at open.
- Alert 21 on ChurchSt-1a (10 Nov 20:59-22:00Z) is the stale-gauge/full-pack side (recovery plan item 23), not a latch (item 22); the 4.05 V exclusion ships with WO-2026-11-11-001 (held until RC1).

**3.4 Reports (completeness and order).** Per unit, per open day: the count of `Ubidots-Sensor-Hook-v1` events and the **maximum gap between report timestamps inside open hours** (expect 60 min; 120 for Trail02; the nightly gap is the closed period). Also check:
- the **close report** (stamp close minus 1 s) is present each night;
- **`cyc - slp == 1` in every hook** (the sleep-accounting invariant). Legacy-format hooks (Vaughan before the OTA: no `fh`, `cyc`, `slp`) are excluded and counted separately;
- compare the device `timestamp` gap with the **publish-time gap**: a late delivery shows a long publish gap but normal device stamps.
- **Late is not lost.** A report is lost only if its `cyc` never appears. Worked example (12 Nov, LAB-01): the 01:00 report looked missing; it arrived in the 02:00 batch (stamps 01:00:02Z and 02:00:02Z, `cyc` 107 then 108). Check the device stamps and `cyc` continuity **before** calling a FINDING. Recheck after the next report and the close if unresolved.

**3.5 Heap.** `fh` (`freeHeap` in hooks) per unit: daily median and minimum. Expect flat medians (~80-85 KB) and minimums ~74-76 KB around cloud connect. The nightly heap guard fires at 45 KB (a 140 data 4); nothing has come near it. Flag a declining median.

**3.6 Silence.** `last_heard` and whether the last hook falls inside the unit's own cadence (open hours, 60 or 120 min). A US unit offline overnight is normal. Count units silent more than 24 h (excluding the inactive list).

**3.7 Bench-only (Dev-09, Dev-14).** From `serialLog`: CloudRecover episodes (stage reached; stage 2 is the bound on the bench link), the 22:00Z wake attach time, `ClockResync` `correctionSec` (expect -2 to 2 s). Serial capture has gaps, so counts are lower bounds.

**3.8 Late-delivery notes.** PCKL1's close report sometimes publishes at the next open (a lag of hours); Trail02 delivers in batches. Record as NOTEs when seen; they are known.

## 4. Verdicts and what to report
Per unit: **CLEAN** (nothing), **NOTE** (known pattern or one-off with no loss: PCKL3 reason 0, Trail02 gate, bench-link episodes, single alert 31, late batch), **FINDING** (an unexplained loss, a 140, a watchdog or 60, a stuck code, a unit off-firmware, a missing report with no `cyc` continuity). Report: a summary table first (device, fw, alerts, resets, reports/max gap, verdict), then FINDINGs with the S3 key as evidence, then NOTEs, then "Not measured / not verified" (causes, alert meanings outside the table, watchdogResetCount where there was no status event, hours inferred rather than read from the ledger). **Never state a cause as fact; mark inference.** The runs kept the report to about 90 lines; the 7 Nov version added firmware, heap, time-zone/pilot and `correctionSec` sections for a release check.

## 5. Window rules
- For a release or OTA check, anchor each unit's window on **its own first boot on the new version** (units take the OTA when they next connect: US units at the next open, about 3.5 h after the default time on 6 Nov). Say "PARTIAL" for windows under 24 h.
- Otherwise use whole UTC days. The 12 Nov run repeated 10 Nov events from the 11 Nov run; say so, and state which findings are new.
- Soak grading for bench release soaks (Dev-09/14, 24 h): (a) no generated report or count lost; (b) every cycle sleeps (`cyc - slp = 1`); (c) closes correct (stamped close minus 1 s, 0 next morning). A late delivery is not a failure.

## 6. The watch list and escalation triggers (12 Nov)
- **ChurchSt-1a:** stage-2 failsafe 140 data 2 on 10 Nov 14:00:53Z = **1 of 3**; cause is the 10 Nov network episode (architect's ruling). Report any new 140 and any alert 21/40.
- **Vaughan-005:** watchdog 10 Nov 18:00:55Z, reason 60, **bc 28, first sleep after OTA, 61-event backlog after a 44 h gap; alert 43 followed**. Known breadcrumb-28 class. Colleague's report: the unit was offline for days on legacy firmware, briefly online, took the OTA, and has reported reliably since; the 61-event queue was most likely the legacy queue replaying. Stable since. Report any repeat.
- **Escalate** (tell Chip and the architect) if **either**:
  1. a **second device has a watchdog within about 60 s of an OTA boot** (reason 70); or
  2. **any watchdog event shows bc 28 with queue over about 50**.
  That would make a large backlog at sleep the pattern, and the v50 bench should look at it. **Test both explicitly every run**: list every reason-70 boot and every watchdog event with its `bc` and `queue`.
- PCKL3 reason-0 restarts (9 Nov 00:02, 10 Nov 04:01, 12 Nov 00:03; the last inside open hours): item 29, not an RC1 blocker; keep counting.
- Other open items to carry: item 14 and item 29 watches, wave-2 devices (Vaughan-007/-008) on return, the six Vaughan units' open-period check.

## 6a. Legacy replays
A device coming from legacy firmware can replay an old queue under the event name `Ubidots-Sensor-Hook-v1` (the legacy occupancy firmware used the same name, `Occupancy-Sensors.cpp:776`). Legacy-format hooks in the archive (no `fh`, `cyc`, `slp`; `battery` as a small integer) before a unit's first 4901 boot are legacy, not GCC. To tell legacy from GCC use the **Ubidots value timestamp or the report's own fields, not the Slack post time**.

## 7. Known limits of the method
- Cloud-side only: no serial except Dev-09/14, no battery or radio readings beyond what the reports carry.
- Open hours inferred from report times unless the ledger is read.
- `watchdogResetCount` is only seen in `status` events, which are published on boot.
- The archive can lag a few minutes; a report in flight at the window end is not a loss.
- An agent doing the read must not run git, flash, rotate tokens or log in to AWS or Particle; it may write only to its own scratch directory.

## 8. Suggested first fleet-ops step
Run the 12 Nov checks once against the same 10-12 Nov window and compare with `hc-1112/report.md` (the Claude Code scratchpad copy of the 12 Nov report; ask Chip for it) to confirm the procedure reproduces: 21 active units on 4901, one watchdog event (Vaughan-005, 10 Nov 18:00:55Z, bc 28, queue 61), six reason-70 boots on 10 Nov (Vaughan-006 11:04:14, -004 11:51:58, -003 12:26:27, -002 13:54:28, -001 14:05:22, -005 18:00:52), ChurchSt-1a 140/2 at 14:00:53Z, PCKL3 reason-0 restarts, and no `cyc - slp` violation in about 1,800 hooks.
