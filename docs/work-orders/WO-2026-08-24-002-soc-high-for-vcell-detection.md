# WO-2026-08-24-002: Detect SOC implausibly HIGH for Vcell (the depletion-hiding direction)

## Context

`BatteryAuthorityPolicy::staleSocConditionsMet()` is one-directional. Both of
its branches require `vcell >= 4.00`:

```c
const bool legacyChargingContextStaleSoc =
    (vcellHighConfidence /* >=4.10 */ || vcellLowConfidence /* 4.05..4.10 */) &&
    sample.soc < 30.0f && externalPowerPresent && (...);
const bool vcellRestingHighWithLowSoc =
    sample.vcell >= 4.00f && sample.soc <= 25.0f;
```

It therefore only recognises **"resting voltage high, SOC low"** — the July
2026 signature it was built from (`vcell=4.028 soc=11.8`). It has no band for
the opposite error, **"SOC high, voltage low"**, which is the direction that
hides depletion rather than exaggerating it.

WO-2026-08-05-002 specified "a single-sample SOC-vs-Vcell consistency check …
using this battery chemistry's known OCV curve". What shipped is narrower: two
hard-coded bands, both gated on high voltage.

### Why this matters — it is the reading that masked Dev-14's death

Boron-Dev-14 reported `soc=71.4` at `vcell=3.069` on 2026-08-17. A cell at
3.069 V is empty. The gauge was overstating charge by roughly **71 points**
while `chg=FAST` sat alongside it looking reassuring. Neither branch above
applies, because both require `vcell >= 4.00`. The mechanism built to catch
implausible SOC could not see the most consequential misreading in the fleet's
history.

## Evidence

Method: 386 `ChargeDiag` samples carrying both `vcell` and `soc`, from
cloud-forwarded serial for Dev-09, Dev-11 and Dev-14 across 2026-08-06 →
2026-08-24. Each sample's reported SOC was compared against an SOC implied by
a conservative single-cell LiPo resting-OCV curve.

**Normal behaviour is well-calibrated.** Median delta `soc - ocv(vcell)` =
**−0.9 points**. The gauge is not generally wrong.

**The dangerous direction is rare, large, and concentrated:**

| delta > | samples |
|---|---|
| +20 pts | 23 |
| +30 pts | 17 |
| +40 pts | 12 |

**Every one of the worst offenders is Dev-14 during its terminal discharge:**

```
2026-08-15T11:00:05  vcell=3.829 soc=83.0  ocv~48%   delta=+35   <- first >+30
2026-08-16T09:59:46  vcell=3.727 soc=80.1  ocv~31%   delta=+48.8
2026-08-16T22:00:12  vcell=3.672 soc=78.5  ocv~23%   delta=+55.4
2026-08-17T01:59:28  vcell=3.581 soc=77.5  ocv~14%   delta=+63.8
2026-08-17T09:59:25  vcell=3.378 soc=72.4  ocv~ 4%   delta=+68.8
2026-08-17T12:59:25  vcell=3.069 soc=71.4  ocv~ 0%   delta=+70.9  <- last output
```

Detection would have tripped repeatedly, more than two days before Dev-14's
last transmission, worsening monotonically throughout. The device died anyway
with no battery-authority mechanism ever engaging. See **Threshold evidence**
below for the selected threshold and its per-threshold lead times — that
section, not this one, is authoritative on the value to implement.

**The guard has never fired.** Zero `STALE_SOC` and zero post-connect authority
lines across 7,410 captured firmware serial lines, and zero across the full
2026-08-06 → 2026-08-24 window queried directly for Dev-09 and Dev-11
(Dev-14 confirmed zero from cached capture). This mechanism has not been
observed engaging once.

**The existing PMIC detector did fire and did nothing.** Dev-14 raised
`PMIC: Stuck in Fast Charging for 6+ hours with no material gain` twice on
2026-08-16, with negative `socGain` and `vcellGain` both times. Detection
existed; escalation did not.

## Fix

Add a **symmetric, SOC-high-for-Vcell** consistency check to
`BatteryAuthorityPolicy`, evaluated on every sample, independent of the
existing `vcell >= 4.00` bands.

### Design constraints — read these before choosing thresholds

1. **Detection must NOT be gated on the existing quiet predicate.** This was an
   error in the first draft of this WO and the data caught it: **15 of the 16
   Dev-14 incident samples are `chg=FAST`**, and
   `quietForFuelGaugeResync()` explicitly excludes `Fast` and `Pre`. Gating
   detection on it would have evaluated on **1 of 16** incident samples —
   near-useless for the exact case this check exists for.
   **Separate detection from action:** detection runs on every sample
   regardless of charge state; only the `quickStart()` resync needs a quiet
   context, because a quickStart during active charging reads badly.
2. **Charge and load bias the comparison in opposite directions.** Terminal
   voltage under charge sits *above* OCV, biasing delta negative — worst
   legitimate negative case is Dev-09 at `vcell=4.157 soc=71.6` (delta −24.1)
   while `chg=FAST`. Voltage under load sags *below* OCV, biasing positive.
   Thresholds must therefore be asymmetric; this WO covers only the positive
   (depletion-hiding) direction.
3. **Require consecutive confirmation.** Follow the existing
   `STALE_SOC_TRIGGER_CONSECUTIVE_COUNT = 2` precedent. A single sagged sample
   must not trip it.
4. **Threshold: +20 points**, justified below. Copilot may propose a
   curve-based equivalent, but must justify any figure against the observed
   distribution rather than by feel.

### Threshold evidence

Across 378 unique samples, excluding Dev-14 from the start of its terminal
decline:

| statistic | value |
|---|---|
| median delta (all samples) | −0.9 |
| p95, non-incident | +12.9 |
| p99, non-incident | +21.7 |
| **max, non-incident** | **+29.7** |
| max, quiet-gated subset (n=15) | **−1.4** |

**The +29.7 ceiling is not a false positive.** Every one of the six
non-incident samples above +20 carries `src=VIN prof=SOLAR`:

```
+29.7  Dev-09  08-17 09:00  chg=OFF   vcell=3.822 soc=79.1  src=VIN prof=SOLAR
+29.4  Dev-14  08-15 10:00  chg=FAST  vcell=3.844 soc=83.2  src=VIN prof=SOLAR
+26.4  Dev-14  08-15 08:00  chg=FAST  vcell=3.860 soc=83.4  src=VIN prof=SOLAR
+22.4  Dev-09  08-16 13:59  chg=FAST  vcell=3.861 soc=79.6  src=VIN prof=SOLAR
+21.7  Dev-14  08-15 07:00  chg=FAST  vcell=3.884 soc=83.5  src=VIN prof=SOLAR
+21.2  Dev-09  08-16 12:59  chg=FAST  vcell=3.868 soc=79.8  src=VIN prof=SOLAR
```

Four are Dev-14 in the early hours of its own decline; two are Dev-09 during
the VIN/Solar episodes it later self-corrected from, which carried a measured
charge cost (`PMIC: Stuck in Fast Charging`, negative `socGain`/`vcellGain`).
These are early-stage instances of the fault, not noise. The genuine noise
band tops out near **+13** (p95).

Lead time on Dev-14 by candidate threshold:

| threshold | first trip | lead before last transmission | trips |
|---|---|---|---|
| +15 | 08-15 06:00 | 2 d 7 h | 20 |
| **+20** | **08-15 07:00** | **2 d 6 h** | **19** |
| +25 | 08-15 08:00 | 2 d 5 h | 18 |
| +30 | 08-15 11:00 | 2 d 2 h | 16 |

+20 is selected: it has ~7 points of clearance over the real noise band, zero
clean false positives in 378 samples, catches Dev-14 four hours earlier than
+30, and additionally catches Dev-09's recovered episodes — which is desirable,
since those had a real charge cost and went unreported.

### Action on detection

Trusting Vcell means the corrected SOC will drop sharply, which will in turn
engage the low-battery reporting policy and protective behaviour. **That is the
desired outcome** — on Dev-14 it would have converted a silent 27-hour
discharge into a reported emergency. It also means a false positive wrongly
throttles a healthy device, which is why constraints 1 and 3 above are not
optional.

Required behaviour on a confirmed detection:

- Raise the alert and record diagnostics **immediately on confirmation**,
  regardless of charge state.
- **Defer** the gauge resync (`quickStart()` or equivalent) until a quiet
  context is available, consistent with the existing stale-SOC path. Do not
  block detection or alerting on that deferral.
- Emit a distinct, greppable log line — do **not** reuse the `STALE_SOC:`
  prefix; this is a different condition and must be separable in serial.
- Record a `PowerDiagnostics` batch entry so it survives a serial blackout, as
  `recordResyncEvent()` already does for the existing path.
- Raise a distinct alert code so it is visible in `alertCount` telemetry.

## Explicitly out of scope

- **Replacing or refactoring the existing pre-radio / post-connect authority
  machinery.** WO-2026-08-05-002 raised the question of whether it becomes
  redundant. Not answered here, not touched here.
- **The runtime profile interlock** (Solar + DPM + charge current at floor →
  fall back to UsbBench), deferred from WO-2026-08-24-001. It is complementary:
  that one detects the *cause*, this one detects the *consequence*. Both would
  independently have caught Dev-14. Keep them separate.
- **Widening `staleSocConditionsMet()`'s existing branches.**

## Acceptance Criteria

1. A confirmed SOC-high-for-Vcell condition is detected, logged with a distinct
   prefix, recorded in the diagnostics batch, and raises a distinct alert code.
2. **Replay regression:** fed the real Dev-14 `ChargeDiag` series as a fixture,
   the check must trip **19 times**, with the first trip at the
   **`2026-08-15T07:00`** sample (`vcell=3.884 soc=83.5`), not later. Detection
   must occur despite those samples carrying `chg=FAST`.
3. **No false positives on real fleet data:** run over all remaining samples,
   the check must produce **zero trips on any sample not carrying
   `src=VIN prof=SOLAR`**. The six `VIN/Solar` samples above +20 listed in the
   threshold evidence are true positives and are expected to trip; they must
   not be suppressed to make this criterion pass. Fixtures are derivable from
   the cloud-forwarded `ChargeDiag` history.
4. Existing `staleSocConditionsMet()` behaviour is unchanged — the existing
   tests must pass untouched, and the new branch must not alter any decision the
   old branches would have made.
5. Size cost measured and reported against a real build.

## Required Tests

- Unit: the new predicate across the threshold boundary, both directions, and
  the consecutive-confirmation counter.
- Replay fixtures for AC2 and AC3, using the real captured sample series rather
  than synthetic values.
- Confirmation that a charging-context sample with elevated voltage
  (`vcell=4.157 soc=71.6`, delta −24.1) does **not** trip the new branch.
- Existing `tests/*.sh` and `tests/*.py` must pass unweakened. Tests are
  `#!/bin/zsh` using `${0:A:h:h}` — run with zsh.

## Permitted Files

- `src/sensors/BatteryAuthorityPolicy.h` / `.cpp` — the new branch and thresholds
- `src/sensors/SensorManager.cpp` — call site, logging, alert raise
- `src/power/PowerDiagnostics.h` / `.cpp` — new batch reason code, if needed
- `tests/` — new tests and fixtures
- `CHANGELOG.md`, `docs/` — changelog entry and this WO's status section

Codex to confirm full scope. Anything outside this list returns to Claude and
Chip rather than being improvised.

## Revision A (2026-08-24) — post Codex Stage 4

Codex returned **REVISE before approval**. Five findings were substantive. One
(the data-integrity concern) has since been audited and cleared; the other four
are corrected below and are binding on the implementation.

### R0. Data-integrity audit — CLEARED, evidence base stands

Codex observed that `ChargeDiag` can log a hybrid pair: when a post-connect
candidate is rejected, `loggedSoc` is the earlier pre-radio authoritative SOC
while `vcell` is the current candidate voltage
(`SensorManager.cpp:725`). Under radio sag that inflates a positive residual.

Audited three independent ways; no hybrid pairs found:

1. **Rejection is unconditionally logged** — the two `Log.warn` lines sit inside
   the same branch that sets `rejectAuthoritativeOverwrite`, with no silent
   path. Across **9,840 captured firmware serial lines**: zero
   `Battery post-connect sample ignored`, zero `Battery authority: keeping
   pre-radio`, zero `authority=ignored-post-connect`. (Note: zero battery
   *detail* lines were captured at all, since `logBatteryDetail` is normally
   false — that third marker's absence is not independently meaningful. The two
   unconditional warnings are.)
2. **Burst contiguity** — for every threshold-setting sample the surrounding
   captured lines span **14–16 ms of device uptime**. The rejection warnings
   would have been emitted inside that window. Absent, not dropped.
3. **Independent second SOC path** — `ChargeDiag` prints `loggedSoc`; the
   adjacent `PowerDiag` prints `PowerManager::instance().soc()`, which never
   routes through `loggedSoc`. A hybrid requires **≥20 points** of divergence by
   construction of the rejection predicate. Across 378 paired samples: **zero**
   disagree by ≥20; one disagrees by 9.9; every threshold-setting sample agrees
   to within **0.5**.

The 378-sample distribution, the +20 selection and the lead-time figures stand
unchanged.

### R1. Reproducibility (Codex revision #1)

The full dataset is committed at
`tests/fixtures/battery-soc-vcell-samples-2026-08.csv` — 378 rows, one per
`ChargeDiag` sample, with device, UTC timestamp, charge state/code, `ichg`,
fault register, `vcell`, reported SOC, power source/profile, the adjacent
`PowerDiag` SOC used for the hybrid cross-check, and a
`dev14_terminal_incident` flag (20 rows). Extraction: cloud-forwarded
`serial.log` events via `tools/telemetry timeline`, deduplicated by `eventId`,
parsed with the `ChargeDiag:` regex; incident window defined as Dev-14 from
`2026-08-15T06:00Z` to its last transmission `2026-08-17T12:59:25Z`, chosen as
the first sample exceeding +15 rather than from residual magnitude.

**Curve knots are now explicit** and live in the implementation
(`kOcvCurve`). Curve sensitivity was tested across three defensible
resting-OCV curves plus the firmware's own linear fallback:

| curve | first raw hit | lead | raw hits |
|---|---|---|---|
| WO analysis curve | 08-15 07:00 | 2 d 6 h | 19 |
| implemented `kOcvCurve` | 08-15 07:00 | 2 d 6 h | 19 |
| independent published LiPo table | 08-15 07:00 | 2 d 6 h | 19 |
| firmware linear `(v-3.0)*100/1.2` | 08-16 11:59 | 1 d 1 h | 10 |

Detection survives all four. Lead time is curve-dependent within
**1 d 1 h – 2 d 6 h**. The threshold must be justified as robust across that
band, not against one curve.

### R2. Confirmation semantics — AC2 was self-contradictory

The original AC2 required both a two-sample confirmation and a first trip at
the first raw hit. Those cannot both hold. Three events must be named
separately and tested separately:

- **raw hit** — a single sample where `soc - ocv(vcell) >= +20`
- **confirmation transition** — the evaluation at which the consecutive counter
  first reaches `SOC_HIGH_FOR_VCELL_CONFIRM_COUNT`
- **confirmed-state sample** — any raw hit while already confirmed

Alerting, diagnostics recording and the pending-action latch fire on the
**confirmation transition**, not on a raw hit.

### R3. Pending-action latch — deferral must not be indefinite

Splitting detection from action left a hole: if `chg=FAST` never clears,
`quickStart()` never runs, authoritative SOC stays high, and low-battery policy
never engages. The WO's earlier claim that this converts the incident into
protective behaviour was **overstated** — it is true of the alert only.

Required: a `retained` pending-resync latch, with explicitly specified

- set condition (confirmation transition),
- clear conditions (resync executed; or evidence revalidated-and-absent in a
  quiet context; or maximum deferral exceeded),
- a **maximum deferral bound** after which behaviour is defined rather than
  open-ended,
- retry/cooldown behaviour, and
- survival across ULP, hibernate, cold boot and reset.

Until a corrected SOC is actually committed, the WO makes **no claim** that
low-battery protective behaviour is engaged.

### R4. Counter units — "consecutive" must be defined

`batteryState()` runs at setup, post-wake, report, connect-success, pre-sleep
and scheduled sampling, so two "consecutive" calls can be seconds apart inside
a single wake and a transient can self-confirm.

Required: define consecutive in terms of **independent wake cycles or a minimum
elapsed separation**, not raw calls; use a counter scoped to this detector
alone; and specify resets for cold boot, ULP return, hibernate/reset, an
intervening non-match, and successful/failed action.

### R5. Alert visibility — verified worse than Codex reported

There is a **single** `alertCode`, not a count. `raiseAlert()` replaces only
when `getAlertSeverity(new) > getAlertSeverity(existing)`
(`MyPersistentData.cpp:825-835`). Alert 22 is absent from the severity table,
so it falls to `default: return 1` — **minor**.

**Alert 21 (`PMIC charge timeout / stuck charging`) is severity 3, critical —
and Dev-14 was raising it during the exact episode this WO targets.** A minor
alert 22 raised alongside an active critical 21 is silently discarded. The
alerting half of the fix would be defeated in precisely the circumstance it
exists for.

Worse on Dev-09 today: its latched code is **19** (watchdog reset), severity
**4** — the highest tier, which nothing can supersede. No new alert can surface
on that device at all until 19 is cleared.

*(This also corrects the week-back report, which described these values as
frozen "alert counters". They are not counters; the telemetry field named
`alertCount` carries the single current `alertCode`. Dev-09's 19 is a latched
watchdog alert, TRAIL02's 42 a latched ledger-publish-failure alert.)*

Required: allocate the code and severity deliberately; decide and document the
masking behaviour against an active 20/21/23; and do **not** rely on
`alertCode` alone for visibility — the condition must also surface through a
channel that cannot be masked. `src/MyPersistentData.cpp` is added to Permitted
Files for the severity mapping.

### Revised Acceptance Criteria (supersede AC1–AC5 above)

B1. Raw-hit replay: fed `tests/fixtures/battery-soc-vcell-samples-2026-08.csv`,
    the predicate produces **19 raw hits** on `dev14_terminal_incident=1` rows,
    first at **2026-08-15T07:00Z**, despite those rows carrying `chg=FAST`.
B2. Confirmation replay: the **first confirmation transition** occurs at the
    second qualifying evaluation, not the first. State which counter unit was
    used and show the resulting transition timestamp.
B3. No false positives: **zero raw hits on any fixture row that is not
    `power_src=VIN, power_profile=SOLAR`**. The `VIN/SOLAR` rows above +20 are
    true positives and must not be suppressed to pass.
B4. Pending-action latch: tested set, each clear path, maximum-deferral
    expiry, and survival across ULP / hibernate / cold boot.
B5. Counter lifecycle: tested for repeated calls within one wake (must **not**
    self-confirm), an intervening non-match, and separation across wakes.
B6. Alert visibility: tested that the condition is observable when alert 21 is
    already active and when alert 19 is latched. If it is masked in
    `alertCode`, the unmaskable channel must carry it.
B7. Curve robustness: the selected threshold justified across the
    1 d 1 h – 2 d 6 h lead-time band in R1, not against one curve.
B8. Size cost measured against a real build; new code confirmed present in the
    compiled artifact by `strings`/disassembly.

### Permitted Files (Revision A — supersedes the list above)

As before, plus:

- `src/MyPersistentData.cpp` — alert severity mapping only
- `tests/fixtures/battery-soc-vcell-samples-2026-08.csv` — the committed dataset

## Status

**Revision A implemented (Stage 6, Copilot) — awaiting Stage 7 (Codex).**
Original approach approved by Chip 2026-08-24; Codex Stage 4 returned REVISE
before approval (R2–R5 below); this revision addresses all four findings and
is now grounded in the committed 378-row CSV fixture rather than
hand-transcribed values.

- **R2 (three named events)**: `socHighForVcellUpdateCounter()` distinguishes
  raw hit, confirmation transition, and confirmed-state sample.
  Alert/diagnostics/latch fire only on the confirmation transition
  (`SOC_HIGH_FOR_VCELL_CONFIRMED:`); a confirmed-state sample logs the
  lighter `SOC_HIGH_FOR_VCELL_ONGOING:` and does not re-fire them.
- **R3 (bounded pending-resync latch)**: `socHighForVcellPendingResync` /
  `socHighForVcellLatchArmedEpoch` (`retained`) implement set (confirmation
  transition) / clear (resync executed; evidence revalidated-and-absent in a
  quiet context, `SOC_HIGH_FOR_VCELL_LATCH_CLEARED:`; or
  `SOC_HIGH_FOR_VCELL_MAX_DEFERRAL_SECONDS` = 4 hours exceeded, forcing the
  resync, `SOC_HIGH_FOR_VCELL_RESYNC:`) / retry-cooldown
  (`SOC_HIGH_FOR_VCELL_RETRY_COOLDOWN_WAKE_CYCLES` = 3) semantics.
- **R4 (wake-cycle-scoped "consecutive")**: the confirmation counter advances
  at most once per independent wake-cycle id
  (`socHighForVcellWakeCycleId`, advanced in `noteWakeFromLowPowerSleep()`),
  not per raw `batteryState()` call. Resets on any genuine MCU reset (cold
  boot/watchdog/panic/HIBERNATE-as-reset, via a non-retained boot-guard),
  on an intervening non-match, and on a resync attempt; deliberately does
  **not** reset on ordinary ULP/STOP wake.
- **R5 (alert severity)**: `MyPersistentData.cpp`'s `getAlertSeverity()` maps
  alert 22 to critical tier 3 (alongside 20/21) instead of the default minor
  tier it fell to before. `alertCode` masking against Dev-09's latched
  watchdog alert 19 (severity 4) is now a documented, deliberate outcome; the
  `SOC_HIGH_FOR_VCELL_CONFIRMED` log line and the `PowerDiagnostics` batch
  entry (reason code 12) are the unmaskable channel independent of
  `alertCode`.

**B1–B3 (CSV replay, `tests/sensor_manager_battery_authority_test.sh`)**: 19
raw hits on the 20 `dev14_terminal_incident=1` rows, first at
`2026-08-15T07:00`; first confirmation transition at the second qualifying
evaluation (`2026-08-15T08:00`); zero raw hits on any of the 378 fixture rows
outside `power_src=VIN,power_profile=SOLAR` (with the true-positive VIN/SOLAR
rows above +20 confirmed still tripping, not suppressed). **B4/B5** (latch
set/clear/max-deferral/wake-cycle survival; counter lifecycle within one wake
vs. across wakes) covered by unit tests directly against the pure functions in
`SensorManagerBatteryAuthorityTests.cpp`. **B6** (alert visibility against
active 19/21) covered by `tests/soc_high_for_vcell_alert_severity_test.sh`.
**B7** (curve robustness across the R1 1d1h–2d6h lead-time band) — the +20
threshold and CSV replay above are evaluated against the implemented
`kOcvCurve` only; the 1 d 1 h floor comes from R1's separately-reported
curve-sensitivity table (firmware linear fallback), not independently
re-derived in this session. **B8**: see size/strings evidence below.

**Known limitation (cold boot vs. the R3/R4 tension)**: R3 requires the
pending-resync latch to survive cold boot; R4 requires the raw-hit counter to
reset on cold boot. Both are implemented via `retained` backup RAM (survives
ULP/STOP sleep and warm MCU resets - watchdog, panic, HIBERNATE-as-reset via
`RESET_REASON_POWER_MANAGEMENT` - on this platform), with the counter's
cold-boot/reset reset forced explicitly via a non-retained boot-guard flag
that naturally reinitializes on any genuine reset. On a **true full
power-loss cold boot** (VBAT actually disconnected), the nRF52 backup RAM
domain itself is cleared by hardware, so the latch would also be lost in that
one specific case - the Permitted Files list for this WO does not include
adding a new field to `MyPersistentData.h`'s flash-backed `CurrentData`
struct, which would be the way to close this gap. This is flagged as a
blocker-adjacent limitation rather than silently claimed as fully solved; see
`/tmp/wo25/REVA_REPORT.md`.

Diagnosis is grounded in 378 real fleet samples committed at
`tests/fixtures/battery-soc-vcell-samples-2026-08.csv`, not source reasoning
or hand-transcription. Per the standing guardrail on this code path, any
claim about what is compiled into a binary must be verified by
`strings`/disassembly against the actual artifact.
`strings /tmp/wo25/revA.bin | grep SOC_HIGH_FOR_VCELL` confirms all four new
log format strings are present in a real
`particle compile boron . --target 6.4.1` build; measured size 147590 bytes
(vs. 146678 bytes previous implementation, vs. 145734 bytes baseline v23 —
delta +912 bytes from previous, +1856 bytes from baseline).
