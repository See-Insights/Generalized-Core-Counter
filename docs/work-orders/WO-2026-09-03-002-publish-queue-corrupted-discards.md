# WO-2026-09-03-002: Publish queue (PublishQueuePosix) discarding files as corrupted, in bursts

**Status:** Drafted, not dispatched. Specification only - **diagnostic first,
not authorized to fix blind.**

**Origin:** Observed live on Boron-Dev-11, 2026-09-02 serial log,
post-carrier-swap.

## The observation

Two discard bursts in one session - approximately 15 files (IDs 4968-4982),
then a second smaller burst (4970 area) shortly after - each logged as:

```
[app.pubq] INFO: discarding corrupted file <id>
```

Both bursts cluster around sleep/wake and disconnect/reconnect transitions.
Each discard is a **silent drop, not a retry** - whatever event data was queued
in those files is lost, not recovered. File IDs in the high 4900s suggest
meaningful queue history predating this observation.

## Open question before this becomes fix work

Is this **expected `PublishQueuePosix` self-cleanup** (documented library
behaviour for files left in an inconsistent state after an abrupt reset or
power loss), or an **active data-loss bug**? These call for very different
responses, and the difference must be established before any code changes.

## Acceptance criteria - diagnostic phase

- Review `PublishQueuePosix` library source/docs for what specifically triggers
  a "corrupted" classification and discard.
- Determine whether the discard bursts correlate with sleep/wake timing, with
  the `ledger_write()` failures in `WO-2026-09-03-003` (same session,
  overlapping timestamps), or with ungraceful resets/disconnects specifically.
- Quantify: how many files, how often. Is this an ongoing low background rate,
  or was 2026-09-02 anomalous?
- **Only after the above:** decide whether a fix - or merely monitoring and
  alerting on discard rate - is warranted.

## Provenance

Surfaced incidentally during the Dev-11 hardware investigation
(`WO-2026-08-31-004`) and is **independent of it**. Filed separately so it is
not conflated with the RTC / backup-rail work.
