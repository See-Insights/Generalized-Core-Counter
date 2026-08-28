# WO-2026-08-25-001: Consolidated power management — four functions, Particle API first

Supersedes WO-2026-08-24-002 (STALE_SOC extension) and the deferred
symptom-based interlock WO. Neither proceeds independently.

## Investigation findings (complete; see chat transcript 2026-08-25 for raw evidence)

### The central finding: Device OS already does what we reimplemented

`system_power_manager.cpp:376-400`, with
`HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE (1)` confirmed for
nRF52840 (`hal/src/nRF52840/hal_platform_nrf52840_config.h:87`):

```c
auto usb_state = HAL_USB_Get_State(nullptr);
case POWER_SOURCE_USB_HOST:
  if (vin || usb_state < HAL_USB_STATE_POWERED) {
    src = POWER_SOURCE_VIN;
```

Device OS already reclassifies USB_HOST → VIN when the USB peripheral is not
at least `POWERED`. Our `PowerSourceOverride` reads `NRF_USBD->USBADDR` /
`NRF_POWER->USBREGSTATUS` and pushes it back VIN → USB_HOST. **Two layers read
the same USB peripheral state at different levels and undo each other.**

Our bar is *stricter*: `USBADDR != 0` implies `ADDRESSED`(6) or
`CONFIGURED`(7), above Device OS's `POWERED`(4). So whenever our override's
condition holds, Device OS should already have reported USB_HOST. **Every
disagreement is timing skew between its power-manager thread and our read**,
not a genuine classification difference. The Device OS workaround exists for
the inverse Gen3 problem (VIN-powered device where the BQ24195 wrongly assumes
USB host, CH34730).

**For the deployment target this is unambiguously correct behaviour.** A
solar-only device has no USB peripheral power, so `usb_state < POWERED` always
holds: spurious USB_HOST is corrected to VIN, genuine VIN stays VIN. Field
evidence agrees — both Morrisville devices reported `VIN`/`Solar` correctly for
a full week with a clean diurnal charge cycle; TRAIL02 correctly reported
`BATTERY` while discharging.

**Conclusion: `System.powerSource()` replaces register decoding entirely.
Retain zero register-level inference.**

### DISABLE_CHARGING persistence — verified from source, on-device pending

`System.setPowerConfiguration()` → `PowerManager::setConfig()` →
`hal_power_store_config()` → **`dct_write_app_data(DCT_POWER_CONFIG_OFFSET)`**
(`hal/shared/power_hal.cpp:37-52`). Non-volatile flash. `loadConfig()` runs at
Power Manager init; `handleCharging()` re-asserts continuously
(`disableCharging()` + `disableSafetyTimer()` whenever the flag is set and
charging is enabled). It writes only when the config actually changed, so no
flash wear from repeated calls.

It is therefore **more persistent than the WO assumed** — it survives reset,
power loss, *and app reflash*, because DCT is separate from application flash.

**Two consequences that shape the design:**

1. **The *uncertainty* fail-safe is not safe on solar.** A device left with the
   flag set never charges again until something clears it — on a solar field
   unit that is a guaranteed kill, the same outcome as the failure it prevents.
   The `SourceUncertain` inhibit is therefore scoped to the USB/ambiguous path
   only. **This does not apply to thermal inhibit**, which is required on every
   device including solar and is self-clearing — see "Charge inhibit — two
   reasons, different safety semantics" under F2. The distinguishing property
   is whether the inhibit has a natural clear condition, not which device it
   is on.
2. **Any `setPowerConfiguration()` call replaces the whole DCT config.** The
   flag must therefore be part of F2's own config object and re-asserted on
   each apply — it cannot be set once and forgotten. This also gives a useful
   self-healing property: restoring normal firmware clears a stuck disable on
   its first profile apply.

Bonus: `BATTERY_DISABLE_CHARGING_SUPRESS_PERIOD = 8000` — Device OS notes VCELL
only reliably reflects VBAT ~8 s after charging is disabled. Briefly disabling
charging is therefore the mechanism that yields a true resting voltage, which
is exactly what F1 needs and removes the charge-voltage bias that forced
WO-2026-08-24-002's asymmetric thresholds.

### Baseline footprint (acceptance criterion: this must go down)

| area | lines |
|---|---|
| dedicated power/battery files | 1,524 |
| `SensorManager::batteryState()` body | ~1,233 |
| **baseline surface** (excl. tier/behaviour) | **~2,757** |

Scattered decisions, quantified: **10 sites in `SensorManager.cpp`** alone
independently touch power-source/PMIC decisions, plus 3 in `PowerPlatform.cpp`
and 2 in `PowerManager.cpp`.

## Component datasheets (repo copies — cite these, not web pages)

| component | file | used for |
|---|---|---|
| Battery cell — PKCELL LIPO803860 2000 mAh (BD310021 rev A2, 2023-11-06) | [`LP803860-2000mAh-3.7V-BD310021-20231106.pdf`](../datasheets/LP803860-2000mAh-3.7V-BD310021-20231106.pdf) | charge/discharge temperature window, charge-current limits, discharge cut-off, PCM thresholds |
| Battery cell — older revision QA.S.0228 | [`PKCELL-LP803860-QA.S.0228-OLDER-REVISION.pdf`](../datasheets/PKCELL-LP803860-QA.S.0228-OLDER-REVISION.pdf) | **provenance only — do not cite for values** |
| Solar panel — Voltaic P126 (Standard) | [`P126_ds.pdf`](../datasheets/P126_ds.pdf) | deliverable charge current on standard-panel devices |
| Solar panel — Voltaic P105 (Large) | [`P105_ds.pdf`](../datasheets/P105_ds.pdf) | brief full-sun peak that `SOLAR_CHARGE_CURRENT_MA` clamps |

Figures quoted in this WO come from these files. Where a web page and a
datasheet disagree, the datasheet wins — the three corrected figures in the
thermal section came from exactly that kind of mismatch.

## Design — the four functions

### The architectural insight that makes the retired machinery unnecessary

WO-2026-08-24-002 tried to *correct the gauge* when SOC and Vcell disagreed,
and failed four Stage 7 reviews doing it (cold-boot persistence, cooldown
deferral, alert masking, no actual SOC commit).

**F1 does not correct anything. It emits a trust signal, and F3 stops depending
on SOC when trust is absent.** The desync problem is solved by not using the
untrustworthy input, rather than by repairing it in place. That removes the
latch, the retry, the cooldown, the pending state and the alert plumbing in one
move — all of which existed only to make an in-place correction stick.

### F1 — SOC / battery health

```cpp
namespace BatteryHealth {

enum class SocTrust : uint8_t { Trusted, Suspect, Untrusted };

struct Reading {
  float    soc;                 // gauge SOC as reported
  float    vcell;               // measured cell voltage
  float    restingSocEstimate;  // from the validated OCV table
  float    residual;            // soc - restingSocEstimate
  SocTrust trust;
  bool     vcellUsable;
};

/// The validated OCV table. Retained from WO-2026-08-24-002 - the only part
/// of it that survives. Knots are explicit; see tests/fixtures/.
float restingSocFromVcell(float vcell);

/// Pure. No state, no latch, no retry, no side effects.
Reading evaluate(float soc, float vcell, bool chargingActive, bool radioActive);

} // namespace BatteryHealth
```

Threshold: **20-point residual**, validated across 378 real samples with clean
noise/fault separation (median −0.9, p95 +12.9) and triangulated across three
independent resting-OCV curves plus the firmware's own linear fallback.
`chargingActive`/`radioActive` select the asymmetric band, since charge
elevates and load sags terminal voltage in opposite directions.

### F2 — split into two rounds

**F2a — charge inhibit. In scope now**, because it is the safety behaviour that
prevents the next Dev-14-class event and does not depend on source
classification at all.

**F2b — source classification.** Replacing register decoding with
`System.powerSource()`, and the `SourceUncertain` fail-safe. **Deferred to the
next round.** `PowerSourceOverride` and the existing source-selection path stay
exactly as shipped until F2b lands and soaks — per the no-protection-gap
sequencing below.

### F2 (a+b) — PowerSource → configuration

```cpp
namespace PowerConfig {

enum class Source  : uint8_t { Unknown, Vin, UsbHost, UsbAdapter, UsbOtg, Battery };

/// Input current/voltage limits only. Charge inhibit is ORTHOGONAL - see below.
enum class Profile : uint8_t { Solar, UsbBench };

/// Why charging is inhibited, if it is. The reason matters: the two have
/// different safety semantics and different clear conditions.
enum class ChargeInhibit : uint8_t { None, Thermal, SourceUncertain };

struct ApplyResult {
  Profile       profile;
  ChargeInhibit inhibit;
  Source        source;
  int           systemResult;   // System.setPowerConfiguration() return value
  bool          verified;       // read back and confirmed, not merely returned OK
};

/// System.powerSource() only. No PMIC or Nordic register decoding.
Source read();

/// Single decision point. Solar-first; USB fail-safe rather than optimised.
/// enclosureTempC drives the thermal inhibit, which applies to ALL devices.
ApplyResult apply(Source src, BatteryHealth::SocTrust trust, float enclosureTempC);

/// Reads back System.getPowerConfiguration() and compares. Closes the
/// decided-but-not-verified-applied gap from the original override fix.
bool verifyApplied(const ApplyResult &result);

} // namespace PowerConfig
```

Mapping:

| source | trust | profile |
|---|---|---|
| `Vin` | any | `Solar` |
| `UsbHost` / `UsbAdapter` / `UsbOtg` | any | `UsbBench` |
| `Unknown` / `Battery` | any | last known good |

`apply()` re-asserts the full config each call (see persistence finding 2).
`verifyApplied()` retries once, then reports; it does not silently continue.

#### Charge inhibit — two reasons, different safety semantics

Charge inhibit is **orthogonal to the input profile**. A device can be on the
`Solar` profile with charging inhibited; the profile still governs input limits
for when charging resumes.

| reason | applies to solar? | clears how? |
|---|---|---|
| `Thermal` — cell outside the safe charge window | **YES — required** | self-clearing with hysteresis when temperature returns to range |
| `SourceUncertain` — source unclassifiable on a USB-capable build | **never** | latching, no natural clear condition |

**This is the distinction that reconciles the "never on solar" rule.** The
uncertainty fail-safe is unsafe on solar because it latches with nothing to
clear it — indefinite non-charging is death. Thermal inhibit is transient and
self-clearing, and *not* inhibiting is the unsafe option: charging a LiPo
outside its window risks the cell.

**Thermal inhibit is a required safety behaviour on every device, solar
included.**

#### Prior art — the field-proven pattern to adopt

Chip supplied working code from an earlier project, in service for some time:

```cpp
bool Take_Measurements::isItSafeToCharge() {
  PMIC pmic(true);
  if (current.get_internalTempC() < 0 || current.get_internalTempC() > 37) {
    pmic.disableCharging();          // too cold or too hot to charge safely
    current.set_batteryState(1);     // reflect "Not Charging"
    current.set_alertCode(10);
    return false;
  }
  pmic.enableCharging();
  return true;
}
```

**Adopt the pattern:** evaluate temperature on *every* battery measurement and
gate charging on it. That cadence is what makes a transient inhibit
self-clearing without any latch — the same reasoning that makes F1 stateless.

Note also `batteryState()` in that project returns true when
`stateOfCharge == -1`, with the comment "bad battery reading should not put
device in low power mode". That is F1's trust concept in primitive form, and
independent validation of the direction.

#### Mechanism correction — the bare PMIC call is a race on Device OS 6.4.1

`pmic.disableCharging()` alone is **undone by Device OS**. When
`HAL_POWER_CHARGE_STATE_DISABLE` is absent from its own config,
`PowerManager::handleCharging()` re-enables unconditionally
(`system_power_manager.cpp:306-313`):

```c
if (!power.isChargingEnabled()) {
    power.enableCharging();
    power.enableSafetyTimer();
}
```

That runs on the power-manager thread each `battMonitorPeriod_`. The prior-art
code survives only because it re-asserts on every measurement and wins the race
often enough; between re-assertions there is a real window in which a hot cell
charges.

**Therefore: thermal inhibit must use
`System.setPowerConfiguration(...feature(DISABLE_CHARGING))`, not the bare PMIC
call.** Device OS checks that flag first and will not re-enable behind it.
Re-assertion every measurement still applies, and on boot the first measurement
clears the inhibit if the board has cooled — so persistence across reset is
bounded, not open-ended.

#### Threshold — the prior art's 37 °C is expensive on this fleet

Modelled against 516 distinct Morrisville reports, week of 2026-08-16:

| ceiling | reports inhibited |
|---|---|
| > 37 °C (prior art) | 119 — **23.1%** |
| > 40 °C | 76 — 14.7% |
| > 42 °C | 57 — 11.0% |
| > 45 °C (LiPo spec) | 25 — 4.8% |

Concentrated in exactly the wrong hours. Share of reports above 37 °C by local
hour: **12:00 67%, 13:00 60%, 14:00 65%, 15:00 68%, 16:00 55%, 17:00 38%,
18:00 30%** — 06:00–10:00 and 20:00–21:00 are unaffected.

A 37 °C ceiling therefore inhibits charging through most of the peak solar
window, on a fleet whose measured net surplus is only **+1.5 to +2.0 points per
day** and where one heavy evening cost MAFC-2 4.35 points and took three days
to repay. That margin cannot absorb losing midday.

**DECIDED 2026-08-25 (Chip): ship at 37 °C.** The relaxed-ceiling question does
not block the thermal fix. Losing peak midday sun is a real but *bounded and
understood* cost; shipping a race condition, or an unverified assumption about
the exact mechanism meant to prevent the next Dev-14, is neither.

Thresholds to implement:

| | arm | release |
|---|---|---|
| ceiling | **> 37 °C** | ≤ **35 °C** |
| floor | **< 0 °C** | ≥ **3 °C** |

The **arming** thresholds are exactly the field-proven values (0 and 37) — this
design is never less conservative than the code already working in the field.
Hysteresis is added only on the *release* side, to stop chatter at the boundary;
the prior art had none. Both **ledger-configurable and per-device overridable**,
per the tier philosophy, so a later relaxation is a threshold change and not a
code change.

**Follow-up, explicitly out of scope here:** relaxing the ceiling. The sensor
reads *enclosure/board* temperature and in direct sun the enclosure is very
likely hotter than the cell, which has thermal mass and sits inside — so an
enclosure ceiling could sit above the cell's own limit without the cell
exceeding it. That is an assumption, not a measurement. Validating it with a
cell-adjacent probe and relaxing the ceiling is a small, well-scoped change to
one ledger-configurable value.

**Required before implementation:** model the energy impact of the chosen
ceiling against `tests/fixtures/` plus the Morrisville SOC series, and confirm
the daily surplus stays positive. A thermal inhibit that protects the cell but
flattens the energy budget has traded one failure mode for another.

#### Datasheet evidence for the ceiling (added 2026-08-25)

Cell: **PKCELL LIPO803860 2000 mAh 3.7 V (BD310021)**, "Li-Polymer Battery
Technology Specification", drafted 2023-10-17, **revision A2 dated
2023-11-06** — supplied by Chip 2026-08-25 and read directly from the document.

**Authoritative copy, committed to the repo:**
[`docs/datasheets/LP803860-2000mAh-3.7V-BD310021-20231106.pdf`](../datasheets/LP803860-2000mAh-3.7V-BD310021-20231106.pdf)
— 9 pages, 541,428 bytes, sha256
`e57c7ceccbb80161a3ba29622bd2397682046548b53e00a967224f1ed9c78e9b`.
All figures in this section are read from that document.

**Revision warning.** A copy retrieved independently from
`cdn-shop.adafruit.com` is document **QA.S.0228**, a *different and older
5-page revision*, retained at
[`docs/datasheets/PKCELL-LP803860-QA.S.0228-OLDER-REVISION.pdf`](../datasheets/PKCELL-LP803860-QA.S.0228-OLDER-REVISION.pdf)
for provenance only. **Do not cite it for values** — the two revisions differ,
and conflating them is what produced the three errors corrected below.

**Corrections to the first draft of this subsection.** Three figures recorded
on 2026-08-25 were wrong and are corrected here:

| item | first recorded | **actual (BD310021 A2)** |
|---|---|---|
| 5 — Standard Charge | 0.5C (1000 mA) | **0.2C** (≈400 mA), CV 4.2 V, 0.01C cut-off, ~8 h |
| 6 — Max Constant **Charging** Current | 1500 mA | **1000 mA** |
| 8 — Max Continuous **Discharging** Current | *(not recorded)* | **1500 mA** |

The 1500 mA figure was the **discharge** limit misattributed as the charge
limit. That error propagated into the thermal-headroom argument, which is
corrected below.

**Verified thermal evidence:**

- **Item 9, Operating Temperature — charge 0–45 °C, discharge −20–60 °C.**
  Confirmed. Manufacturer hard operating spec, distinct from the marketing
  guide's 0–30 °C longevity recommendation.
- **Item 6.3.1, High Temperature reliability test** — fully charged, rest at
  **60 ± 2 °C for 2 hours**; requirement "electrochemical performance, visual
  test not changed". Confirmed.
- **Item 6.1.6, High Temperature Characteristics** — fully charged, stored at
  **55 ± 2 °C for 2 hours**, then discharged: **≥1600 mAh retained** of 2000.
  A second elevated-temperature data point, with a *quantified* capacity
  outcome.
- **Item 6.3.10, Thermal shock** — ramped to **50 ± 2 °C, held 30 min**: no
  fire, explosion or leakage.
- **Item 10, Storage Temperature** — −20–45 °C for 1 month; −10–35 °C for
  6 months.

#### The charge-current argument was backwards — it cuts the other way

The first draft argued that this fleet charges at "a small fraction of standard
rate", so lower internal heating justifies extra thermal margin. **That is not
supported by the configuration.**

| | configured | datasheet |
|---|---|---|
| `SOLAR_CHARGE_CURRENT_MA` | **900 mA** | standard 400 mA (0.2C); max 1000 mA |
| `USB_BENCH_CHARGE_CURRENT_MA` | **896 mA** | as above |
| `SOLAR_CHARGE_VOLTAGE_MV` | **4208 mV** | charging cut-off **4.2 V** |
| `USB_BENCH_CHARGE_VOLTAGE_MV` | 4112 mV | under cut-off |

The configured charge current is **2.25× the datasheet standard rate and 90% of
the absolute maximum** — not a small fraction of anything. At 900 mA into a
cell specified for 400 mA standard, internal heating is *higher*, which argues
for **less** thermal headroom above 45 °C, not more. Delivered current on solar
is panel-limited and often far below the configured ceiling, but the
configuration itself is not conservative and cannot be used as a margin
argument.

#### Panel capability — the regime is panel-dependent

Fleet panels (Voltaic Systems), supplied by Chip 2026-08-25. Datasheets
committed to the repo:

| | part | datasheet | peak power | Vmp | **Imp** |
|---|---|---|---|---|---|
| Standard | **P126** | [`P126_ds.pdf`](../datasheets/P126_ds.pdf) | 2.3 W | 7.3 V | **330 mA** |
| Large | **P105** | [`P105_ds.pdf`](../datasheets/P105_ds.pdf) | 4.6 W (listing) / 5.75 W (product page) | 5.4 / 6.12 V | **840 / 940 mA** |

*(The two Voltaic sources disagree on P105 — 4.6 W/840 mA in the comparison
table, 5.75 W/940 mA on the product page. Likely different test conditions or
revisions. Both are used below as a range.)*

The BQ24195 is a buck charger, so panel watts convert to battery current at
roughly 90% into a ~4.0 V cell:

| panel | available charge current | vs configured 900 mA | vs cell standard 400 mA |
|---|---|---|---|
| **P126** | ≈ **520 mA** | never reached — **panel-limited** | ≈1.3× |
| **P105** | ≈ **1035–1295 mA** | **exceeds it — config is binding** | **2.25×** |

**This splits the earlier finding — and then largely retires it.**

**Deployment context (Chip, 2026-08-25):** standard panels are the default;
large panels are fitted **only where shade from trees or other blockage makes
them necessary**, i.e. deliberately poorly-lit sites. Such a site can still see
**brief periods of full sun**, and in that window a P105 could deliver
~1035–1295 mA into a cell whose absolute maximum is **1000 mA**.

**`SOLAR_CHARGE_CURRENT_MA = 900` is therefore a protective clamp, not a slack
ceiling.** It exists precisely to hold a large panel's brief full-sun peak
below the cell's 1000 mA maximum, with 100 mA of margin. The earlier
characterisation of it as "not conservative" was wrong: it is the one setting
standing between a shaded-site large panel and an over-current charge event.
**Keep it as is.**

Consequences, corrected:

- **The per-panel-type charge current suggested in the previous draft is
  withdrawn.** Lowering it for large panels would defeat its protective
  purpose; lowering it for standard panels would achieve nothing, since they
  are panel-limited to ~520 mA and never approach it. One setting is correct.
- **MAFC-1 and MAFC-2 are both standard panels.** Every temperature and
  charging measurement in this WO therefore comes from devices limited to
  ~520 mA — roughly **1.3× the cell's 400 mA standard rate**, which is modest.
  The internal-heating objection to relaxing the thermal ceiling is
  correspondingly **weak for the devices we actually have data on**.
- **The "charges hardest when hottest" compounding argued in the previous draft
  does not match the deployment.** Large panels are in shaded sites, so they do
  not systematically charge hardest at peak ambient. Withdrawn.

**Genuine remaining gap:** there is **no temperature data from any large-panel
device**. A shaded-site unit catching brief full sun while already warm is the
one case where high charge current and elevated temperature could coincide, and
it is entirely unmeasured. This is a monitoring item, not a design change —
worth capturing enclosure temperature and charge current together on one
large-panel device before the thermal ceiling is relaxed.

**Two incidental findings, both worth their own follow-up:**

1. `SOLAR_CHARGE_VOLTAGE_MV = 4208` exceeds the datasheet's **4.2 V** charging
   cut-off by 8 mV. Below the PCM's 4.25 ± 0.05 V overcharge detection, so no
   protection trip — but it is above the stated cell spec, on the solar path,
   permanently.
2. The pack's PCM **overdischarge detection is 2.5 ± 0.1 V**. Boron-Dev-14
   reached 3.069 V — well above that — so **pack protection never engaged**.
   The device browned out on its own; the cell was never electrically
   protected, and its subsequent full recovery is consistent with that.
   It also confirms TRAIL02's 3.0 V discharge-test stop matches the datasheet's
   own discharge cut-off (item 4) exactly.

#### Three sourced candidates

| ceiling | source | character |
|---|---|---|
| **30 °C** | PKCell marketing guide | longevity best-practice; most conservative |
| **37 °C** | field-proven prior-art code; **shipped as F2a** | working, but below both real sources |
| **45 °C** | PKCell datasheet item 9 | manufacturer hard spec; best-evidenced |

#### Energy-impact model, all three candidates

Run against 516 distinct Morrisville reports, week of 2026-08-16 — the same
series used for the earlier 37/42 comparison:

| ceiling | reports inhibited | share of all | share of the 11:00–17:00 charging window |
|---|---|---|---|
| > 30 °C | 334 | 64.7% | **86.7%** (156 of 180) |
| > 37 °C | 119 | 23.1% | **51.1%** (92 of 180) |
| > 45 °C | 25 | 4.8% | **12.8%** (23 of 180) |

Peak observed enclosure temperature **48.7 °C** — 3.7 °C above the datasheet
charge ceiling, and **11.3 °C below** the 60 °C rest condition the cell passed
in item 6.3.

Reading: 30 °C would suppress charging through essentially the whole solar
window and is not viable on this fleet. 37 °C — what is shipping — costs about
half the window. 45 °C costs about an eighth and is the option with
manufacturer backing.

#### Enclosure thermal coupling — the proxy is better than first assumed

Chip, 2026-08-25: the deployment uses a **small natural-coloured plastic
enclosure** with the **carrier board and the battery within 3 cm of each
other**, at low discharge rates.

That materially narrows the enclosure-vs-cell gap this WO has been flagging.
With low power dissipation in both parts and only 3 cm of separation inside a
small volume, the dominant heat source is external (solar gain on the
enclosure) and heats the whole interior together rather than one component;
board and cell should equilibrate close to the same temperature. Natural
(light) coloured plastic also absorbs less solar energy than a dark enclosure,
which lowers the internal peak for a given insolation.

Residual biases, both small at this fleet's currents and in opposite
directions, so they partly cancel:

- **modem transmit bursts** (~800 mA) heat the *board* locally, so the sensor
  may read transiently warmer than the cell;
- **charging** heats the *cell*, so the cell may read warmer than the board —
  though at a small fraction of the 1000 mA standard rate this is minor.

Net: board temperature is a reasonable stand-in for cell temperature here, not
a loose guess. This strengthens the case for the 45 °C datasheet ceiling —
though a cell-adjacent measurement is still the clean way to close it, and the
argument above is reasoning from construction, not a measurement.

**This is evidence to inform a future deliberate decision. It authorizes
nothing.** F2a ships at 37 °C exactly as agreed. Moving the ceiling is its own
follow-up decision, and the remaining gap is that all of the above is
*enclosure* temperature: the cell-adjacent measurement is still the missing
input, and it is what would justify treating a 45 °C enclosure ceiling as
compliant with a 45 °C *cell* spec.

#### What exists today: nothing proactive

- **No thermal gating exists in the power path.** Verified: no temperature
  input anywhere under `src/power/`.
- The only thermal handling is **reactive and after the fact** — the BQ24195's
  own *die* thermal shutdown surfaces as `REG09 CHRG_FAULT == 0x02` and the
  firmware logs it and raises alert 20 (`SensorManager.cpp:1001-1003`). That
  protects the chip, not the cell, and only once it has already tripped.
- **The NTC / battery-thermistor fault bits (REG09 bits 2:0) are not decoded
  at all**, so pack-temperature protection via the PMIC TS pin is neither used
  nor observed.
- The available temperature is **enclosure temperature** (TMP112A, TMP36
  fallback — `SensorManager.cpp:1928` logs it as "Enclosure temperature
  (effective)"). It is a *proxy* for cell temperature, not cell temperature.
  In direct sun the enclosure likely reads hotter than the cell, which makes it
  conservative on the hot side; that should be stated rather than assumed away.

#### This is not hypothetical on the deployment fleet

Morrisville enclosure temperatures, week of 2026-08-16:

| | |
|---|---|
| peak observed | **48.7 °C** (MAFC-2, 2026-08-17) |
| samples ≥ 45 °C (typical LiPo charge limit) | **25** of 518 |
| samples ≥ 42 °C | **57** of 518 |

The solar field devices already operate above the safe charge window during
midday, with no proactive inhibit, relying solely on a die-temperature
shutdown that protects the PMIC rather than the pack. This also matches the
observed charging profile: MAFC-2's best charging hours were 17:00–18:00 local,
*after* the heat came off, while 14:00–16:00 were depressed despite more sun.

#### Cross-function interaction

While `ChargeInhibit::Thermal` is active, not charging is the *expected* state.
F1 and F3 must not read non-charging during thermal inhibit as evidence of a
fault, and the 6-hour stuck-charging detector must be suppressed for its
duration.

### F3 — Power state / tier

```cpp
namespace PowerTier {

enum class Tier : uint8_t { Full, Reduced, Low, Critical };

/// Critical is gated on vcell (<= 3.5 V), NOT on soc, per the WO-002/003
/// SOC-desync findings. When trust != Trusted, evaluation is vcell-only -
/// this is where F1's signal is consumed and why no gauge correction is needed.
Tier evaluate(float vcell, float soc, BatteryHealth::SocTrust trust);

} // namespace PowerTier
```

Phase 2's design, reused unchanged: ledger-configurable thresholds,
per-device overridable.

### F4 — Behaviour from power state

```cpp
namespace PowerBehavior {

struct Policy {
  uint32_t reportIntervalSec;
  uint32_t connectBudgetSec;
  uint8_t  connectivityMultiplier;
  bool     cellularEnabled;
};

Policy forTier(PowerTier::Tier tier, bool windowOpen);
void   apply(const Policy &policy);

} // namespace PowerBehavior
```

Phase 2's design, reused unchanged: connectivity multipliers, local-time-gated
check-ins.

## Retired, folded in, or kept

- **STALE_SOC latch machinery** (`SOC_HIGH_FOR_VCELL_*`) — retired. Currently
  uncommitted, so `git checkout` discards it; no revert commit needed. **Keep**
  the predicate, the OCV table, and `tests/fixtures/battery-soc-vcell-samples-2026-08.csv`.
- **Symptom-based interlock** — superseded, with one residual: verify-applied.
  Folded into `PowerConfig::verifyApplied()`.
- **6-hour stuck-charging detector** — folded in. Keep the detection; route its
  output into F1's trust signal instead of a standalone alert. Its
  non-retained-state vulnerability then stops mattering, because it is no
  longer the primary signal.
- **PowerDiag / ChargeDiag** — **kept.** They are what made this investigation
  possible.
- **The `pdiag` batch apparatus** — reconsider for removal. It dropped a
  reason-12 entry under its own creator's full-batch probe during Stage 7 on
  2026-08-24. Capacity 12 with silent drop is not a reliable diagnostic
  channel; the serial path plus a ledger-backed field would be.

## Sequencing (no protection gap)

The shipped `PowerSourceOverride` exists only on Dev-09/11/14 (v23). Morrisville
and TRAIL02 run v21 and never had it. **There is no protection to gap on the
solar fleet.**

1. Land F1 + F3 + F4 **+ F2a (thermal charge inhibit)**. Solar-relevant; zero
   interaction with the override or with source classification.
2. Land F2b — `System.powerSource()` + the USB-only fail-safe — **override
   still present**.
3. Soak on bench. If F2 is correct the override should never fire — a positive
   falsification test, not an assumption.
4. Retire the override after N days of zero firings with F2 active.

## Acceptance Criteria

- Net lines in the power-management area decrease against the ~2,757-line
  baseline above, measured on the same files.
- Each of the four functions exists as one clearly-identifiable point; the 15
  scattered decision sites collapse into F2.
- Solar-path correctness verified against real Morrisville telemetry.
- USB path is simple and fail-safe, not elaborately correct.
- No regression to Phase 2's Critical-tier design.
- No protection gap during the transition.
- `DISABLE_CHARGING` for **source uncertainty** never armed on a solar-configured
  device; `DISABLE_CHARGING` for **thermal protection** armed on every device
  including solar, with hysteresis and verified self-clearing.
- Thermal inhibit verified against real Morrisville temperature data
  (25 samples ≥ 45 °C in the reference week).
- Every claim verified against the compiled artifact, per standing guardrail.

## Open — blocks final design sign-off

**On-device `DISABLE_CHARGING` persistence test.** Authorized 2026-08-25.
Test firmware built and safety-hardened at `/tmp/dctest/` (self-driving via
retained phase counter, stays cloud-connected so it remains OTA-recoverable,
always ends with the flag cleared, serial observable via the Pi forwarder).

**Blocked:** Boron-Dev-09 entered cellular-acquire failure at 2026-08-25
02:11Z and 03:11Z — `ConnSummary: fail elapsed=660001 last=CELLULAR_ACQUIRE
sig=0/0`, modem reset on network-registration timeout. It cannot be reached for
OTA, and flashing a queued image to a device that cannot connect risks leaving
it unrecoverable without physical access. Needs either USB/DFU access (the
device is on the Pi) or for the cellular fault to clear first.

**RESOLVED 2026-08-25 (Chip):** the battery pack has **no NTC** — the
thermistor is on the **carrier board**. So no cell-temperature hardware
protection exists to defer to; any TS-pin reading would be board temperature,
the same proxy quality as the TMP112A. **Firmware gating on measured
temperature is the only protection available**, which is what the prior-art
pattern above does. The remaining open item is the *threshold*, not the
mechanism.

## Status

**Design-first, awaiting review.** No implementation dispatched. The two
superseded WOs are held.

---

# Amendment B (2026-08-25) — architectural decisions, post Codex Stage 7

Codex returned **Not verified** on the round-2 implementation
(`/tmp/wo26/CODEX_STAGE7.md`). Two of the four blockers were not implementation
defects: they were **architectural questions this WO left unsettled**, which the
implementer then resolved on its own and recorded in a header comment. This
amendment settles them before round 3, so the implementer is not deciding them
again.

This is the second time this WO has been caught by a decision that looked like
an implementation detail and was not — the first was the line-count measurement
scope, where the baseline file set was never written down and two parties
measured differently (490 vs 169). The pattern is the point: **if a choice
changes what the firmware does, or how we judge whether it worked, it belongs
here and not in a header.**

## Decision B1 — F3/F4 guard the legacy tier path; they do not replace it

### The unsettled question

The WO said "Phase 2's design, reused unchanged" (lines 594, 615) and also
specified F3/F4 as new functions, without saying whether the new tier system
**replaces** `BatteryBackoff` for runtime cadence or **supplements** it. Codex
found F3/F4 with zero production callers and garbage-collected from the image.
That was not an oversight — `PowerBehavior.h` states the rewire was judged "out
of scope for this round," and `PowerTier.h` describes itself as "a second,
purely advisory signal."

The implementer answered the question correctly *as an implementer* — it
declined to rewire the state machine unasked. The failure was the WO's, for
shipping a spine with no specified endpoint.

### What the legacy path actually does today

`BatteryBackoff::calculateTier(currentSoC, previousTier)` is consumed at:

- `src/reporting/RuntimeReportingPolicy.cpp:19-23` — reporting cadence, reading
  the persisted previous tier;
- `src/Generalized-Core-Counter.cpp:1584` `applyBatteryAwareConnectionModePolicy()`
  — connection-mode policy;
- `src/Generalized-Core-Counter.cpp:333` `currentBatteryTierForFailsafe()`;
- `sysStatus.currentBatteryTier` — persisted across resets.

It carries **hysteresis dead-zones at 70–75, 50–55, and 30–35** to stop the
reporting cadence chattering across a boundary.

### Why replacement is the wrong answer

`PowerTier::evaluate()` is pure and takes **no previous-tier argument**, so it
has no hysteresis. Wiring it into reporting cadence in place of
`BatteryBackoff` would reintroduce exactly the chatter Phase 2's dead-zones
exist to prevent — a regression, and one the acceptance criterion "no
regression to Phase 2's Critical-tier design" (line 658) already forbids. It
would also duplicate the same 75/70/55/50/35/30 breakpoints in two places, free
to drift apart.

### The decision

**`BatteryBackoff` remains the single authority for tier, hysteresis, and
persistence.** F3 does not become a parallel tier system. Instead, the two
things F3 genuinely adds — which `BatteryBackoff` does not have — are applied
as a **guard on its input and a floor on its output**:

1. **Trust-gated input.** Today `RuntimeReportingPolicy` feeds
   `current.stateOfCharge` straight into `calculateTier()`. That is the defect
   Dev-14 exposed: a wrong SOC (0.4 % → 97.8 %) silently drove real behavior.
   Round 3 routes the SOC input through F1 first — when
   `BatteryHealth::evaluate()` returns a trust other than `Trusted`, substitute
   `BatteryHealth::restingSocFromVcell(vcell)` for the gauge SOC. Hysteresis,
   persistence, and the breakpoints are untouched.
2. **Unconditional vcell floor.** `PowerTier::kCriticalVcell` (3.5 V) clamps the
   resulting tier to at least `TIER_SURVIVAL` regardless of SOC or trust. A
   low-but-plausible SOC during a real brownout must not be able to report
   "healthy."

This makes F1 **load-bearing** rather than advisory — which was the WO's actual
purpose — without touching the cadence machinery.

### Consequences for F3 and F4

- `PowerTier::evaluate()` is retained for the vcell floor and the untrusted-SOC
  substitution, and must gain a production caller on the path above. Its
  duplicated SoC breakpoints must be **derived from or asserted equal to**
  `BatteryBackoff`'s, not independently restated.
- `PowerBehavior` as specified — a parallel cadence/connect-budget/multiplier
  mapping — **duplicates `BatteryBackoff::intervalMultiplier()` and is
  withdrawn.** `PowerBehavior::forTier()`'s numbers are already Phase 2's own,
  restated. Round 3 should delete `PowerBehavior.{h,cpp}` and its test, or
  reduce it to the single place that applies the resolved policy if the
  implementer finds a real consumer that has none today. Deleting it is the
  expected outcome and is **not** a line-count optimisation; it is removal of a
  duplicate authority.
- `PowerBehavior::apply()` as advisory logging is withdrawn outright. Logging a
  policy nothing applies is the failure mode this amendment exists to prevent.

**Rejected alternative:** giving `PowerTier::evaluate()` a previous-tier
parameter so it can carry its own hysteresis and replace `BatteryBackoff`
wholesale. This is defensible and would be cleaner in isolation, but it means
rewriting cadence, connection-mode, and failsafe selection plus the persisted
`sysStatus.currentBatteryTier` — materially larger and riskier than the rest of
this WO combined, on a fleet with two devices already in a degraded state. If
we want it, it is its own WO with its own soak, not a round of this one.

## Decision B2 — the thermal inhibit may never arm from an unmeasured temperature

### The unsettled question

The WO specified thresholds and mechanism but never stated what happens when
**no valid temperature has been measured yet this boot**. Codex found the
resulting path: `isItSafeToCharge()` (`SensorManager.cpp:1013`) reads
`current.get_internalTempC()` — a **filesystem-persistent** value — and its
`inhibited` latch is a plain function static, not retained. Called at
`SensorManager.cpp:695` before a fresh TMP36 average completes, a stale 38 °C
re-arms the DCT flag on every boot; closed-hours hibernate resets the 8-sample
counter to zero each cycle, so a physically cool device can carry
`DISABLE_CHARGING` indefinitely.

`DISABLE_CHARGING` survives reset, power loss, and reflash. **This is the
mechanism that killed Dev-14's pack, reached by a new road.**

### The decision

Temperature acquisition must carry an explicit **validity state**, distinct from
its value:

- **A stale or unmeasured temperature may not ARM the inhibit.** Arming requires
  a reading measured this boot. This is the load-bearing rule.
- **A stale or unmeasured temperature may not silently HOLD an armed inhibit
  either.** If the DCT flag is set and no fresh measurement is available, that
  state must be visible in telemetry rather than persisting unobserved.
- **Release remains permitted on a fresh reading alone** — releasing on good
  evidence is the safe direction; the cell is protected by the 4.2 V cut-off and
  the 900 mA clamp independently of this path.
- Shortening the sample window is **not** an acceptable fix. It reduces the
  probability of the bug without removing the property that causes it: a device
  can arm from a temperature it did not just measure.

The implementer chooses the mechanism (a retained validity flag, a boot-scoped
sample counter, an explicit `Unknown` temperature state) and must state its
failure modes. If the safe fix is architectural rather than local, say so
rather than forcing a local patch.

### Threshold validation

Ledger-supplied thresholds are currently accepted anywhere in −40…85 °C. Round 3
must additionally enforce `armHigh > releaseHigh`, `armLow < releaseLow`, and a
hard ceiling at the cell's **45 °C** charge maximum
(`docs/datasheets/LP803860-2000mAh-3.7V-BD310021-20231106.pdf`). A malformed
ledger must not be able to invert the hysteresis or configure an unsafe ceiling.

## Decision B3 — scope narrowing is escalated, never documented and shipped

`PowerBehavior.h` and the round-2 implementation report both **correctly
recorded** that the state-machine rewire had been dropped. Nobody read them in
time, and the artifact shipped with a dead spine that passed every test.

For this WO and future ones: when an implementer concludes that part of a
specified design should not be built, that is a **Stage 6 stop**. Report it and
halt for a decision. Do not build the reachable subset, document the gap in a
header comment or a known-limitations section, and report success. A recorded
scope reduction is still a scope reduction, and prose in a header is not a
decision anyone approved.

The corresponding verification change — proving a production call site and an
ELF symbol for every new module — is now a permanent standard in
`AI_DEVELOPMENT_WORKFLOW.md` (Stage 7, "Mandatory: linkage verification"),
alongside a retirement-replacement check prompted by the Boron `stateOfCharge`
regression.

## Amended acceptance criteria

These **supersede** the corresponding items in the original Acceptance Criteria:

- **AC-B1.** `BatteryBackoff` remains the sole tier authority. No second tier
  system reaches a production consumer, and the 75/70/55/50/35/30 breakpoints
  exist in exactly one place or are asserted equal by test.
- **AC-B2.** F1's trust signal demonstrably changes the tier fed to
  `RuntimeReportingPolicy` when trust is not `Trusted` — with a test proving an
  untrusted SOC does not drive cadence.
- **AC-B3.** The vcell floor clamps the tier to at least `TIER_SURVIVAL` at
  ≤ 3.5 V regardless of SOC or trust, with a test.
- **AC-B4.** An accepted Boron fuel-gauge sample reaches
  `current.stateOfCharge`, with an integration test. (Round 2 shipped this
  regression with a fully green suite.)
- **AC-B5.** The thermal inhibit cannot arm from a temperature not measured this
  boot, with a test covering the reset/hibernate path.
- **AC-B6.** Ledger thresholds enforce arm/release ordering and the 45 °C
  ceiling, with a test for malformed input.
- **AC-B7.** Every module retained by round 3 has a production call site and an
  `nm`-visible ELF symbol, evidenced in the report. Modules that cannot earn
  one are deleted, not retained as advisory.
- **AC-B8.** The net-line-count criterion is **withdrawn for round 3.** The
  +169 overage is accepted (line 650's criterion is satisfied in substance by
  `batteryState()` 1,233 → 557). Correctness is not to be traded for lines.

## Documentation correction

The solar panel figures in this WO do not match the committed datasheets. Codex
measured P105 at approximately **4.61 W expected at 4.69 V / 0.98 A**
(5.51 W nominal), against this document's 4.6/5.75 W, 5.4/6.12 V, 0.84/0.94 A
columns; P126 is close but also inexact (~2.31 W at 6.84 V / 0.29 A). The
energy-impact conclusion is unaffected — the 900 mA clamp remains a protective
ceiling below the cell's 1000 mA maximum — but the cited figures must be
corrected against `docs/datasheets/P105_ds.pdf` and `P126_ds.pdf`. Tracked
separately from round 3; no firmware dependency.

---

# Amendment C (2026-08-26) — thermal coupling, guard totality, PMIC completion

Codex Stage 7 on round 3 returned **Not verified**
(`/tmp/wo26/CODEX_STAGE7_R3.md`). Blocker A — zero-initialised thresholds — was
split out, fixed, and independently verified on 2026-08-25. This amendment
covers the remaining findings B–E.

Decision C1 exists because the thermal path has now failed Stage 7 **twice**.
Amendment B stated the *property* required ("may never arm from an unmeasured
temperature") and left the mechanism to the implementer. The result satisfied
the literal rule and left the field property unestablished. This amendment
states the **mechanism**, not just the property.

## Decision C1 — measurement and evaluation must be structurally coupled

### What is actually wrong today

All of this lives inside `SensorManager::batteryState()` (line 463):

- **`isItSafeToCharge()` is called at line 703**, *before* the temperature
  measurement block at lines 890–1040. It therefore evaluates against whatever
  value is already in storage — from an earlier call, or an earlier boot.
- **The TMP36 path early-returns at line 990** on samples 1–7, never reaching
  the evaluation at line 1039.
- **The two halves communicate through `current.internalTempC`** —
  filesystem-persistent storage — rather than through a passed value. The
  measurement writes it (lines 942, 961, 1027); the evaluation reads it back
  independently (line 1046).
- There are **three** evaluation call sites (703, 973, 1039) and any of them can
  run with no measurement having occurred.

The defect is not merely ordering. Two functions that happen to run back-to-back
would still be unsafe here, because the second reads its input from durable
storage that survives reboots rather than from the first's result.

### The requirement

**One function performs the temperature read and, in the same call and without
returning, evaluates and applies the charge decision from the value it just
read.**

Precisely:

1. **One entry point.** There must be no way to obtain a temperature reading
   without the charge decision being evaluated and applied in the same call, and
   no way to evaluate the charge decision except as part of that call. Neither
   half may retain a separately callable public entry point.
2. **The evaluation consumes the just-read value directly** — passed as a
   parameter or held in a local — and **must not** re-read
   `current.internalTempC` or any other persisted field to obtain the
   temperature it evaluates. Persisting the reading for telemetry is fine; that
   persisted copy must not be the evaluation's input.
3. **No early return between the two halves.** The sample-accumulation path at
   line 990 must not be able to return with a temperature read but no decision
   applied. If a partial sample cannot yield a usable reading, the call must
   still reach the decision and treat the temperature as **unmeasured** — which
   under Amendment B's rule cannot arm.
4. **The coupling must be structural, not conventional.** It must not be
   possible for a later edit to separate the halves without the code failing to
   compile or a test failing. Two private functions called in sequence inside a
   third do not satisfy this if either can be called independently.

Whether validity remains a boolean, becomes part of the returned reading, or is
expressed some other way is the implementer's choice — but validity must travel
**with the value**, not as separate state that can drift from it. Blocker D
(`_temperatureMeasuredThisBoot` set once and never invalidated after a later
sensor failure) is a direct consequence of validity living apart from the
reading, and is closed by this coupling rather than by a separate patch.

### Confirmed: no early-boot readiness race on the measurement side

Chip confirmed 2026-08-26 that the TMP36 read has **no I²C, initialisation, or
settling dependency** — it is an ADC read on `TMP36_SENSE_PIN`. There is
therefore no readiness race that would justify deferring the measurement half,
and no reason the coupled call cannot run as early as the first
`batteryState()`. (The TMP112A path is I²C; where both exist, the TMP112 branch
must satisfy the same coupling or defer to the TMP36 path, not bypass the
decision as line 973 does today.)

### Boot-ordering corollary (Blocker C)

`PowerManager::setup()` → `refreshInputProfile()` applies a DCT configuration
constructed **without** `DISABLE_CHARGING` (`PowerPlatform.cpp:79-98`), and
`System.setPowerConfiguration()` replaces the entire configuration. This runs
*before* the one-time DCT sync, so a reboot while physically hot clears a
persisted inhibit.

Once C1 holds, the first coupled call re-establishes the correct state from a
real measurement. That is the fix. But the window between the input-profile
write and the first coupled call must be **bounded and stated** — the
implementer must say how long charging can be enabled on a hot device after
reboot, and why that bound is acceptable. If the honest answer is "unbounded in
short hibernate cycles," that is a Stage 6 stop, not a footnote.

## Decision C2 — the guard must be total over voltage states (Blockers B)

`RuntimeReportingPolicy.cpp:35-47` skips **both** the trust substitution and the
3.5 V floor when `cachedBatteryVoltage()` returns false — which it does for
vcell ≤ 2.5, ≥ 5, NaN, or before the first sample. Those are precisely the
readings the guard exists to catch. AC-B2 and AC-B3 therefore fail in
production while their unit tests pass, because the tests call the guard
directly and never exercise the adapter.

The guard must be **total**: every voltage state — Known, Invalid, Unavailable —
must have a defined, tested outcome. "No usable vcell" must not silently
degrade to the legacy raw-SOC path. A vcell that is *invalid* is not evidence
that the battery is healthy.

Choose and state the outcome for each state. Where vcell is unavailable, an
untrusted SOC must not drive cadence as though trusted. Tests must exercise the
**production adapter**, not only the guard in isolation — that gap is why this
shipped.

## Decision C3 — PMIC handling per fault class (Blocker E)

Round 3 improved classification but left four defects:

1. `thermallyCorrelatedFault` is true for **any** nonzero NTC code, so a
   simultaneous independent input or safety-timer fault is suppressed with it.
   Classification must be per fault, not per register-read.
2. The no-fault branch (`PmicFaultMonitor.cpp:278-288`) still zeroes
   `remediationInProgress`, active level, and phase after phase 0 disabled
   charging, without completing its re-enable.
3. `chargeDisableVerified` is **DCT readback, not verified PMIC state**. Device
   OS `setConfig()` queues an asynchronous reload and acts on the PMIC later in
   `handleCharging()`. Nothing checks `PMIC::isChargingEnabled()`. Either verify
   actual PMIC state or rename the field and its comments to describe what it
   really proves — the current naming overstates the guarantee.
4. Device OS reads the BQ24195 fault register **twice and ORs** the readings,
   because one reflects prior state; this monitor reads once.

## Acceptance criteria

- **AC-C1.** Temperature read and charge-decision evaluation occur in one call,
  with the evaluation consuming the just-read value directly. No independently
  callable entry point to either half.
- **AC-C2.** A test exercises the coupled call and confirms the charge decision
  updates **atomically with the measurement** — not as a consequence of call
  ordering. The test must fail if the halves are separated.
- **AC-C3.** No path can return from the measurement call with a temperature
  read but no decision applied, including the partial-sample path.
- **AC-C4.** Validity travels with the reading. A genuine reading followed by
  repeated sensor failures does not leave a stale reading treated as current.
- **AC-C5.** The post-reboot window during which charging may be enabled on a
  hot device is bounded, measured, and stated.
- **AC-C6.** The tier guard is total over Known / Invalid / Unavailable vcell,
  each with a tested outcome, exercised **through the production adapter**.
- **AC-C7.** PMIC faults are classified per fault, not per register-read; no
  remediation sequence is abandoned mid-phase; and `chargeDisableVerified`
  either verifies PMIC state or is renamed to match what it proves.
- **AC-C8.** Every new function has a production call site and an `nm`-visible
  ELF symbol (permanent standard). Tests must be mutation-checked by the
  implementer: revert the fix, confirm the test fails.

## Still open, not blocking

- `PowerTier::label` retained with no caller and no symbol.
- Blank line at EOF, `RuntimeReportingPolicy.cpp:60`.
- `stats.thermal_inhibit_held_without_fresh_temp` has no serializer, publisher,
  or reader. Amendment B required this be *visible*; it is not. Either wire it
  to a real field channel or stop claiming telemetry visibility for it.

## Decision C4 (2026-08-26) — TMP36 sampling moves inline, resolving the AC-C5 stop

Copilot raised AC-C5 as a Stage 6 stop, correctly: on the field-deployed
TMP36-only configuration the post-reboot hot-charging window is **unbounded**.

    reboot -> refreshInputProfile() writes a DCT config without DISABLE_CHARGING
           -> a genuine reading requires 8 accumulated samples
           -> hibernate resets the MCU, wiping the non-retained sampleIndex
           -> measuredThisCall never becomes true
           -> the inhibit never re-arms; a hot device charges indefinitely

This is the mirror image of Blocker A. Amendment B's rule that an unmeasured
temperature **cannot arm** protects against a device that never charges; it
exposes the opposite failure, a hot device charging unprotected. Both damage the
cell. The rule is right; the thing that made it dangerous is that a genuine
reading is not reliably obtainable.

### The decision

**Take all 8 TMP36 samples inline, within the single coupled call.** Remove the
cross-call accumulator entirely: `sampleIndex` and `tmpRawSum`
(`SensorManager.cpp:1043-1044`) stop being `static` and become locals of the
sampling loop.

The existing rationale — *"Non-blocking sampling: spread 8 samples across
multiple batteryState() calls to avoid blocking the main loop"* — is protecting
against **~40 microseconds**. The code's own comment documents each sample as a
**~5 µs ADC read**. Chip confirmed 2026-08-26 that 40 µs inline is acceptable.

That spreading is the root cause of this entire bug class, including the
original round-3 blocker 3 (hibernate resetting the counter so the inhibit could
never release) and now its inverse. Removing it removes both.

Chip confirmed the TMP36 read has no I²C, initialisation, or settling
dependency, so there is no readiness constraint on sampling inline.

### Consequences

- A genuine reading is available on the **first** coupled call, on every
  platform. AC-C5's window becomes **one call**, and the hibernate interaction
  disappears rather than being mitigated.
- **No `static` accumulator state may remain** in the temperature path. Static
  state that survives across calls but not across reset is what produced both
  failures; it must not be replaced with retained memory either — samples
  spanning hours do not average into a current temperature.
- A sensor failure still yields `measuredThisCall = false`. Inline sampling
  changes *when* a genuine reading is obtainable, not what counts as one.
- The partial-sample fall-through path added for AC-C3 becomes unreachable for
  TMP36. Remove it rather than leaving dead code, but keep the AC-C3 guarantee
  intact for any path that still cannot produce a reading.

**Rejected alternative:** retaining the accumulator across hibernate. It would
technically complete the average, but from samples taken hours apart across
different thermal conditions — a number that is not the current temperature and
would be trusted as though it were.

### Amended acceptance criteria

- **AC-C5 (revised).** The post-reboot window during which charging may be
  enabled on a hot device is **one coupled call** on every platform. State the
  measured worst case.
- **AC-C9.** No `static` (or retained) accumulator state remains in the
  temperature-acquisition path. A test must fail if cross-call sampling state is
  reintroduced.
- **AC-C10.** A single coupled call, from a cold start, produces
  `measuredThisCall = true` on a working TMP36 — no second call required.

## Decision C5 (2026-08-26) — profile application composes; it does not replace

From Codex's bounded architectural assessment (`/tmp/wo26/CODEX_AUTHORITY.md`),
commissioned after Claude's authority trace (`/tmp/wo26/CHARGE-STATE-AUTHORITY-TRACE.md`).

### What the assessment established

Charging-disabled state has **no single owner**. Two application layers, and on
Layer 1 two writers that do not compose:

- **W1** `ChargeInhibit::apply()` — the thermal authority.
- **W2** `PowerPlatform::applyInputProfile()` — writes a **default-constructed**
  `SystemPowerConfiguration`, so `DISABLE_CHARGING` is dropped. It replaces the
  whole config rather than composing onto it.

Claude's trace overstated W2 as unconditional. It is **conditional**:
`refreshInputProfile()` reaches `applyInputProfile()` only when `!report_.valid`
or the selected profile changes (`PowerManager.cpp:190-191`). A stable-profile
refresh writes nothing. The corrected scope makes this narrower but still
reachable, on any real profile transition (VIN/Solar ↔ USB).

Two damage-capable paths follow:

1. **Trailing setup refresh.** `Generalized-Core-Counter.cpp:1352` runs after the
   coupled decision at :1351. On a profile transition it clears the just-applied
   inhibit. The interval to the next thermal re-assertion is **not bounded**:
   connected open-hours with no report due skips the scheduled-measurement block,
   and `lastReport` persistence with a `(now - lastReport) >= intervalSec` test
   admits deferrals up to a stored `uint16_t` interval (≈18 h 12 min), or
   arbitrarily after a backward RTC correction.
2. **Asynchronous Device OS race.** Device OS runs a separate power-manager
   thread. On a profile transition at `SensorManager.cpp:717`, the cleared config
   is persisted and `ReloadConfig` queued; that worker can load it and call
   `enableCharging()` (`system_power_manager.cpp:306-310`) before the coupled
   call reaches `ChargeInhibit::apply()`. Final DCT state is correct; live
   charging is transiently enabled while hot. **Ordering cannot fix this** — the
   race is between an application write and a Device OS thread.

### The decision

**Profile application must compose onto the existing charge-inhibit state, not
replace it.**

1. Remove the trailing `refreshInputProfile()` at
   `Generalized-Core-Counter.cpp:1352`. It is redundant: the internal refresh at
   `SensorManager.cpp:717` always runs before the coupled decision on Boron, and
   the trailing call retries nothing — a failed internal apply still records the
   selected profile, so the trailing same-profile call skips `applyInputProfile()`.
2. `applyInputProfile()` must **preserve the existing `DISABLE_CHARGING` bit**.
   Copy **only that bit** onto a freshly constructed profile configuration — not
   the whole prior config, which would preserve unrelated or obsolete flags. A
   profile transition while inhibited then changes voltage/current limits without
   ever creating an intermediate DCT state that authorizes charging.

Device OS 6.4.1 supports this: `System.getPowerConfiguration()` returns the
current configuration, and `SystemPowerConfiguration` exposes `feature()`,
`clearFeature()`, and `isFeatureSet()`. Device OS's own Tinker board config uses
the same read-modify-write pattern.

**Limits to state honestly, not paper over:** the Wiring `getPowerConfiguration()`
wrapper returns by value and discards the read status, and Device OS sanitizes a
failed read to defaults. Preservation improves normal and concurrent behaviour;
it is **not** proof the DCT read succeeded, and must not be documented as such.
Preserving a set bit can hold charging off until the next coupled measurement —
that is the safe direction, and the thermal policy already clears it on a genuine
cool reading.

### Scope boundary

Full authority unification — one application owner composing thermal, PMIC-
remediation, and future source-uncertain reasons, with bounded post-reload
observation instead of immediate DCT readback — is **a separate work order**. It
spans PowerManager, PowerPlatform, ChargeInhibit, PmicFaultMonitor, SensorManager,
their stubs and tests, and needs on-device timing validation. The two fixes above
close the reachable damage paths without that surface.

Note for that future WO: an application owner cannot be the *sole* writer in the
shipped stack. Device OS writes the PMIC internally by design; "sole owner" can
only mean sole **application** owner.

### Acceptance criteria

- **AC-C11.** No `refreshInputProfile()` call occurs after `batteryState()` in
  `setup()`. A structural test must fail if one is reintroduced.
- **AC-C12.** A profile transition (USB↔Solar, both directions) while
  `DISABLE_CHARGING` is set retains the bit. Reverting preservation must fail
  the test.
- **AC-C13.** A subsequent genuine cool coupled decision still clears the bit —
  preservation must not create a latch.
- **AC-C14.** On-device validation observes DCT **and live PMIC state** across
  both profile transitions. Host tests cannot schedule the Device OS worker race,
  so this is required before the async path is claimed closed.

## AC-C14 — SATISFIED by on-device observation (2026-08-26)

Boron-Dev-09, running on **battery** (USB disconnected so `PowerSourceOverride`
could not intercept the injected source), temporary diagnostic build
`ENABLE_AC_C14_RACE_DIAGNOSTIC=1`.

**Result, read over the cloud via `particle get <device> acc14sum`:**

```
attempts=20/20,confirmed=19,hazard=0,benign=0,complete=1,truncated=0,
firstHazardAttempt=0,firstHazardLabel=
```

RGB verdict: **GREEN** — run complete, transitions confirmed, no hazard.

### What this establishes

Across **19 confirmed Solar35W ↔ UsbBench profile transitions** on real
hardware, with the thermal inhibit armed (synthetic 40.01 °C, above the 37 °C
arm threshold) and `DISABLE_CHARGING` set in DCT, the DCT belief and the **live
PMIC `isChargingEnabled()` register state never disagreed** — sampled
immediately after the profile write (W2), immediately after
`ChargeInhibit::apply()` (W1), and again after a 75 ms settle.

This is positive evidence that Decision C5's composition fix — `applyInputProfile()`
preserving the existing `DISABLE_CHARGING` bit rather than replacing the config —
closes the asynchronous Device OS `ReloadConfig` race Codex identified.

One of the 20 attempts produced no confirmed transition. **Which one, and why,
is not established.** An earlier claim in this section that the device began the
run already in attempt 1's target state was a guess and is retracted: the
`pdiag` batch captured immediately before the run shows the device on
**UsbBench** on battery (`vb=0 pg=0`), and attempt 1 injects VIN toward
`Solar35W` — a genuine change that should have counted. The non-transitioning
attempt was correctly reported as INVALID and excluded from the evidence rather
than counted as a clean pass, which is what matters for the result's validity.

That same `pdiag` batch is direct evidence the injection reached production
profile selection: entry 8 shows injected source VIN producing profile
`Solar35W`, and entry 11 shows injected source UsbAdapter producing `UsbBench` —
both on battery, with no `PowerSourceOverride` interception. (The batch also
reported `"trunc":1`, hitting its 12-entry capacity — the second time that known
limitation has dropped data. It did not affect this result, which came from the
retained snapshot, not the batch.)

### What this does NOT establish — stated deliberately

1. **It does not prove the race is impossible.** Device OS 6.4.1 provides no
   scheduling bound that would make the absence of a disagreement provable by
   observation. 19 clean transitions is evidence, not proof.
2. **The observation method biases toward a clean result.** The diagnostic uses
   flag-gated read-only probes inline in the production path rather than a
   separate observer thread (chosen to avoid untested I²C-lock contention with
   the code under test). Those probes add real latency *inside the very window
   being measured*, which makes the race easier to avoid than in production. A
   clean result is therefore weaker evidence than a zero-overhead observer would
   have produced.
3. **It says nothing about other schedules** — different cellular timing, thermal
   state, or Device OS load could interleave differently.
4. **The three probes are discrete samples, not a continuous trace.** A short
   enable/disable excursion occurring entirely between `AFTER_W2`,
   `AFTER_W1_IMMEDIATE`, and `SETTLED` would not be counted. This temporal
   aliasing is a sharper limit than "other schedules untested" and applies even
   within an exercised schedule. (Added 2026-08-26 on Codex's assessment,
   `/tmp/wo26/CODEX_C5.md`.)
5. **The run observes the PMIC charge-enable register, not charge current.**
   That satisfies AC-C14 as written, but it does not directly measure electrical
   energy delivered into a hot pack.
6. **A failed DCT read is outside what this proves.** The Wiring
   `getPowerConfiguration()` wrapper discards the read status and Device OS
   sanitizes a failed read to defaults. A clean run under normal reads says
   nothing about that path — see Decision C5's "limits to state honestly".

### Independent assessment of this evidence (Codex, 2026-08-26)

Codex verified the diagnostic artifact itself — correct Particle footer CRC, and
its strings contain the transition assertion, all three probe labels, and the
invalid/truncated verdicts — confirming the summary format belongs to the build
that produced it.

Its characterisation of the result, which supersedes any stronger phrasing
elsewhere in this document: *"'Zero sampled disagreements under these
conditions' is exactly the observation; that observation is useful evidence for
the fix, not a universal scheduling theorem."*

On the three earlier INVALID runs, Codex drew a distinction worth preserving:
they strengthen the diagnostic's **internal validity** — the transition
assertion demonstrably prevented a false "20 attempts, zero disagreements" —
but they do **not** strengthen representativeness across schedules. Greater
confidence that the 19 transitions were classified honestly; no increase in the
sample's breadth.

On the unexplained twentieth attempt: correctly excluded from both clean and
hazardous evidence, and no causal explanation should be attached to it. It would
become material only if failures to transition correlated systematically with
the schedule most likely to expose the race — which this run does not establish.

### Method validity

The diagnostic injected **only raw sensor inputs**: the TMP36 ADC count at
`analogRead(TMP36_SENSE_PIN)` and the raw source at `System.powerSource()`.
Everything downstream ran as unmodified production code —
`measureTemperatureAndApplyChargeDecision()`, `selectInputProfile()`, the
`shouldApplyProfile` comparison, the real `System.setPowerConfiguration()` and
DCT write, `ChargeInhibit::apply()`, and Device OS's own asynchronous worker on
its genuine schedule.

Three earlier runs were correctly reported as **INVALID** rather than clean:
two where `PowerSourceOverride` intercepted the injected VIN source on a
USB-connected device (no transition occurred), and one truncated by low-power
idle after 7 of 20 attempts when a fast cellular connection beat the run. The
diagnostic's own transition assertion and truncation state caught all three.
Without them, the first of those runs would have been reported as
"20 attempts, 0 disagreements" — a false clean result.

### Disposition

AC-C14 is **satisfied as written**: on-device validation observed DCT and live
PMIC state across both profile-transition directions. The diagnostic build is
temporary, was never committed, and **must now be removed** — that removal is
the final step of the authorization under which it was built.
