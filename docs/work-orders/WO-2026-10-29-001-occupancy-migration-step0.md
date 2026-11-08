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
