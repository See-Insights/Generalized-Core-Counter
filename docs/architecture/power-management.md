# Power Management Architecture

## Overview

Power management is split between firmware policy and Particle-supported power APIs. `PowerManager` chooses the input-power profile from the Device OS power-source classification, and `PowerPlatform` translates that profile into a `SystemPowerConfiguration`. Device OS then applies the configuration to the platform power stack and, on Boron-class Gen 3 cellular devices, to the BQ24195 PMIC.

The architecture intentionally avoids direct PMIC register overrides for normal power-source selection. Custom PMIC reads are used for diagnostics and forensics; supported Particle APIs are used for durable charging configuration.

## Supported Platforms

- Boron / Gen 3 cellular PMIC platforms: full power-source detection, fuel gauge reads, charging control, and input-power profile application.
- Non-PMIC or unsupported platforms: report power state where available and use `NotApplicable` for PMIC/input-power profile application.

## Components

- `src/power/PowerManager.cpp`: selects the active input profile and records the reason for the decision.
- `src/power/PowerPlatform.cpp`: reads `System.powerSource()` and applies `SystemPowerConfiguration`.
- `SystemPowerConfiguration`: Particle-supported API used to set input-current limit, minimum input voltage/VINDPM behavior, charge current, and charge voltage.
- Particle Device OS: classifies the power source and owns the low-level PMIC programming path.
- BQ24195 PMIC: charger and input-power controller used on Boron-class devices.

## Power Profiles

| Profile | Input current limit | VINDPM / min input voltage | Charge current | Charge voltage | Selected for | Intended scenario |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| `USB_BENCH` | 900 mA | 3880 mV | 896 mA | 4112 mV | `USB_HOST`, `USB_ADAPTER`, `USB_OTG` | USB bench/debug power, USB charging power, and conservative mixed-power fallback |
| `SOLAR` | 900 mA | 5080 mV | 900 mA | 4208 mV | `VIN` | Dedicated VIN/solar input with enough voltage headroom |
| `NotApplicable` | n/a | n/a | n/a | n/a | unsupported PMIC/platform path | Platforms where input-power profiles cannot be applied |

`PowerInputProfile::Auto` exists in the enum but is not currently selected or applied as an active profile.

## Power-Source Selection

`PowerManager` treats the Device OS power-source classification as the source of truth:

| Device OS power source | Firmware profile | Reason |
| --- | --- | --- |
| `POWER_SOURCE_USB_HOST` | `USB_BENCH` | USB power source |
| `POWER_SOURCE_USB_ADAPTER` | `USB_BENCH` | USB power source |
| `POWER_SOURCE_USB_OTG` | `USB_BENCH` | USB power source |
| `POWER_SOURCE_VIN` | `SOLAR` | VIN power source |
| `POWER_SOURCE_BATTERY` | keep last profile, or configured fallback if none | avoid thrashing profile while external power is absent |
| unknown/unavailable | keep last profile, or configured fallback if none | conservative continuity during ambiguous readings |
| unsupported platform | `NotApplicable` | no PMIC input profile support |

For field builds, the configured fallback can use `solarPowerMode`; non-field builds fall back to `USB_BENCH`. Fallback is only used when the power source is battery, unknown, unavailable, or unsupported. A valid USB classification always selects `USB_BENCH`; a valid VIN classification always selects `SOLAR`.

## Design Rules

1. Never infer the physical power source from the active profile.
2. Never infer the physical power source from PMIC charge state.
3. `USB_HOST`, `USB_ADAPTER`, and `USB_OTG` always map to `USB_BENCH`.
4. `VIN` is the only Device OS source that selects `SOLAR`.
5. Any future use of `SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST` requires explicit architectural review.

## Critical Warning

Do not enable `SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST` for mixed USB/Solar deployments.

That Device OS feature exists for hardware designs where VIN power can appear as USB host power because the USB peripheral is active without USB providing the actual input supply. When enabled, Device OS can classify a real `USB_HOST` condition as `POWER_SOURCE_VIN` and apply VIN settings to that source.

In this firmware, VIN maps to `SOLAR`. On Boron / Device OS 6.4.1, enabling `USE_VIN_SETTINGS_WITH_USB_HOST` caused a physical USB input to be treated as VIN, which selected the `SOLAR` profile. The `SOLAR` profile sets VINDPM to 5080 mV. With a USB source near 5.1 V, the PMIC entered input-voltage DPM and throttled charging current to near zero even though the PMIC reported Fast Charge and no fault.

## Diagnostics

A dangerous signature is:

```text
PowerApply: profile=SOLAR source=VIN
PowerDiag: vbus=USB chg=2 fault=0x00
PMIC DPM active
Battery SOC declining while USB is attached
```

`chg=FastCharge` and `fault=0x00` do not prove meaningful charge current is flowing. DPM can be active and limiting input current while the charge-state machine still reports Fast Charge.

## References

- `src/power/PowerManager.cpp`
- `src/power/PowerPlatform.cpp`
- `docs/postmortems/2026-06-pmic-charging-root-cause.md`
- Particle Device OS power-management APIs: `System.powerSource()` and `System.setPowerConfiguration()`
