# WO-2026-08-31-003: Debug-only RTC skew hook for bench validation

**Type:** Bench tooling. Debug-only, compile-gated, never shipped in a
field release.

**Raised because** both `WO-2026-08-29-002` and `WO-2026-08-29-001` have an
outstanding, commit-blocking bench acceptance test that cannot currently be
performed: there is no way to put an AB1805 into the state the field defect
actually produces.

## What already exists (investigated 2026-08-31, read-only)

- **Direct RTC write: nothing.** Exactly one `ab1805.setRtcFromSystem()`
  call exists in the whole codebase (`Generalized-Core-Counter.cpp:2207`,
  the WO-2026-08-29-002 write-back). No `setRtcFromTime`/`setRtcFromTm`
  callers anywhere in `src/`.
- **Cloud function surface: none.** `cloud/Particle_Functions.cpp`
  registers two variables (`thrashTrips`, `thrashResets`) and zero
  `Particle.function()` handlers. The `fn=freq/stay/sim/pwr` verbs in
  Dev-11's Particle device notes are historical, from an earlier codebase;
  nothing in this tree dispatches them.
- **Forced hibernate: no hook needed.** `openHour`/`closeHour` are
  ledger-settable at runtime (`cloud/ConfigApply.cpp:288-292`) and
  `isWithinOpenHours()` reads them live, so narrowing the operating window
  induces a closed-hours hibernate on demand with no code change.
- **Precedent for the flag:** `ENABLE_PMIC_CHARGE_CYCLE_TEST`
  (`BuildProfile.h:194`) is documented as "Set to 1 to enable test, then
  reflash. Test runs automatically during setup(). One-shot guard prevents
  multiple executions per boot. After test completes, set back to 0 and
  reflash to remove test from binary." This WO follows that shape exactly.

## Why physical power-cycling is not sufficient

Disconnecting LiPo and USB clears the AB1805 (no backup cell is populated -
see `WO-2026-08-31-004`). That leaves the RTC **unset**, which is a
DIFFERENT state from the field defect:

| | RTC unset (power-cycle) | RTC skewed (field defect) |
|---|---|---|
| `ab1805.isRTCSet()` | false | true |
| `Time.isValid()` after `ab1805.setup()` | **false** | **true** |
| `setup()` path taken | `!Time.isValid()` forced connect | falls through to normal scheduling |
| Can hibernate? | **No** - `shouldUseBoronRtcAlarmHibernate()` bails at `State_Sleep.cpp:287` with `HibernateDiag: fail=rtc_not_set` | Yes |

The unset case exercises a path that already worked and, critically,
**cannot hibernate at all** - so the "correction survives a hibernate"
acceptance step is unreachable that way. Reproducing the defect requires a
plausible-but-wrong RTC value, which can only be written.

## Requested change

A single build flag, `ENABLE_RTC_SKEW_TEST`, default `0`, in
`BuildProfile.h`, following the existing conventions there:

- The `#if (X != 0) && (X != 1)` / `#error` guard every other flag carries.
- A `#warning` when enabled, matching `CONNECTIVITY_FAILSAFE_TEST_MODE`
  (`ConnectivityFailsafeTest.cpp:12`), so an enabled test build cannot be
  shipped silently.
- Boron-only (`#if PLATFORM_ID == PLATFORM_BORON`); the AB1805 is Boron
  hardware.

Behaviour when enabled: once per boot, behind a one-shot guard, after
`ab1805.setup()` and before the hibernate-wake classification block, write
a deliberately wrong RTC value and log the before/after readings. Nothing
else. No cloud function, no serial command, no runtime surface - keeping it
off field builds by construction rather than by policy.

The skew magnitude should be a compile-time constant, defaulting to
something close to the observed field value (Dev-11: -13.46h), and must be
large enough that `isRtcTimeValidForHibernate()` still accepts it - a skew
that fails that check would test nothing.

## Explicitly out of scope

- Any change to the wake-validation gate, the sleep path, the clock-trust
  mechanism, or the hibernate duration computation.
- Any runtime-reachable trigger. Compile-time only.
- The RTC-loss root cause - that is `WO-2026-08-31-004`.

## Bench procedure this enables

1. Power-cycle the device (LiPo + USB disconnected) to clear the RTC to a
   known-empty state.
2. Flash the `ENABLE_RTC_SKEW_TEST=1` build. It writes the known skew on
   boot, so the starting offset is known rather than inherited.
3. Confirm `Time.isValid()` reads true and the device believes the wrong
   time - the field defect reproduced deliberately.
4. Let it connect. Confirm `checkClockResync()` requests a sync, the sync
   completes, `ab1805.setRtcFromSystem()` succeeds, and the RTC is
   corrected.
5. Confirm the deferred status republish delivers `clock.trusted=true`.
6. Narrow `closeHour` via the device-settings ledger to force a
   closed-hours hibernate; confirm the corrected time survives the wake and
   the device is not re-seeded from the old skew.
7. With `WO-2026-08-29-001` also flashed, force a gate failure and confirm
   the `hibernate_wake` event reports the correct `gateArm`, agreeing with
   the simultaneous serial line.
8. Reflash with the flag back to `0` and confirm it is absent from the
   binary.

Steps 4-6 close `WO-2026-08-29-002`'s outstanding acceptance finding; step
7 closes `WO-2026-08-29-001`'s.

## Amendment A (2026-09-01) - after Stage 7 pass 1 returned FAIL

Stage 7 failed the first implementation with one HIGH and three MEDIUM
findings. This amendment resolves all of them before the next
implementation round. Where this amendment conflicts with anything above,
this amendment governs.

### A.1 Skew constant revised: 48,456s -> 28,800s (8h)

**The original constant was unreachable by construction.** 48,456s was
chosen to mirror Dev-11's live accumulated drift at time of writing. That
was the wrong reference point: it reflects THREE nights of compounding
loss, not one hibernate cycle, and it structurally exceeds
`kMaxHibernateSleepSec` (36,000s, `State_Sleep.cpp:281`).

Stage 7's HIGH finding follows from that. With the flag still enabled at
bench step 6, the per-boot guard re-fires on the acceptance wake and
subtracts the skew again, after `ab1805.setup()` has seeded correct time.
Because the skew exceeded the maximum hibernate duration,
`rtcTime >= retainedHibernateRtcBefore` could never hold - so step 6
("confirm the corrected time survives the wake") was unreachable, and
hibernate-duration classification was suppressed as a side effect.

Revised constant: **28,800s (8h)** - sized to Dev-11's second observed
SINGLE-CYCLE loss (8h29m) rather than its cumulative drift. Comfortably
under the 36,000s cap, large enough to be unambiguous against normal RTC
jitter, small enough that the cap logic does not interfere with the test
it exists to exercise.

Acceptance criterion, unchanged in intent and now actually satisfiable:
bench step 6 must be reachable at this constant, i.e.
`rtcTime >= retainedHibernateRtcBefore` must be satisfiable after one
hibernate cycle at the revised skew. **Verify this explicitly against both
constants; do not assume it.**

### A.2 Flag lifecycle: one-shot, and stated explicitly

The first implementation left this implicit - it fell out of the
function-local static guard being reconstructed each boot, rather than
being a decision.

**Required behaviour: one-shot.** The skew fires once on the first boot
after flashing, the guard clears after the first confirmed correction, and
it does not re-arm without a fresh flash or an explicit reset. The
implementation must make this explicit, not incidental.

Rationale: a build that re-skews every cycle only demonstrates "correction
survives one wake". It does not demonstrate the build is safe to leave
running across a multi-day bench soak - those are different guarantees and
this WO claims the latter. One-shot also matches the real defect's shape
more closely (the RTC loses time on an affected hibernate, not
deterministically every cycle) and avoids a bench device silently
re-corrupting its own clock indefinitely if left powered.

### A.3 `Time.setTime()` removed entirely

Resolved as Option 2 of the three put to Stage 7. **The hook writes the
RTC only. Nothing else. No conditional variant.**

Bench procedure step 3 is amended accordingly: verify `Time.isValid()` and
the wrong-time belief **after the post-flash power-cycle**, which is
already step 1 of the procedure - not on the flashing boot.

This keeps the hook's blast radius to a single I2C transaction against one
peripheral, which is the entire basis for permitting it near a live
RTC-write path.

### A.4 Reminder: check the RTC write's return value

Stage 7 found the first implementation called `Time.setTime()`
**regardless of whether the preceding `setRtcFromTime()` returned true**.
A failed write would therefore still produce valid-but-wrong system time,
making step 3 appear to succeed while the RTC was unset and hibernation
unreachable - and causing the later "RTC restored system time" diagnostic
to misattribute the transition.

Per A.3 that call is gone, so the specific defect is moot. It is recorded
here so it is not silently reintroduced if the hook's design changes
again: **any write to the RTC peripheral in this codebase must check the
return value before treating the write as done.**

### A.5 Test harness must not use hidden filenames

`rtc_skew_test.sh` creates `tests/.rtc_skew_range_harness.cpp` and
`tests/.rtc_skew_range_harness_bin`, and leaves the hidden harness behind
on a compile failure. This violates the temporary-artifact rule committed
as `d11694e` (`AI_DEVELOPMENT_WORKFLOW.md`, "Temporary build and test
artifacts"), which exists because a dot-prefixed test binary triggered a
real CrowdStrike alert on 2026-09-01.

Fix: use a **visible** path - `build-tmp/rtc_skew_harness.cpp` and
`build-tmp/rtc_skew_harness_bin` - add `/build-tmp/` to `.gitignore` (no
such path is covered today), and remove the artifacts on all paths
including compile failure.

Note the rule prohibits a leading `.` on **directories** as well as files,
so the temporary directory itself must be `build-tmp/`, not `.build-tmp/`.

### A.6 Test coverage gaps to close

Stage 7 found three mutations the suite does not catch:

- Replacing the production `if (rtcSkewTestGuard.shouldRun())` with an
  unconditional block leaves all tests passing.
- Changing the real `isRtcTimeValidForHibernate()` to always return false
  leaves `rtc_skew_test.sh` passing, because the script extracts only its
  constants and reimplements the predicate separately.
- No test models the correction -> hibernate -> fresh-boot sequence, which
  is why the A.1 defect survived.

Also: the claimed 2024-06 to 2034-06 sweep actually stops at 2033-05-30.

Each of these must fail under mutation before this WO passes Stage 7.

## Verification

- Host tests for any pure logic introduced (skew computation), following
  the `ChargeInhibitPolicy.h`/`ClockTrust.h` pattern.
- A test or check proving the code is absent from a default (`=0`) build.
- Boron build with the flag both `0` and `1`.
- Codex Stage 7. Standard tier is sufficient - this is bench tooling that
  cannot reach a field build - but the "absent when disabled" property must
  be verified explicitly, not assumed.

---

# Amendment B (2026-09-03) - A.2 re-arm semantics, finalised

**Decision by the Chief Engineer, 2026-09-03.** This closes the one open
question from implementation round 3. It is a resolution, not new scope.

## The mechanism as built

Round 3 pairs the fired-flag with `retained uint32_t
retainedRtcSkewTestArmToken`, compared each boot against
`fnv1aHash32(__DATE__ " " __TIME__ " " __FILE__)`. A mismatch forces the
fired-flag back to false before `OneShotGuard` runs.

This was required because a bare `retained bool` cannot satisfy A.2: Device OS
copies retained initial values only when the backup-RAM signature is invalid
(`wiring/src/user.cpp`, `backup_ram_was_valid_` ->
`system_initialize_user_backup_ram()`), and the signature is a fixed constant
(`0x9A271C1E`), not layout-derived. A flash with power maintained therefore
preserves the old byte and the `= false` initializer never runs.

## What it guarantees, and what it does not

- **Guarantees:** re-arms on any recompile of that translation unit.
- **Does not guarantee:** re-arming on a re-flash of a byte-identical image
  that was not recompiled.

A.2's literal wording is "fires once after a fresh flash". The token keys on
**recompilation, not flashing**, and those are not the same event.

## Resolution: accepted as a documented narrowing

The narrowing is **accepted**. Rationale:

- The failure mode A.2 exists to prevent is a bench device silently re-skewing
  its own clock across a multi-day soak. The token prevents that completely.
- Any realistic bench workflow compiles before flashing, so the token differs
  in practice.
- The residual case - flashing a byte-identical, un-recompiled image and
  expecting a re-arm - is not a scenario the bench procedure contains.

The header must state both halves plainly (round 3 already does). This is a
**known, accepted limitation**, not a defect, and **Stage 7 must not raise it
as a finding.** If a future bench procedure comes to depend on re-arming
without recompilation, reopen this amendment rather than working around it.

## Note on build reproducibility

`__DATE__`/`__TIME__` make a flag-enabled build non-reproducible. Both retained
variables are inside the `ENABLE_RTC_SKEW_TEST` guard
(`Generalized-Core-Counter.cpp:507-508`, within the `#if` at 490 and `#endif` at
509), so **default (`=0`) builds are unaffected** and remain reproducible.
Verified: flag=0 rebuilds to `text 148360 data 1090` with no `rtcskew` symbols.
