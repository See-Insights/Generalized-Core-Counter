# WO-2026-09-22-001: occupancyStartTime future-date clamp is structurally unreachable

**Type:** Backlog investigation (deferred finding from the RuntimeReportingPolicy
Clock-routing dispatch).

**Status:** OPEN. Filed 2026-09-22. Not blocking that dispatch's close - see
its report for why.

## The observation

`currentStatusData::validate()` (`src/MyPersistentData.cpp`, occupancy-mode
sanity-check block) used to guard a future-dated `occupancyStartTime` clamp
with `else if (Time.isValid())`. Investigating this branch (prompted by a
second-review flag on a different, nearby `Time.isValid()` site) found it is
not merely usually-false - it is **structurally unreachable on every boot**:

- `validate()` is called exactly once per boot, from every
  `StorageHelperRK` backend's `load()` (checked all three vendored variants:
  `PersistentDataFile`, `PersistentDataFileSystem`, `PersistentDataFRAM`),
  itself called once from `setup()`.
- `currentStatusData::setup()` runs at `Generalized-Core-Counter.cpp:977`,
  over 150 lines before `ab1805.withFOUT(WKP).setup()`
  (`Generalized-Core-Counter.cpp:1130`) - the first thing that seeds `Time`
  from the RTC on any boot.
- `Time.isValid()` is therefore false at this call site on every boot,
  unconditionally. `Clock::isTrusted()` would be even further from true at
  this point (it additionally needs a completed sync this boot) - converting
  the gate to the trust signal would not fix anything, only rename an
  always-false condition.

The dead branch was removed (see `MyPersistentData.cpp`, same block) and
replaced with an explicit comment recording why, rather than left silently
gated on a signal that can never be true - the Clock-routing dispatch that
found this was scoped to `RuntimeReportingPolicy.cpp`, not to redesigning
this check, so it stopped at "make the dead state visible" rather than
relocating the check.

## What this means for the underlying safety check

If a corrupted/future-dated `occupancyStartTime` is ever read back from
persisted storage, it is **never clamped today** - not because of which
clock signal gates the clamp, but because the clamp only ever runs before
any clock signal exists. This is a real (if narrow) gap: `validate()`'s job
is exactly to catch this kind of corruption on load, and this one check
inside it cannot do its job at its current location.

## Recommended next step

Relocate the check to a point later in boot where a time source can
plausibly exist - options, not evaluated in depth here:

- Re-run this specific check (or a narrowed version of it) once from
  `checkClockResync()`'s success path, or once immediately after
  `ab1805.setup()` seeds `Time`, gated on `Clock::isTimeValid()` (RTC-seeded
  is enough for a plausibility clamp; full trust is not required for this
  purpose - a wrong-but-plausible clamp target is still better than an
  unclamped corrupted value).
- Or accept that a corrupted `occupancyStartTime` is instead caught by
  `total_occupied_seconds`'s existing 24-hour sanity check
  (`MyPersistentData.cpp`, a few lines above this block) acting as a
  downstream backstop, and formally retire the future-date clamp as
  redundant rather than relocating it - if that backstop is confirmed to
  cover the same corruption cases.

Either resolution is small; this is filed rather than folded into the
Clock-routing dispatch specifically to keep that dispatch's diff scoped to
the question it was dispatched for.

## Related work orders

- The RuntimeReportingPolicy Clock-routing dispatch (2026-09-22, not yet
  given its own WO number as of this filing) - the investigation that
  surfaced this finding while checking a sibling `Time.isValid()` site.
