# DRAFT GitHub issue for particle-iot/device-os (for Chip to review and file; not filed)

**Suggested title:** AM18x5 external RTC: `setTime()` never sets WRTC and `writeContinuousRegisters()` ignores I2C errors, so `hal_rtc_set_time(EXTERNAL)` can return success without changing the chip

**Labels (if available):** bug, hal, external-rtc

## Summary
With the built-in AM18x5 driver (`hal/shared/am18x5.cpp`), `hal_rtc_set_time()` with `HAL_RTC_SOURCE_EXTERNAL` can return `SYSTEM_ERROR_NONE` while the chip's time registers are unchanged. Two causes, both from reading the 6.6.0 source (**not yet reproduced on hardware by us**; a reproduction step is below):

1. **`Am18x5::setTime()` never sets the WRTC bit.** The AM18x5 only accepts writes to the time registers while `CONTROL1.WRTC` (bit 0, `CONTROL1_WRTC_MASK` 0x01) is 1. `setTime()` builds the 8-byte calendar and calls `writeContinuousRegisters(HUNDREDTHS, ...)` directly (`hal/shared/am18x5.cpp:770-788`). `CONTROL1_WRTC_MASK` is defined at `hal/shared/am18x5_defines.h:67` and is not used anywhere else in the driver.
2. **`writeContinuousRegisters()` ignores every I2C result and returns the requested length.** It calls `hal_i2c_begin_transmission`, `hal_i2c_write` (per byte) and `hal_i2c_end_transmission` without checking any of them and ends with `return len;` (`hal/shared/am18x5.cpp:1350-1359`). `CHECK()` treats any non-negative value as success (`services/inc/check.h:141-149`), so `setTime()` returns success after a NACK, a bus error, or a write the chip ignored.

## Why it matters
Chips that another driver has touched can have WRTC = 0. For example the `AB1805_RK` library (vendored in our project) sets WRTC, writes the time, then **clears** WRTC after each write (`AB1805::setRtcFromTm()`, and its `isRTCSet()` is "WRTC is clear"). On such a chip, switching to the built-in driver makes `hal_rtc_set_time(EXTERNAL)` a silent no-op that reports success. Applications that rely on the return value (for example to confirm an RTC write-back after a cloud time sync) are told the chip was updated when it was not.

## Suggested fix
- In `Am18x5::setTime()` (and any other path that writes the time or calendar registers): set `CONTROL1.WRTC`, write the registers, then clear WRTC (or leave it set, which is the chip's power-on default), under the existing `Am18x5Lock`.
- In `writeContinuousRegisters()` (other write helpers in the driver should be checked for the same pattern; we did not audit them): check the return values of `hal_i2c_write` and `hal_i2c_end_transmission` and return an error code on failure.

## Reproduction (to run on hardware; ours is pending)
1. Boron on a carrier with an AM1805/AB1805 at 0x69 whose CONTROL1 has WRTC = 0 (e.g. after `AB1805_RK` has set the RTC once).
2. Bind the built-in driver (`Am18x5.enable(config)`).
3. `hal_rtc_set_time()` with `HAL_RTC_SOURCE_EXTERNAL` and a time several minutes away from the chip's.
4. Read the chip back with `hal_rtc_get_time(HAL_RTC_SOURCE_EXTERNAL)`: expected the new time; per the source, the old time with the set having returned 0.

## Related (could be filed separately)
`bindIfNeededAndResync()` (`hal/shared/exrtc_hal.cpp:236-249`): after `bind()`, it reads the **internal** clock and writes it to the chip unconditionally via `hal_exrtc_set_time_internal()`. On a first bind on a device whose internal clock has not been set (year 2000), a **valid chip time is overwritten** with the year-2000 internal time and the internal clock is never seeded from the chip. Suggested: after bind, read the chip, and if its time is valid seed the internal clock from it; only write internal → chip when the chip time is invalid.

## Environment
Device OS 6.6.0 (source from the local toolchain tree); Boron (nRF52840) with an external AB1805/AM1805 on the carrier board; `HAL_PLATFORM_EXTERNAL_RTC`.
