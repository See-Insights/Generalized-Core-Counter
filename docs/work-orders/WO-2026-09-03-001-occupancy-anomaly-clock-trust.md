# WO-2026-09-03-001: Occupancy-anomaly tracker reads clock before trust established

**Status:** Drafted, not dispatched. Specification only.

**Origin:** Observed live on Boron-Dev-11, 2026-09-02, during post-carrier-swap
bench monitoring (the observing session predates this WO's filing).

## The defect

```
OccAnom: path=idle now=1788352291 start=946684802 prev=12807 dur=841667489 occ=1 alert=19
```

`start=946684802` is Unix epoch approximately `2000-01-01 00:00:02 UTC` - an
uninitialized/default clock value, not a real occupancy start time. This
produced a spurious `dur=841667489` (~9.7 days) and fired `alert=19`.

## Root cause hypothesis

Same failure class as the six gated Finding-4 call sites in
`WO-2026-08-29-002`: a `Time.now()`-adjacent read occurring before clock trust
is established, the untrusted value gets stored, and a garbage duration is
computed once the real clock later resyncs. This is likely a **7th, currently
ungated call site** - the occupancy-anomaly tracker's start timestamp.

## Severity

Not safety-relevant. Produces a false/noisy alert only. Does not affect sleep,
wake, or reporting correctness.

## Acceptance criteria

- Identify the exact call site setting the occupancy tracker's `start` value.
- Gate it against clock trust the same way the six existing Finding-4 sites are
  gated (per `WO-2026-08-29-002`).
- Regression test: boot with clock untrusted, trigger an occupancy transition,
  confirm no anomaly alert fires until clock trust is established - or confirm
  the start value is deferred / re-baselined once trust arrives.
- Confirm reproducibility: does this fire on **any** fresh boot with the clock
  not yet trusted, or only under the specific fresh-MCU + carrier-pairing
  conditions observed on Dev-11 on the night of 2026-09-02?

## Provenance

Surfaced incidentally during the Dev-11 hardware investigation
(`WO-2026-08-31-004`) and is **independent of it**. Filed separately so it is
not conflated with the RTC / backup-rail work.
