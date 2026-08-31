# WO-2026-08-29-002: Clock-trust mechanism - RTC write-back, non-blocking sync observation, and a trustworthy clock signal

> **Title corrected 2026-08-31.** This WO was originally titled
> "Guarantee time sync, make clock validity self-known, and stop a wrong
> RTC re-seeding itself". After the scope narrowing below, it makes no
> wake-time *guarantee* - that moved to `WO-2026-08-31-002`. The H1 has
> been corrected; the FILENAME is deliberately unchanged, because the
> number and path are referenced from source comments, the sibling Work
> Orders, and this project's Stage 7 records. Only the heading was
> misleading, and only the heading needed to change.

> **Numbering note.** Dispatched as `WO-2026-08-29-001`, but that number was
> taken earlier the same day by
> `WO-2026-08-29-001-hibernate-wake-cloud-observability.md`. Renumbered to
> `-002`. The two overlap only at item 7 below; see "Relationship to -001".

> **Routing.** Investigation (items 1-4) is complete and recorded here.
> Implementation (items 5-8) routes to Copilot per the standing rule that
> real behavior work is not self-implemented. Verification is mandatory
> Codex Stage 7, same tier as the AB1805/watchdog and thermal-inhibit work.

## Premise: confirmed, after an earlier incorrect retraction

Dev-11's wall clock is ~5h09m behind and has not self-corrected. This was
first derived from `lastConnectionAgeSec`, then wrongly retracted by this
investigation, then re-confirmed independently. Recording both the finding
and the false step, because the false step is instructive.

The device `timestamp` field in the hourly report payload, against cloud
`published_at`, over nine consecutive post-wake reports:

```
cloud 2026-08-29 03:09:12   device 2026-08-28 22:00:01   -18551 s
cloud 2026-08-29 04:08:57   device 2026-08-28 23:00:02   -18535 s
cloud 2026-08-29 05:09:16   device 2026-08-29 00:00:03   -18553 s
cloud 2026-08-29 06:08:59   device 2026-08-29 01:00:03   -18536 s
cloud 2026-08-29 07:24:08   device 2026-08-29 02:12:21   -18707 s
```

Both clocks step ~hourly, so the rate is right and the offset is constant
- this is a fixed displacement, not drift, and not publish backlog (these
are isolated hourly publishes, not a flush; a flush shows many reports
sharing one cloud minute). Independently reproduces the 18,517 s deficit
computed from `lastConnectionAgeSec`, agreeing to ~30 s.

The earlier retraction rested on two status events 2.2 s apart reporting
`lastConnectionAgeSec` of 21646 and 28796, read as "both cannot be
elapsed time." They were never comparable: `appBreadcrumbMs` is 32045188
on the first (the pre-hibernate session, a queued publish) and 1155 on the
second (the fresh boot). Different sessions.

## Investigation findings

### 1. Explicit `Particle.syncTime()` - exists, fired, did not help

Exactly one call site: `Generalized-Core-Counter.cpp:2525`, inside
`dailyCleanup()`, gated on `Particle.connected()`, reached once per
*local* day from `REPORTING_STATE`.

It ran during this incident - `dailyCleanup()` also calls
`current.resetEverything()` (`:2531`), and the 03:09:12Z telemetry shows
`resetCount` 2 -> 0 and `dailyOccupancy` 449 -> 0. The clock was still
~5h09m wrong four hours and nine publishes later.

Two defects around it:
- `sysStatus.set_lastTimeSync(Time.now())` (`:2526`) stamps the *request*,
  not the completion, using the very clock under suspicion.
- `get_lastTimeSync()` has **no callers anywhere** in `src/` or `tests/`.
  The field is write-only. There is no elapsed-since-sync gate in the
  codebase today.
- The daily schedule is itself computed from the suspect clock, so a
  wrong clock can move or skip its own corrective sync.

### 2. Hourly reports are session resumes, not handshakes

Dev-11's Particle `last_handshake_at` is `2026-08-28T08:54:21.224Z` while
it has reconnected hourly since. Automatic sync-at-handshake therefore
does **not** recur hourly, and cannot be relied on as the resync path.

### 3. `Time.isValid()` returns true while the clock is wrong - the crux

`Generalized-Core-Counter.cpp:2067` emits `-1` for `lastConnectionAgeSec`
unless `Time.isValid()`. Dev-11's post-wake status published `28796`, not
`-1`. So **`Time.isValid()` was true throughout the wrong-clock window.**

This is the more serious of the two outcomes the dispatch anticipated.
`Time.isValid()` means "system time has been set from some source", and
`ab1805.setup()` (`:1057`) sets it from the RTC on every boot. It carries
no information about correctness. `Particle.timeSyncedLast()` and
`Particle.syncTimeDone()` are used **zero** times in `src/`.

Consequence for the dispatched plan: item 6 as written ("gate on
`Time.isValid()`") would not have caught this incident.

### 4. Ungated `Time.now()` consumers

42 `Time.now()` sites against 39 `Time.isValid()` sites. Confirmed
ungated reads used for persisted or reported values include
`MyPersistentData.cpp:683` (`set_lastCountTime`), `:880`
(`set_lastAlertTime`), `Generalized-Core-Counter.cpp:948`, `:2229`
(`set_lastHookResponse`), `:2526` (`set_lastTimeSync`),
`State_Sleep.cpp:1592`, `:1613`. Per finding 3, gating these on
`Time.isValid()` alone is necessary but not sufficient.

### 5. Root cause of persistence - not in the dispatched list

`AB1805::loop()` (`lib/AB1805_RK/src/AB1805_RK.cpp:50-63`) is the **only**
path that ever writes the RTC in this build:

```cpp
if (!timeSet && Time.isValid() && Particle.connected() && Particle.timeSyncedLast() != 0) {
    timeSet = true;
    setRtcFromTime(Time.now());
}
```

`timeSet` (`AB1805_RK.h:1001`) is a plain per-boot member. So the RTC is
written **at most once per boot**, at the first instant those conditions
hold. `ab1805.setup()` has already made `Time.isValid()` true from the
possibly-wrong RTC before this runs, so the one write can fire while
system time is still the wrong RTC value - a no-op - after which
`timeSet = true` blocks every later correction for that boot.

`AB1805::setRtcFromSystem()` (`:250`) exists and is the right API for a
periodic push. It has **no callers**.

Net: a wrong RTC seeds system time on each hibernate wake, the single
per-boot write-back can be spent before any real correction arrives, and
nothing ever re-writes it. The error is self-perpetuating across wakes.
**Item 5 alone (more `syncTime()`) cannot fix this** - correcting system
time never reaches the RTC, and the RTC is what seeds the next wake.

## Implement

6. **Periodic, monotonic-gated resync.** Elapsed-time gate <= 24h driven
   by `millis()`/uptime, not wall clock, so a wrong clock cannot postpone
   its own repair. ~~Fire around HIBERNATE wake specifically~~ - **struck
   2026-08-31, see "## SCOPE NARROWED" below.** That was a wake-time
   GUARANTEE and it is no longer this WO's to make: the gate becomes
   *eligible* after a hibernate wake (`timeSyncedLast()` reads 0 on every
   fresh boot) but a request is only issued when `Particle.connected()`,
   so a device that wakes with a wrong clock and sleeps again before
   connecting is never repaired here. Closing that loop requires waiting
   on sync completion in the sleep gate and belongs to
   `WO-2026-08-31-002`. What remains in scope here: record *completion*
   (`timeSyncedLast()` advancing), not the request, and make
   `get_lastTimeSync()` actually read.

   Note also that `Particle.syncTimeDone()` appears above as a completion
   signal. It is NOT one - it means "no longer pending" and goes true on
   timeout and disconnect. It was the round-1 defect and is no longer
   referenced anywhere in `src/`.

7. **Push corrected time back into the RTC.** Call
   `ab1805.setRtcFromSystem()` after a *confirmed* sync, and on a
   recurring basis - not once per boot. Do this app-side; do not patch
   the vendored `lib/AB1805_RK`. Without this, item 6 fixes RAM and the
   next hibernate wake reinstates the error.

8. **Gate the item-4 consumers on a trustworthy signal**, which per
   finding 3 is *not* `Time.isValid()` alone. Use sync recency
   (`Particle.timeSyncedLast()` age) as the trust signal, optionally
   corroborated by an RTC-vs-cloud agreement check.

9. **Publish clock trust per cycle.** Add sync recency and the trust
   verdict to the existing per-cycle diagnostic line and the status
   payload, so "was this timestamp trustworthy" is readable from
   telemetry rather than reconstructed. `ChargeDiag`/`PowerDiag` are the
   precedent for the line; `pinResetAb1805Fields`
   (`Generalized-Core-Counter.cpp:2090`) is the precedent for the payload.
   Check the payload budget first - `LedgerPayloadStatus: bytes=612/896`
   observed on Dev-11.

### Acceptance criteria amendment (Round 4 review, Codex Stage 7 Finding 5)

Codex Stage 7 flagged that `isClockTrusted()` (item 8's trust signal)
retains `Time.isValid()` as one of two ANDed terms, alongside sync recency.
The Chief Engineer has reviewed this and accepts it AS-IS - this is not a
defect to fix, so this amendment records the reasoning to avoid it being
re-raised:

`isClockTrusted()` is `Time.isValid() && (Particle.timeSyncedLast() is
recent)`. **Sync recency is the decisive term** - it is what actually
distinguishes "trusted" from "not" in every real scenario this Work Order
investigates, including the incident that motivated it (`Time.isValid()`
was `true` throughout; only sync recency correctly identified the clock as
untrustworthy). `Time.isValid()` is retained as a defensive, necessary-but-
not-sufficient co-requirement: it costs nothing in the failure mode this WO
targets (it is always `true` in that scenario, so it never blocks the fix
from working), and it correctly denies trust in the separate, degenerate
case where system time is not set at all (`Time.isValid() == false`) even
if a stale `timeSyncedLastMs` were somehow still nonzero from a prior boot
(defensive; RAM state does not survive a reboot in practice, but the
`isTrusted()` signature accepts these as independent parameters and should
not assume that invariant silently holds). Removing the `Time.isValid()`
term would not fix anything this Work Order is about and would remove a
free, harmless safety net - so it stays.

## SCOPE NARROWED 2026-08-31 (after two Stage 7 FAIL verdicts)

Codex Stage 7 failed this change twice. Both times the *mechanism* was
confirmed correct and every remaining HIGH finding sat in the **wake
sequencing** wrapped around it - territory this WO's own constraints
forbade touching ("do not restructure the state machine / sleep
scheduling"). Round 5 obeyed that fence and produced a fix that both
regressed existing behaviour (Stage 7 re-review finding 1: the forced-
connect branch made the opening-hour alert-40 suppression at
`Generalized-Core-Counter.cpp:1311` unreachable on every normal hibernate
wake) and still did not deliver the stated guarantee (finding 2: the
sleep gate waits on queues, ledgers, webhooks and updates - but not on a
clock sync, so a session-resumed device can hibernate again before
syncing).

The fence was the binding problem, not a safeguard. This WO is therefore
**narrowed to the clock-trust mechanism only**, and the wake-sequencing
guarantee moves to `WO-2026-08-31-002`, which is explicitly permitted to
touch the sleep gate and connection-mode policy.

### Retained here (independently confirmed correct by BOTH Stage 7 passes)

- The RTC write-back driven by observed change in
  `Particle.timeSyncedLast()`, independent of `syncTimeDone()` and of any
  request/pending bookkeeping.
- `observedTimeSyncedLastMs()` - the connected-only cached accessor that
  removes the blocking-API call from the main loop and from `setup()`.
- Wrap-aware trust and resync gating.
- Item 8 gating, including the three deliberate control-flow-sensitive
  stops (`lastAlertTime`, `lastHookResponse`, `occupancyStartTime`) and
  all `lastCountTime` sites.
- The accepted `Time.isValid() && recentSync` trust rationale (Stage 7
  judged the rationale sound; recency is the decisive term).

### Removed from this WO

- **Round 5's `setup()` forced-connect gate** (the
  `hibernateWakeWithoutConfirmedSyncThisBoot` OR term). REVERT it. It
  belongs to `WO-2026-08-31-002`. Reverting it also removes the alert-40
  regression, so this WO should clear Stage 7 on its own.
- The item 6 claim that resync "fires around HIBERNATE wake". This WO no
  longer claims a wake-time guarantee; it provides a correct mechanism
  that a connected device uses. The guarantee is `-002`'s job.

### Remaining work before sign-off (cleanup, not design)

1. **Pace the RTC-write retry (Stage 7 finding 5).** The consumed-write
   defect is fixed, but a permanent AB1805/Wire fault now retries
   `setRtcFromSystem()` on every main-loop pass - potentially hundreds of
   locked I2C transactions per second on a bus shared with PMIC work,
   plus matching log volume. Add a monotonic retry floor. The existing
   test asserts 200 attempts in 200 checks and therefore *codifies* the
   hot loop; it must be changed to assert the pacing.

2. **Republish status on confirmed sync (Stage 7 finding 3).** Assessed
   2026-08-31 as severable from the sleep gate and kept here. The status
   payload is published on connect, before the corrective sync, so it
   correctly reads `trusted=false` - and nothing republishes it when the
   sync later succeeds, so the ledger may never show `trusted=true`. The
   deferred-republish machinery already exists: `Cloud::loop()` at
   `Cloud.cpp:684` republishes when `pendingStatusPublish` is set, and
   retries on failure (`DeviceStatusPublisher.cpp:310`).
   `ConfigApply.cpp:150-152` is the precedent for setting it externally.
   `pendingStatusPublish` is private (`Cloud.h:412`), so this needs a
   small public entry point on `Cloud`. No sleep-gate, state-machine or
   connection-mode change is involved.

3. **Tests must assert behaviour, not code shape (Stage 7 finding 7).**
   `clock_resync_wiring_test.py` checks substrings, statement order and
   named calls - it would still pass if `requestClockResync()` stopped
   calling `Particle.syncTime()` altogether.
   `clock_rtc_writeback_test.cpp` exercises a hand-written simulator
   rather than the production function. Every changed test must be
   mutation-tested: break the behaviour, show the test fails.

## Out of scope

- **The 12-hour local-time-conversion issue.** Per dispatch item 8. It is
  a separate defect with a separate signature (wrong sleep *entry*,
  correct 8h duration) and is not addressed here.
- **Modifying `lib/AB1805_RK`.** Vendored; fix app-side.

## Verification

### Mandatory: Codex Stage 7 on the complete diff

Full Stage 7, same tier as the AB1805/watchdog and thermal-inhibit work -
not the lighter standard. Plus the existing host tests and a clean local
build.

### Primary functional test: a deliberately skewed RTC on a bench device

The failure path must be reproduced deliberately rather than waited for:

1. Set the AB1805 to a known-wrong time (a few hours off is enough; the
   observed natural case is -5h09m).
2. Boot. Confirm `ab1805.setup()` seeds system time from the wrong RTC and
   that `Time.isValid()` reports true - this is the precondition that makes
   the bug invisible today, and the test should assert it explicitly.
3. Confirm the new periodic write-back corrects the RTC after a confirmed
   sync, and that it does so on a recurring basis rather than being spent
   once per boot.
4. Force a hibernate wake and confirm the corrected time survives it. This
   is the step that actually distinguishes a fix from a cosmetic one: the
   pre-existing defect is that a correction to system time never reaches
   the RTC, and the RTC is what seeds the next wake.
5. Confirm the item-9 telemetry fields report the clock as untrusted during
   the skewed window and trusted after correction.

### Do NOT verify against Dev-11

Dev-11 is the live device that surfaced this and is still running ~5h09m
behind (confirmed 2026-08-29 across 15 reports spanning 03:09Z-14:15Z, and
again indirectly on 2026-08-30 when it entered hibernate at 19:10:00Z real
time, which is 22:01 SGT by its own displaced clock - exactly its
`closeHour=22`).

It is nonetheless the wrong verification vehicle, for a methodological
reason. Deploying this fix to Dev-11 requires an OTA, which produces
`RESET_REASON_UPDATE` and a full handshake. A full handshake performs an
automatic cloud time sync, so on that boot `Time.now()` is already correct
when the pre-existing `AB1805::loop()` one-shot fires - and the **old**
code writes the correct time to the RTC. Dev-11's clock would come back
right whether or not this change works. The act of deploying destroys the
condition under test, and the resulting pass would be unattributable.

Dev-11's value is as a preserved natural specimen, not a test target. Its
wrong RTC is perishable: any future boot that happens to include a full
handshake will silently correct it under the old code. Treat it as
read-only evidence until the bench test above has passed.

## Relationship to WO-2026-08-29-001

`-001` asks for hibernate-wake gate diagnostics (`wakeReason`, `rtcReadOk`,
requested-vs-actual sleep) when the qualification gate fails. `-002` item 9
asks for clock-trust fields. Same payload, same budget, adjacent
motivation - land them together if convenient, but they are independently
justified and neither blocks the other.

## Provenance

2026-08-29. Nine post-wake report payloads under
`particle-events/2026-08-29/Ubidots-Sensor-Hook-v1/e00fce683f6063bf254283dd/`;
status payloads under the same date's `status/` prefix; Particle device
API for `last_handshake_at`.
