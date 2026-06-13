# 2026-06 PMIC Charging Root Cause

## Summary

Some Boron devices discharged while externally powered over USB. The PMIC reported Fast Charge with no fault, but input-voltage DPM was active and charge current was effectively throttled away.

The root cause was a power-source classification/configuration mismatch. The firmware repeatedly selected the `SOLAR` profile while the physical source was USB. `SOLAR` configured VINDPM to 5080 mV, which left almost no headroom for a USB source around 5.1 V.

## Symptoms

- Battery SOC declined while USB power was attached.
- Charge LED was continuously illuminated.
- PMIC reported Fast Charge.
- PMIC fault register was `0x00`.
- PMIC DPM was active.
- Device repeatedly selected the `SOLAR` input profile.
- USB source voltage was approximately 5.1 V.

## Investigation Timeline

1. Field observations showed battery SOC falling while the device was attached to USB power.
2. Firmware diagnostics showed `profile=SOLAR`, `vbus=USB`, `chg=FastCharge`, and `fault=0x00`.
3. PMIC register decoding showed `REG00` configured VINDPM at 5080 mV and `REG08` reporting DPM active.
4. Bench testing at 5.1 V produced essentially no charging current.
5. Raising the bench supply to 6.1 V immediately increased current draw to approximately 150 mA and charging resumed.
6. Device OS source review showed that `SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST` can cause USB host power to be reported as `POWER_SOURCE_VIN`.
7. Removing that feature restored proper `USB_HOST` classification, selected `USB_BENCH`, and normal USB charging resumed.

## Evidence Collected

- Power diagnostics consistently reported `prof=SOLAR`, `vbus=USB`, `src=1`, `chg=2`, and `fault=0x00`.
- PMIC `REG00=0x7B` decoded to an input-current limit of 400 mA and VINDPM of 5080 mV during the failure investigation.
- PMIC `REG08` showed USB VBUS, Fast Charge, and DPM active.
- The battery discharged while externally powered.
- The same hardware charged when the input was raised from approximately 5.1 V to 6.1 V.
- After removing `USE_VIN_SETTINGS_WITH_USB_HOST`, Device OS reported USB host power correctly and the firmware selected `USB_BENCH`.

## Key Diagnostic Breakthrough

The 5.1 V to 6.1 V bench-supply test proved the PMIC, battery, fuel gauge, and charge path were functional. The failure depended on input-voltage headroom. That pointed to the configured VINDPM threshold and DPM behavior rather than a battery fault, PMIC fault, or fuel-gauge reporting error.

## Root Cause

`SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST` was enabled in the solar power configuration. On Boron / Device OS 6.4.1, that feature can remap a real USB host source to `POWER_SOURCE_VIN`.

The firmware interpreted `POWER_SOURCE_VIN` as solar/VIN power and applied the `SOLAR` profile. `SOLAR` set VINDPM to 5080 mV. With a real USB input around 5.1 V, the PMIC entered DPM and throttled charge current to near zero. The PMIC still reported Fast Charge and no fault, which made the failure look healthy unless DPM and actual current were considered.

## Fix Implemented

- Removed `SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST` from the default `SOLAR` profile.
- Preserved Device OS power-source classification as the source of truth.
- Kept USB classifications mapped to `USB_BENCH`.
- Kept VIN classification mapped to `SOLAR`.
- Avoided direct PMIC register overrides and periodic profile refresh logic.

## Validation

- USB power was classified as `USB_HOST` after the fix.
- `USB_BENCH` was selected for USB power.
- Charging resumed normally on USB.
- DPM no longer prevented USB charging.
- Solar/VIN still selects the `SOLAR` profile when Device OS reports `POWER_SOURCE_VIN`.

## Known Failure Signature

```text
PowerApply profile=SOLAR
PowerDiag vbus=USB chg=2 fault=0x00
PMIC DPM active
Battery SOC declining
```

Interpretation: USB source misclassified as VIN, causing the `SOLAR` profile and 5080 mV VINDPM threshold to be applied to physical USB power.

## Lessons Learned

- PMIC Fast Charge state does not guarantee useful charge current.
- A zero-fault PMIC can still be input-limited by DPM.
- Device OS power-source classification should drive profile selection.
- Profile names are policy labels, not physical source measurements.
- Avoid firmware-side PMIC register overrides when Particle-supported APIs can express the required configuration.

## Future Guidance

- Do not enable `USE_VIN_SETTINGS_WITH_USB_HOST` for mixed USB/Solar deployments.
- Keep USB sources on `USB_BENCH` unless Particle adds a clearer supported distinction for the deployment hardware.
- Use VIN-only classification for the solar/VIN charging profile.
- When diagnosing charging, collect profile, Device OS power source, PMIC VBUS state, charge state, fault register, DPM state, input voltage, and battery SOC trend together.
