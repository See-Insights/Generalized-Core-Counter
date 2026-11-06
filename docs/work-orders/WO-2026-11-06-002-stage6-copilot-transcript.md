# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `1309f327-bc93-4ed7-9759-6d769ebf83a8`  
> - **Started:** 11/6/2026, 8:54:06 PM  
> - **Duration:** 2m 10s  
> - **Exported:** 11/6/2026, 8:56:17 PM  

---

<sub>6s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-06-002-stage6-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>8s</sub>

### `bash`

**Read dispatch doc**

`$ cat docs/work-orders/WO-2026-11-06-002-stage6-copilot-dispatch.md; git status --short | head`

<details>
<summary>13 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/` (the S1b sites: a new pre-bind header under `src/time/` or equivalent, `src/Generalized-Core-Counter.cpp` before `ExternalRtc::ensureBound`, and the comment-only corrections the WO lists), tests under `tests/` (new S1b decoder/WRTC/seed/structural tests; adjust `tests/stubs/` only if needed), and write the report; run host tests (zsh for .sh, python3 for .py), the local Device OS toolchain build and nm/objdump; scratch `build-tmp/wo-2026-11-06-002-stage6/` (create it; use only that) / NOT authorized: committing, pushing, merging, flashing, creating PRs or tags, touching `lib/`, changing the persisted or `retained` layout, touching S1's read-back/bind logic beyond the call-site insertion, deleting anything you did not create (including `build-tmp/` itself).

# Stage 6 dispatch: WO-2026-11-06-002 (v50 piece S1b: pre-bind access to the AB1805)
Binding spec: `docs/work-orders/WO-2026-11-06-002-v50-s1b-pre-bind-rtc-access.md` **including Amendment 1** (12/24-hour normalisation, Wire lock and `Wire.begin()`, the updated 256-value property; it supersedes "bit 0 only") and its parent `docs/work-orders/WO-2026-11-05-002-v50-ab1805-switch.md`. Base: branch `wo/2026-11-06-002-v50-s1b-pre-bind-rtc-access` on `integration/v50-ab1805` at d6da67c (S1 merged, PR #195). **Budget: `src/` at most +15 net** (removals count; comment corrections count). Leave changes uncommitted. **Run at the dispatched reasoning level (medium); if the harness runs you lower, say so first in the report.**

## Do
1. **Step 0 first, answered in the report with file:line** (the WO's six questions, with item 1 now covering the Wire lock/`Wire.begin()` of Amendment 1). Read Device OS 6.6.0 from `~/.particle/toolchains/deviceOS/6.6.0`. If any answer breaks the plan, **stop and report**. Re-cite every line number in the WO against this tree.
2. **Implement "The change" items 1-5 with Amendment 1:** decode registers 0x00-0x07 in the chip's ORIGINAL 12/24 mode (pure, host-testable; year 2000-2099; invalid BCD, year 00, all-zero, all-FF not plausible); then, under one `WITH_LOCK(Wire)` spanning the raw read and the CONTROL1 read-modify-write (and `Wire.begin()` first if Wire is not enabled), write CONTROL1 = `(v | 0x01) & ~0x40`; seed `Time` only if the chip time is plausible (`Clock::isPlausibleEpoch` or the equivalent in the tree) and `Time` is not valid; run only when `!Am18x5.status().bound()`, strictly before `ensureBound`; failures log and the boot continues (never a reset). Correct the S1 comments that say the first unbound boot "can miss" the seed.
3. **Tests and mutations** exactly as the WO's Tests section plus Amendment 1 item 4 (each mutation must fail; back up the file, mutate, restore from the backup; never use `git checkout`/`git restore` while your own uncommitted work is present). **Verify:** host suite `N/N (sh via zsh, py via python3)` (count with `p=$((p+1))`, never `((p++))`; `thermal_coupling_structural_test.py` also on a clean export of tracked files; some tests need `git`, run them in the checkout); local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE` passed as a make argument) with text/data/bss, and `nm`/objdump showing the new code referenced from `setup()`; `git diff --numstat -- src` net <= +15 (state the number); layouts and fixtures untouched.
## Stop and report if
the budget cannot hold; a raw `Wire` access before `Am18x5.enable()` conflicts with Device OS's own interface setup; `Time.setTime()` before `enable()` is not seen by the bind; any S1 behaviour would have to change.
## Report
`docs/work-orders/WO-2026-11-06-002-stage6-copilot-report.md`: model and reasoning actually used (say plainly if lower than dispatched), interpreter line, Step 0 answers with file:line, net `src/` lines per file, test names and mutation results, build and `nm` evidence, deviations, open questions. The bench (Dev-14, `settime ... hal`) is Chip's later step on the integrated head: do not claim it.
<shellId: 0 completed with exit code 0>
```

</details>


---

<sub>10s</sub>

### `bash`

**Read WO and tree**

`$ cat docs/work-orders/WO-2026-11-06-002-v50-s1b-pre-bind-rtc-access.md; git log --oneline | head -3; git branch --show-current; ls src src/time tests | head -80`

<details>
<summary>137 lines</summary>

```
# WO-2026-11-06-002: v50 piece S1b — pre-bind access to the AB1805: seed the clock, set WRTC

**Author:** Claude Code, 6 Nov 2026, DRAFT for Chip's review; not dispatched · **Status:** **APPROVED by Chip, 6 Nov 2026** (rulings after Codex's Stage 7 round 1 on S1); dispatch after S1's Stage 7 round 2
**Parent:** `WO-2026-11-05-002-v50-ab1805-switch.md` · **Origin:** `WO-2026-11-05-003-stage7-r1-verdict.md` findings 1 and 2; S1 Amendment 4 · **Position:** on `integration/v50-ab1805` **between S1 and S2** (S1 → S1b → S2 → S3, S4)
**Branch (when dispatched):** `wo/…-v50-s1b-…`, from `integration/v50-ab1805` after S1 has passed Stage 7 round 2 and its PR has been merged to the integration branch; S1b's PR goes to the integration branch, **never to `main` alone**; the set merges to `main` only under the 6 Nov merge gate.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule; the 90-minute Stage 7 time-box once #194 merges); §5 (boot-time hardware and time: Codex xhigh); §12; §13 (c), (j) · **Agents:** Stage 6 Copilot standard/medium (re-run at the dispatched level if a run comes back lower on anything larger than a few lines); Stage 7 Codex `gpt-5.6-sol` **xhigh**, at most two rounds · **Budget:** `src/` at most **+15 net** (removals count); stop and report rather than trim style.
**Dispatch gate:** S1 reviewed and merged to the integration branch (it needs S1's bind call to sit just after this code).

## Plain goal
Two defects found in Device OS 6.6.0's built-in AM18x5 driver by S1's Stage 6 and Stage 7 round 1 make the switch unsafe on the real fleet:
1. **Writes can silently no-op (WRTC).** The chip only accepts writes to its time registers while the control-register write-enable bit **WRTC** is 1. Device OS's `Am18x5::setTime()` never sets it and `writeContinuousRegisters()` ignores every I2C result (`am18x5.cpp:770-788`, `:1350-1359`), so a chip whose WRTC is 0 returns success and keeps its old time. **Every chip the vendored library has touched has WRTC = 0:** the library sets WRTC, writes, then clears it (`lib/AB1805_RK/src/AB1805_RK.cpp:256-295`; `isRTCSet()` is literally "WRTC is clear", `AB1805_RK.h:97`). S1's read-back (Amendment 3) detects the failure; it cannot make the write work.
2. **The first unbound boot throws the chip time away.** On the first v50 boot after v49 (no stored binding), Device OS's bind copies the internal clock (year 2000, nothing has seeded it) into the chip and never reads the valid chip time into the internal clock (`exrtc_hal.cpp:236-249`). Once WRTC works that overwrite takes effect.

S1b fixes both from the application side, in the one window where the application is the only owner of the chip: **before `Am18x5.enable()`, on a boot where the chip is not bound, one raw I2C access to address 0x69.** (A Device OS issue is drafted for Chip to file; if Device OS fixes it later this piece becomes unnecessary but is harmless.)

## Rulings that bind this piece (Chip, 6 Nov; not reopened)
1. **Where and when:** on a boot where the chip is **not bound**, strictly **before** `Am18x5.enable()` (S1's `ExternalRtc::ensureBound`, `Generalized-Core-Counter.cpp:~296`); never on a bound boot (Device OS owns the chip then). Single owner: nothing else touches the chip at that moment (`ab1805` is not started: S1 removed `.setup()`).
2. **(a) Seed:** read the chip's time; **if it is plausible and system `Time` is not valid, seed `Time` from it.**
3. **(b) WRTC:** **set WRTC = 1** (the chip's power-on default; Device OS never clears it).
4. **Then S1's `enable()`.** S1's read-back stays as the detector.
5. **First unbound boot:** with (a), the internal clock is valid before the bind, so the bind's copy writes the same time back (within rounding) and the first v50 boot keeps valid time. (This reverses S1 Amendment 3 ruling 2.)

## The change (lines at the integration head after S1; **re-cite at Step 0**)
1. A small pure, host-testable decoder (BCD registers 0x00-0x07 → `time_t`, honouring the chip's 12/24-hour bit; year 2000-2099) and a read-modify-write of CONTROL1 (set WRTC, clear the 12/24 bit, keep every other bit: **Amendment 1 supersedes "bit 0 only"**). A new header is allowed if the budget holds (S1 added `time/ExternalRtc.h` the same way).
2. In `setup()`, immediately before S1's `ensureBound`: `if (!Am18x5.status().bound()) { PreBindRtc::run(...); }` with failures **logged and the boot continued** (never a reset; the bind and the internal-clock fallback still happen).
3. Plausibility is `Clock::isPlausibleEpoch` (`Clock.cpp:551-555`) or the equivalent already in the tree; no new rule.
4. **Comments S1 wrote that this piece makes false** are corrected in the same change (comment-only, counted in the budget): S1 documented that the first unbound boot "can miss" the seed (`src/BuildProfile.h`, `src/MyPersistentData.h`, `src/persist/CurrentReadings.h`, `src/Generalized-Core-Counter.cpp` near the bind, `src/time/ClockTrust.h`); with S1b the first unbound boot is seeded before the bind.
5. Nothing else: no change to the write-back, the payload, the failsafe, `retained` or persisted layouts.

Register facts (Device OS 6.6.0 headers; Step 0 re-verifies): `HUNDREDTHS` at 0x00 (`am18x5.h:39`), `CONTROL1` at 0x10 (`am18x5.h:57`), `CONTROL1_WRTC_MASK` 0x01 (`am18x5_defines.h:67`; the only use of WRTC in the driver), `CONTROL1_1224_MASK` 0x40; the vendored library's `REG_CTRL_1_DEFAULT` is 0x13 (WRTC set), `AB1805_RK.h:795`.

## Step 0 (read-only, inside Stage 6, before editing; stop and report if any answer breaks the plan)
1. **The I2C window.** Who owns `Wire` at that point in `setup()` (the fuel gauge and PMIC share the bus); whether `Wire.begin()` has run and `WITH_LOCK(Wire)` is needed; whether Device OS's own `enableInterfaceIfNeeded()` (`am18x5.cpp:~1352`) conflicts with an application `Wire` session that ends before `enable()`.
2. **Decode details:** the time-register layout and the BCD/hour-mode rules (`am18x5.cpp:770-788` is the write side; read the `getTime` side `:791-814`); behaviour on an invalid BCD digit or a chip reading year 00 (not plausible: do not seed).
3. **Seeding:** `Time.setTime()` versus the HAL clock Device OS seeds; whether setting `Time` before `enable()` is seen by `bindIfNeededAndResync` (it must be, so the bind writes the right time back); no cloud-sync or event side effect of an early `Time.setTime()`.
4. **WRTC set when already 1** is a no-op; confirm the read-modify-write does not disturb the other CONTROL1 bits (STOP, 12/24, OUT, RSO, PWR2).
5. **Rollback (ruled by Chip, 6 Nov): accepted, no code change.** After v50 has set WRTC = 1, the vendored library's `isRTCSet()` (WRTC clear = set) reads the chip as unset, so a rollback to v49 waits for the cloud for its time. This is the same bounded exposure as any untrusted boot, and **v49's first sync re-writes the time and clears WRTC.** Step 0 still reads v49's `ab1805.setup()` and the clock code to confirm there is no reset loop and no bad seed (a check, not a reopening), and the answer goes into the rollback bench as the expected result.
6. **Count** the net `src/` lines per file; if over +15, stop and report the cut.

## Tests (host; the real functions)
- Decoder table: valid 24 h and 12 h (AM/PM) times, year rollover, leap day, an invalid BCD digit, year 00, all-zero and all-FF registers → not plausible.
- Seed: plausible chip time and `Time` invalid → seeded; `Time` already valid → not seeded; implausible chip time → not seeded.
- WRTC: CONTROL1 `0x12` → writes `0x13`; `0x13` → same value; result = `(v | 0x01) & ~0x40` with every other bit preserved (property over all 256 values; **Amendment 1**).
- Control flow: not run when the chip is bound; runs once, **before** `enable()` (structural, in order); an I2C failure logs and the boot continues to `enable()` with no reset.
- Mutations (each must fail): WRTC write clears other bits; seed even when `Time` is valid; run on a bound boot; run after `enable()`; skip the plausibility check; wrong I2C address; reset on I2C failure; the 12/24 bit ignored.

## Bench (Chip; part of the integrated head's bench, not alone)
On **Dev-14 (old-library history: WRTC = 0)** with the spike binary's `settime <epoch> hal`:
1. **Without S1b** (S1 only): `settime … hal` fails (the read-back catches the no-op) or the chip keeps its old time; record it.
2. **With S1b:** the same `settime … hal` succeeds and reads back within 2 s.
3. **First v50 boot after v49** (no stored binding): the chip keeps valid time through the bind (read it before the cloud sync: not year 2000), and `Time` is valid at the first log lines.
4. **Rollback v50 → v49 (expected result, ruled by Chip):** on the first v49 boot after v50 has set WRTC = 1 the vendored `isRTCSet()` reads the chip as **unset**; v49 has no valid time until the cloud sync (untrusted, like any untrusted boot); **v49's first sync re-writes the chip's time and clears WRTC** (confirm WRTC reads 0 afterwards and a second v49 boot reads the chip as set). No reset loop; no bad seed. A different result is a finding.

## Protected / not in scope
Device OS source; S1 (the read-back, the bind), S2 (alarm, watchdog, wake classification, `ab1805.loop()`), S3, S4, L, I21; the clock-trust rule; the persisted and `retained` layouts.

## Amendment 1 (Chip, 6 Nov 2026, after Codex's Stage 7 round 2 on S1; supersedes "set bit 0 only")
Origin: Codex round 2, High design gap. Device OS's `setTime()` always writes the hour as 24-hour BCD (`am18x5.cpp:782`) and the bind changes only ARST (`:616`), so a chip left in 12-hour mode would be decoded correctly but then be written a 24-hour hour and corrupt the first-boot time. Ruling:
1. **Decode the chip's time in its ORIGINAL 12/24 mode** (as item 1 already says).
2. **Then read-modify-write CONTROL1: set WRTC and clear the 12/24 bit (`CONTROL1_1224_MASK`, 24-hour mode); preserve every other bit** (STOP, OUT, RSO, PWR2, ...). This replaces "set bit 0 only, keep the other bits" in "The change" item 1 and in Step 0 item 4.
3. **Wire ownership:** hold `WITH_LOCK(Wire)` across the raw time read AND the CONTROL1 read-modify-write (one critical section), and call `Wire.begin()` first if `Wire` is not enabled. Step 0 item 1 records how `setup()` order guarantees this; a Wire left enabled must not conflict with Device OS's `enableInterfaceIfNeeded()`.
4. **Tests:** the WRTC property over all 256 CONTROL1 values becomes: result = `(v | 0x01) & ~0x40`, every other bit unchanged (so `0x12` -> `0x13`, `0x52` -> `0x13`, `0x53` -> `0x13`). Add: chip in 12-hour PM mode decodes correctly, and after S1b the chip reads back in 24-hour mode with the same wall time. The existing mutation "12/24 bit ignored" stays; add "12/24 bit not cleared" and "Wire lock not held across the read-modify-write" (structural).
5. **Budget unchanged: `src/` at most +15 net.** Stop and report if the amendment does not fit.

## Rollback
Revert the S1b commit on the integration branch before the set merges. After the set ships, rollback is to v49.1 (the guard deletes any stored binding); Step 0 item 5 and bench item 4 cover the WRTC state it leaves.
96769a0 WO-2026-11-06-002 (S1b): work order (with Amendment 1) and Stage 6 Copilot dispatch
d6da67c Merge pull request #195 from See-Insights/wo/2026-11-05-003-v50-s1-boot-config-time
e6c4e73 WO-2026-11-05-003: Stage 7 round 2 verdict (PASS for S1 as scoped on ed6ca1b; S1b 12/24-hour design gap for the architect)
wo/2026-11-06-002-v50-s1b-pre-bind-rtc-access
src:
BuildProfile.h
Config.cpp
Config.h
FirmwareVersion.h
Generalized-Core-Counter.cpp
MyPersistentData.cpp
MyPersistentData.h
ResetCause.h
ThrashGuard.cpp
ThrashGuard.h
cloud
device_pinout.cpp
device_pinout.h
diagnostics
observability
persist
power
reporting
sensors
state
time

src/time:
Clock.cpp
Clock.h
ClockDiagnostics.cpp
ClockDiagnostics.h
ClockTrust.h
DailyBoundary.cpp
DailyBoundary.h
ExternalRtc.h
HibernateCycle.cpp
HibernateCycle.h
HibernateEligibility.cpp
HibernateEligibility.h
HibernateWakeDiagnostics.h
LocalTimeCache.cpp
LocalTimeCache.h
RtcSkewTest.h

tests:
README.md
SensorManagerBatteryAuthorityTests.cpp
alert19_clears_after_report_test.sh
awake_cycle_counters_test.sh
battery_authority_commit_sites_structural_test.py
battery_authority_seam_structural_test.py
battery_authority_structural_test.py
battery_health_test.cpp
battery_health_test.sh
battery_tier_guard_test.cpp
battery_tier_guard_test.sh
boron_soc_commit_test.py
build_flags_witness_test.sh
bump_version_scheme_test.sh
charge_inhibit_thermal_test.cpp
charge_inhibit_thermal_test.sh
clock_hibernate_helpers_move_structural_test.py
clock_openness_test.cpp
clock_openness_test.sh
clock_owner_structural_test.py
clock_resync_wiring_test.py
clock_rtc_writeback_test.cpp
clock_status_republish_test.cpp
clock_status_republish_test.sh
clock_trust_standard_structural_test.py
clock_trust_test.cpp
close_before_night_sleep_test.py
config_window_hold_test.cpp
config_window_hold_test.sh
conn_phase_modem_off_test.sh
connect_attempt_counter_test.sh
connect_no_modem_wait_structural_test.py
connected_online_last_connection_test.cpp
connected_online_last_connection_test.sh
connection_mode_downgrade_structural_test.py
connection_mode_downgrade_test.cpp
connection_mode_downgrade_test.sh
connection_signal_validity_test.sh
<shellId: 1 completed with exit code 0>
```

</details>


---

<sub>14s</sub>

### `bash`

**Inspect S1 code sites**

`$ cat src/time/ExternalRtc.h; grep -n "ensureBound\|Am18x5\|Wire\|ExternalRtc\|setup()" src/Generalized-Core-Counter.cpp | head -40; grep -rn "can miss\|miss the seed\|miss" src/BuildProfile.h src/MyPersistentData.h src/persist/CurrentReadings.h src/time/ClockTrust.h src/Generalized-Core-Counter.cpp | head -20; ls tests | grep -i "rtc\|external\|s1"; sed -n 545,560p src/time/Clock.cpp; git diff --numstat integration/v50-ab1805 -- src`

<details>
<summary>129 lines</summary>

```
#pragma once
// v50 S1: Device OS owns the carrier AB1805. Every app read/write of the chip goes through hal_rtc_* (external source).
#include "Particle.h"

namespace ExternalRtc {
constexpr time_t kWriteVerifyToleranceSec = 2;  // read-back must land within this of the written time
inline bool get(time_t &t) {
  struct timeval tv = {};
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  if (hal_rtc_get_time(&tv, &opt) != 0) return false;
  t = tv.tv_sec;
  return true;
}
inline bool set(time_t t) {
  struct timeval tv = {};
  tv.tv_sec = t;
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  if (hal_rtc_set_time(&tv, &opt) != 0) return false;
  time_t back = 0;
  return get(back) && (back > t ? back - t : t - back) <= kWriteVerifyToleranceSec;  // a silent no-op write fails
}
inline bool setFromSystem() { return Time.isValid() && set(Time.now()); }
inline bool isSet() {
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  return hal_rtc_time_is_valid(&opt);
}
// The fields the fleet configuration fixes (including the clock source: Device OS reports the actual one).
template <typename C> bool sameFleetConfig(C &a, C &b) {
  return a.valid() && a.address() == b.address() && a.interruptPin() == b.interruptPin() &&
         a.watchdogPin() == b.watchdogPin() && a.capabilities().value() == b.capabilities().value() &&
         a.defaultTimeSource() == b.defaultTimeSource() && a.clockSource() == b.clockSource() && a.halDevice()->i2c.interface == b.halDevice()->i2c.interface;
}
// Bind once with the fleet configuration; a failure is logged and the boot continues on the internal clock.
template <typename R, typename C> int ensureBound(R &rtc, const C &fleet) {
  int r = 0;
  if (!rtc.status().bound()) {
    r = rtc.enable(fleet);
    Log.info("AB1805: enable r=%d", r);
  } else {
    C stored = rtc.getConfig();
    C want = fleet;
    if (!sameFleetConfig(stored, want)) {
      r = rtc.enable(fleet);
      Log.warn("AB1805: stored config differs, re-bound r=%d", r);
    }
  }
  return r;
}
}  // namespace ExternalRtc
48:#include "time/ExternalRtc.h"
88: * - State machine: setup()/loop() implement a simple state machine
106: * 2) setup()/loop() and top-level lifecycle wiring
126:AB1805 ab1805(Wire);   // AB1805 RTC / Watchdog
200:// One struct with a magic and version (checked first in setup()) pins their layout.
215:// bench-only RTC skew hook (below, in setup()) has already fired.
246:// boot (see setup()). Only meaningful when startupPinResetAb1805Checked is
253:// Oscillator state captured once in setup() and published by DeviceStatusPublisher.cpp.
267:// further below in setup()). The global SerialLogHandler's constructor
268:// already calls Serial.begin() before setup() runs, so Serial.isConnected()
291:void setup() {
296:  ExternalRtc::ensureBound(Am18x5, particle::Am18x5Configuration().i2c(Wire).interruptPin(WKP).watchdogPin(WATCHDOG_DONE_PIN)
490:  SystemConfig::setup();    // Initialize persistent storage
491:  SystemConfig::SensorSettings::setup(); // Initialize the sensor configuration
492:  CurrentReadings::setup();      // Initialize the current status data
507:  // (AC-C5): PowerManager::instance().setup() is deliberately NOT called
524:  // setup(), and no other PowerManager::instance() call site exists in this
536:  // see the "WATCHDOG RESET CLASSIFICATION" block further down in setup().
629:    // is handled later in setup() after timezone configuration is complete.
652:      .setup(); // Initialize the publish queue
655:  // Device OS seeds Time from the chip before setup() when it is already bound; ensureBound() above covers the first boot.
659:  startupOscUsingRC = Am18x5.status().clockSource() == particle::RtcClockSource::INTERNAL;
660:  const auto oscCaps = Am18x5.getConfig().capabilities();
674:  // hibernate-wake classification block below, so the rest of setup() sees
678:  // A.3: RTC write only - ExternalRtc::set() - nothing else. In
719:      const bool rtcSkewReadBeforeOk = ExternalRtc::get(rtcSkewRawBefore);
722:      const bool rtcSkewWriteOk = ExternalRtc::set(rtcSkewAfter);
725:        const bool rtcSkewVerifyReadOk = ExternalRtc::get(rtcSkewVerify);
737:        Log.error("RtcSkewTest: ONE-SHOT bench hook FAILED - ExternalRtc::set() write did not "
749:  // independently, at two different points in setup(). Both are safe to
752:  // updateWakeReason() call AB1805::setup() already made) - but there is no
779:  // Moved here (from immediately after SystemConfig::setup(), before AB1805 init)
818:  // earlier in setup(), not by this read itself).
835:             ExternalRtc::isSet() ? "true" : "false",
883:  Cloud::instance().setup(); // Initialize the cloud functions
937:  // point in setup() (no connection has been attempted yet this boot), the
1032:  // (AC-C5): PowerManager::instance().setup() (the DCT rebuild formerly done
1033:  // at the top of setup(), before the blocking USB serial wait) now runs
1049:  PowerManager::instance().setup();
1254: * @note Large payload (~896 bytes). Called once during setup(); queued for first connection.
src/BuildProfile.h:229: * When enabled, each PowerDiag/ChargeDiag/stale-SOC-resync diagnostic emission
src/BuildProfile.h:277:// here, expanded at both emission sites (the Boot: serial line's "flags=" and
src/MyPersistentData.h:16: * Permission is hereby granted, free of charge, to any person obtaining a copy
src/MyPersistentData.h:23: * The above copyright notice and this permission notice shall be included in all
src/MyPersistentData.h:631:	 * Device OS seeds Time only on a bound boot, so that clamp can miss the first boot (see
src/persist/CurrentReadings.h:47: * Device OS seeds Time only on a bound boot, so it can miss the first boot (WO-2026-09-22-001).
clock_rtc_writeback_test.cpp
external_rtc_structural_test.py
external_rtc_test.cpp
rtc_skew_test.cpp
rtc_skew_test.sh
bool isTimeValid() {
  return Time.isValid();
}

bool isPlausibleEpoch(time_t epoch) {
  static const time_t kEpochMin = 1704067200; // 2024-01-01 00:00:00 UTC
  static const time_t kEpochMax = 2051222400; // 2035-01-01 00:00:00 UTC
  return epoch >= kEpochMin && epoch < kEpochMax;
}

} // namespace Clock

// ===== New in Step 3b =====

namespace Clock {

1	1	src/BuildProfile.h
29	64	src/Generalized-Core-Counter.cpp
1	1	src/MyPersistentData.h
1	1	src/ResetCause.h
3	5	src/cloud/DeviceStatusPublisher.cpp
1	0	src/device_pinout.cpp
1	0	src/device_pinout.h
1	1	src/persist/CurrentReadings.h
4	6	src/time/Clock.cpp
2	2	src/time/ClockTrust.h
55	0	src/time/ExternalRtc.h
3	2	src/time/HibernateEligibility.cpp
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>16s</sub>

### `bash`

**Read Device OS driver**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; ls $D | head; F=$(find $D -name am18x5.cpp | head -1); echo $F; sed -n 600,625p $F; sed -n 765,820p $F; sed -n 1335,1365p $F`

<details>
<summary>112 lines</summary>

```
CHANGELOG.md
CONTRIBUTING.md
Doxyfile
ISSUE_TEMPLATE.md
LICENSE
PULL_REQUEST_TEMPLATE.md
README.md
STYLE_GUIDE.md
bootloader
build
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/am18x5.cpp
        oscControl &= ~OSC_CONTROL_OFIE_MASK;
    }
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
    CHECK(writeRegister(Am18x5Register::OSC_CONTROL, oscControl));

    // Digital calibration to improve accuracy.
    CHECK(xtOscillatorDigitalCalibration(configuredXtalCalibration()));

    // Enable square wave output on the CLKOUT pin
    if (config_.clk_out_en) {
        CHECK(enableClkOut(config_.clk_out_freq));
    } else {
        CHECK(disableClkOut());
    }

    // Automatically clear interrupt flags after reading the the status register.
    CHECK(writeRegister(Am18x5Register::CONTROL1, 1, false, true, CONTROL1_ARST_MASK, CONTROL1_ARST_SHIFT));
    return SYSTEM_ERROR_NONE;
}

int Am18x5::getOscillatorSource(Am18x5Oscillator* source) {
    Am18x5Lock lock;
    CHECK_TRUE(source, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t oscStatus = 0;
    CHECK(readRegister(Am18x5Register::OSC_STATUS, &oscStatus));
        return SYSTEM_ERROR_NONE;
    }
    return onOscillatorEvent(events, events ? exRtcOscEventHandler : nullptr, this);
}

int Am18x5::setTime(const struct timeval* tv) const {
    struct tm calendar;
    struct timeval canonical = *tv;
    CHECK(canonicalizeTimeval(&canonical));
    CHECK(timevalToCalendar(&canonical, &calendar));

    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t buff[8] = {0};
    buff[0] = CHECK(decToBcd(canonical.tv_usec / MICROS_IN_HUNDREDTH));
    buff[1] = CHECK(decToBcd(calendar.tm_sec));
    buff[2] = CHECK(decToBcd(calendar.tm_min));
    buff[3] = CHECK(decToBcd(calendar.tm_hour));
    buff[4] = CHECK(decToBcd(calendar.tm_mday));
    buff[5] = CHECK(decToBcd(calendar.tm_mon + 1)); // Month in tm structure ranges from 0 - 11.
    buff[6] = CHECK(decToBcd(calendar.tm_year % 100));
    buff[7] = CHECK(decToBcd(calendar.tm_wday));
    CHECK(writeContinuousRegisters(Am18x5Register::HUNDREDTHS, buff, sizeof(buff)));
    return SYSTEM_ERROR_NONE;
}

int Am18x5::getTime(struct timeval* tv) const {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);

    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t buff[8] = {0};
    CHECK(readContinuousRegisters(Am18x5Register::HUNDREDTHS, buff, sizeof(buff)));

    struct tm calendar = {};
    calendar.tm_sec = CHECK(bcdToDec(buff[1]));
    calendar.tm_min = CHECK(bcdToDec(buff[2]));
    calendar.tm_hour = CHECK(bcdToDec(buff[3]));
    calendar.tm_mday = CHECK(bcdToDec(buff[4]));
    calendar.tm_mon = CHECK(bcdToDec(buff[5]));
    calendar.tm_mon -= 1;
    calendar.tm_year = CHECK(bcdToDec(buff[6]));
    calendar.tm_year += TM_CALENDAR_YEAR_2000;
    calendar.tm_wday = CHECK(bcdToDec(buff[7]));
    calendar.tm_isdst = -1;

    tv->tv_sec = mktime(&calendar);
    tv->tv_usec = CHECK(bcdToDec(buff[0])) * MICROS_IN_HUNDREDTH;
    return SYSTEM_ERROR_NONE;
}

bool Am18x5::isTimeValid(struct timeval* tv) const {
    struct timeval tempTv = {};
    if (getTime(&tempTv) == SYSTEM_ERROR_NONE) {
        if (tempTv.tv_sec > UNIX_TIME_20180101000000) {
            if (tv) {
    if (rw) {
        CHECK(readRegister(reg, &currValue));
    }
    currValue &= ~mask;
    if (bcd) {
        CHECK(val = decToBcd(val));
    }
    currValue |= (val << shift);
    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(reg), nullptr);
    hal_i2c_write(config_.i2c_if, currValue, nullptr);
    hal_i2c_end_transmission(config_.i2c_if, true, nullptr);
    return SYSTEM_ERROR_NONE;
}

int Am18x5::writeContinuousRegisters(const Am18x5Register start_reg, const uint8_t* buff, size_t len) const {
    Am18x5Lock lock;
    CHECK(enableInterfaceIfNeeded());
    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(start_reg), nullptr);
    for (size_t i = 0; i < len; i++) {
        hal_i2c_write(config_.i2c_if, buff[i], nullptr);
    }
    hal_i2c_end_transmission(config_.i2c_if, true, nullptr);
    return len;
}

int Am18x5::readRegister(const Am18x5Register reg, uint8_t* const val, bool bcd, uint8_t mask, uint8_t shift) const {
    Am18x5Lock lock;
    CHECK(enableInterfaceIfNeeded());
    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
<shellId: 3 completed with exit code 0>
```

</details>


---

<sub>21s</sub>

### `bash`

**Read enableInterface, bind, call sites**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; F=$D/hal/shared/am18x5.cpp; grep -n "enableInterfaceIfNeeded" -A14 $F | sed -n 1,50p | grep -v "^--$" | head -45; grep -n "bindIfNeededAndResync" -B3 -A40 $D/hal/src/nRF52840/exrtc_hal.cpp $D/hal/shared/exrtc_hal.cpp 2>/dev/null | head -90; find $D -name exrtc_hal.cpp; sed -n 285,300p src/Generalized-Core-Counter.cpp; sed -n 650,658p src/Generalized-Core-Counter.cpp; sed -n 625,634p src/MyPersistentData.h; sed -n 44,50p src/persist/CurrentReadings.h; grep -n "miss\|first.*unbound\|first boot" src/time/ClockTrust.h src/BuildProfile.h`

<details>
<summary>177 lines</summary>

```
1323:int Am18x5::enableInterfaceIfNeeded() const {
1324-    if (!hal_i2c_is_enabled(config_.i2c_if, nullptr)) {
1325-        hal_i2c_begin(config_.i2c_if, I2C_MODE_MASTER, 0x00, nullptr);
1326-        CHECK_TRUE(hal_i2c_is_enabled(config_.i2c_if, nullptr), SYSTEM_ERROR_INTERNAL);
1327-    }
1328-    return SYSTEM_ERROR_NONE;
1329-}
1330-
1331-int Am18x5::writeRegister(const Am18x5Register reg, uint8_t val, bool bcd, bool rw, uint8_t mask, uint8_t shift) const {
1332-    Am18x5Lock lock;
1333:    CHECK(enableInterfaceIfNeeded());
1334-    uint8_t currValue = 0x00;
1335-    if (rw) {
1336-        CHECK(readRegister(reg, &currValue));
1337-    }
1338-    currValue &= ~mask;
1339-    if (bcd) {
1340-        CHECK(val = decToBcd(val));
1341-    }
1342-    currValue |= (val << shift);
1343-    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
1344-    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(reg), nullptr);
1345-    hal_i2c_write(config_.i2c_if, currValue, nullptr);
1346-    hal_i2c_end_transmission(config_.i2c_if, true, nullptr);
1347-    return SYSTEM_ERROR_NONE;
1352:    CHECK(enableInterfaceIfNeeded());
1353-    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
1354-    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(start_reg), nullptr);
1355-    for (size_t i = 0; i < len; i++) {
1356-        hal_i2c_write(config_.i2c_if, buff[i], nullptr);
1357-    }
1358-    hal_i2c_end_transmission(config_.i2c_if, true, nullptr);
1359-    return len;
1360-}
1361-
1362-int Am18x5::readRegister(const Am18x5Register reg, uint8_t* const val, bool bcd, uint8_t mask, uint8_t shift) const {
1363-    Am18x5Lock lock;
1364:    CHECK(enableInterfaceIfNeeded());
1365-    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
1366-    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(reg), nullptr);
1367-    hal_i2c_end_transmission(config_.i2c_if, false, nullptr);
1368-    if (hal_i2c_request(config_.i2c_if, AM18X5_I2C_ADDR, 1, true, nullptr) == 0) {
1369-        return SYSTEM_ERROR_INTERNAL;
1370-    }
1371-    *val = hal_i2c_read(config_.i2c_if, nullptr);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-233-    return SYSTEM_ERROR_NOT_FOUND;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-234-}
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-235-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp:236:int bindIfNeededAndResync(RtcBinding& desired, RtcBinding* current = nullptr) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-237-    if (current && Am18x5::getInstance().isPresent() && desired == *current) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-238-        return SYSTEM_ERROR_NONE;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-239-    }
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-240-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-241-    CHECK(Am18x5::getInstance().bind(desired.binding()));
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-242-    hal_rtc_option_t rtc = {};
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-243-    rtc.source = HAL_RTC_SOURCE_INTERNAL;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-244-    struct timeval tvInt = {};
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-245-    if (!hal_rtc_get_time(&tvInt, &rtc)) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-246-        return hal_exrtc_set_time_internal(&tvInt);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-247-    }
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-248-    return SYSTEM_ERROR_NONE;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-249-}
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-250-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-251-} // anonymous
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-252-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-253-int hal_exrtc_init(void) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-254-    RtcBinding stored;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-255-    if (!stored.load(SystemCacheKey::EXRTC_CONFIG_DATA)) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-256-        return Am18x5::getInstance().bind(stored.binding());
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-257-    }
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-258-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-259-    auto platformDefault = hal_exrtc_default_binding();
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-260-    if (platformDefault) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-261-        return Am18x5::getInstance().bind(platformDefault);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-262-    }
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-263-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-264-    return SYSTEM_ERROR_NOT_FOUND;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-265-}
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-266-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-267-int hal_exrtc_bind(hal_exrtc_instance_t instance, const hal_exrtc_binding_t* binding, void* reserved) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-268-    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-269-    CHECK_TRUE(binding, SYSTEM_ERROR_INVALID_ARGUMENT);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-270-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-271-    RtcBinding loaded;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-272-    CHECK(loaded.load(binding));
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-273-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-274-    RtcBinding current;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-275-    const int currentRet = loadCurrentBinding(&current);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-276-    CHECK_TRUE(currentRet >= 0 || currentRet == SYSTEM_ERROR_NOT_FOUND, currentRet);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-277-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-278-    CHECK(loaded.store(SystemCacheKey::EXRTC_CONFIG_DATA));
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp:279:    return bindIfNeededAndResync(loaded, currentRet >= 0 ? &current : nullptr);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-280-}
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-281-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-282-int hal_exrtc_get_device(hal_exrtc_instance_t instance, hal_exrtc_device_t* device, void* reserved) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-283-    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-284-    CHECK_TRUE(device && device->size >= sizeof(uint16_t) * 2, SYSTEM_ERROR_INVALID_ARGUMENT);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-285-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-286-    if (Am18x5::getInstance().isPresent()) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-287-        return Am18x5::getInstance().getDevice(device);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-288-    }
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-289-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-290-    RtcBinding binding;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-291-    CHECK(loadCurrentBinding(&binding));
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-292-    auto src = binding.device();
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-293-    CHECK_TRUE(src, SYSTEM_ERROR_NOT_FOUND);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-294-    memcpy(device, src, std::min<size_t>(device->size, src->size));
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-295-    return 0;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-296-}
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-297-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-298-int hal_exrtc_unbind(hal_exrtc_instance_t instance, void* reserved) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-299-    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-300-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-301-    // Ignore errors
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-302-    Am18x5::getInstance().end();
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-303-    SystemCache::instance().del(SystemCacheKey::EXRTC_CONFIG_DATA);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-304-    return 0;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-305-}
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-306-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-307-int hal_exrtc_get_status(hal_exrtc_instance_t instance, hal_exrtc_status_t* status, void* reserved, void* reserved1) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-308-    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-309-    CHECK_TRUE(status && status->size >= sizeof(uint16_t) * 2, SYSTEM_ERROR_INVALID_ARGUMENT);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-310-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-311-    RtcBinding binding;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-312-    int r = loadCurrentBinding(&binding);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-313-    CHECK_TRUE(!r || r == SYSTEM_ERROR_NOT_FOUND, r);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-314-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-315-    hal_exrtc_status_t tmp = {};
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-316-    tmp.size = sizeof(tmp);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-317-    tmp.version = HAL_EXRTC_API_VERSION;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-318-    if (Am18x5::getInstance().isPresent()) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-319-        CHECK(Am18x5::getInstance().getStatus(&tmp));
--
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-344-    RtcBinding current = binding;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp-345-    CHECK(binding.setConfig(const_cast<hal_exrtc_config_t*>(config), const_cast<hal_exrtc_vendor_config_t*>(vendor)));
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/tracker/exrtc_hal.cpp
 *
 * @note Transitions to IDLE_STATE unless boot conditions (button press, OTA update,
 *       invalid config/time) force CONNECTING_STATE for immediate cloud access.
 *
 * @see docs/architecture/startup-sequence.md for detailed initialization phases
 */
void setup() {
  validateRetainedBoot(retainedBoot);
  waitForDebugSerialIfConnected();

  // v50: Device OS owns the AB1805; bind once before anything reads Time.
  ExternalRtc::ensureBound(Am18x5, particle::Am18x5Configuration().i2c(Wire).interruptPin(WKP).watchdogPin(WATCHDOG_DONE_PIN)
      .capabilities(particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL).clockSource(particle::RtcClockSource::EXTERNAL));

  ensureRetainedLoopForensicsInitialized();

  PublishQueuePosix::instance()
      .withFileQueueSize(800)
      .setup(); // Initialize the publish queue

  // ===== TIME AND RTC =====
  // Device OS seeds Time from the chip before setup() when it is already bound; ensureBound() above covers the first boot.
  const bool timeValidBeforeRtc = Time.isValid();

  // Oscillator snapshot: usingRC from the live clock source, AOS/FOS from the configuration. The raw OSC registers are not readable through the public API (published as null).
	/**
	 * @brief Re-run the occupancyStartTime future-date corruption clamp once a
	 *        time source exists this boot.
	 *
	 * validate()'s own future-date clamp for occupancyStartTime runs once,
	 * from the .load() call in setup() - which is called long before
	 * Device OS seeds Time only on a bound boot, so that clamp can miss the first boot (see
	 * WO-2026-09-22-001). Call this once, opportunistically, from global
	 * setup() as soon as a time source is available (Clock::isTimeValid()),
	 * to give the same corruption check a point in boot where it can
 *        time source exists this boot.
 *
 * validate()'s own clamp runs from the .load() in setup(), long before
 * Device OS seeds Time only on a bound boot, so it can miss the first boot (WO-2026-09-22-001).
 * Call this once, opportunistically, from global setup() as soon as
 * Clock::isTimeValid(). No-op if occupied is false or occupancyStartTime is
 * already plausible.
src/BuildProfile.h:229: * When enabled, each PowerDiag/ChargeDiag/stale-SOC-resync diagnostic emission
src/BuildProfile.h:277:// here, expanded at both emission sites (the Boot: serial line's "flags=" and
<shellId: 4 completed with exit code 0>
```

</details>


---

<sub>26s</sub>

### `bash`

**Find comments and test style**

`$ git diff d6da67c~0 integration/v50-ab1805 --stat | tail -1; git log -3 --format=%h integration/v50-ab1805; git diff 'integration/v50-ab1805~1' integration/v50-ab1805 -- src/BuildProfile.h src/time/ClockTrust.h src/MyPersistentData.h 2>/dev/null | head -5; grep -n "bound\|bind" src/BuildProfile.h src/time/ClockTrust.h src/time/Clock.cpp src/time/HibernateEligibility.cpp | head -20; grep -n "Wire\|Am18x5\|ExternalRtc" tests/external_rtc_structural_test.py | head -20; head -30 tests/external_rtc_test.cpp; ls tests/stubs; cat tests/README.md | head -30`

<details>
<summary>92 lines</summary>

````
46 files changed, 437 insertions(+), 11264 deletions(-)
06aacd9
daec561
ad0a798
src/BuildProfile.h:198: * immediately after the bind and before the hibernate-wake
src/time/ClockTrust.h:51:// retry-pacing floor, not a trust or gate boundary, so it is intentionally
src/time/ClockTrust.h:144:// reason (bound the retry rate of something that can fail repeatedly), but
src/time/ClockTrust.h:249:    // that floor exists only to bound repeated failures of the SAME value.
src/time/ClockTrust.h:306:// against a bounded ceiling (kMaxSyncAgeMs) - it never needs to represent
src/time/Clock.cpp:332: *          past a watchdog boundary during a slow cellular connect.
src/time/Clock.cpp:487: *          DECISION (bounded by `kMaxSyncAgeMs` before it can ever be
2:"""v50 S1: structural checks on src/ plus the ExternalRtc helper/bind host test and its mutations.
13:HDR = SRC / "time" / "ExternalRtc.h"
28:    if "RESET_CAUSE_EXRTC_BINDING_DELETE" in m or "disable(" in m and "Am18x5" in m:
34:    if "ExternalRtc::ensureBound(Am18x5" not in m:
35:        errs.append("setup() does not call ExternalRtc::ensureBound(Am18x5, ...)")
36:    if not re.search(r"\.i2c\(Wire\)\.interruptPin\(WKP\)\.watchdogPin\(WATCHDOG_DONE_PIN\)", m):
37:        errs.append("fleet config is not Wire/WKP/D5")
62:        (Path(d) / "time" / "ExternalRtc.h").write_text(hdr_text)
114:        "setup-left-in": (main_text.replace("ExternalRtc::ensureBound(Am18x5", "ab1805.withFOUT(WKP).setup(); ExternalRtc::ensureBound(Am18x5", 1), rc, pub),
// v50 S1: ExternalRtc helpers and the bind-once logic, compiled from the real src/time/ExternalRtc.h
// against tests/stubs/external_rtc/Particle.h. Not a bench result: the chip itself is untested here.
#include "time/ExternalRtc.h"
#include <cstdlib>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

struct FakeRtc {
  bool bound_ = false;
  int enableResult = 0, enables = 0;
  particle::Am18x5Configuration stored, lastEnabled;
  particle::Status status() const { return particle::Status{bound_}; }
  int enable(const particle::Am18x5Configuration &c) {
    enables++;
    lastEnabled = c;
    if (enableResult) return enableResult;
    bound_ = true;
    stored = c;
    return 0;
  }
  particle::Am18x5Configuration getConfig() const { return stored; }
};

static particle::Am18x5Configuration fleet() {
  return particle::Am18x5Configuration().i2c(0).interruptPin(8).watchdogPin(5).capabilities(0x3).clockSource(particle::RtcClockSource::EXTERNAL);
}

int main() {
  FakeHal &h = fakeHal();
Config.h
MyPersistentData.h
Particle.h
clock_status_republish_overrides
connection_mode_overrides
diag_overrides
external_rtc
hour_rules_overrides
ledger_no_retry_overrides
modem_health
persist
persistence_backend
pir_lifecycle
pmic_fault_monitor_overrides
power
power_composition_overrides
power_source_override_overrides
report_golden
reporting_policy_adapter_overrides
sleep_teardown
# Host regression tests

The reporting-policy calculation is isolated from Particle Device OS so its
battery tiers, effective intervals, boundary alignment, and open-window filter
can be checked with the host compiler:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/reporting_policy_test.cpp src/reporting/ReportingPolicy.cpp \
  -o /tmp/reporting_policy_test
/tmp/reporting_policy_test
```

The runtime adapter itself (`ReportingPolicyResolver::resolveRuntime()` in
`src/reporting/RuntimeReportingPolicy.cpp`) is compiled and exercised directly
against a stub Particle/SensorManager surface - see
`tests/reporting_policy_adapter_test.sh` (WO-2026-08-25-001 Amendment C,
Decision C2 / AC-C6). This closes the gap where the guard's unit tests passed
in isolation while the production adapter still let an Invalid or Unavailable
vcell silently bypass the trust substitution and the 3.5V floor. The normal
Particle firmware compile remains the final integration check.

The boot-scoped startup snapshot has a separate host test:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/startup_snapshot_test.cpp src/observability/StartupSnapshot.cpp \
  -o /tmp/startup_snapshot_test
/tmp/startup_snapshot_test
```
<shellId: 5 completed with exit code 0>
````

</details>


---

<sub>29s</sub>

### `bash`

**Read stub and existing tests**

`$ sed -n 192,202p src/BuildProfile.h; grep -n "seed\|Seed" src/time/ClockTrust.h src/BuildProfile.h | head; ls tests/stubs/external_rtc; cat tests/stubs/external_rtc/Particle.h; cat tests/external_rtc_structural_test.py; sed -n 30,80p tests/external_rtc_test.cpp; ls tests | grep -i "external"; grep -rn "external_rtc" tests/*.sh | head`

<details>
<summary>268 lines</summary>

```
#ifndef ENABLE_RTC_SKEW_TEST
/**
 * @brief TEMPORARY DIAGNOSTIC: Write a deliberately wrong RTC value once on boot.
 *
 * Set to 1 to enable, then reflash. Runs automatically during setup(),
 * immediately after the bind and before the hibernate-wake
 * classification block: it reads the current AB1805 RTC value, writes back
 * a compile-time-skewed value (see RtcSkewTest::kSkewSeconds in
 * src/time/RtcSkewTest.h), and logs both readings. Deliberately NOT tied to
 * BUILD_PROFILE_BENCH: a bench build must not silently corrupt the RTC.
src/time/ClockTrust.h:8: *          validity self-known, and stop a wrong RTC re-seeding itself";
src/time/ClockTrust.h:108: *          Device OS had seeded system time from the RTC - it carries
src/time/ClockTrust.h:215: *          inside that window would re-seed system time from the older,
Particle.h
#pragma once
// Host fake of the Device OS surface src/time/ExternalRtc.h uses.
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <sys/time.h>

enum hal_rtc_source_t { HAL_RTC_SOURCE_INTERNAL = 0, HAL_RTC_SOURCE_EXTERNAL = 1 };
struct hal_rtc_option_t {
  uint16_t size;
  int source;
};

struct FakeHal {
  int getResult = 0, setResult = 0;
  time_t chipTime = 0;
  int lastSource = -1;
  int lastUsec = -1;
  int sets = 0;
  bool valid = false;
  bool noStick = false;      // the write returns success but the chip keeps its time
  time_t writeSkew = 0;      // the chip stores written time + this
  int readbackResult = 0;    // get error once a set has happened (the read-back)
  bool afterSet = false;
};
inline FakeHal &fakeHal() { static FakeHal h; return h; }

inline int hal_rtc_get_time(struct timeval *tv, const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  if (fakeHal().getResult) return fakeHal().getResult;
  if (fakeHal().afterSet && fakeHal().readbackResult) return fakeHal().readbackResult;
  tv->tv_sec = fakeHal().chipTime;
  tv->tv_usec = 0;
  return 0;
}
inline int hal_rtc_set_time(const struct timeval *tv, const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  fakeHal().lastUsec = (int)tv->tv_usec;
  fakeHal().sets++;
  if (fakeHal().setResult) return fakeHal().setResult;
  fakeHal().afterSet = true;
  if (!fakeHal().noStick) fakeHal().chipTime = tv->tv_sec + fakeHal().writeSkew;
  return 0;
}
inline bool hal_rtc_time_is_valid(const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  return fakeHal().valid;
}

struct FakeTime {
  bool valid = true;
  time_t now_ = 1800000000;
  bool isValid() { return valid; }
  time_t now() { return now_; }
};
inline FakeTime &fakeTime() { static FakeTime t; return t; }
#define Time fakeTime()

struct FakeLog {
  int infos = 0, warns = 0;
  template <typename... A> void info(const char *, A...) { infos++; }
  template <typename... A> void warn(const char *, A...) { warns++; }
};
inline FakeLog &fakeLog() { static FakeLog l; return l; }
#define Log fakeLog()

namespace particle {
enum class RtcClockSource : uint8_t { NONE = 0, INTERNAL = 1, EXTERNAL = 2 };
struct HalDevice { struct { int interface; } i2c; };
struct Caps { uint32_t v = 0; uint32_t value() const { return v; } };
struct Am18x5Configuration {
  bool valid_ = false;
  uint8_t addr = 0x69;
  int intPin = -1, wdtPin = -1, wire = 0, tsrc = 0;
  uint32_t caps = 0;
  RtcClockSource src = RtcClockSource::INTERNAL;
  HalDevice dev{};
  Am18x5Configuration &i2c(int w) { wire = w; dev.i2c.interface = w; valid_ = true; return *this; }
  Am18x5Configuration &interruptPin(int p) { intPin = p; return *this; }
  Am18x5Configuration &watchdogPin(int p) { wdtPin = p; return *this; }
  Am18x5Configuration &capabilities(uint32_t c) { caps = c; return *this; }
  Am18x5Configuration &clockSource(RtcClockSource c) { src = c; return *this; }
  RtcClockSource clockSource() const { return src; }
  bool valid() const { return valid_; }
  uint8_t address() const { return addr; }
  int interruptPin() const { return intPin; }
  int watchdogPin() const { return wdtPin; }
  Caps capabilities() const { Caps c; c.v = caps; return c; }
  int defaultTimeSource() const { return tsrc; }
  const HalDevice *halDevice() const { return &dev; }
};
struct Status { bool b; bool bound() const { return b; } };
}  // namespace particle
#!/usr/bin/env python3
"""v50 S1: structural checks on src/ plus the ExternalRtc helper/bind host test and its mutations.

Each mutation is applied to a copy of the real source in a temp dir; the check must then FAIL.
Run from anywhere; zsh not required.
"""
import os, re, subprocess, sys, tempfile, shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
MAIN = SRC / "Generalized-Core-Counter.cpp"
HDR = SRC / "time" / "ExternalRtc.h"
TEST = ROOT / "tests" / "external_rtc_test.cpp"
STUB = ROOT / "tests" / "stubs" / "external_rtc"


def strip(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    return re.sub(r"//[^\n]*", "", code)


def structural(main_text, hdr_text, resetcause_text, publisher_text):
    errs = []
    m = strip(main_text)
    if "ab1805.withFOUT" in m or "ab1805.setup()" in m:
        errs.append("ab1805.setup() is still called")
    if "RESET_CAUSE_EXRTC_BINDING_DELETE" in m or "disable(" in m and "Am18x5" in m:
        errs.append("the v49 binding-delete guard is still present")
    sm = re.search(r"inline bool set\(time_t t\) \{(.*?)\n\}", strip(hdr_text), re.S)
    body = sm.group(1) if sm else ""
    if "hal_rtc_set_time" not in body or "get(back)" not in body or body.index("hal_rtc_set_time") > body.index("get(back)"):
        errs.append("set() does not read the chip back after hal_rtc_set_time")
    if "ExternalRtc::ensureBound(Am18x5" not in m:
        errs.append("setup() does not call ExternalRtc::ensureBound(Am18x5, ...)")
    if not re.search(r"\.i2c\(Wire\)\.interruptPin\(WKP\)\.watchdogPin\(WATCHDOG_DONE_PIN\)", m):
        errs.append("fleet config is not Wire/WKP/D5")
    if ".capabilities(particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL)" not in m:
        errs.append("fallback capability missing from the fleet config")
    if ".clockSource(particle::RtcClockSource::EXTERNAL)" not in m:
        errs.append("clock source EXTERNAL missing from the fleet config")
    if m.index("ensureBound") > m.index("ensureRetainedLoopForensicsInitialized();"):
        errs.append("bind must come before the first Time-dependent setup work")
    if "RESET_CAUSE_EXRTC_BINDING_DELETE" in strip(resetcause_text):
        errs.append("reset cause 8 is still declared")
    if "kLedgerSchemaVersion = 3" not in publisher_text:
        errs.append("kLedgerSchemaVersion is not 3")
    if not re.search(r'name\("oscStatus"\)\.nullValue\(\)', publisher_text) or not re.search(r'name\("oscCtrl"\)\.nullValue\(\)', publisher_text):
        errs.append("oscStatus/oscCtrl are not null")
    if "startupOscStatusReg" in publisher_text or "startupOscStatusReg" in m:
        errs.append("raw osc register global still present")
    for f in ("time/Clock.cpp", "time/HibernateEligibility.cpp"):
        t = strip((SRC / f).read_text())
        if re.search(r"ab1805\.(getRtcAsTime|setRtcFromSystem|isRTCSet|setRtcFromTime)", t):
            errs.append(f"{f} still reads/writes the RTC through ab1805")
    return errs


def build_and_run(hdr_text):
    with tempfile.TemporaryDirectory() as d:
        (Path(d) / "time").mkdir()
        (Path(d) / "time" / "ExternalRtc.h").write_text(hdr_text)
        exe = Path(d) / "t"
        r = subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-I", str(STUB), "-I", d, str(TEST), "-o", str(exe)],
                           capture_output=True, text=True)
        if r.returncode:
            return False, r.stderr
        r = subprocess.run([str(exe)], capture_output=True, text=True)
        return r.returncode == 0, r.stdout


def main():
    hdr = HDR.read_text()
    errs = structural(MAIN.read_text(), hdr, (SRC / "ResetCause.h").read_text(), (SRC / "cloud" / "DeviceStatusPublisher.cpp").read_text())
    if errs:
        for e in errs:
            print("FAIL:", e)
        sys.exit(1)
    print("OK: structural checks")
    ok, out = build_and_run(hdr)
    if not ok:
        print("FAIL: host test\n" + out)
        sys.exit(1)
    print("OK: host helper/bind test")

    def rep(a, b):
        assert a in hdr, a
        return hdr.replace(a, b, 1)

    hmut = {
        "enable-unconditionally": rep("if (!rtc.status().bound()) {", "if (true) {"),
        "skip-bound-check": rep("if (!sameFleetConfig(stored, want)) {", "if (false) {"),
        "reset-on-enable-failure": rep("r = rtc.enable(fleet);\n    Log.info", "r = rtc.enable(fleet); if (r) std::exit(1);\n    Log.info"),
        "write-via-internal-source": hdr.replace("HAL_RTC_SOURCE_EXTERNAL", "HAL_RTC_SOURCE_INTERNAL"),
        "no-readback": rep("return get(back) && (back > t ? back - t : t - back) <= kWriteVerifyToleranceSec;", "return true;"),
        "threshold-removed": rep("(back > t ? back - t : t - back) <= kWriteVerifyToleranceSec", "true"),
        "readback-error-ignored": rep("time_t back = 0;\n  return get(back) &&", "time_t back = t;\n  get(back); return"),
        "wrong-sign": rep("(back > t ? back - t : t - back) <= kWriteVerifyToleranceSec", "back - t <= kWriteVerifyToleranceSec"),
        "ignore-write-error": rep("if (hal_rtc_set_time(&tv, &opt) != 0) return false;", "hal_rtc_set_time(&tv, &opt);"),
        "ignore-fallback-capability": rep("a.capabilities().value() == b.capabilities().value()", "true"),
        "ignore-clock-source": rep("a.clockSource() == b.clockSource()", "true"),
        "wrong-clock-source-value": rep("a.clockSource() == b.clockSource()", "a.clockSource() == particle::RtcClockSource::INTERNAL"),
        "ignore-pin": rep("a.watchdogPin() == b.watchdogPin()", "true"),
        "nonzero-usec": rep("tv.tv_sec = t;", "tv.tv_sec = t; tv.tv_usec = 5;"),
    }
    bad = 0
    for name, text in hmut.items():
        ok, _ = build_and_run(text)
        print(("OK" if not ok else "FAIL"), f"mutation {name} is caught")
        bad += ok
    main_text, rc, pub = MAIN.read_text(), (SRC / "ResetCause.h").read_text(), (SRC / "cloud" / "DeviceStatusPublisher.cpp").read_text()
    smut = {
        "readback-before-set": (main_text, rc, pub, hdr.replace("hal_rtc_set_time(&tv, &opt) != 0) return false;\n  time_t back = 0;\n  return get(back)", "time_t back = 0; get(back);\n  if (hal_rtc_set_time(&tv, &opt) != 0) return false;\n  return true && get(back)", 1)),
        "setup-left-in": (main_text.replace("ExternalRtc::ensureBound(Am18x5", "ab1805.withFOUT(WKP).setup(); ExternalRtc::ensureBound(Am18x5", 1), rc, pub),
        "guard-kept": (main_text + "\nRESET_CAUSE_EXRTC_BINDING_DELETE", rc, pub),
        "wrong-pin": (main_text.replace("interruptPin(WKP)", "interruptPin(D4)", 1), rc, pub),
        "no-fallback-cap": (main_text.replace(".capabilities(particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL)", ".capabilities(particle::RtcCap::NONE)", 1), rc, pub),
        "no-clock-source": (main_text.replace(".clockSource(particle::RtcClockSource::EXTERNAL)", "", 1), rc, pub),
        "cause8-kept": (main_text, rc + "\nRESET_CAUSE_EXRTC_BINDING_DELETE = 8,", pub),
        "schema-2": (main_text, rc, pub.replace("kLedgerSchemaVersion = 3", "kLedgerSchemaVersion = 2")),
    }
    for name, tup in smut.items():
        a, b, c = tup[:3]
        caught = bool(structural(a, tup[3] if len(tup) > 3 else hdr, b, c))
        print(("OK" if caught else "FAIL"), f"mutation {name} is caught")
        bad += not caught
    if bad:
        sys.exit(1)
    print("external_rtc_structural_test: all checks passed")


main()
  FakeHal &h = fakeHal();

  time_t t = 0;
  h.chipTime = 1700000000;
  CHECK(ExternalRtc::get(t) && t == 1700000000 && h.lastSource == HAL_RTC_SOURCE_EXTERNAL);
  h.getResult = -1;
  t = 5;
  CHECK(!ExternalRtc::get(t) && t == 5);
  h.getResult = 0;

  CHECK(ExternalRtc::set(1750000000) && h.lastSource == HAL_RTC_SOURCE_EXTERNAL && h.lastUsec == 0);
  time_t back = 0;
  CHECK(ExternalRtc::get(back) && back == 1750000000);
  h.setResult = -3;
  CHECK(!ExternalRtc::set(1760000000) && h.chipTime == 1750000000);
  h.setResult = 0;

  // Read-back verification: the write only counts if the chip holds the time within the tolerance.
  CHECK(ExternalRtc::kWriteVerifyToleranceSec == 2);
  h.writeSkew = 2;
  CHECK(ExternalRtc::set(1750000000));
  h.writeSkew = -2;
  CHECK(ExternalRtc::set(1750000000));
  h.writeSkew = 3;
  CHECK(!ExternalRtc::set(1750000000) && h.sets > 0);
  h.writeSkew = -3;
  CHECK(!ExternalRtc::set(1750000000));
  h.writeSkew = 0;
  h.noStick = true;
  h.chipTime = 1700000000;
  CHECK(!ExternalRtc::set(1750000000));
  h.noStick = false;
  h.readbackResult = -5;
  CHECK(!ExternalRtc::set(1750000000));
  h.readbackResult = 0;
  h.setResult = -3;
  h.writeSkew = 0;
  CHECK(!ExternalRtc::set(1750000000));
  h.setResult = 0;
  CHECK(ExternalRtc::set(1750000000));

  h.sets = 0;
  fakeTime().valid = false;
  CHECK(!ExternalRtc::setFromSystem() && h.sets == 0);
  fakeTime().valid = true;
  CHECK(ExternalRtc::setFromSystem() && h.chipTime == fakeTime().now_);
  h.noStick = true;
  h.chipTime = 1;
  CHECK(!ExternalRtc::setFromSystem());
  h.noStick = false;

external_rtc_structural_test.py
external_rtc_test.cpp
<shellId: 6 completed with exit code 0>
```

</details>


---

<sub>36s</sub>

### `bash`

**Step 0 Device OS reads**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; grep -n "i2c_end\|hal_i2c_" $D/hal/shared/am18x5.cpp | head -30; grep -n "setTime" -A8 $D/wiring/src/spark_wiring_time.cpp | head -30; grep -n "void TwoWire::begin\|bool TwoWire::lock\|isEnabled" -A12 $D/wiring/src/spark_wiring_i2c.cpp | head -50; grep -n "hal_rtc_set_time\|hal_rtc_get_time" -A30 $D/hal/shared/rtc_hal.cpp 2>/dev/null| head -80; grep -rn "hal_exrtc_default_binding" $D/hal/src/nRF52840 $D/hal/src/boron 2>/dev/null | head -3`

<details>
<summary>90 lines</summary>

```
491:        if (!hal_i2c_is_enabled(config_.i2c_if, nullptr)) {
492:            hal_i2c_init(config_.i2c_if, nullptr);
493:            hal_i2c_begin(config_.i2c_if, I2C_MODE_MASTER, 0x00, nullptr);
494:            CHECK_TRUE(hal_i2c_is_enabled(config_.i2c_if, nullptr), SYSTEM_ERROR_INTERNAL);
497:            CHECK(hal_i2c_reset(config_.i2c_if, 0, nullptr));
501:        // hal_i2c_lock/hal_i2c_unlock, which do not function unless hal_i2c_init is called.
656:    return hal_i2c_lock(config_.i2c_if, nullptr);
660:    return hal_i2c_unlock(config_.i2c_if, nullptr);
1324:    if (!hal_i2c_is_enabled(config_.i2c_if, nullptr)) {
1325:        hal_i2c_begin(config_.i2c_if, I2C_MODE_MASTER, 0x00, nullptr);
1326:        CHECK_TRUE(hal_i2c_is_enabled(config_.i2c_if, nullptr), SYSTEM_ERROR_INTERNAL);
1343:    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
1344:    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(reg), nullptr);
1345:    hal_i2c_write(config_.i2c_if, currValue, nullptr);
1346:    hal_i2c_end_transmission(config_.i2c_if, true, nullptr);
1353:    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
1354:    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(start_reg), nullptr);
1356:        hal_i2c_write(config_.i2c_if, buff[i], nullptr);
1358:    hal_i2c_end_transmission(config_.i2c_if, true, nullptr);
1365:    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
1366:    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(reg), nullptr);
1367:    hal_i2c_end_transmission(config_.i2c_if, false, nullptr);
1368:    if (hal_i2c_request(config_.i2c_if, AM18X5_I2C_ADDR, 1, true, nullptr) == 0) {
1371:    *val = hal_i2c_read(config_.i2c_if, nullptr);
1383:    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
1384:    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(start_reg), nullptr);
1385:    hal_i2c_end_transmission(config_.i2c_if, false, nullptr);
1386:    if (hal_i2c_request(config_.i2c_if, AM18X5_I2C_ADDR, len, true, nullptr) == 0) {
1389:    int32_t size = hal_i2c_available(config_.i2c_if, nullptr);
1395:        buff[i] = hal_i2c_read(config_.i2c_if, nullptr);
284:void TimeClass::setTime(time_t t)
285-{
286-    struct timeval tv = {
287-        .tv_sec = t,
288-        .tv_usec = 0
289-    };
290-    hal_rtc_option_t opt = {};
291-    opt.size = sizeof(opt);
292-    opt.source = static_cast<hal_rtc_source_t>(source_);
64:void TwoWire::begin(void)
65-{
66-	hal_i2c_begin(_i2c, I2C_MODE_MASTER, 0x00, NULL);
67-}
68-
69:void TwoWire::begin(uint8_t address)
70-{
71-	hal_i2c_begin(_i2c, I2C_MODE_SLAVE, address, NULL);
72-}
73-
74:void TwoWire::begin(int address)
75-{
76-  begin((uint8_t)address);
77-}
78-
79-void TwoWire::end()
80-{
81-	hal_i2c_end(_i2c, NULL);
82-}
83-
84-size_t TwoWire::requestFrom(uint8_t address, size_t quantity, uint8_t sendStop)
85-{
86-  return requestFrom(WireTransmission(address).quantity(quantity).stop(sendStop));
--
99:void TwoWire::beginTransmission(uint8_t address)
100-{
101-	hal_i2c_begin_transmission(_i2c, address, NULL);
102-}
103-
104:void TwoWire::beginTransmission(int address)
105-{
106-  beginTransmission((uint8_t)address);
107-}
108-
109:void TwoWire::beginTransmission(const WireTransmission& transfer) {
110-  auto conf = transfer.halConfig();
111-  hal_i2c_begin_transmission(_i2c, conf.address, &conf);
112-}
113-
114-//
115-//	Originally, 'endTransmission' was an f(void) function.
116-//	It has been modified to take one parameter indicating
117-//	whether or not a STOP should be performed on the bus.
118-//	Calling endTransmission(false) allows a sketch to
119-//	perform a repeated start.
120-//
121-//	WARNING: Nothing in the library keeps track of whether
--
203:bool TwoWire::isEnabled()
204-{
<shellId: 7 completed with exit code 0>
```

</details>


---

<sub>39s</sub>

### `bash`

**More Step 0 reads**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; sed -n 284,305p $D/wiring/src/spark_wiring_time.cpp; sed -n 480,505p $D/hal/shared/am18x5.cpp; grep -n "WITH_LOCK\|lock()" $D/wiring/inc/spark_wiring_i2c.h | head; grep -rn "int hal_rtc_set_time" -A25 $D/hal/src/nRF52840/rtc_hal.cpp | head -50; grep -n "hal_i2c_begin" -A25 $D/hal/src/nRF52840/i2c_hal.cpp | head -50; grep -rn "hal_exrtc_default_binding" $D/hal --include=*.cpp | head`

<details>
<summary>130 lines</summary>

```
void TimeClass::setTime(time_t t)
{
    struct timeval tv = {
        .tv_sec = t,
        .tv_usec = 0
    };
    hal_rtc_option_t opt = {};
    opt.size = sizeof(opt);
    opt.source = static_cast<hal_rtc_source_t>(source_);
    if (!hal_rtc_set_time(&tv, &opt)) {
        system_notify_time_changed((uint32_t)time_changed_manually, nullptr, nullptr);
    }
}

TimeSource TimeClass::timeSource() {
    hal_rtc_option_t opt = {};
    opt.size = sizeof(opt);
    opt.source = static_cast<hal_rtc_source_t>(source_);
    struct timeval tv = {};
    hal_rtc_get_time(&tv, &opt);
    return static_cast<TimeSource>(opt.source);
}

int Am18x5::begin() {
    int ret = SYSTEM_ERROR_INTERNAL;
    SCOPE_GUARD ({
        if (ret != SYSTEM_ERROR_NONE) {
            end();
        }
    });
    CHECK_TRUE(config_.size >= sizeof(am18x5_config_t), SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(config_.i2c_if < HAL_PLATFORM_I2C_NUM, SYSTEM_ERROR_INVALID_STATE);
    if (!initialized_) {
        if (!hal_i2c_is_enabled(config_.i2c_if, nullptr)) {
            hal_i2c_init(config_.i2c_if, nullptr);
            hal_i2c_begin(config_.i2c_if, I2C_MODE_MASTER, 0x00, nullptr);
            CHECK_TRUE(hal_i2c_is_enabled(config_.i2c_if, nullptr), SYSTEM_ERROR_INTERNAL);
            // Make sure to reset the I2C bus to avoid potentially corrupting the AM18x5 configuration if
            // we start communication with it during an ongoing write transaction (which may happen e.g. after a hard reset)
            CHECK(hal_i2c_reset(config_.i2c_if, 0, nullptr));
        }
        Am18x5Lock lock;
        // NOTE: acquire lock only after initializing the I2C peripheral, as this will actually call into
        // hal_i2c_lock/hal_i2c_unlock, which do not function unless hal_i2c_init is called.
        CHECK(detect());
        if (config_.wdi_pin != PIN_INVALID) {
            hal_gpio_mode(config_.wdi_pin, OUTPUT);
            hal_gpio_write(config_.wdi_pin, 1);
138:  bool lock();
139:  bool unlock();
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp:116:int hal_rtc_set_time(const struct timeval* tv, hal_rtc_option_t* opt) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-117-    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-118-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-119-#if HAL_PLATFORM_EXTERNAL_RTC
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-120-    // Update both is possible
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-121-    int r = 0;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-122-    if (!opt || opt->source != HAL_RTC_SOURCE_INTERNAL) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-123-        r = hal_exrtc_set_time_internal(tv);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-124-    }
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-125-    if (opt && opt->source == HAL_RTC_SOURCE_EXTERNAL) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-126-        return r;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-127-    }
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-128-#endif // HAL_PLATFORM_EXTERNAL_RTC
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-129-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-130-    return setUnixTime(tv);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-131-}
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-132-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-133-int hal_rtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-134-    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-135-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-136-#if HAL_PLATFORM_EXTERNAL_RTC
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-137-    if (hal_exrtc_is_default() && !hal_exrtc_set_alarm(tv, flags, handler, context)) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-138-        return 0;
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-139-    }
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-140-#endif // HAL_PLATFORM_EXTERNAL_RTC
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/nRF52840/rtc_hal.cpp-141-
412:            hal_i2c_begin(i2c, I2C_MODE_MASTER, 0x00, nullptr);
413-        }
414-    }
415-#endif // #if HAL_PLATFORM_PMIC_BQ24195
416-
417-    return SYSTEM_ERROR_NONE;
418-}
419-
420-void hal_i2c_set_speed(hal_i2c_interface_t i2c, uint32_t speed, void* reserved) {
421-    if (i2c >= HAL_PLATFORM_I2C_NUM) {
422-        return;
423-    }
424-
425-    I2cLock lk(i2c);
426-    i2cMap[i2c].speed = speed;
427-}
428-
429-void hal_i2c_stretch_clock(hal_i2c_interface_t i2c, bool stretch, void* reserved) {
430-    // always enabled
431-}
432-
433:void hal_i2c_begin(hal_i2c_interface_t i2c, hal_i2c_mode_t mode, uint8_t address, void* reserved) {
434-    if (i2c >= HAL_PLATFORM_I2C_NUM) {
435-        return;
436-    }
437-    if (i2cMap[i2c].state == HAL_I2C_STATE_NOT_INITIALIZED) {
438-        return;
439-    }
440-
441-    I2cLock lk(i2c);
442-#if PLATFORM_ID == PLATFORM_TRACKER
443-    /*
444-     * On Tracker both I2C_INTERFACE1 and I2C_INTERFACE3 use the same peripheral - m_twim0,
445-     * but on different pins. We cannot enable both of them at the same time.
446-     */
447-    if (i2c == HAL_I2C_INTERFACE1 || i2c == HAL_I2C_INTERFACE3) {
448-        hal_i2c_interface_t dependent = (i2c == HAL_I2C_INTERFACE1 ? HAL_I2C_INTERFACE3 : HAL_I2C_INTERFACE1);
449-        if (hal_i2c_is_enabled(dependent, nullptr)) {
450-            // Unfortunately we cannot return an error code here
451-            return;
452-        }
453-    }
454-    /*
455-     * On Tracker both I2C_INTERFACE3 and USART_SERIAL1 use the same pins - D8 and D9,
456-     * We cannot enable both of them at the same time.
457-     */
458-    if (i2c == HAL_I2C_INTERFACE3) {
--
559:void hal_i2c_begin_transmission(hal_i2c_interface_t i2c, uint8_t address, const hal_i2c_transmission_config_t* config) {
560-    if (i2c >= HAL_PLATFORM_I2C_NUM || !hal_i2c_is_enabled(i2c, nullptr)) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp:230:    if (auto platformDefault = hal_exrtc_default_binding()) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp:259:    auto platformDefault = hal_exrtc_default_binding();
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp:331:    if (hal_exrtc_default_binding()) {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/exrtc_hal.cpp:437:hal_exrtc_binding_t* __attribute__((weak)) hal_exrtc_default_binding() {
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/src/tracker/exrtc_hal.cpp:27:hal_exrtc_binding_t* hal_exrtc_default_binding() {
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>44s</sub>

### `bash`

**Check set\_time\_internal, Wire init, locks**

`$ D=~/.particle/toolchains/deviceOS/6.6.0; grep -n "hal_exrtc_set_time_internal" -B2 -A22 $D/hal/shared/exrtc_hal.cpp | sed -n 1,70p; grep -n "TwoWire::TwoWire" -A8 $D/wiring/src/spark_wiring_i2c.cpp; grep -n "source_" $D/wiring/src/spark_wiring_time.cpp | head -5; grep -n "TwoWire::lock\|TwoWire::unlock" -A4 $D/wiring/src/spark_wiring_i2c.cpp; grep -n "define WITH_LOCK" -A3 $D/wiring/inc/*.h | head; grep -n "hal_i2c_init" -A12 $D/hal/src/nRF52840/i2c_hal.cpp | head -30; grep -n "I2cLock" -B1 -A10 $D/hal/src/nRF52840/i2c_hal.cpp | head -30`

<details>
<summary>128 lines</summary>

```
244-    struct timeval tvInt = {};
245-    if (!hal_rtc_get_time(&tvInt, &rtc)) {
246:        return hal_exrtc_set_time_internal(&tvInt);
247-    }
248-    return SYSTEM_ERROR_NONE;
249-}
250-
251-} // anonymous
252-
253-int hal_exrtc_init(void) {
254-    RtcBinding stored;
255-    if (!stored.load(SystemCacheKey::EXRTC_CONFIG_DATA)) {
256-        return Am18x5::getInstance().bind(stored.binding());
257-    }
258-
259-    auto platformDefault = hal_exrtc_default_binding();
260-    if (platformDefault) {
261-        return Am18x5::getInstance().bind(platformDefault);
262-    }
263-
264-    return SYSTEM_ERROR_NOT_FOUND;
265-}
266-
267-int hal_exrtc_bind(hal_exrtc_instance_t instance, const hal_exrtc_binding_t* binding, void* reserved) {
268-    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
--
410-}
411-
412:int hal_exrtc_set_time_internal(const struct timeval* tv) {
413-    if (Am18x5::getInstance().isPresent()) {
414-        return Am18x5::getInstance().setTime(tv);
415-    }
416-    return SYSTEM_ERROR_NOT_FOUND;
417-}
418-
419-int hal_exrtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context) {
420-    if (Am18x5::getInstance().isPresent()) {
421-        return Am18x5::getInstance().setAlarm(true, flags, tv, handler, context, true);
422-    }
423-    return SYSTEM_ERROR_NOT_FOUND;
424-}
425-
426-int hal_exrtc_cancel_alarm(void) {
427-    if (Am18x5::getInstance().isPresent()) {
428-        return Am18x5::getInstance().setAlarm(false);
429-    }
430-    return SYSTEM_ERROR_NOT_FOUND;
431-}
432-
433-bool hal_exrtc_is_default(void) {
434-    return Am18x5::getInstance().isDefault();
34:TwoWire::TwoWire(hal_i2c_interface_t i2c, const hal_i2c_config_t& conf)
35-{
36-  _i2c = i2c;
37-  int result = hal_i2c_init(_i2c, &conf);
38-  if (result == SYSTEM_ERROR_NOT_ENOUGH_DATA && 
39-      (conf.flags & HAL_I2C_CONFIG_FLAG_FREEABLE)) {
40-    free(conf.rx_buffer);
41-    free(conf.tx_buffer);
42-  }
85:        : source_(source),
229:    opt.source = static_cast<hal_rtc_source_t>(source_);
292:    opt.source = static_cast<hal_rtc_source_t>(source_);
301:    opt.source = static_cast<hal_rtc_source_t>(source_);
374:    opt.source = static_cast<hal_rtc_source_t>(source_);
213:bool TwoWire::lock()
214-{
215-  return hal_i2c_lock(_i2c, NULL) == 0;
216-}
217-
218:bool TwoWire::unlock()
219-{
220-  return hal_i2c_unlock(_i2c, NULL) == 0;
221-}
222-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_thread.h:54:#define WITH_LOCK(lock) for (bool __todo = true; __todo;) for (std::lock_guard<decltype(lock)> __lock((lock)); __todo; __todo=0)
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_thread.h-55-#define TRY_LOCK(lock) for (bool __todo = true; __todo; ) for (particle::UniqueLock<typename std::remove_reference<decltype(lock)>::type> __lock##lock((lock), std::try_to_lock); __todo &= bool(__lock##lock); __todo=0)
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_thread.h-56-
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_thread.h-57-#else
--
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_thread.h:60:#define WITH_LOCK(x)
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_thread.h-61-#define TRY_LOCK(x)
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_thread.h-62-#endif
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/inc/spark_wiring_thread.h-63-
328:int hal_i2c_init(hal_i2c_interface_t i2c, const hal_i2c_config_t* config) {
329-    CHECK_TRUE(i2c < HAL_PLATFORM_I2C_NUM, SYSTEM_ERROR_INVALID_ARGUMENT);
330-
331-    if (i2cMap[i2c].mutex == nullptr) {
332-        os_mutex_recursive_t mutex = nullptr;
333-        CHECK_TRUE(os_mutex_recursive_create(&mutex) == 0, SYSTEM_ERROR_NO_MEMORY);
334-
335-        os_thread_scheduling(false, nullptr);
336-        if (i2cMap[i2c].mutex == nullptr) {
337-            std::swap(mutex, i2cMap[i2c].mutex);
338-        }
339-        os_thread_scheduling(true, nullptr);
340-
66-
67:class I2cLock {
68-public:
69:    I2cLock() = delete;
70-
71:    I2cLock(hal_i2c_interface_t i2c)
72-            : i2c_(i2c) {
73-        hal_i2c_lock(i2c, nullptr);
74-    }
75-
76:    ~I2cLock() {
77-        hal_i2c_unlock(i2c_, nullptr);
78-    }
79-
80-private:
81-    hal_i2c_interface_t i2c_;
82-};
83-
84-/* TWI instance. */
85-static nrfx_twim_t m_twim0 = NRFX_TWIM_INSTANCE(0);
86-static nrfx_twim_t m_twim1 = NRFX_TWIM_INSTANCE(1);
--
346-
347:    I2cLock lk(i2c);
348-
349-// sc-137389
350-#if HAL_PLATFORM_PMIC_BQ24195
351-    bool wasEnabled = false;
352-    if (i2cMap[i2c].state == HAL_I2C_STATE_ENABLED) {
353-        wasEnabled = true;
<shellId: 9 completed with exit code 0>
```

</details>


---

<sub>56s</sub>

### `bash`

**How tests get isPlausibleEpoch**

`$ grep -ln "isPlausibleEpoch" tests/*; grep -n "isPlausibleEpoch" src/time/Clock.h src/time/*.h | head; sed -n 1,25p src/time/Clock.h; cat tests/clock_openness_test.sh | head -30`

<details>
<summary>60 lines</summary>

```
grep: tests/fixtures: Is a directory
grep: tests/stubs: Is a directory
tests/README.md
tests/clock_owner_structural_test.py
tests/rtc_skew_test.cpp
tests/rtc_skew_test.sh
src/time/Clock.h:32: *          `Clock::isPlausibleEpoch()` (absorbing the former
src/time/Clock.h:151:bool isPlausibleEpoch(time_t epoch);
src/time/Clock.h:32: *          `Clock::isPlausibleEpoch()` (absorbing the former
src/time/Clock.h:151:bool isPlausibleEpoch(time_t epoch);
/**
 * @file Clock.h
 * @brief Single owner of "what time is it, and can it be trusted" -
 *        open-hours evaluation, the resync/RTC-write-back cycle, and the
 *        sync-recency trust signal.
 *
 * @details WO-2026-09-18 Step 3a of the Structural Ownership Map roadmap
 *          (`docs/architecture-review-2026-09-03.md`'s "Next session -
 *          agreed direction" item 1: "Name the owner... a consolidated
 *          `time/` component"). Before this step, `isWithinOpenHours()`,
 *          `isWithinOpenHoursAt()`, `secondsUntilNextOpen()`,
 *          `checkClockResync()`/`requestClockResync()`, `isClockTrusted()`,
 *          and `reportedSyncAgeMs()` were all defined directly inside
 *          `Generalized-Core-Counter.cpp`, callable only because every
 *          consumer happened to link into the same binary - no single file
 *          could answer "what time is it after this wake" on its own.
 *
 *          This is a RELOCATION, not a redesign: every function below kept
 *          its existing global-scope name and signature so every existing
 *          call site across the tree (every `.cpp` file under `state/`, `reporting/`,
 *          `cloud/`, `diagnostics/`) keeps working unchanged - only the
 *          `.cpp` file that defines them moved. `time/ClockTrust.h` (the
 *          pure, dependency-free trust-gate math these functions call into)
 *          is unchanged and continues to be this module's own dependency,
 *          not something it re-implements.
#!/bin/zsh
set -euo pipefail

# WO-2026-09-19 Step 3b: Clock::openness()'s truth table.
#
# Part 1 (clock_openness_test.cpp) runs a hand-written mirror of the
# three-input decision on the host. Part 2 here traces the REAL
# src/time/Clock.cpp to confirm the real openness() actually checks trust
# FIRST, then config validity, then falls through to the same open/closed
# computation isWithinOpenHours() itself uses - so the host-side mirror
# cannot silently drift from what production actually does.

repo_root="${0:A:h:h}"
binary="$TMPDIR/clock_openness_test"
clock_src="$repo_root/src/time/Clock.cpp"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/src" \
  "$repo_root/tests/clock_openness_test.cpp" \
  -o "$binary"
"$binary"

echo ""
echo "--- Part 2: Clock::openness() fidelity checks against the real source ---"

openness_fn=$(python3 - "$clock_src" <<'PY'
import sys

path = sys.argv[1]
with open(path) as f:
<shellId: 10 completed with exit code 0>
```

</details>


---

<sub>1m 35s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1b/src/time/PreBindRtc.h.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1b/src/time/PreBindRtc.h`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1b/src/time/PreBindRtc.h with 2116 characters
```


---

<sub>1m 43s</sub>

### `bash`

**Count lines; check scratch dir and v49 rollback**

`$ wc -l src/time/PreBindRtc.h; ls build-tmp 2>&1 | head -3; grep -n "bool AB1805::isRTCSet\|isRTCSet" lib/AB1805_RK/src/AB1805_RK.h lib/AB1805_RK/src/AB1805_RK.cpp | head; grep -n "AB1805::setup" -A40 lib/AB1805_RK/src/AB1805_RK.cpp | grep -n "isRTCSet\|setRtc\|reset\|Time" | head; grep -n "ab1805\.\(setup\|isRTCSet\|getRtc\)" -r $(git show v49:src/Generalized-Core-Counter.cpp >/dev/null 2>&1 && echo) src 2>/dev/null | head -3; git tag | tail -3`

<details>
<summary>19 lines</summary>

```
38 src/time/PreBindRtc.h
ls: build-tmp: No such file or directory
lib/AB1805_RK/src/AB1805_RK.h:97:    bool isRTCSet() { return isBitClear(REG_CTRL_1, REG_CTRL_1_WRTC); };
11:33-        if (isBitClear(REG_CTRL_1, REG_CTRL_1_WRTC) && !Time.isValid()) {
15:37-            getRtcAsTime(time);
16:38-            Time.setTime(time);
18:40-            _log.info("set system clock from RTC %s", Time.format(time, TIME_FORMAT_DEFAULT).c_str());
25:47-    System.on(reset, systemEventStatic);
31:53-    if (!timeSet && Time.isValid() && Particle.connected() && Particle.timeSyncedLast() != 0) {
34:56-        time_t time = Time.now();
35:57-        setRtcFromTime(time);
38:60-        getRtcAsTime(time);
39:61-        _log.info("set RTC from cloud %s", Time.format(time, TIME_FORMAT_DEFAULT).c_str());
src/time/HibernateWakeDiagnostics.h:42:  kRtcRead,        // ab1805.getRtcAsTime() failed
src/time/ClockDiagnostics.cpp:7:// platform (ab1805.setup()/setWDT() run unconditionally below), so this name
v48.90
v48.91
v49.1
<shellId: 11 completed with exit code 0>
```

</details>


---

<sub>1m 46s</sub>

### `bash`

**Move draft out of src**

`$ mkdir -p build-tmp/wo-2026-11-06-002-stage6 && mv src/time/PreBindRtc.h build-tmp/wo-2026-11-06-002-stage6/PreBindRtc.h.draft && git status --short`

```

<shellId: 12 completed with exit code 0>
```


---

<sub>2m 5s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1b/docs/work-orders/WO-2026-11-06-002-stage6-copilot-report.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1b/docs/work-orders/WO-2026-11-06-002-stage6-copilot-report.md`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1b/docs/work-orders/WO-2026-11-06-002-stage6-copilot-report.md with 5493 characters
```


---

<sub>2m 10s</sub>

### Copilot

I stopped without implementing S1b because it can't fit the +15 net `src/` budget. I didn't compile or run any tests, mutations, build or `nm` check.

- **Why it doesn't fit:** my draft of the Amendment 1 scope is 38 lines in a new `src/time/PreBindRtc.h`, plus about 4 lines at the call site. That is roughly +42 net against +15. The scope covers BCD and 12/24-hour decode, the CONTROL1 read-modify-write, the Wire lock and `Wire.begin()`, and the plausibility- and `Time`-gated seed.
- **Step 0:** no blockers on the other stop conditions. `Wire` isn't touched before `ensureBound`. Device OS's `enableInterfaceIfNeeded()` accepts a Wire that is already enabled. `Time.setTime()` before `enable()` is seen by the bind. One caveat: Device OS's bus reset during `Am18x5::begin()` is skipped when `Wire` is pre-enabled. The rollback to v49 is as the WO expects.
- **Working tree:** no changes under `src/` or `tests/`. The draft is at `build-tmp/wo-2026-11-06-002-stage6/PreBindRtc.h.draft`.
- **WO discrepancy:** `BuildProfile.h` and `ClockTrust.h` contain no "can miss the seed" comment, although WO item 4 lists them. The other three comment sites are in the report.
- **Reasoning level:** I couldn't confirm I ran at medium.

The architect needs to raise the budget to about +45 or decide what to cut. Step 0 answers with file:line are in `docs/work-orders/WO-2026-11-06-002-stage6-copilot-report.md`.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>
