# WO-2026-10-22-003 bench: Dev-14 stored values before the P7-A USB flash (recorded 23 Oct 2026 about 02:50Z)

**Why:** the bench step (WO-003) is: seed distinctive v45 values, flash the P7-A build, check each live value reads back, flash v45 back, check again with no "initialize" line. This is the "before" record. Sources: S3 `particle-events/2026-10-22..23/{Ubidots-Sensor-Hook-v1,status,hibernate_wake}/e00fce688e592afaf23ac4fb/` and Particle API GET (product 42131: device record and the `device-status`, `device-settings`, `device-data` ledger instances). Read-only.

## Build to flash

`~/Downloads/p7a-persistence-freeze-boron-6.6.0-app.bin`: built from the `#142` head `426fbd6` (src tree hash `3da3961c20e6308f…`), Boron, Device OS 6.6.0, `-frandom-seed=p6`; **152,966 bytes; sha256 `96a5b6dd6b52997d62f8197ce6a69850838fb381c6ff89d4b0bd244baa248980`**. It is exactly 88 bytes smaller than the v45 binary (153,054 B), as the ELF delta predicts. **It reports itself as `v44-DeviceOS660` (product version 44):** #142 branches from main, which has no v45 version bump.

## Dev-14 before the flash

| Item | Value (source) |
|---|---|
| Firmware | v45-PowerSplit, Device OS 6.6.0, `firmware_version 45`, online, last heard 02:38:29Z (API) |
| Last status (22 Oct 22:00:33.774Z) | reset 30 (hibernate wake), `resetReasonData 0`, `appBreadcrumb 28`, **`watchdogResetCount 126`**, `lastWatchdogBreadcrumb 28`, `lastWatchdogUptimeMs 1687188`, `failsafeStage/Count 0/0`, `pmicAnomalyCount 0`, `hibernateCount 8`, `freeHeap 89576` (S3 `status`) |
| Last hook (23 Oct 02:37:45.597Z, fw 45) | `occupancy 1`, **`dailyoccupancy 40`**, battery 80.59, `vc 4.03`, `Charging`, `cyc 52`, `slp 51`, `connecttime 152`, `fh 80744`, `alerts 0`, `resets 0` (S3 hook); the three before it: dailyoccupancy 35 (02:21:47Z), 40 (02:26:55Z), 40 (02:32:40Z) |
| `device-data` ledger (02:37:45Z) | `occupied true`, **`totalOccupiedSec 2430`** (40 min truncated), soc 80.6, `resetReason 30`, `resetReasonData 0`, timestamp 1792723063 (API) |
| `device-settings` ledger (updated 02:43:27Z) | timezone `SGT-8`, openHour 6, closeHour 22, `reportingIntervalSec 3600`, `enableHibernateSleep true`, **`verboseTimeoutMin 58`** (the non-default seed); `device-status` shows `configuredIntervalSec 1800`, `config.generation 4A9E5686`, power profile UsbBench, overrideActive true (API) |

The live stored values to check after the flash: `totalOccupiedSeconds` (about 2430 s and rising while the exerciser runs), `occupied`, hourly/daily counts, `lastReport`, `lastConnection`, `lastDailyCleanup`, `watchdogResetCount 126`, the ledger-derived settings (`verboseTimeoutMin 58`, 1800 s interval until the next apply), and `hibernateCount` 8 (retained, P-0).

## Two cautions for Chip

1. **The flashed image says v44.** If Dev-14 is locked to firmware 45 in the console (the API shows `desired_firmware_version` empty, `firmware_updates_enabled true`), the cloud may push v45 back at its next connection and end the test early; check or unlock first. The same applies to the v45 re-flash.
2. **Expected after the flash:** the 11 retired fields read 0 or their old stored values (they are no longer read); **no "initialize" log line** (a valid same-size record); a normal boot logs the stored `watchdogResetCount 126`.

## Grading the boot logs ("initialize")

When Chip sends the serial log, grade: (a) no StorageHelperRK `initialize`/"Loading System Defaults"/"Loading system defaults" lines after the flash boot (the code logs "Loading System Defaults" in `initialize()`); (b) the stored counters above read back; (c) after re-flashing v45, the same again.
