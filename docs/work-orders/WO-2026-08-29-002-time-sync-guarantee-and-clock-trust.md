# WO-2026-08-29-002: Guarantee time sync, make clock validity self-known, and stop a wrong RTC re-seeding itself

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
   its own repair. Fire around HIBERNATE wake specifically, and record
   *completion* (`Particle.syncTimeDone()` / `timeSyncedLast()`), not the
   request. Make `get_lastTimeSync()` actually read.

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

## Out of scope

- **The 12-hour local-time-conversion issue.** Per dispatch item 8. It is
  a separate defect with a separate signature (wrong sleep *entry*,
  correct 8h duration) and is not addressed here.
- **Modifying `lib/AB1805_RK`.** Vendored; fix app-side.

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
