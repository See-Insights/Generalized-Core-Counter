**1. F2 re-check**

Observation: You are correct. `occupancyStartTime` and `occupied` live in the persisted `CurrentData` structure (`src/MyPersistentData.h:650-678`, specifically `:675` and `:677`). On boot, `src/Generalized-Core-Counter.cpp:512-513` captures `session.occupancySessionBootAnchor` from those persisted fields (max of start and last report when occupied) before clearing the `millis()`-based `lastOccupancyEvent`. SessionState itself remains RAM-only by design, but the open-session start is not.

Inference: HIBERNATE after the daily close (v48 close-at-close) leaves no open session, so that path does not lose an in-progress occupancy. F2 reduces to the deferred-write window before an unflushable reset (OOM, hardware WDT, ApplicationWatchdog). That matches the bounded loss already tracked as item 15. I withdraw the stronger claim that the open session start is RAM-only.

**2. Concrete recommendations**

**F1 — RTC continuity beyond the planned carry rule**

The planned rule (valid RTC, <24 h since last sync, no oscillator-fault event since bind, next sync correction <60 s) is sound as a first cut. Additional continuity evidence that can be checked on a Boron with the AB1805:

- At each confirmed sync write-back, record the RTC’s own pre-write reading (`hal_rtc_get_time(EXTERNAL)`) and the delta. On the next boot, if the RTC has advanced by more than the wall-clock elapsed since that write (plus a small tolerance for the sleep/hibernate interval), treat continuity as broken and withdraw trust. The firmware already has the pre-write value in the write-back path (`Clock.cpp` lastRtcCorrectionSec logic).
- On every boot that finds a bound chip, read `Am18x5.status().clockSource()` (or the equivalent) and the oscillator-status bits. If the source is the RC fallback and it was not the source at the last successful write, withdraw. The S1 plan already snapshots `clockSource()`; persist the last-known good source in the retained or sysStatus block (one byte) so the comparison is possible across the restart.
- Bound the maximum credible drift. The field rate-halving on Dev-11 was 56–82 % of expected. A continuity check that rejects any boot-to-boot advance outside 0.5–1.5× the expected interval (or outside ±60 s of the expected wake) catches both that class of oscillator fault and a jump that the <60 s correction rule would otherwise accept on the next sync.

None of these require a new I2C transaction beyond what S1 already does; they only require storing one prior RTC reading and one clock-source byte.

**F4 — hard ceiling on consecutive stage-2 resets**

The existing cooldown and stage-3 escalation protect against a single long disconnect. They do not protect against a cycle in which stage 2 succeeds in resetting the device, the device comes back with a wrong-but-valid RTC (or a transient modem wedge that clears), the failsafe age base resets, and the ladder starts over. Sequence:

1. Device is disconnected long enough for stage 2. Cooldown has elapsed. Stage 2 reset fires.
2. Boot occurs. Clock is untrusted or wrong-but-valid (`isTimeValid()` true, trust false). Failsafe sees a fresh lastConnection or an age base that is now “today’s open”, so the stale threshold is not yet met.
3. Device connects briefly (or the modem comes up just long enough for a status publish), lastConnection is updated, then the modem wedges again.
4. Repeat. Each cycle burns a stage-2 reset and a cellular attach attempt. After N such cycles the device has never reached stage 3, the battery is lower, and the connectivity problem is still present. The cooldown only spaces the actions within one disconnect; it does not bound the number of stage-2 attempts across successive boots that each look “fresh”.

A hard ceiling (e.g., 3 consecutive stage-2 actions without a successful cloud sync that clears the recovery stage) forces the ladder to stage 3 even if each individual age calculation restarts. The existing low-battery suppression still applies.

**3. v50 switch plan review**

| ID | Severity | Area | Finding | Evidence | Recommendation | Confidence |
|---|---|---|---|---|---|---|
| V1 | High | Dual ownership / rollback | S1 leaves `ab1805.loop()` and the vendored object alive (S2 removes the loop). If a v50 build that has bound the chip is rolled back to v49 before S2’s watchdog replacement is complete, or if the rollback bench is only run on the integrated head, an armed ExternalWatchdog (instance 2 on D5) can still be running when v49’s vendored `setWDT`/`loop()` takes over. The parent WO’s bench item 2 covers the integrated case; it does not explicitly cover a partial S1-only rollback or a rollback that occurs after `enable()` but before the first ExternalWatchdog refresh. | Parent WO §Pre-ship bench item 2; S1 WO §1 and §6; S2 WO §3 | Add a bench step that binds under S1-only (or under the integrated head), arms the ExternalWatchdog, then flashes v49 and measures time-to-reset. Confirm v49’s first `setWDT` occurs before the chip’s remaining timeout expires. | Med |
| V2 | Medium | Wake classification | Reason 20 inside a due window is treated as a power-up wake. The S2 Step 0 item 4 correctly flags that `retainedHibernatePending` (and the RTC) can be lost on a power-down (spike P7). If the window cannot be evaluated, the rule falls back to “pin reset”. That is conservative, but a native POWER_OFF that returns with reason 20 and a cleared retained block will be classified as a pin reset even when the off-time was the intended recovery duration. S3’s bench is required to record the actual reason; the classification table is corrected only after that observation. | S2 WO ruling 2 and Step 0 item 4; S3 WO ruling 2 and bench; parent WO ruling 2 | Make the S3 bench item 4 a hard gate: if the observed post-POWER_OFF reason is not 20, or if the retained window is cleared, update the S2 table before the set merges. Do not ship with the “inside a due window” rule unverified against a real native power-off. | High |
| V3 | Medium | Power-off attribution | S2 Amendment 1 (b) and S3 Amendment 2 derive the “power-off requested” evidence from existing persisted stage/action fields rather than a new flag. This avoids a layout change, but only if every caller that can reach `deepPowerDown`/`POWER_OFF` has already written a field that survives the power cycle and is flushed. A path that calls the helper without that write (or whose write is lost in the 250 ms window) will be counted as an inferred watchdog. | S2 WO Amendment 1–2; S3 WO Amendment 1–2; parent WO §S3 | Step 0 must name the exact persisted field and the flush site for both callers. If either caller has no surviving field, stop and add the spare-byte flag before implementation. | Med |
| V4 | Low | Oscillator-fault latch | The “fault seen since bind” latch is process-lifetime RAM (S1 Amendment 2). It is set on an ExternalTime.onEvent or at registration if the clock source is already RC. It does not survive the restart that I21 is meant to handle. I21 will therefore see a cleared latch on every boot unless the last-known source is also persisted (see F1 recommendation above). | S1 WO Amendment 2; parent WO §I21 | Persist the last-known clock source (one byte) so the carry rule can withdraw on a source change across the restart. The RAM latch alone is insufficient for item 21. | High |

**One bench test to add**

On a bound device under the integrated v50 head: stop refreshing the ExternalWatchdog, confirm the chip resets the board in ~124 s, then immediately flash v49 and confirm that v49’s first watchdog service occurs before any residual chip timeout fires. Record the reset reason and the time from flash-complete to first v49 `setWDT`/`loop()`. This closes the gap between “v50 arms the chip watchdog” and “v49’s guard and vendored path reclaim the chip without a surprise reset”.

**What I did not read**

I read the parent plan (WO-2026-11-05-002), S1 (WO-2026-11-05-003), S2 (WO-2026-11-05-004) and S3 (WO-2026-11-05-005). I did not find a drafted I21 work order under the expected name; the parent’s description of I21 was used instead. I did not open the Device OS 6.6.0 hal sources, the spike binary logs, or the current v49.1 binding-guard implementation beyond the citations already in the progress pack.
