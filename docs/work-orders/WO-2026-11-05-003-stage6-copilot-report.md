# WO-2026-11-05-003 S1 - Stage 6 Copilot report

AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium · python3 3.14.6 (.py), zsh (.sh) · Branch `wo/2026-11-05-003-v50-s1-boot-config-time` on d5327ed. Nothing committed, pushed, merged or flashed. **The bench is not run and nothing here claims it.**

## Step 0 (Device OS 6.6.0, `~/.particle/toolchains/deviceOS/6.6.0`)

1. **Setter.** `hal/src/nRF52840/rtc_hal.cpp:93-114` (get), `:116-131` (set). With `source == HAL_RTC_SOURCE_EXTERNAL` the set calls `hal_exrtc_set_time_internal` and returns its result without touching the system clock (chip-only write). The external get returns `hal_exrtc_get_time_internal`; `SYSTEM_ERROR_NOT_FOUND` if no chip. `hal/shared/exrtc_hal.cpp:405-417`; `hal/shared/am18x5.cpp:770-789` (`setTime`), `:791-814` (`getTime`). Whole seconds with `tv_usec=0` survive (usec is ceiled to hundredths); pre-2000 gives `INVALID_ARGUMENT`; year stored `%100`. Exported in the dynalib. **The plan holds; no alternative needed.**
2. **Seeding/clamp.** `hal_rtc_init` (`rtc_hal.cpp:77-86`) seeds system time from the chip only when a binding exists. Unbound (first v50 boot after v49), Time is invalid until bind/cloud sync, so `revalidateOccupancyStartTimeIfTimeAvailable()` stays (comment fixed).
3. **`enable()` semantics.** `ExRtcT::enable` (`wiring/inc/spark_wiring_exrtc.h:586-592`) → `hal_exrtc_bind` (`exrtc_hal.cpp:267-280`) stores the binding and binds **in the same boot** (`am18x5.cpp:208-263`); no reboot needed. Identical config on a present chip is a no-op (`exrtc_hal.cpp:237`); a differing one re-binds. `status().bound()` is true for stored or platform-default bindings (`:328-330`; Boron has none).
4. **Payload.** Osc keys are in the **status** payload (`DeviceStatusPublisher.cpp:313-317`). `kLedgerSchemaVersion` (`:53`) is **shared by the status and the data ledger**, so 2→3 changes both (the WO says "device-data"; both are documented). `oscStatus`/`oscCtrl` are now `null`; `usingRC`/`aos`/`fos` stay.
5. **Count.** `System.reset()` sites 8 → 7 (cause 8 removed).
6. **Amendment 2 latch (`CLOCK_SOURCE_EXTERNAL_FAILURE`).** Verified: `RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE=0x02` (`exrtc_hal.h:86-88`), callback `void(RtcEvents, void*)`, mapped from XT_OSC_FAILURE at `am18x5.cpp:695-715`; OFIE is enabled only with the external-crystal oscillator (`:595`, `:635`), and registering a handler clears the OF flag (`:646`). It was implemented and compiled, but **did not fit** the +20 `src/` budget (about +9 lines), so it was removed and is **reported as S4**. `usingRC` at boot (from `status().clockSource()`) is published meanwhile.

## What changed

- `src/time/ExternalRtc.h` (new): `get`, `set`, `setFromSystem` (keeps the `Time.isValid()` precondition), `isSet`, `sameFleetConfig`, `ensureBound`. All over `hal_rtc_*`/external source, bool results as before.
- `setup()`: v49 binding-delete guard and `ab1805.withFOUT(WKP).setup()` removed. `ExternalRtc::ensureBound(Am18x5, Am18x5Configuration().i2c(Wire).interruptPin(WKP).watchdogPin(D5).capabilities(AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL))` runs first; enable failure logs and continues (no reset); a stored-config mismatch logs a warning and re-binds. Osc snapshot now uses `Am18x5.status().clockSource()` and `getConfig().capabilities()`.
- `Clock.cpp`, `HibernateEligibility.cpp` (RTC read + `isSet`), RTC-skew hook, and the `isRTCSet` log use the helpers; `AB1805_RK.h`/`extern ab1805` removed from `Clock.cpp`.
- `ResetCause.h`: cause 8 removed (reserved comment). `device_pinout.{h,cpp}`: logical `WATCHDOG_DONE_PIN = D5`. Stale "ab1805.setup() seeds" comments fixed.
- Docs: `docs/contracts/ledger-contracts.md` (both examples `schemaVersion: 3`, field note), `docs/FIELD_MEANINGS_REFERENCE.md`. Fleet-ops consumers of `oscStatus`/`oscCtrl`/schema 2 are **not in this repo**; they need checking.
- Left as the WO says (S2/S3): `AB1805 ab1805(Wire)`, `setWDT`, `ab1805.loop()`, `classifyWake(..., ab1805)`, `HibernateCycle.cpp:78` RTC read, `HibernateEligibility.cpp:30-47` register clearing, power-off.

`git diff --numstat -- src` (new file counted via `git add -N`, intent-to-add only): +99/−83 = **net +16** (≤ +20). Main .cpp 29/64, ExternalRtc.h 52/0, Clock.cpp 4/6, DeviceStatusPublisher 3/5, HibernateEligibility 3/2, others 1/1 or 1/0.

## Tests

- New: `tests/external_rtc_test.cpp` (real header vs `tests/stubs/external_rtc/Particle.h`: success, read error, write error, write→read-back, external source, usec=0, `setFromSystem` precondition, bind, no re-bind, enable failure, stored mismatch incl. pin and capability) driven by `tests/external_rtc_structural_test.py` (no `ab1805.setup()`, guard gone, config is Wire/WKP/D5 + fallback, bind before first Time use, cause 8 gone, schema 3, null osc keys, no `ab1805.` RTC use in `Clock`/`HibernateEligibility`).
- **Mutations, each fails (14/14 caught):** enable unconditionally; skip bound check; reset on enable failure; write via wrong source; ignore write error; ignore fallback capability; ignore pin; non-zero usec; `ab1805.setup()` left in; guard kept; wrong pin; fallback capability missing; cause 8 kept; schema stays 2.
- Retired: `binding_delete_guard_test.sh`, `binding_delete_guard_order_test.py` (guard gone; replaced above).
- Updated: `reset_cause_structural_test.py` (7 sites, cause 8 must be absent), `clock_resync_wiring_test.py`, `rtc_skew_test.sh`, `occupancy_start_time_clamp_test.sh`, five `sleep_*_structural_test.py` (allow the new header in their "no new src file" check), three JSON-writer stubs (`nullValue()`).
- **Host suite: 116/116** (baseline 117, minus 2 retired, plus 1 new). `thermal_coupling_structural_test.py` passes on a clean export of tracked files (exit 0).

## Build

Local Device OS 6.6.0, Boron, gcc-arm 10.2.1, buildtools 1.1.1, fresh `BUILD_PATH_BASE` (`build-tmp/wo-2026-11-05-003-stage6/bpb2`). Built; text/data/bss **152604 / 1114 / 2240** (v49.1 baseline 151732 / 1114 / 2200: +872 text, +40 bss). `nm`: `ExRtcT<Am18x5Configuration>::enable`, `hal_exrtc_bind`, `hal_rtc_get_time/set_time/time_is_valid` present. `objdump`: `setup()` calls `Am18x5Rtc::instance()`, `Am18x5Configuration()`, `getConfig` and `enable` (two sites: unbound and mismatch); `hal_rtc_set_time` is reached from `ExternalRtc::setFromSystem`, `hal_rtc_get_time` from `checkClockResync`, `shouldUseBoronRtcAlarmHibernate` and the skew path.

## Risks and findings for Chip (read before bench)

1. **WRTC (highest).** Device OS never sets or clears the AM18x5 WRTC bit (`am18x5_defines.h:67`, no other use) and ignores the I2C end-transmission result in `writeContinuousRegisters`. The vendored `AB1805::setRtcFromTm` (`lib/AB1805_RK/src/AB1805_RK.cpp:256-295`) sets WRTC, writes, then **clears it** ("Can only write RTC registers when WRTC is 1"). Every chip the vendored lib has touched has WRTC=0, so `hal_rtc_set_time` may **silently no-op and return success**, which would make the write-back's "confirmed success only" false. Source cannot settle this. Bench step 2 (`settime <epoch> hal`, then read back) must be done on a chip with WRTC=0.
2. **First boot after OTA from v49 loses the chip time.** `bindIfNeededAndResync` (`exrtc_hal.cpp:236-249`) copies the internal time into the chip after binding. On an unbound device internal time is ~2000-01-01, so the valid chip time is overwritten; no valid time until the first cloud sync. This contradicts bench step 1 ("RTC valid at boot before cloud") for an unbound device, and is only avoidable by seeding from the chip before binding (not in S1 scope). Open question for you.
3. Schema constant is shared: both ledgers go to 3.
4. `isRTCSet()` (vendored: WRTC bit clear) → `ExternalRtc::isSet()` (`hal_rtc_time_is_valid`, external). I did not use `TimeClass::isValid()` because it can block in `waitUntil(Particle.syncTimeDone)`. Semantics differ and are noted in the code sites.
5. The vendored `ab1805.loop()` (kept for S2) still writes the chip once per boot after cloud sync and toggles WRTC: a second I2C owner until S2.
6. The latch is deferred to S4 (see Step 0, item 6).

Scratch: `build-tmp/wo-2026-11-05-003-stage6/` (build output only; left in place, not committed).
