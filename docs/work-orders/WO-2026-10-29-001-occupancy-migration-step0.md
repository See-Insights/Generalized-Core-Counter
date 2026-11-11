# WO-2026-10-29-001: migrate the remaining legacy occupancy counters to GCC: Step 0 (read-only inventory and plan)

**Author:** architect, 2026-10-29 · **Status:** ready
**Branch:** `wo/2026-10-29-001-occupancy-migration-step0`, from main
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 1–5 (intake, evidence, investigation); §12.1; §13 (c), (i), (j)

## Plain goal

Chip's next expansion is the **remaining dozen occupancy sensors still running the old firmware.** Moving them to GCC (v47 now, v48 once item 10 is fixed) also takes them to Device OS 6.6.0. This Step 0 finds out exactly what each device runs and what the move would change, so the migration is one simple, safe, repeatable procedure. **It changes nothing** on any device, ledger or webhook.

## Questions (observation and inference marked separately; every number with its source, §13 (j))

1. **Inventory.** One table, one row per legacy occupancy device. Find them from the Particle API: the products other than 42131 that hold occupancy devices; ask Chip if it's ambiguous. For each, give:
   - device name and id, product id, site;
   - **app firmware version and Device OS version;**
   - the last heard time and online status;
   - battery and power source (solar or USB, if visible);
   - connection mode;
   - which webhook it publishes, and the Ubidots device it feeds.
2. **The firmware gap.** What the old occupancy firmware is (repo and version) and how its occupancy logic differs from GCC's:
   - debounce and its default;
   - when a session is credited;
   - the daily reset or rollover;
   - reports on change, or on schedule.

   The old firmware's numbers will shift when a device moves to GCC. Say which way and roughly how much.
3. **The payload and dashboard gap.** Old payload keys against GCC's occupancy payload (`ReportPublisher.cpp`, `PIRSensor.cpp:26–29`):
   - can each device's Ubidots history continue unchanged;
   - does any webhook template or Ubidots variable need changing;
   - Ubidots auto-creates a variable for any new top-level field.
4. **The product and settings path.** The two options:
   - **(A) Move devices into GCC product 42131,** taking 42131's `default-settings` and a per-device `device-settings` for each outlier (timezone, hours, `setting1`).
   - **(B) Upload GCC to their current product.**

   Recommend one. List each device's settings that differ from 42131's defaults.
5. **The Device OS 6.6.0 jump.** For each starting Device OS version:
   - what the OTA downloads (system parts, bootloader, NCP or softdevice), with the size and time if known. Use the v44 bench record of Dev-14's OTA;
   - the risk on weak cellular;
   - any intermediate version needed;
   - that it is one-way.
6. **The cutover rule.** When in the day the OTA should land so a day's counts aren't split between firmwares (the convergence rule was the first connection after opening). Say how to apply that per device, and what a mid-day cutover would cost.
7. **The order and the pilot.** Propose a **pilot of 2–3** chosen for variety (a solar site, a weak-signal site, a busy court), a soak (72 h suggested), then the rest in one or two waves. Give the checks for each wave, reusing the v46 and v47 fleet health format, and the rollback for each wave. A Device OS downgrade isn't practical, so say what "rollback" means here.
8. **Gates.** What must be done first:
   - the v47 fleet health check;
   - the v48 counting fixes: item 10 decided; items 19 and 15 in Step 0.

   Say whether the pilot can start on v47 and the waves wait for v48.
9. **Stop decision:** STOP if a device can't move without losing its Ubidots history or its counts, or needs a firmware change beyond v47/v48; otherwise PROCEED with the plan.

## Standing rules

- Read-only: no device, ledger, webhook, product or Ubidots change.
- The visitation fleet (product 12529, v50) is out of scope; it's the Visitation-Counters thread's v54 work and the GCC convergence series.

## Agents and models

- **Evidence gathering (Particle API, S3, the old firmware repo):** Claude Code (controller), read-only, with Chip's tokens.
- **Analysis and plan:** Codex `gpt-5.6-sol`, **high**, read-only, stdin closed. It's a fleet migration with a one-way OS jump; a wrong plan costs field devices.
- **Controller check:** every number and citation, against its source (§13 (j)).

## Budget

- 0 `src/` lines.
- The report: at most 150 lines, committed on the WO branch, then stop for the architect and Chip.

## Wave plan (Chip, 7 Nov 2026)
Recorded by Claude Code from Chip's message of 7 Nov; Chip performs every device move and pin. Nothing here is a Claude Code action on a device.
- **Wave 1:** **WAKEFOREST-PC01** (`e00fce68878d84d810c25d41`) and **Morrisville-Tennis-ChurchSt-1a** (`e00fce686f8cb2133cd3f8ed`). Chip moves both from product 14220 to **42131** and pins them to firmware **4901** (pin rule: never rely on a pin for a Development-marked device: flash or un-mark first; confirm the Console shows 4901 after the next connect).
- **Wave 2:** all eight **Vaughan** units (Vaughan-001..008; none retire).
- **Retiring:** OCCUPANCY-DEVSAM01, 02 and 03, plus the four already marked retired (Morrisville-Tennis-MCP-01-RETIRED, MCP-04-RETIRED, Retired-029, Retired-20).
- **Stale (site visit needed):** Morrisville-Tennis-ChurchSt-3.
- **HollySprings-Tennis-1, -2, -3:** Chip is cleaning those up (no wave assigned).
- Counts: 2 + 8 move (10 units); 7 retire (3 DEVSAM + 4 marked); 1 stale; 3 HollySprings pending. Total 21 devices in product 14220.

### First-boot checks per wave-1 device (Claude Code, read-only, after each device's first 4901 boot)
The 14220 units read offline, so the first 4901 boot may come well after the move (the device must connect, take the OTA including the Device OS 6.6.0 jump, and boot). Watchers poll the S3 archive for a `status` event reporting `v49.1` for each device ID. Then, per device, UTC: (1) **Eastern schedule** in effect (EST5EDT, open and close in local time); (2) **fw 4901**; (3) **status reset 70** at the OTA boot; (4) **Ubidots continuity**: reports arrive with no gap and the counters continue from the old product's last values. Each device is reported separately.

### Wave rules (Chip, 7 Nov 2026)
- **Move after close, before open.** Move each device to 42131 after its close and before its next open, so day one starts at 0 on the new product. **Applies to the Vaughan wave (wave 2).**
- **The migration-day count reset is expected.** The device starts a fresh GCC store on the new product, so its daily count restarts at 0 on the move day. This is not a continuity failure; the first-boot "Ubidots continuity" check judges reporting gaps, and treats the counter reset as expected.
- **WAKEFOREST-PC01 (Device OS 4.2.0):** the cloud should deliver the Device OS dependency with the 4901 OTA. Watch its next connections (about 18:00Z) and report per window. **After three daily windows without a 4901 boot, flag it for a USB flash at a site visit.** Window 1 is 7 Nov ~18:00Z.

### Wave 1 results (Claude Code, read-only; UTC)
**Morrisville-Tennis-ChurchSt-1a** (`e00fce686f8cb2133cd3f8ed`): first 4901 boot 7 Nov 12:57:06Z (archive `status/.../2026-11-07T12-57-06-780Z.json`; a 34-event Ubidots backlog flushed 12:55:53-12:57:09Z). Checked ~13:04Z.
- fw 4901 (`v49.1-DurableResets`, product 42131): PASS. Cloud agrees: firmware 4901, desired 4901.
- Reset 70 at the OTA boot (`resetReasonData` 0): PASS.
- Ubidots continuity: PARTIAL. No gap (12:48:42Z, 12:55:59Z, 13:00:02Z). No 14220-era archive exists in S3 to compare. `dailyoccupancy` went 11 to 0 across the move: the expected migration-day reset (fresh GCC store).
- Nothing abnormal: alert 0, watchdogResetCount 0, failsafeStage 0, free heap 89488.
- **Eastern schedule: PASS (checked 8 Nov, 11:45Z).** Last report before the close stamped 02:59:59Z (22:00 EST), nothing between that and the first report of the open stamped 11:00:31Z (06:00 EST). The old firmware's 02:00-06:00Z window would have shown reports in that gap. Day-count: `dailyoccupancy` 425 at the close, 0 at the open. Method: S3 archive plus the Ubidots payload `timestamp`; no variable reads.

**WAKEFOREST-PC01** (`e00fce68878d84d810c25d41`): no 4901 boot as of 13:04Z on 7 Nov. Cloud: offline, `last_heard` 5 Nov 18:02:02Z (about 43 h), last handshake 4 Nov 18:00:25Z, product 42131, last ran a 14220 build (`firmware_product_id` 14220), desired and targeted firmware 4901, system firmware **4.2.0**. No archived events in S3 for it (15 Oct to 7 Nov). It checked in around 18:00Z on 4 and 5 Nov (possible once-a-day pattern; two timestamps only). Window 1 (7 Nov ~18:00Z): pending.


**WAKEFOREST-PC01 update (8 Nov, 11:45Z):** first 4901 boot 7 Nov 17:04:55Z (archive `status/.../2026-11-07T17-04-55-358Z.json`; `v49.1-DurableResets`, `resetReason` 70, `resetReasonData` 0, alert 0, `watchdogResetCount` 0, `failsafeStage` 0, free heap 89472): PASS, single boot. Device OS dependency: proven by the v49.1 status event (a 6.6.0 app cannot run on 4.2.0, Chip's ruling); the system firmware version itself was not observed. The three-window USB-flash rule is moot (it booted 4901 in window 1). **Eastern schedule: PASS (checked 8 Nov, 11:45Z).** Last report before the close stamped 02:59:59Z, next report stamped 11:00:23Z, nothing between. **Occupancy: all zero.** `occupancy` and `dailyoccupancy` read 0 in every report seen from 6 Nov to 8 Nov (at least 56 h). The date of its last non-zero reading is **not established** from the S3 archive (no 14220-era events exist there); The architect checked the Ubidots history (8 Nov): counted normally until 16 Mar 2025 (daily max about 690), 55 on 17 Mar, one 5-minute blip on 19 May 2025, then zero in every report since (7,700+ reports since Sep 2025). The device is healthy; sensor or cabling is suspected, or a site change. **Not a firmware issue.**

**Site-visit items (architect, 8 Nov 2026):** (1) WAKEFOREST-PC01: sensor, cabling or site change (zero occupancy since 19 May 2025); also a chance to read its Device OS version and USB-flash if ever needed. (2) Morrisville-Tennis-ChurchSt-3: stale, needs a site visit. (3) Court1: battery swap (after the 14 Nov bench).

**Pin rule, clarified (Chip, 10 Nov 2026):** pin a device only when the wave's **target firmware differs from the product default**. When the target **is** the product default, no pin is needed, and the device then follows future product defaults. The earlier text in this WO ("pins them to firmware 4901", the wave 1 wording) applied while 4901 was not the default; it is superseded by this rule. **Wave 2 is not a pin deviation** (v4901 is the default; no pin was set or needed). The Development-marked-device caution stands: confirm the console shows the target after the next connect.

### Wave 2 results (Claude Code, read-only; UTC; status from Chip's console at about 11:30Z on 10 Nov, S3 from the archive)
Chip moved the eight Vaughan units to 42131 in the 10 Nov window (after close, before open). **They were not pinned:** v4901 is the product default and the console shows each targeting v4901. Device IDs (Ubidots labels): -001 `e00fce68b1f342cd08f63d0a`, -002 `e00fce68cb9f5de5f85641d3`, -003 `e00fce684c2509c83d1b3e2f`, -004 `e00fce68cd055d063747df0b`, -005 `e00fce686f116b11854587c2`, -006 `e00fce6853f037b582c06bdf`, -007 `e00fce6817db65e5bfeae171`, -008 `e00fce68fc5616f88c1b2d1f`.

**Vaughan-006: first 4901 boot 10 Nov 11:04:14Z** (archive `status/e00fce6853f037b582c06bdf/2026-11-10T11-04-14-119Z.json`; console: v4901, Device OS 6.6.0, online). Checked 11:32Z.
- fw 4901: PASS (event `fw_version` 4901, `product_id` 42131; status `version` `v49.1-DurableResets`).
- Reset 70 at the OTA boot (`resetReasonData` 0): PASS.
- Nothing abnormal: alert 0, lastAlert 0, watchdogResetCount 0, failsafeStage 0, free heap 89360, pmicAnomalyCount 0.
- Ubidots continuity (day one from 0): PASS for the first report: 11:04:12Z, payload stamp 11:00:01Z (the open, 06:00 EST), occupancy 0, dailyoccupancy 0, resets 0, connecttime 14 s, battery 62. No earlier archive exists in S3 to compare (the move reset the count, expected).
- **Pending: Eastern schedule, first night** (last report before the close stamped 02:59:59Z and nothing until about 11:00Z), check about 03:10Z on 11 Nov.

**Vaughan-001, -002, -003, -004: not yet on 4901.** Console: still Device OS 6.4.1 on the legacy firmware; each logged `spark/flash/status: failed` at about 11:07-11:09Z (likely a two-part Device OS plus app OTA interrupted by the legacy firmware's short connect). The cloud retries at each connection. **Watch their first 4901 boot over the next 24 h** (watcher polls S3 `status` every 5 minutes for each ID). **If none by 11:30Z on 11 Nov, report it for an on-site or update-settings fix.** Per-device first-boot checks run as each boots.

**Vaughan-005, -007, -008: offline for weeks** (last seen 28 Oct, 16 Sep, 4 Oct). Watched with no deadline; site-visit list below.

**Site-visit items added (Chip, 10 Nov 2026):** (4) Vaughan-005, (5) Vaughan-007, (6) Vaughan-008: offline for weeks; need a site visit. Add Vaughan-001..004 if no 4901 boot by 11:30Z on 11 Nov.

### Wave 2 update, 10 Nov 13:10Z (Claude Code, read-only; UTC; S3 archive; Chip's console for the Device OS and OTA state)
**Vaughan-004: first 4901 boot 10 Nov 11:51:58Z** (`status/e00fce68cd055d063747df0b/2026-11-10T11-51-58-193Z.json`). fw 4901 (event `fw_version` 4901, `v49.1-DurableResets`): PASS. Reset 70, data 0: PASS. alert 0, watchdogResetCount 0, failsafeStage 0, free heap 89376: nothing abnormal. Ubidots reports since: 12:00:08Z, 12:25:31Z, 12:30:27Z, 13:00:08Z, 13:06:42Z, stamps within 14 s of their publish times, `dailyoccupancy` 0 then 5 at 12:30:22Z (counting from 0 after the open): PASS. Two queued events published at boot (stamps 11:00:01Z with resets 0 and 12:00:02Z with resets 1 and alerts 40) are the old firmware's queue (see below). Eastern-schedule check on the first night: pending (about 03:10Z on 11 Nov).

**Vaughan-003: first 4901 boot 10 Nov 12:26:27Z** (`status/e00fce684c2509c83d1b3e2f/2026-11-10T12-26-27-950Z.json`). fw 4901: PASS. Reset 70, data 0: PASS. alert 0, watchdogResetCount 0, failsafeStage 0, free heap 89344: nothing abnormal. Ubidots continuity: PARTIAL, because the first events are the replayed backlog below; no report from v49.1 itself has arrived yet in the archive (the 12:34:28Z-stamped event is in that list). Eastern-schedule check on the first night: pending.

**Observation (Chip, from Vaughan-003): the old firmware's persisted publish queue is replayed by v49.1. EXPECTED migration effect** (same mechanism as ChurchSt-1a's 34-event backlog on 7 Nov). Chip saw `Ubidots-Sensor-Hook-v1` events with `dailyoccupancy` 383 and 392, `counting-webhook-v1`, and `Daily Cleanup` in the console. What S3 shows for Vaughan-003 (all published 12:26:12Z-12:26:26Z, seconds before the status event; seven `Ubidots-Sensor-Hook-v1` events; `Daily Cleanup` and `counting-webhook-v1` are **not** archived in S3, which keeps only the hook, status, hibernate_wake and serialLog events):

| payload stamp | daily | resets | alerts | note |
|---|---|---|---|---|
| 10 Nov 02:30:07Z | 383 | 0 | 0 | previous night, old firmware |
| 02:38:59Z | 392 | 0 | 0 | |
| 03:00:10Z | 392 | 0 | 0 | the old firmware's close report |
| 11:00:01Z | 0 | 0 | 0 | open |
| 12:00:02Z | 0 | 1 | 40 | resets 1, alert 40 (webhook timeout): old firmware, around the failed OTA |
| 12:01:38Z | 0 | 2 | 40 | |
| 12:34:28Z | 0 | 2 | 0 | **stamp is about 8 minutes AFTER its publish time (12:26:26Z)**: the device clock was ahead; an anomaly to check, not diagnosed |

**Open reads (not done: this session has no Ubidots access; Chip reads them in the Ubidots app, read-only, or authorises a token):**
1. Did these values land at their **own timestamps or at arrival time (12:26Z)**? The payloads carry the stamps, so own-timestamp is expected; confirm for the 02:30Z, 02:38Z, 03:00Z and 12:34Z points (the last would sit about 8 minutes in the future).
2. Did `counting-webhook-v1` (and any `Daily Cleanup`) reach any webhook in product 42131? Not answerable from S3. Chip's console integrations/logs for 42131 are the source.
3. Clean-up, if any: points whose stamps collide with a later report, or the future-stamped 12:34:28Z point; the 383/392 values describe the previous (old-firmware) day and are correct for 9 to 10 Nov.
Per the wave rule, the move-day count reset is expected; these backlog values are not a continuity failure.

**Resolution of the three open reads (architect's Ubidots read, 10 Nov, Vaughan-003; read-only, Ubidots app; Claude Code did not read Ubidots):**
1. **Own timestamps, not arrival time.** The replayed legacy values landed at their own timestamps (`created_at` 12:26Z): 02:30Z 383, 02:38Z 392, 03:00Z 392 (which **completes the 9 Nov day**: it had stopped at 368 before the replay), and 11:00Z, 12:00Z, 12:01Z 0. The 12:34:28Z point (0) was stamped about 8 minutes ahead by the legacy firmware's clock; it is harmless, in order, with no collision. (This closes the "clock ahead" question: it was the old firmware's clock, as inferred from S3.) v49.1's own reports: 13:00Z 0, 13:17Z 13, 13:39Z 23, 14:00Z 23, batched and arriving at about 14:00Z (the hook delay is recovery plan item 27 and the 13:00Z cluster noted above).
2. **`counting-webhook-v1`: likely dropped (INF).** No 42131 integration exists for it. No Ubidots data depends on it.
3. **No clean-up needed.**
**Recorded as an expected migration effect:** a migrating device replays the old firmware's persisted publish queue after its first v49.1 boot (ChurchSt-1a 7 Nov, Vaughan-001..004 10 Nov). The replayed events keep their own timestamps and can complete the previous day's count; the old-format events carry no cyc, slp or heap fields and may show the legacy alert 40 and resets 1 or 2; the old clock can stamp the last one a few minutes ahead of publish. The first-boot check treats the replay as expected, not a continuity failure.

### Wave 2 update, 10 Nov 15:45Z (Claude Code, read-only; S3 archive)
**All five active Vaughan units are now on 4901** (reset 70, data 0, `v49.1-DurableResets`, alert 0, watchdogResetCount 0, failsafeStage 0): -006 11:04:14Z, -004 11:51:58Z, -003 12:26:27Z, **-002 13:54:28Z (heap 89432)**, **-001 14:05:22Z (heap 89432)**. -001 and -002 passed the first-boot checks; each published an old-firmware queue replay at boot (alert 40 and resets 1, 2 on some replayed events), cleared on the first new-format hook. After boot every unit shows cyc minus slp = 1, alerts 0, resets 0, no second boot, heap 80 to 85 KB. **The 24 h retry watch for -001..-004 is closed: no on-site fix needed.** Vaughan-003's later events are not clock-ahead (14:00:02Z lag 27 s; 15:00:00Z lag 56 s); the 12:34:28Z stamp was the last old-format event, and the old-format last events on -001..-004 are stamped 5 to 8 minutes ahead of publish (old firmware; inference). Vaughan-005, -007, -008: still no events (site-visit list). **Pending for the five active units: the Eastern-schedule check on the first night, about 03:10Z on 11 Nov.**
**Hook delay at 13:00Z (observation, cause not proven):** the 13:00Z hooks of Vaughan-003, Vaughan-006, Trail02 and ChurchSt-1a were published together at 14:00:16 to 14:01:15Z (lag about 3615 to 3673 s); ChurchSt-1a's 11:00 and 12:00 hooks were held too (lag 10828, 7248 s). Court, PCKL, MAFC and the other units published on time.
**ChurchSt-1a (wave 1) failsafe stage 2 at 14:00:53Z:** status reason 140, data 2, failsafeStage 2, failsafeCount 1, failsafeLastAction 1 (stamp 1794319201), alert 40, appBreadcrumb 10, heap 89512; alert 40 on the 12:00 and 13:00 hooks (connecttime 660); alert cleared on the 15:00Z hook. This is the first stage-2 event on 4901 in the checks so far (earlier checks counted 0). **Architect's ruling (10 Nov): no separate item. This is the failsafe ladder working as designed:** about three hours of held delivery from the 11:00Z open, the stage-2 reset at 14:00:53Z, recovery, and **all held reports delivered** (the 11:00, 12:00 and 13:00Z hooks, published 14:00:49 to 14:00:52Z with their own stamps; alert 40 cleared on the 15:00Z hook). **The 13:00Z delay cluster** (Vaughan-003, Vaughan-006, Trail02, ChurchSt-1a: sites in North Carolina and Ontario) is **likely a carrier or cloud event** (inference; not proven). **The stage-2 ceiling trigger stays: three or more stage-2 events in a week on any device** raises an item. This is event 1 for ChurchSt-1a (and the first on 4901 in the fleet checks).
**PCKL3's 04:01Z closed-hours report** (10 Nov, reason 0, hook stamped 04:00:48Z) is the **expected boot report after an item-29 reset**, not a schedule failure.

### Wave 2 and fleet update, 11 Nov 2026 (Claude Code, read-only; S3 archive; UTC)
**Vaughan-005 is migrated.** Stale since 28 Oct, it sent about 100 replayed old-format events from 17:01Z on 10 Nov, then booted 4901 at 18:00:52Z (status reset 70, data 0, alert 0, heap 89432). A `watchdog` event at 18:00:55Z (reset watchdog, osReason 60, bc 28, stage sleep, elapsed 38 s, queue 61, connAge 112 s) and a second status at 18:00:57Z (reset 60, alert 19, watchdogResetCount 1) followed: the recovery plan item 14 signature on its first boot, with 61 replayed events queued (the link to the queue is inference). It did **not** reboot again: no later status or watchdog event, and `cyc` rises continuously to 1989 by the 11 Nov close; it reports hourly (`dailyoccupancy` 100 by 23:00Z). **Vaughan-007 and Vaughan-008 stay on the site-visit list** (no events on 10 or 11 Nov). All six reporting Vaughan units passed the first-night Eastern check (last pre-close hook stamped 02:59:59Z, delivered 03:00 to 03:05Z, nothing between close and now); the open check is pending.

**ChurchSt-1a, 10 Nov: weather test (architect's test, run from S3 hooks, 10 Morrisville units, 11:00Z to 23:00Z).** `dailyoccupancy` at 11, 13, 15, 17, 19, 21, 23Z: ChurchSt-1a 0, 0, 0, 0, 0, 0, **67** (alerts 21 and 40); ChurchSt-2 0, 6, 6, 6, 6, 6, **88**; MAFC-1 0, 0, 5, 10, 10, 10, 10; MAFC-2 0, 0, 0, 150, 175, 205, 298; Court1 0, 0, 22, 142, 188, 198, 257; Court2 0, 0, 26, 147, 200, 200, 268; Court3 0, 20, 50, 50, 50, 436, 442; PCKL1 0, 0, 28, 28, 28, 91, 91; **PCKL2 0 all day; PCKL3 0 all day** (both counted hundreds on 8 Nov: 409 and 586). Batteries (all flat or slightly rising, none draining): ChurchSt-1a 86.9 to 88.5 (min 85.5, max 89.1), ChurchSt-2 82.4 to 81.7, MAFC-1 86.8 to 88.5, MAFC-2 85.3 to 86.8, Court1 89.6 to 91.2, Court2 89.4 to 91.2, Court3 87.9 to 90.2, PCKL1 87.9 to 88.8, PCKL2 88.8 to 89.1, PCKL3 87.9 to 88.1.
- **Neither of the architect's two clean outcomes is met.** Not all units were near zero (Court1, Court2, Court3 and MAFC-2 counted 140 to 440 by midday or evening), and ChurchSt-1a was **not** the only unit at zero (PCKL2 and PCKL3 were zero all day; ChurchSt-2 had 6 until 21Z; MAFC-1 and PCKL1 were low).
- **Reading (inference; no weather data was read):** a site-level pattern: Church St, the PCKL units and MAFC-1 had little or no use while the Court units and MAFC-2 were used normally. ChurchSt-1a's sensor shows no fault signature: it counted 67 within minutes at 22:06Z and ChurchSt-2 jumped at the same hour (6 to 88). **The sensor site-visit trigger is not met; ChurchSt-1a's sensor is not added to the site-visit list** (architect to confirm).
- **Alert 21 is not a fleet-wide weather effect:** it appears only on ChurchSt-1a (hooks stamped 20:59:32, 21:00:01 and 22:00:01Z; cleared at 22:06:58Z), while every other unit's battery was equally flat and raised no alert. Alert 21 is PMIC charge timeout or stuck per `docs/reference/alert-codes.md` (fast charge 6 h or more with no material gain); ChurchSt-1a's battery sat at 88.5 to 89.1 percent. Watch: a recurrence would point at the charge path (USB charge-latch WO area), not at weather.

**PCKL3's reason-0 boot rate is unchanged (item 29).** Its 04:01:11Z 10 Nov boot and the closed-hours report are the known pattern.

