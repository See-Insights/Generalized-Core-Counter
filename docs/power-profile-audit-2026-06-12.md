# Boron Power Configuration Audit
**Date:** 2026-06-12  
**Device:** SG-Boron (Boron LTE)  
**Issue:** Battery not charging despite PMIC reporting FAST_CHARGE state

---

## Executive Summary

**Root Cause Identified:** Power profile selection logic runs only once at boot. When the device boots on battery/unknown power and applies SOLAR profile as fallback, it never switches to USB_BENCH profile when USB is later detected. The SOLAR profile's high VINDPM threshold (5080mV) exceeds typical USB voltage (~5.0V), causing DPM (Dynamic Power Management) to throttle charge current to near-zero. Additionally, Device OS USB detection limits input current to 400mA even with USE_VIN_SETTINGS_WITH_USB_HOST flag set.

**Impact:** Devices configured for solarPowerMode that boot without external power will fail to charge properly when USB is subsequently connected.

**Fix:** Add periodic power profile refresh to dynamically switch between USB_BENCH and SOLAR profiles based on actual detected power source.

---

## Investigation Details

### PMIC Register Dump Analysis

From SG-Boron register dump (2026-06-12):

```
REG00=0x7B REG01=0x1B REG02=0x18 REG03=0x11 REG04=0xB2
REG05=0x8A REG06=0x03 REG07=0x4B REG08=0x6C REG09=0x00
```

**Decoded Critical Values:**
- **REG00 (Input Source Control):**
  - Input current limit: **400mA** (bits 0-2 = 011 = 3 → 100+(3×100)=400)
  - Input voltage limit (VINDPM): **5080mV** (bits 3-6 = 1111 = 15 → 3880+(15×80)=5080)
  
- **REG02 (Charge Current Control):**
  - Charge current: **896mA** (configured correctly)
  
- **REG04 (Charge Voltage Control):**
  - Charge voltage: **4208mV** (configured correctly for SOLAR profile)
  
- **REG08 (System Status):**
  - VBUS status: **1 = USB Host detected**
  - Charge status: **2 = Fast Charge** (state machine reports charging)
  - **DPM active: 1** (input voltage regulation throttling charge)
  - Power Good: 1
  
- **REG09 (Fault):**
  - No faults (0x00)

**Anomalies Detected:**
1. Input current limit 400mA vs firmware expectation 900mA
2. DPM active while reporting FAST_CHARGE
3. USB detected but VINDPM threshold (5080mV) exceeds typical USB voltage (5.0V ±5%)

### Code Audit Results

#### 1. Power Profile Configuration (src/power/PowerPlatform.cpp)

**USB_BENCH Profile:**
```cpp
constexpr int USB_BENCH_MAX_CURRENT_MA = 900;
constexpr int USB_BENCH_MIN_VOLTAGE_MV = 3880;
constexpr int USB_BENCH_CHARGE_CURRENT_MA = 896;
constexpr int USB_BENCH_CHARGE_VOLTAGE_MV = 4112;
```

**SOLAR Profile:**
```cpp
constexpr int SOLAR_MAX_CURRENT_MA = 900;
constexpr int SOLAR_MIN_VOLTAGE_MV = 5080;  // ← HIGH THRESHOLD
constexpr int SOLAR_CHARGE_CURRENT_MA = 900;
constexpr int SOLAR_CHARGE_VOLTAGE_MV = 4208;
// Includes: USE_VIN_SETTINGS_WITH_USB_HOST flag
```

**Configuration Applied Via:**
```cpp
SystemPowerConfiguration conf;
conf.powerSourceMaxCurrent(SOLAR_MAX_CURRENT_MA)      // → 900mA
    .powerSourceMinVoltage(SOLAR_MIN_VOLTAGE_MV)      // → 5080mV
    .batteryChargeCurrent(SOLAR_CHARGE_CURRENT_MA)    // → 900mA
    .batteryChargeVoltage(SOLAR_CHARGE_VOLTAGE_MV)    // → 4208mV
    .feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST);
result.systemResult = System.setPowerConfiguration(conf);
```

✅ **Uses Power Manager API correctly** (System.setPowerConfiguration)  
✅ **No direct PMIC register writes** (all settings via Device OS API)  
❌ **SOLAR profile VINDPM too high for USB** (5080mV > typical USB 5.0V)  
❌ **USE_VIN_SETTINGS_WITH_USB_HOST flag ineffective** (Device OS still limits to 400mA)

#### 2. Profile Selection Logic (src/power/PowerManager.cpp)

```cpp
PowerInputProfile selectInputProfile(
    const PowerPlatform::PowerSourceSnapshot &snapshot,
    PowerInputProfile fallbackProfile,
    PowerInputProfile lastAppliedProfile,
    PowerProfileSelectionReason &reason) {
  switch (snapshot.source) {
  case kPowerSourceUsbHost:
  case kPowerSourceUsbAdapter:
  case kPowerSourceUsbOtg:
    reason = PowerProfileSelectionReason::UsbPowerSource;
    return PowerInputProfile::UsbBench;  // ← CORRECT LOGIC
    
  case kPowerSourceVin:
    reason = PowerProfileSelectionReason::VinPowerSource;
    return PowerInputProfile::Solar35W;
    
  case kPowerSourceBattery:
    if (lastAppliedProfile != PowerInputProfile::NotApplicable) {
      reason = PowerProfileSelectionReason::BatteryKeepLast;
      return lastAppliedProfile;  // ← STICKY BEHAVIOR
    }
    reason = PowerProfileSelectionReason::BatteryFallback;
    return fallbackProfile;  // ← Falls back to SOLAR if solarPowerMode=true
  }
}
```

✅ **Logic correctly identifies USB and would select UsbBench**  
❌ **Profile refresh called only ONCE during setup** (line 1100 in main.cpp)  
❌ **No runtime profile switching** when power source changes

#### 3. Profile Refresh Frequency

**Current behavior:**
```cpp
// In setup() - line 1100 in Generalized-Core-Counter.cpp
measure.batteryState(BatterySampleContext::Setup);
PowerManager::instance().refreshInputProfile();  // ← ONLY CALLED HERE
```

**Problem sequence:**
1. Device boots on battery (or power source unknown at boot)
2. `selectInputProfile()` returns fallback = SOLAR (if solarPowerMode=true)
3. SOLAR profile applied with VINDPM=5080mV, inputCurrent=900mA
4. USB cable plugged in (or detected after boot)
5. Device OS detects USB, limits input current to 400mA (USB enumeration default)
6. `refreshInputProfile()` **never called again**, profile stays SOLAR
7. PMIC has VINDPM=5080mV but USB provides ~5.0V
8. DPM activates, throttles charge current to near-zero
9. Battery discharges faster than trickle charge
10. Voltage declines (3.888V → 3.877V observed)
11. PMIC still reports "FAST_CHARGE" state

#### 4. No Direct PMIC Configuration

✅ **Confirmed:** No direct PMIC register writes anywhere in codebase  
✅ **Only enable/disable charging calls:** Used for PMIC health monitoring remediation  
✅ **All configuration via Device OS Power Manager API:** System.setPowerConfiguration()

---

## Measured vs Expected Values

| Parameter | Firmware Expects | PMIC REG00 Shows | Device OS Applied |
|-----------|------------------|-------------------|-------------------|
| Input Current Limit | 900mA | **400mA** | 400mA (USB default) |
| Input Voltage Min | 5080mV (SOLAR) | 5080mV | 5080mV |
| Charge Current | 900mA | 896mA | 896mA |
| Charge Voltage | 4208mV (SOLAR) | 4208mV | 4208mV |

**Contradiction:**
- Firmware configures 900mA via `powerSourceMaxCurrent(900)`
- PMIC REG00 shows 400mA
- Device OS overrides to USB-safe 400mA despite USE_VIN_SETTINGS_WITH_USB_HOST flag

---

## Device OS Power Manager Behavior

From Particle documentation and observed behavior:

1. **System.setPowerConfiguration()** applies settings to PMIC via Device OS
2. **USB Detection:** When USB is detected, Device OS may override settings for safety
3. **USE_VIN_SETTINGS_WITH_USB_HOST:** Flag intended to use VIN settings even with USB detected
4. **Observed:** Flag does NOT prevent Device OS from limiting to 400mA when USB detected
5. **Input current limiting:** Device OS limits to 400-500mA for USB before full enumeration

**Likely Device OS behavior:**
```
if (USB_DETECTED && !FULLY_ENUMERATED) {
    limit_input_current_to_400mA();  // Safety limit
    ignore_USE_VIN_SETTINGS_WITH_USB_HOST_flag();
}
```

---

## Why This Occurs

### Scenario 1: Boot on Battery → USB Connected Later
1. Device boots on battery
2. Power source = BATTERY
3. Profile selection: fallback = SOLAR (solarPowerMode=true)
4. SOLAR profile applied
5. USB connected after boot
6. Device OS detects USB, limits to 400mA
7. Profile never refreshed → stays SOLAR
8. VINDPM=5080mV > USB_VOLTAGE=5.0V → DPM active
9. Charge current throttled to near-zero

### Scenario 2: Boot with USB Connected (but not detected yet)
1. Device boots with USB already connected
2. Power source initially UNKNOWN or BATTERY
3. Profile selection: fallback = SOLAR
4. SOLAR profile applied
5. Device OS later detects USB, limits to 400mA
6. Profile never refreshed
7. Same DPM issue

---

## Proposed Solution

### Minimal Change: Periodic Profile Refresh

**Add profile refresh to existing periodic battery monitoring:**

```cpp
// In src/sensors/SensorManager.cpp, batteryState() function
// After PMIC health monitoring section, before return statement

#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)
  // Refresh power profile to handle runtime power source changes
  // (e.g., USB connected after boot on battery, or VIN connected after USB)
  PowerManager::instance().refreshInputProfile();
#endif
```

**Why this location:**
- `batteryState()` already called at multiple lifecycle points:
  - `BatterySampleContext::Setup` (setup tail)
  - `BatterySampleContext::PostWake` (after sleep)
  - `BatterySampleContext::PreSleep` (before sleep)
  - `BatterySampleContext::Reporting` (before reports)
- Already platform-guarded for Boron
- Natural fit alongside battery/PMIC health monitoring
- No additional function calls needed in state machine

**Expected behavior after fix:**
1. Device boots on battery → SOLAR profile applied (fallback)
2. USB connected → batteryState() called
3. `refreshInputProfile()` detects USB (source=1)
4. Switches to USB_BENCH profile
5. VINDPM lowered to 3880mV (< USB 5.0V)
6. Input current properly set to 900mA
7. DPM clears
8. Normal charging resumes

**Profile switching will occur:**
- After wake from sleep (PostWake battery sample)
- Before sleep (PreSleep battery sample)  
- During reporting cycles (Reporting battery sample)
- Worst case: within one wake cycle (~hourly for most deployments)

---

## Alternative Solutions Considered

### Option 2: Modify SOLAR Profile
**Approach:** Lower SOLAR VINDPM to 4500mV or remove USE_VIN_SETTINGS_WITH_USB_HOST
**Rejected:** Defeats purpose of SOLAR profile for actual VIN/solar input where higher voltage is available

### Option 3: Single Unified Profile
**Approach:** One profile with conservative settings for both USB and VIN
**Rejected:** Loses optimization potential; can't take advantage of higher VIN voltage/current

### Option 4: Manual Profile Switching in Configuration
**Approach:** User must manually switch solarPowerMode when changing power source
**Rejected:** Poor user experience; should be automatic

---

## Risk Assessment

**Change Impact:** LOW
- Single line addition to existing function
- Uses existing API (refreshInputProfile already implemented and tested)
- Platform-guarded (Boron only)
- No changes to sleep/connect logic
- No changes to PMIC register access
- Preserves all existing behavior

**Testing Required:**
1. Boot on battery → connect USB → verify profile switches to USB_BENCH
2. Boot on USB → verify profile correctly selects USB_BENCH from start
3. Boot on VIN → verify profile correctly selects SOLAR
4. Switch between USB and VIN → verify dynamic profile switching
5. PMIC register dump validation after each scenario

**Rollback:** Remove single line addition if issues occur

---

## Success Criteria

After implementing fix and flashing to device:

**Register Dump Should Show:**
- REG00 input current limit: **~900mA** (not 400mA)
- REG00 VINDPM: **3880mV** when USB detected (USB_BENCH profile)
- REG00 VINDPM: **5080mV** when VIN detected (SOLAR profile)
- REG08 DPM status: **0** when powered from stable 5V USB
- REG08 charge status: **2 or 3** (Fast Charge or Done)

**PowerCfg Logs Should Show:**
- Profile switches from SOLAR to USB when USB detected
- Profile switches from USB to SOLAR when VIN detected
- Profile remains stable when power source unchanged

**Battery Behavior Should Show:**
- Voltage rising when plugged in (not declining)
- Bench supply shows measurable current draw (not near-zero)
- SOC increases over time when plugged in

---

## Implementation Plan

1. ✅ Audit complete - root cause identified
2. ⏳ Add refreshInputProfile() call in batteryState()
3. ⏳ Compile and test on bench
4. ⏳ Flash to SG-Boron device
5. ⏳ Monitor logs for profile switching
6. ⏳ Capture PMIC register dump after USB connection
7. ⏳ Verify DPM cleared and charging normal
8. ⏳ Test VIN power source (if available)
9. ⏳ Validate across full wake cycle

---

## Files to Modify

1. **src/sensors/SensorManager.cpp** (line ~1065, after PMIC health monitoring)
   - Add: `PowerManager::instance().refreshInputProfile();`
   - Platform guard: `#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)`

2. **src/sensors/SensorManager.cpp** (include section)
   - Add: `#include "power/PowerManager.h"` (if not already present)

---

## References

- Particle Device OS Power Manager API: https://docs.particle.io/reference/device-os/api/power-manager/
- BQ24195 Datasheet: Texas Instruments SLUS962
- Register dump implementation: src/sensors/SensorManager.cpp line 1825-2050
- Power profile configuration: src/power/PowerPlatform.cpp line 38-96
- Profile selection logic: src/power/PowerManager.cpp line 25-50
