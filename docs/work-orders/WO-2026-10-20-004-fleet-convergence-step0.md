# WO-2026-10-20-004: one firmware for occupancy, trail and car counters (Step 0 only)

**Author:** architect, 2026-10-20 · **Status:** Step 0 (read-only); no implementation until the architect accepts a plan
**Branch:** `wo/2026-10-20-004-fleet-convergence-step0`, from main
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 2–4; §12.1 (history first); §13 (c), (j)
**Repos:** this one (GCC) and **See-Insights Visitation-Counters (read-only)**, which holds the v50/v54 source. Attach it read-only before starting.

## Plain goal

Chip's goal (20 Oct): **the occupancy and visitation counters run identical firmware, and only the ledger configuration differs.** About 400 legacy devices run v50 (going to v54, which is v50 on Device OS 6.6.0, as the interim). **All of them use the same Boron, carrier board and battery as GCC.** There are two visitation types:
- **trail counters:** the same PIR sensor as occupancy;
- **car counters:** a pressure sensor ("no driver needed, only difference is the LEDs").

Find what stands between today's GCC and running both, and propose the shortest safe path.

## Step 0 questions (report ≤ 150 lines on the WO branch, committed and pushed)

1. **COUNTING semantics, v50 against GCC.** For trail counters, compare what v50 counts and when with GCC's COUNTING mode:
   - the debounce, and what counts as one count;
   - the hourly and daily totals and when they reset;
   - the report cadence and the "final report before night sleep" rule;
   - closed-hours behavior;
   - LED behavior;
   - low-battery behavior.

   Every difference gets file:line on both sides, and a verdict: same, harmless, or must match.
2. **Car counters.** How v50 handles the pressure input: pin and edge, debounce, any pulse logic (axle pairing, minimum gap), and the LEDs. Can GCC serve it as a ledger choice with no new driver, by (a) a second `sensor.type` sharing the PIR's interrupt handling or (b) a setting (for example `sensor.setting1–4` or a LED mode) on the PIR type? Recommend one, with the smallest firmware change. Note the deferred 2b-7/2b-8 items (the report request, and delivering settings 1–4 and `samplingMode` to the sensor) where they become necessary.
3. **Payload.** v50's webhook payload field for field (names, order, formats, event name) against GCC's COUNTING layout (the golden fixture). What do Ubidots and the fleet tools consume? Can GCC's counting payload replace v50's without back-end changes, and if not, what's the smallest change on either side?
4. **Settings migration.** v50's on-device settings (opening hours, debounce, low-power mode, reporting interval and others) mapped to GCC's ledger keys (`default-settings` and `device-settings`). How can each device's current values be read from the fleet (Particle variables, status events, a function call) to pre-fill its `device-settings` before the switch? What happens if a device boots GCC before that?
5. **Product and rollout.** Which Particle product or products the legacy fleet is in; whether GCC goes into that product or the devices move; the OTA size from v54 (same Device OS, so app only); and **a pilot plan:** a few trail counters and a few car counters, with what to check and how long.
6. **v50 behaviors GCC lacks** that visitation customers rely on (anything else, from the source), and **GCC behaviors that would surprise them** (connection mode, alerts, extra events, ledger traffic, data use).
7. **Sequence.** WOs from here to "one firmware", each ≤ +20 net `src/` lines, behavior-neutral for the existing occupancy devices, plus the pilot. Mark what can run alongside the sleep split (S-1 to S-6) and what has to wait for it.
8. **Stop and report** if convergence needs a persisted field, a hardware-specific build, or a back-end change you can't size.

## Agents and models

- **Step 0:** Codex `gpt-5.6-sol`, **high**, read-only, across both repos.
- **Controller:** Claude Code attaches Visitation-Counters read-only, and checks every file:line in the report against both trees before it reaches the architect (§13 (j)). No builds, no device actions.

## Budget

Step 0: 0 `src/` lines in either repo.
