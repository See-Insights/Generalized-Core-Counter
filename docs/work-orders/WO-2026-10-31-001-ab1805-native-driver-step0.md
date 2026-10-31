# WO-2026-10-31-001: replace the vendored AB1805_RK library with Device OS's built-in AM18x5 driver: Step 0 (read-only)

**Author:** architect, 2026-10-31 · **Status:** ready
**Branch:** `wo/2026-10-31-001-ab1805-native-driver-step0`, from main
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 4 (independent investigation); §12.1, §12.5; §13 (c), (j)

## Plain goal

GCC drives the carrier board's AB1805 (RTC, hardware watchdog, deep power-down) through a vendored, locally patched copy of `AB1805_RK`. Its known defects:
- three wrong register constants, patched (PR #41);
- `usingRCOscillator()` reading the wrong flag;
- a suspected missing config-key write before the PWGT write in `deepPowerDown()`/`resetConfig()`, unverified.

**Device OS now has its own AM18x5 driver,** checked by the architect against `particle-iot/device-os` at `v6.6.0`:
- `CHANGELOG.md:146–147` (6.4.0): "built-in AM18x5 driver in Device OS" and "`POWER_OFF` sleep mode support through AM18x5" (#2906);
- `:91` (6.5.0): a `POWER_OFF` race fix (#2927);
- `hal/src/boron/hal_platform_config.h:105`: `#define HAL_PLATFORM_AM18X5 (1)`, also in `hal/src/msom/`;
- sources in `hal/shared/am18x5.{h,cpp}` and `hal/inc/exrtc_hal_am18x5.h`.

**The aim:** one owner of the chip, the platform's own and shared with the M-SoM, with less of our code. **This Step 0 decides whether, and how,** with no change to any code or device.

## Questions (observation and inference marked separately; cite file:line in both trees)

1. **Who touches the chip today?** On 6.6.0 with `HAL_PLATFORM_AM18X5 (1)`, does Device OS initialize or talk to an AM18x5 on its own at boot, or only when the app enables or configures it? **This matters now, not only for the switch:** if Device OS already drives it, GCC's `AB1805_RK` would be a second owner on the same I2C chip today. Cite the init path.
2. **Configuration.** What the app must set for a carrier-board AB1805 on a Boron: the I2C bus and address, the interrupt (wake) pin, the watchdog pin (our D5 DONE, D8 wake), and any setup call or system config. Our pins are in `src/device_pinout.h` and `.cpp`.
3. **The API map.** Every `AB1805_RK` call and member GCC uses, found by bare-name search (§13 (c)), mapped to the built-in equivalent:
   - time get and set;
   - alarms and `interruptAtTime` (hibernate);
   - the watchdog: `stopWDT`, `resumeWDT`, the DONE pin, `Watchdog` class integration;
   - wake-reason reads;
   - `deepPowerDown` (failsafe stage 3);
   - stale-interrupt clearing;
   - the oscillator and flag reads used by the clock-trust logic and diagnostics.

   **No equivalent:** list each one as a decision for Chip, not a reason to keep the library.
4. **The known defects.** Does the built-in driver avoid each `AB1805_RK` defect above (read its register defines and power-down sequence)? **Especially `deepPowerDown()`:** if the library's PWGT write really is ignored, failsafe stage 3, which #151 now relies on, may not power the device down. Say what the bench test of #151 (TEST_MODE, stage 2 then stage 3) would reveal about either driver.
5. **GCC logic that stays GCC's.** Clearing the sticky OF flag, the clock-trust rule (recovery item 21), `HibernateCycle`, `ConnectivityFailsafe`. Show which of these call the chip through `AB1805_RK`, and how they'd call the built-in driver instead.
6. **Lines.** The `src/` and `lib/` lines removed (the vendored library, our wrapper code) against those added, as a net estimate.
7. **Sequence.** Small WOs of at most +20 net `src/` lines each; removals count. It's all-or-nothing on the chip, so no mixing. A likely shape:
   - (a) a bench spike build, not merged, proving time, alarm, watchdog and `POWER_OFF` on Dev-14;
   - (b) the switch WO;
   - (c) removing `lib/AB1805_RK`.

   Give each its bench test. Order it against the M-SoM work and the CI Phase 1 (one vendored library fewer).
8. **Stop decision:** STOP if the built-in driver can't drive a carrier-board AB1805 on a Boron, or lacks something GCC must have with no workable alternative. Otherwise PROCEED.

## Standing rules

- Read-only. No `src/`, `lib/` or device change.
- `retained` RAM and the persisted layout are untouched.

## Agents and models

- **Evidence (both trees, the device-os source at v6.6.0):** Claude Code, read-only.
- **Analysis:** Codex `gpt-5.6-sol`, **high**, read-only, stdin closed. It's the chip behind time, the watchdog and the failsafe's last resort.
- **Controller check:** every citation and number, against both trees (§13 (j)).

## Budget

- 0 `src/` lines.
- The report: at most 150 lines, committed on the WO branch, then stop for the architect.
