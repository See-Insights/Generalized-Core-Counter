# WO-2026-09-20-001: `ENABLE_RTC_SKEW_TEST` can silently no-op

**Type:** Bench-tooling defect.

**Status:** Drafted, not dispatched. Investigation only - **no fix
authorized.**

**Origin:** Surfaced during Step 3b's (`WO-2026-09-19-001`) bench acceptance
run on Dev-14, 2026-09-20. The run needed a genuinely untrusted clock at a
real sleep decision and planned to reuse the existing
`ENABLE_RTC_SKEW_TEST` bench hook (`WO-2026-08-31-003`) to produce it. The
hook did not fire - the RTC was never skewed - and the acceptance run
proceeded by a different route (blocking cellular connectivity directly)
rather than diagnosing this in the moment. Filed here so the tool itself
gets fixed before the next bench cycle that needs it.

## The observation

`strings` on the flashed binary confirmed both hook log literals are
present:

```
RtcSkewTest: ONE-SHOT bench hook fired - before(raw=%ld readOk=%d) anchor=%s skew=%ldsec after=%s verify=%s verifyReadOk=%d
RtcSkewTest: ONE-SHOT bench hook FAILED - setRtcFromTime() write did not succeed; RTC left unchanged (before(raw=%ld readOk=%d) attemptedAfter=%s)
```

So `ENABLE_RTC_SKEW_TEST=1` genuinely compiled into this binary - this is
not the "flag silently compiled out" failure mode `WO-2026-08-24-001`
documented for a different flag. Despite that, the device's RTC was never
skewed: the boot that should have taken effect read a correct, trusted-
looking time, not the expected plausible-but-wrong one.

## Two undistinguished candidates

Neither could be confirmed or ruled out in the field, because the deciding
log line (`ONE-SHOT bench hook fired` or `FAILED`) is emitted before USB CDC
has enumerated - the same structural blind spot `WO-2026-09-19-001` names
for `TimeDiag:` observability during this same run, and the same shape as
the `hibernate_wake` boot-log-capture gap from Steps 1/2's bench work
(`WO-2026-08-29-001`).

1. **The build-token never re-armed.** `WO-2026-08-31-003`'s Round 4 fix
   widened the arm-token to a 64-bit FNV-1a hash of
   `__DATE__ " " __TIME__ " " __FILE__`, but that token only advances when
   `Generalized-Core-Counter.cpp` itself recompiles (it's the one
   translation unit the token is computed from). If this bench build was
   produced by a process that didn't force a fresh compile of that specific
   file - or if the flag was flipped and reflashed without the toolchain
   detecting a source change against a cached object - `retainedRtcSkewTestFired`
   could still read `true` from a prior arming, and the one-shot guard would
   correctly (from its own logic's point of view) decline to fire again.
2. **The hook fired and the write failed.** `ab1805.setRtcFromTime()` is a
   real I2C write; a failure would produce the `ONE-SHOT bench hook FAILED`
   line and leave the RTC untouched - functionally indistinguishable, from
   the outside, from "never attempted," since both leave the RTC exactly as
   it was.

## Why this matters

A bench tool that can silently no-op is worse than one that visibly fails:
the person running the induction has no signal that anything went wrong
until they notice the expected effect (a skewed clock, an untrusted-clock
sleep decision) never showed up - by which point the actual root cause
(stale token vs. failed write) has already scrolled past the one log line
that could have distinguished them, unobserved. This is exactly the failure
mode that made Step 3b's acceptance run take a different, unplanned route
rather than diagnosing and retrying the intended one.

## Fix shape (assessed, not implemented)

Give the hook a **retained flag or other signal readable on the next
boot**, independent of the transient serial line - something a bench
operator (or an automated check) can read back from cloud telemetry after
the CDC-enumeration window has passed, the same fix class Steps 1/2 already
applied to `hibernate_wake`. Candidates, not decided here:

- A retained outcome code (`kNotAttempted` / `kFiredOk` / `kFiredWriteFailed`
  / `kSkippedAlreadyFired`) surfaced in the next boot's `status` or a
  dedicated bench-only event, so a bench operator can confirm outcome from
  cloud history rather than a live serial session timed exactly right.
- Alternatively (or additionally): have the arm-token comparison itself log
  its own verdict (token matched / token changed / guard already fired)
  through the same retained-and-reported mechanism, so candidate 1 above is
  directly distinguishable from candidate 2 without inference.

Either shape needs its own review before implementation - this WO stops at
naming the problem and the two candidate causes.

## Severity

Low functional risk (bench-only tooling, `ENABLE_RTC_SKEW_TEST` defaults to
0 and cannot reach a field build - unaffected by this finding). Real
process risk: this is the second bench cycle to depend on this hook
(`WO-2026-08-31-003`'s original RTC-rate work, now Step 3b's acceptance
run), and the second time its own observability has been the limiting
factor rather than the mechanism itself.

## Not authorized in this WO

Any code change - the retained-signal fix shape above is assessed, not
implemented. Root-causing which of the two candidates actually occurred
during Step 3b's run is also not done here (the run moved on to a different
induction route instead); a future bench cycle with this fix in place would
resolve it going forward without needing to reconstruct what happened this
time.

## Provenance

Surfaced 2026-09-20 during `WO-2026-09-19-001` (Step 3b)'s bench acceptance
run on Dev-14, while attempting to reuse the `ENABLE_RTC_SKEW_TEST` hook for
induction.

## Related work orders

- `WO-2026-08-31-003` - the RTC skew bench hook itself, including the
  build-token arming mechanism this finding's candidate 1 concerns.
- `WO-2026-09-19-001` - Step 3b, whose bench acceptance run surfaced this
  finding and proceeded via a different induction route instead.
- `WO-2026-08-24-001` - the prior "flag silently compiled out" incident this
  finding is explicitly NOT a recurrence of (the flag did compile in here;
  the hook's own outcome is what's unobservable).
- `WO-2026-08-29-001` - established the retained-signal-plus-cloud-event
  pattern this WO's fix shape proposes reusing.
