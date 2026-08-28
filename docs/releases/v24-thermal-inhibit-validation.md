# v24-Thermal-Inhibit — Hardware Validation Record

**Verdict: PASS.** The thermal charge-inhibit armed at the configured threshold, actually held charging off, and released on the tighter hysteresis band.

| | |
|---|---|
| Device | Boron-Dev-11 (`e00fce683f6063bf254283dd`) |
| Build | `v24-Thermal-Inhibit`, commit `516332a`, branch `release/v24-thermal-inhibit` |
| Date | 2026-08-28, 05:09:39 – 05:14:54 UTC |
| Work order | WO-2026-08-25-001 F2a |
| Inhibited for | 4 min 27.5 s |
| Peak temperature | 51.22 °C |
| Procedure | [Test 7 in bench-validation.md](../bench-validation.md) |
| Visual report | <https://claude.ai/code/artifact/fab2b7c6-5176-45b9-b969-e902a1091bb7> |

## What was tested

A heat gun was played across an un-enclosed TMP36 on Boron-Dev-11 while the device charged over USB, with the battery drawn down beforehand so charging would be active and interruptible. The battery itself was never heated — this validates the control path (sensor → policy → PMIC), not the thermal physics of a hot enclosure.

Thresholds in force, per the arm line's own echo: `armHigh=37.0 armLow=0.0`, release 35.0 / 3.0 °C.

## Findings

### 1. The inhibit arms at the configured threshold and stops charging

At the first sample above 37.0 °C the inhibit armed and the PMIC charge state went to `OFF(0)` within 200 ms. Cell voltage fell 4.110 → 3.984 V over the next 27 s as the pack came off charge, with the device still on `power=USB_HOST(2)` throughout — so this is not a power-source artifact.

```
05:10:26.520  Enclosure temperature (effective): 37.19 C (raw=1082)
05:10:26.722  WARN: Charging inhibited due to enclosure temperature: 37.19 C (armHigh=37.0 armLow=0.0)
05:10:26.725  ChargeDiag: chg=OFF(0) ... vcell=4.110 soc=72.7
```

### 2. The hysteresis dead band holds — directly witnessed

230 ms after arming, the next sample read **36.87 °C — below the 37.0 °C arm threshold — and charging stayed off**. A design without hysteresis would have released immediately and begun chattering at the boundary.

```
05:10:26.748  Enclosure temperature (effective): 36.87 C (raw=1078)
05:10:26.773  ChargeDiag: chg=OFF(0) ... vcell=4.110 soc=72.7
```

### 3. Release uses the tighter band, not the prior-art one

The inhibit cleared at 33.73 °C, below `releaseHighC=35.0`. It was still inhibited at 39.45 °C on the way down, which rules out the v23 prior-art `isItSafeToCharge()` path — that released at 43 °C and would have cleared much earlier. The next sample after release read 34.05 °C and stayed charging, correctly holding inside the band.

```
05:13:11.658  39.53 C   chg=OFF(0)      <- prior-art would have released by now
05:14:54.004  33.73 C
05:14:54.210  INFO: Charging inhibit cleared; enclosure temperature: 33.73 C
05:14:54.210  ChargeDiag: chg=FAST(2) ... vcell=3.971
```

### 4. The v23→v24 threshold migration resolved correctly

The arm line echoes `armHigh=37.0 armLow=0.0`. On a device upgraded from v23 the appended persistent-storage fields are zero-filled, which is invalid *as a set* — `armHighC(0) > releaseHighC(0)` is false. The echo confirms `resolveStoredThermalThresholds()` substituted the compiled defaults whole rather than mixing zeroed fields with live ones.

Corroborated by behaviour: charging was still running at 35.02 °C, so the arm band is the wider one and the release threshold is not being reused for both directions.

## Trace

One wake cycle produces two samples ~0.1 s apart; both are listed. The device sleeps between cycles and evaluates only while awake, so sampling is irregular.

| Time (UTC) | °C | ADC raw | Charge | vcell | Event |
|---|---|---|---|---|---|
| 05:09:38.992 | 27.04 | 956 | FAST(2) | 4.110 | |
| 05:09:39.052 | 27.04 | 956 | FAST(2) | 4.110 | |
| 05:09:41.671 | 27.12 | 957 | FAST(2) | 4.110 | |
| 05:09:41.730 | 27.04 | 956 | FAST(2) | 4.110 | |
| 05:10:13.424 | 35.02 | 1055 | FAST(2) | 4.108 | above release thr., still charging |
| 05:10:13.476 | 35.02 | 1055 | FAST(2) | 4.108 | |
| **05:10:26.520** | **37.19** | 1082 | **OFF(0)** | 4.110 | **ARM** — `armHigh=37.0 armLow=0.0` |
| **05:10:26.748** | **36.87** | 1078 | **OFF(0)** | 4.110 | **below arm thr., held inhibited** |
| 05:10:53.194 | 37.68 | 1088 | OFF(0) | 3.984 | vcell falling off charge |
| 05:10:53.241 | 37.60 | 1087 | OFF(0) | 3.984 | |
| 05:11:15.799 | 44.37 | 1171 | OFF(0) | 3.980 | |
| 05:11:15.868 | 44.53 | 1173 | OFF(0) | 3.980 | |
| 05:11:39.911 | 50.33 | 1245 | OFF(0) | 3.977 | |
| 05:11:39.967 | 50.09 | 1242 | OFF(0) | 3.977 | |
| 05:12:09.682 | 51.05 | 1254 | OFF(0) | 3.975 | |
| 05:12:09.750 | 51.22 | 1256 | OFF(0) | 3.975 | peak — 28.8 °C below the 80 °C validity ceiling |
| 05:12:48.684 | 43.56 | 1161 | OFF(0) | 3.974 | cooling |
| 05:12:48.731 | 43.64 | 1162 | OFF(0) | 3.974 | |
| 05:13:11.658 | 39.53 | 1111 | OFF(0) | 3.973 | below prior-art 43 °C release — still off |
| 05:13:11.712 | 39.45 | 1110 | OFF(0) | 3.973 | |
| 05:13:38.573 | 37.44 | 1085 | OFF(0) | 3.971 | |
| 05:13:38.620 | 37.84 | 1090 | OFF(0) | 3.971 | |
| **05:14:54.004** | **33.73** | 1039 | **FAST(2)** | 3.971 | **RELEASE** — inhibit cleared |
| **05:14:54.231** | **34.05** | 1043 | **FAST(2)** | 3.971 | inside band, stays released |

Source: `~/dev11-thermal.log`, 1644 lines, captured via `particle serial monitor --follow --timestamp --utc | tee -a`.

## Three things that nearly invalidated the run

### The fleet pin silently reverted the build

The first flash of v24 reported success, and the device then ran v23. Dev-11 carried `desired_firmware_version=23`, so the cloud reconciled it back down on its next connect — during a long publish-queue drain, well after the flash appeared to have worked. Being marked `development: true` did **not** prevent this; the target-version field is the operative one, and it was still set even during the successful run.

### Firmware version does not identify the running build

Boron-Dev-09 reports `v23-Diag-Soak` but emits the v24-era `ChargeDiag` format, so it has been running a working-tree build for some time. Version strings on bench units are unreliable. The cheap discriminator is the log format: `ichg=` means v22/v23/main and no thermal inhibit; `vbus= pg= th= vsys=` means the v24 work is present.

This also corrects an earlier assessment. A go/no-go review of Dev-09's 36-hour window concluded from git alone that the deployed build could not contain thermal charge-inhibit. The log format shows otherwise — it very likely did. The conclusion of that review (hold, get the thermal observation) still stood, but only because the enclosure never exceeded 33.16 °C against a 37 °C arm, not because the feature was absent.

### The transition lines are one-shot, and serial has holes

Both lines fire only on a state change, and the USB CDC drops on every ULP standby — so a transition landing in an unobserved wake cycle is gone from the terminal. The arm line here was recovered only because the session was piped to a file with `tee`.

The reliable technique is in [Test 7](../bench-validation.md): let the part settle undisturbed with no PIR triggers so the device sleeps and cannot evaluate, then wave once with the monitor attached.

## Open items

- **Dev-11 firmware target still set to 23.** Clear `desired_firmware_version` or release v24 to the product, or the next cloud connect reverts the build again.
- **Verbose mode is still on.** There is no auto-expiry — `verboseModeStartTime` is written once and never read — so it stays on until `messaging.verboseMode` is set false.
- **Two unexplained power-disturbance episodes on Dev-09** (13:09 UTC 08-27, 00:08 UTC 08-28): `vbus=NONE src=VIN` with the override correcting, each alongside a `pdiag` burst and a ~5 °C drop. Worth explaining before expanding the soak.
- **`tests/reporting_policy_adapter_test.sh` is committed failing.** A `"../MyPersistentData.h"` relative include cannot be intercepted by the test's `-I` stub directory. The other 14 host tests pass, including both thermal suites.
