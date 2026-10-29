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
