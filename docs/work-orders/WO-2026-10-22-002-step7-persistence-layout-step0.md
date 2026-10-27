# WO-2026-10-22-002: Step 7 Step 0: the persistence layout (read-only investigation)

**Author:** architect, 2026-10-22 · **Status:** ready for Codex
**Branch:** `wo/2026-10-22-002-step7-persistence-step0`, cut from main `df6d99d`
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 4 (independent investigation); §12.1 (history first); §13 (c) Step 0, (j) every number with its source
**Background:**
- `docs/architecture-review-2026-09-03.md` (Grok), "Where complexity is concentrated" and next-session item 2;
- `WO-2026-09-23-001-step5-persistent-data-facade-split.md` §8 ("No layout change to `SysData`/`CurrentData` (Step 7)");
- `WO-2026-10-15-003-step6-power-owner-split-step0.md:8` (Step 7 waits until field ownership settles; it has now).

## Plain goal

The Grok review called `MyPersistentData` a persistence bag. Mode settings, recovery stage, watchdog forensics, webhook configuration and thermal thresholds all sit in one stored record, so every new field pays a tax. Step 5 gave callers narrow facades (`persist/*.h`, 7 includers instead of 22), but **the stored layout is unchanged**, and `MyPersistentData.{h,cpp}` is now 2,065 lines (it was 1,842).

**This Step 0 answers four questions:**
- What actually needs to be stored on the device?
- What is dead?
- What is already stored elsewhere, in the Particle ledgers?
- What is the simplest layout that lets a new field touch one concern?

Prefer deleting over reorganizing. **No code changes in this WO.**

## What exists (main `df6d99d`; re-check)

- **Three StorageHelperRK files:**
  - `sysStatusData` / `SysData`, `SYS_DATA_VERSION = 4` (`MyPersistentData.h:448`);
  - `sensorConfigData` / `SensorData`, `SENSOR_DATA_VERSION = 1` (`:593`);
  - `currentStatusData` / `CurrentData`, `CURRENT_DATA_VERSION = 1` (`:824`).

  Fields are append-only. StorageHelperRK zero-pads a smaller stored record on validate (`:182`, `MyPersistentData.cpp:507–508`).
- **Facades:** `persist/SystemConfig.h`, `PowerConfig.h`, `RecoveryState.h` and `CurrentReadings.h`. `persist/RetainedBoot.h` is retained RAM, owned by P-0, and **out of scope**.
- **Configuration comes from the ledgers:** `Cloud::loadConfigurationFromCloud()` reads `defaultSettingsLedger.get()` and `deviceSettingsLedger.get()` (`Cloud.cpp:634–636`).
- **Fields that look dead or misplaced** (verify each):
  - `reservedFloat0`, and `reservedByte0` stored as `enableHibernateSleep` (`MyPersistentData.h:159–161`);
  - `faceNumber`, `faceScore`, `gestureType` and `gestureScore` (`:677–680`), left from a removed sensor;
  - `externalTempC` (ultrasonic);
  - `lastCountTime` (the code says it has no consumers);
  - `lastOccupancyEvent`, a `millis()` value persisted across resets, where `millis()` restarts at 0.

## Questions (answer each; observation and inference marked separately)

1. **Field inventory.** One table for the three records. For each field give:
   - its offset and size;
   - its facade;
   - its writers and readers, found by **bare-name** search (§13 (c));
   - one class:
     - **(a) config** re-derived from the ledgers;
     - **(b) state** that must survive a reset or power loss (counts, occupancy session, daily cleanup time, last report and connection);
     - **(c) forensics or diagnostics**;
     - **(d) dead or reserved**;
     - **(e) wrong to persist** (for example `millis()` values).

   Give the totals per class in bytes and in lines of accessor code.
2. **Is the stored config a redundant cache?**
   - At boot, before any connection, does `.get()` return the last synced ledger contents from Device OS's local copy? Cite the code path, the boot order and when config is applied.
   - What happens when a ledger was never synced, and with `hasValidLedgerConfig`?
   - Which class (a) fields would become redundant if config were always re-applied from the local ledger at boot?
   - Where Device OS 6.6.0 behavior can't be shown from the tree, say **"cannot tell"** and specify the bench test that would show it (reboot and power cycle offline, then read config).
3. **Wrong-to-persist fields:** which (e) fields cause wrong behavior after a reset? Cross-check recovery plan item 15 (occupancy lost across a reset) and `closeOccupancySessionSafely`'s boot anchor (`State_Common.h:355–360`).
4. **Storage mechanics.** Answer each from StorageHelperRK and our `validate()`/`initialize()`:
   - what a version bump does and what is lost;
   - what an older firmware does with a newer or larger record (the **downgrade** path);
   - the save frequency and flash wear per file;
   - whether one file per concern costs anything (RAM, flash, boot time).
5. **Target shape.** Compare, with net `src/` lines (deletions count), migration risk and downgrade safety:
   - **(A) No layout change:** delete dead accessors only, leaving reserved bytes in place.
   - **(B) Split by concern:** new versioned files (config cache, daily state, recovery and forensics) with a one-time migration from the current records.
   - **(C) Ledger as the source of config:** store no config, re-apply from the local ledger at boot, and keep only state and forensics.
   - Combinations are allowed.

   Recommend the **simplest** shape that meets the plain goal and reduces lines.
6. **The fleet path.** For the visitation fleet converging to GCC (`WO-2026-10-20-004`, cutover at the first connection after opening):
   - what the first GCC boot creates;
   - whether any field must be seeded at cutover (counts on the cutover day);
   - how the recommended shape changes that.
7. **History (§12.1):** how each record grew (append-only additions, reserved fields, version bumps), with commits. Note anything that can be restored rather than built.
8. **Sequence.** WOs of at most +20 net `src/` lines each, removals first. For each, give its bench test: power cycle and reboot readback, an OTA up from v45, and a downgrade to v45.
9. **Stop decision:** STOP or PROCEED. STOP if the recommended shape needs a migration that cannot be made downgrade-safe, or any WO over +20.

## Standing rules

- Read-only. No `src/` changes.
- `retained` RAM (`RetainedBoot`, P-0) is out of scope.
- No behavior fixes. Wrong-to-persist fields get their own WOs with evidence later.

## Agents and models

- **Investigation:** Codex `gpt-5.6-sol`, **high**, read-only. Launch it with stdin closed (`</dev/null`) per the vendor notes. It's the stored layout of every device, migration and downgrade safety, and the ledger question; a wrong answer here costs data in the field.
- **Controller:** Claude Code checks every file:line, offset and count in the report against the tree before it reaches the architect (§13 (j)). No ARM build.

## Budget

- Step 0: 0 `src/` lines.
- The report: at most 150 lines (§13 (f)), committed on the WO branch.

## Architect's rulings on the Step 0 report (22 Oct)

1. **Shape A plus freeze is accepted.** The three records stay byte-for-byte as they are and no field is appended; a future stored field goes in a file owned by its concern. Shapes B and C are rejected for now.
2. **Compatibility lasts while a rollback target reads these records** (v45 or any version that reads them); "forever" is too strong. Revisited after the fleet convergence.
3. **P7-1, P7-2 and P7-3 are one WO:** `WO-2026-10-22-003-step7-p7a-remove-dead-persistence.md` (ships in v47).
4. **`lastCountTime` waits** for the cleanup series, after the open sleep PRs (#128 to #139) merge, since its writers are in `State_Sleep.cpp`/`State_Modes.cpp`.
5. **P7-4 is deferred, not cancelled** (the Device OS local-ledger bench test); it decides P7-5 and is scheduled after v46 settles.
6. **P7-6 is dropped** and recorded in `docs/RECOVERY_PLAN_2026-09-26.md` as a known wrong-to-persist field handled at boot (`lastOccupancyEvent`, cleared at `Generalized-Core-Counter.cpp:497–505`).
