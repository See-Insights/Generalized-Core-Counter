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

## Verification

- Host tests for any pure logic introduced (skew computation), following
  the `ChargeInhibitPolicy.h`/`ClockTrust.h` pattern.
- A test or check proving the code is absent from a default (`=0`) build.
- Boron build with the flag both `0` and `1`.
- Codex Stage 7. Standard tier is sufficient - this is bench tooling that
  cannot reach a field build - but the "absent when disabled" property must
  be verified explicitly, not assumed.
