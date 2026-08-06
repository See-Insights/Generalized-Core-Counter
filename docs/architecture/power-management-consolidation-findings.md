# Power Management Consolidation — foundation-mapping investigation

Status: investigation only, no implementation plan or WO scope proposed.
Companion to `power-management-consolidation-vision.md`. Produced via
direct source reading (Claude) plus an independent Codex investigation,
cross-verified — see "Verification" note at the end of each major section.

## Headline findings

1. **The codebase has several genuinely incompatible notions of "low
   battery,"** not just scattered code implementing one consistent
   concept. Confirmed concretely (see "Practical disagreements" below).
2. **Sleep-mode selection (HIBERNATE vs. ULTRA_LOW_POWER) takes zero
   battery/SOC/tier input.** Verified directly:
   `shouldUseBoronRtcAlarmHibernate(requestedSleepSec, rtcNow)` — its full
   parameter list — is gated only by the `enableHibernateSleep` config
   flag, Device OS version, and sleep-duration bounds.
   `SystemSleepMode::STOP` is used only as an error fallback, also without
   battery input.
3. **`PowerManager`'s `PowerReading`/`PowerPolicy`/`PowerAction` structs
   already have almost exactly the vision doc's four-function shape at
   the interface level — but the fields matching functions 1/3/4 are
   entirely dead.** Confirmed by grep: nothing in the repo reads or
   writes `.reading.soc`, `.reading.batteryContext`, `.policy.tier`,
   `.policy.lowPowerSuggested`, `.action.shouldEnableCharging`, or
   `.action.chargingActionNeeded`, including inside `PowerManager.cpp`
   itself. Only capability detection, power-source reading, and
   input-profile selection/application are actually implemented.
   Historical check: this shape was introduced complete, in one commit
   (`c254467e`, 2026-05-22), well before the consolidation vision doc
   existed (2026-08-06) — the existing `power-management.md` architecture
   doc already describes only the narrow (profile-selection) scope that's
   actually implemented. No evidence this was a working implementation
   that got stripped down; reads as an intentionally broad interface that
   was never filled in, not an abandoned one.
4. **No production code path currently performs a critical-battery
   shutdown or broadly suspends operations.** The one thing that looks
   like a critical-SOC gate — `SensorManager::batteryState()` returning
   `soc > 20` — has zero consumers; every caller discards the return
   value (already independently confirmed during WO-2026-08-05-003's
   investigation, reconfirmed here).
5. **Two areas straddle the vision doc's four-function boundaries** rather
   than mapping cleanly — see "Boundary cases" below.

## 1. Supported-operations decision inventory (function 3)

| Decision | Location | Battery input used | Notes |
|---|---|---|---|
| Battery tier calculation | `cloud/BatteryBackoff.cpp:6` | SOC + previous tier (hysteresis) | Healthy ≥75, Conserving ~55–70, Critical ~35–50, Survival <30, with retained-tier bands at the boundaries. |
| Reporting-interval backoff | `state/State_Report.cpp:141`, `BatteryBackoff.cpp:47` | Persisted `BatteryTier` | 1x/2x/4x/12x multiplier on connection-alignment interval. Does **not** affect measurement/sampling/sleep cadence — only cloud-connection timing. |
| Event bypass of cadence backoff | `state/State_Report.cpp:179` | none (bypasses tier entirely) | Service requests, occupancy changes, and webhook-health conditions connect immediately regardless of tier — Survival does not categorically suspend connections. |
| Full vs. abbreviated connection attempt | `state/State_Connect.cpp:94-116`, applied at `:257` | **Raw persisted SOC**, `DEEP_ATTEMPT_SOC_THRESHOLD = 50.0f` (`ConnectivityPolicy.h:82`) | Independent of `BatteryTier` entirely — verified directly. Every 4th attempt also forces a full attempt regardless of SOC. |
| Sleep mode selection (HIBERNATE vs. ULP) | `state/State_Sleep.cpp:264` (`shouldUseBoronRtcAlarmHibernate`), applied `:1047` | **None** | Verified directly — gated by config flag, Device OS version, sleep-duration bounds only. |
| Network-standby eligibility | `state/State_Sleep.cpp:350` | None directly | Only indirectly affected if tier policy already downgraded `KEEP_ALIVE` to `INTERMITTENT`. |
| Failsafe hard-action suppression (device reset / AB1805 deep powerdown) | `Generalized-Core-Counter.cpp:2090` (production), `ConnectivityFailsafeTest.cpp:171` (**duplicate reimplementation**, not shared logic) | External power + persisted tier + `lowBatteryMode` flag (OR'd together) | Radio reset stays allowed regardless. The diagnostic test file reimplements this formula rather than calling the same decision function — a duplication risk if the production logic ever changes without the test being updated in lockstep. |
| `lowBatteryMode` persisted flag | Set in `Generalized-Core-Counter.cpp:1406-1430` | Derived from tier transitions | Confirmed consistent with the tier system, not an independent notion — but it's a *sticky* flag (survives until an explicit recovery check clears it), and gets force-cleared on unrelated config changes (`ConfigApply.cpp:422`, on any connection-mode config push) regardless of current SOC. |
| Thermal charging permission | `SensorManager.cpp:1727` | Temperature only, no SOC/tier | Functions 2 (state) and 4 (actuation) are currently fused here — the same function both classifies and directly calls PMIC enable/disable. |

### Practical disagreements this produces (all verified against the actual thresholds)

- A device at 60% SOC is `TIER_CONSERVING` (55–70% band) → reporting
  cadence backs off 2x. But the *same device* still gets full 11-minute
  connection attempts, since 60% > the independent 50% `DEEP_ATTEMPT_SOC_THRESHOLD`.
- A device below 50% SOC normally gets abbreviated connection attempts —
  except every 4th attempt is still full, regardless of SOC.
- An occupancy device can set `lowBatteryMode` on entering `Conserving`
  (as high as ~69% SOC), which can suppress hard failsafe resets at a
  battery level far above the actual `Survival` threshold (<30%) that
  flag's suppression logic is nominally protecting against draining
  further.
- `MyPersistentData.h:67`'s comment claims keep-alive is disabled below
  65% and restored above 75% — this doesn't match the actual hysteretic
  70%/75% transition-band implementation. Stale comment, not a code bug,
  but worth fixing if this area gets touched.
- Pushing a new connection-mode config clears `lowBatteryMode`
  unconditionally (`ConfigApply.cpp:422`), regardless of the device's
  actual current SOC, until the tier policy happens to run again.

## 2. Full read/write/decision inventory (functions 1, 2, 4)

### SOC authority (function 1) — matches WO-2026-08-05-002/003's design

Primary gauge read (`SensorManager.cpp:530`), wake-time plausibility/
stabilization (`:595`), voltage-based fallback estimate (`:184`),
pre-radio authoritative baseline (`:660`), post-radio delta comparison
(`:672`), P2/non-PMIC analog fallback (`:1547`), and the centralized
commit point (`:1273`, from WO-003) are all function 1 — they collectively
decide which SOC value is trusted for the ledger. `BatteryAuthorityPolicy.h/.cpp`
already holds the pure decision logic for most of this (delta/corroboration
rules, stale classification, conditional commit) — the natural function-1
core, per both Claude's and Codex's independent read.

Two sites duplicate raw hardware access outside this authority path,
both diagnostic-only, not feeding the canonical SOC: the power-mismatch
diagnostic reread (`SensorManager.cpp:1356`) and the dormant
`runPmicChargeCycleTest()` snapshot (`:1826`, no caller — see
WO-2026-08-05-003's completeness audit).

### Battery state + PMIC (function 2) and corrective actions (function 4)

PMIC fault/status acquisition (`SensorManager.cpp:833`) feeds state
derivation (`:230`, committed `:1105` — produces only Fault/Charging/
Charged/NotCharging, no distinct Discharging or Charging-Disabled state),
immediate fault-reset remediation (`:852`), repeated-fault escalation
(`:963`), alert mapping to ledger alerts 20/21/23 (`:920`, `:1088`),
gauge/PMIC contradiction forensics (`:1116`), and the WO-002/003
stale-gauge detection + quickStart resync (`:1173`, action `:1265`) — the
natural function-4 core, per both investigations: quiet-radio
prerequisite, debounce, cooldown, and the resync action/result contract
already live in `BatteryAuthorityPolicy.cpp`; `SensorManager` owns the
actual `quickStart()`/settle/reread orchestration and counter resets.

**Note on the latching-fault-register hazard** (the same BQ24195 REG09
concern that shaped WO-2026-08-05-003's Option A/B decision): there are
multiple independent PMIC status/fault reads across diagnostics and
remediation. This inventory only confirms these are separate hardware
calls — it does not re-verify whether any of them actually corrupt the
"authoritative" remediation snapshot; that would need the same
close reading WO-003 did for its specific change, not assumed from this
broader pass.

Stuck-fast-charging detection (`:1485`) and thermal charge control
(`:1727`) both consume 1/2 to drive 4-style actuation — thermal control
in particular fuses state-classification and direct PMIC actuation in the
same function, one of the vision doc's flagged "open calls."

### Doesn't fit cleanly

Input-source profile selection (`PowerManager.cpp:26`, choosing/applying
USB-bench vs. solar PMIC charge profiles) is adjacent supply
configuration — not SOC estimation, battery-state derivation, operational
policy, or fault correction. It's the one thing `PowerManager` actually
implements today, and it doesn't map to any of the four functions.

## 3. WO-2026-08-05-002/003 cross-reference

Confirmed (both investigations agree): the runtime integration matches
the WOs' intended design, including the WO-003 ordering fix — a candidate
SOC is validated before being committed as the ledger value, not after.

**The pre-radio/post-connect delta-comparison machinery** (the vision
doc explicitly floats retiring this) is primarily **function 1, not
function 3** — it decides whether a post-radio sample is trustworthy, not
whether an operation is authorized; radio/cloud state is a measurement-context
input here, not the thing being gated. It also covers a materially
different failure mode than the newer stale-SOC/Vcell rule: a one-wake
large delta with weak voltage corroboration, versus a persistent SOC/vcell/PMIC
contradiction across wakes. Retiring it outright would remove real,
distinct coverage, not just redundant overlap — worth weighing carefully
at scoping time rather than assuming it's now fully subsumed.

## Boundary cases (things that straddle the four-function split)

1. **Thermal charging permission/actuation** (`SensorManager.cpp:1727`) —
   state classification and direct PMIC actuation are fused in one
   function today. The vision doc already flags this as an open call
   (does thermal-disable logic belong in function 2 or function 4) —
   this investigation confirms it's currently neither cleanly, it's both
   at once.
2. **Pre-radio/post-connect delta comparison** — functionally function 1,
   but its *trigger* is tied to radio/cloud lifecycle state, which is
   conceptually function-3 territory (operational context). See above.

## Verification note

Every load-bearing claim above that materially affects a future scoping
decision was independently verified by direct source reading (Claude),
not just accepted from Codex's independent investigation — specifically
the `PowerManager` dead-field claim, the `DEEP_ATTEMPT_SOC_THRESHOLD` raw-SOC
threshold, and the `shouldUseBoronRtcAlarmHibernate()` battery-blind
signature. The full call-site-by-call-site tables from both passes are
more granular than what's condensed here; ask if the complete detail is
needed before scoping.
