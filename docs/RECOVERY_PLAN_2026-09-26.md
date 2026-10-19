# Recovery Plan: 2026-09-26

**Goal:** every report the device makes should reach Ubidots, using the design already built: send with `WITH_ACK`, wait for Ubidots' reply on a specific response topic, and escalate through the error state if the reply doesn't come. Restore that design; don't invent new mechanisms.

## What we know

- **Why reports go missing:** `WITH_ACK` was accidentally dropped in `eda6b7e` (v3.24, 2026-02-09) from the report, status, and diagnostic publishes. The whole fleet has run without it since then. Not caused by Step 5 or any WO this month.
- **The original design was sound.** The problems come from drift away from it:
  - `WITH_ACK` was lost (the missing reports);
  - an extra subscription to the whole `hook-response/` prefix was added. The original response topic (the device ID) is still subscribed; the broad prefix lets any reply release the wait (WO-2026-09-25-002).
- **The alert ladder is already fixed.** The old overwrite bugs in `alertResolution()` (cases 12, 13, 40) are gone: today's `resolveErrorAction()` returns alert 40's action directly. The webhook supervision block in `State_Report.cpp` is a *delayed* escalation to the error state, not a workaround for a broken case.
- **Cloud builds do not use the vendored libraries.** Because `project.properties` lists them, a cloud build installs the registry copies of AB1805_RK 0.0.4, StorageHelperRK 0.0.5, LocalTimeRK 0.1.3, PublishQueuePosixRK 0.0.7 (and its dependencies BackgroundPublishRK 0.0.2 and SequentialFileRK 0.0.2) over `lib/`. Confirmed with a cloud build of `599038e`: `REG_OSC_STATUS_OMODE` compiled as `0x01` (registry), so **PR #41's AB1805 fix is absent from every cloud-built binary**.
- **The fix is mostly restoration.**

## Worth keeping from the abandoned branch (`archive/wo-2026-09-25-001-round3`)

- The overflow guard, which fixes a stack overwrite (WO-2026-09-25-003).
- Verifying the actual binary (with strings, disassembly, or a compile-time probe), not only the source.
- The loss mechanism: without `WITH_ACK`, the queue deletes an event once it leaves the device.
- Duplicates: fleet-ops keys records by `published_at`, so resends create second records (WO-2026-09-25-004).
- The breadcrumb-18 connect stall (WO-2026-09-25-005), and the status ledger headroom (WO-2026-09-25-003).
- Diagnostic D: 7 lines that log each report's exact payload, for bench use.
- The method of matching reports to webhooks, for measuring delivery.

## Phase 1 investigation (answered 2026-09-26)

Codex (`gpt-6-astra`, reasoning high, read-only, against `599038e`), plus one scratch cloud build by Claude Code.

1. **Can Idle keep the device awake once `WITH_ACK` is back? Yes.** If Particle never acknowledges while connected, Idle refuses to sleep and its safety ceiling is disabled (`State_Idle.cpp:233`, `:280`, `:303`). v3.23 could also hang. A one-line handoff (`State_Idle.cpp:238` → `if (!updatesPending)`) passes the wait to Sleep's existing bounded gate (30–120 s, then alert 43 and disconnect).
2. **Alerts: already fixed** (see "What we know").
3. **Response topic:** the device ID, still subscribed; the extra `hook-response/` subscription is the problem.
4. **Library provenance: cloud builds use the registry copies**; PR #41 is absent from them (see "What we know").
5. **Queue publishes:** all six (report, `status`, `watchdog`, `hibernate_wake`, diagnostic helper, `pdiag`) are `PRIVATE` only.

## Way forward

**Phase 1: restore delivery** (first, because production has lost data since February).
- **1. WO-2026-09-25-001:** `PRIVATE | WITH_ACK` on all six queue publishes; the one-line Idle handoff (`if (!updatesPending)`) with the unused gate lines removed; a structural test that every queue publish carries `WITH_ACK`. About 15 lines of `src/`.
- WO-2026-09-25-002: remove the extra `hook-response/` subscription, keeping the device-ID response topic.
- Bench on Dev-14 (stacked with WO-2026-09-24-001, plus diagnostic D, with USB serial): 20 cycles of "report offline, then connect", one close, and one occupancy start and end. Pass: every report is delivered and the device sleeps every cycle.
- Then the fleet. Merge order: WO-2026-09-24-001, then 25-001, then 25-002.

**Phase 2: the alert ladder.** Only the `deepPowerDown()` bench check (the AB1805 configuration-key issue). The ladder itself needs no code change. Field evidence so far: failsafe stage 3 did power down successfully on 1 Oct (status at 12:43:05Z: reset reason 20, breadcrumbs wiped; WO-2026-10-01-001; the 4 Oct Codex report, `2026-10-04-pre-step6-known-issues-codex-report.md`, §C). The PWGT key fix is still open (see "Upstream reports").

**Phase 3: clarity (Step 5.5).** Pilot: the reporting state in four plain steps (always report → daily cleanup if closing → already connected? → connect now or later), about 20 lines, with the details in named owners (`DailyBoundary`, `Connectivity::shouldConnectNow`) and the response wait restored explicitly. Then `setup()` as named phases, then the other states.

**Step 6: the power-owner split.** Items deferred to it so far: *(Citations checked against main `e78d59d`, v37 merged, on 2026-10-07.)*
- **Duplicated power-source codes and PMIC fault masks** (WO-2026-09-29-002). Now **P-6** below.
- **Boron versus M-SoM build selection** (WO-2026-09-29-002). **Leftover:** goes with the Muon bring-up, split into capability, battery sampling, charging and PMIC WOs with actual target builds (WO-2026-10-15-003 Step 0 open points).
- **Connection-mode tug-of-war: FIXED in v39** (architect, 2026-10-17). *Original entry, kept for history:* (2026-10-03, found while explaining Trail02's 2-hour delivery spacing on v34):
  - `ConfigApply.cpp:445–455` restores keep-alive on every config apply;
  - `BatteryAuthorityCommand.cpp:80–84` re-applies intermittent in CONSERVING or worse;
  - the mode flips, and is persisted, on every connection.

  **Fix:** separate the configured mode from the battery downgrade. The effective mode is derived, and neither overwrites the other.
- **The failsafe's allowance for deliberate spacing: FIXED in v40** (architect, 2026-10-17). *Original entry, kept for history:* Its only allowance today is the low-battery block (`Generalized-Core-Counter.cpp:2657–2672`), which works only because the tug-of-war above sets the low-battery flag. **Goal:** the failsafe counts only the time when the device was expected to connect.
- **The power-split cut: P-0 to P-6** (architect ruling, 2026-10-17; from the WO-2026-10-15-003 Step 0 report's closing table, cited at `71e346f`). Each WO has a hard ≤ +20 net `src/` lines budget and keeps the 2b golden payload byte-identical. **P-0 and the six moves ship together as v45.**
  - **P-0 (WO-2026-10-17-001): pin the retained layout first** (architect, 2026-10-17). P-1's Stage 6 stopped on the retained-address check: moving code out of main reordered `bootStormCount`, `bootStormWindowStart`, `appBreadcrumb` and `appBreadcrumbMs`, because GCC lays retained variables out by first use. Device OS keeps retained RAM across an application OTA and checks only one signature word (`wiring/src/user.cpp:202–204`, same in 6.6.0), so the shifted values would be read as valid on the first boot after the update.
    - **Change:** one `retained` struct for the nine unvalidated retained variables in main (`Generalized-Core-Counter.cpp:491–499`), with a magic and a version first, checked as the first statement of `setup()` (before `ensureRetainedLoopForensicsInitialized()` at `:788`); a wrong magic or version resets them to today's initial values. Budget ≤ +20.
    - **Rule for P-1 to P-6:** decision-bearing retained values must be validated (magic and version, sentinel, or guarded) or stay put; report-only diagnostics (the ThrashGuard trio, the PMIC-anomaly fields) may shift, and each release records "first post-OTA values invalid" for them (architect, 2026-10-18; P-0 round 1 moved the ThrashGuard trio +4). P-1's saved patch is re-applied after P-0 merges (WO-2026-10-16-001).
  - **P-1:** move loop forensics and the awake watchdog to `observability/` (about 346 main lines; WO-2026-10-16-001).
  - **P-2:** move the connectivity failsafe to `power/ConnectivityFailsafe` (about 274).
  - **P-3:** move report, webhook and diagnostic publication to `cloud/` (about 262). **`dailyCleanup` moves with P-3** (`Generalized-Core-Counter.cpp:2697`, about 23 lines; the publish-then-cleanup order at `State_Report.cpp:69–82` stays).
  - **P-4:** move the clock and hibernate helpers to `time/` (85 Sleep and 93 main lines).
  - **P-5:** centralize the modem-health helpers and every latch writer in `power/Connectivity` (0 to +12).
  - **P-6:** centralize the source codes and PMIC masks, and delete 6c/6d (about −52).
  - **Leftovers, each its own WO:** the sleep gate and modem teardown extraction (its own Step 0 after P-6; they share cross-pass statics at `State_Sleep.cpp:335–349`, `:397`, `:428–448`); the modem behavior fix (after measured teardown traces settle the modem-off completion contract, before alert 15 or the latch change); Boron versus M-SoM portability (with the Muon bring-up).
- **Battery trust and tier.** The gauge is compared with a resting-voltage table (`kOcvKnots`, `BatteryHealth.cpp:19–32`) using a voltage sampled while awake, so healthy devices were marked untrusted and dropped to CRITICAL: Court3 at 82%, PCKL1 at 77% (the 4 Oct Codex report's ledger snapshot: 79.8% and 76.5%, both Untrusted/CRITICAL), and Dev-14 showing CRITICAL at 65%. Decide only after a week of v37's `vc` data (WO-2026-10-04-001 item D). If that isn't enough, add the fuller battery-decision snapshot proposed in the 4 Oct Codex report §D (raw and accepted SOC, vcell, charge state, radio state, sample age, source, resulting tier).
- **WO 3 (battery): which sensor modes a low tier downgrades.**
  - **Today's rule:** the connection-mode downgrade applies only to OCCUPANCY devices configured for KEEP_ALIVE (`PowerManager::downgradeActive()` from v39; the `:83` write on v38). A COUNTING device in a low tier stays in its configured mode.
  - **Decision for WO 3:** whether low tiers should downgrade every sensor mode.
  - **WO 3b: recalibrate the gauge (architect, 2026-10-10; part of this WO, after the Dev-14 controlled drain).** When SOC is Untrusted and the battery is at rest (no charge current, radio off), re-seed the gauge with `quickStart`.
    - **Open question for the evidence:** what caused Trail02's 52 → 2.6% jump at 16:00:41Z on 8 Oct? vcell was steady, and no firmware `quickStart` path matches it.
  - **The Trail02 evidence run is complete (architect, 2026-10-10).** The battery never went flat; the gauge re-estimated. See the evidence notes below and PR #87.
  - **Live data, 2026-10-08:** the only device below HEALTHY is Trail02. It runs OCCUPANCY (`sensorMode` 1) in KEEP_ALIVE (`connectionMode` 3), from the product defaults, at SURVIVAL on v38, **so it is downgraded**.
  - **Its flag flips on v38.** `device-status` showed `battery.lowBatteryMode: true` at 03:00:16Z (the AWS fleet-ops copy) and `false` at 03:00:22Z (Particle's current version, written after that connection's config apply cleared the flag). That is the flip-flop WO-2026-10-07-002 fixed in v39.
  - **Consequence for checks:** on v38, a single ledger read can show either value. Fleet checks should judge a downgrade from tier and modes, not from the flag alone.
  - **WO 3 evidence (Trail02, `device-status` 2026-10-08 03:00Z):** SURVIVAL at **53.8% SOC marked Untrusted, with vcell 3.61 V** (`vcellState` Known). Also `power.profile: UsbBench` while `power.source: BATTERY`, a profile and source mismatch (see "Power-source misreads").
  - **WO 3 evidence: SOC against vcell divergence (Trail02, on v38, discharging).** From 2026-10-08 03:00Z to 2026-10-09 03:00Z, SOC fell from **53.8% to 2.2%** (Untrusted, then Suspect) while vcell only went from **3.61 V to 3.57 V**. The tier stayed SURVIVAL throughout. This is a case for the tier following voltage when SOC is untrusted.
  - **WO 3 evidence: the tier already follows voltage when SOC is not Trusted (code, main = v38 for these files).** `socTrust` comes from `BatteryHealth::evaluate()`, which compares the gauge SOC with a resting estimate from vcell (`kOcvKnots`, `BatteryHealth.cpp:19-32`). It returns Untrusted at |residual| ≥ 20 and Suspect at ≥ 12; each band widens by 8 on the low side while charging and on the high side while the radio is on (`:38-44`, `:94-105`). When SOC is Suspect or Untrusted, the tier is computed from the resting estimate (`BatteryTierGuard.cpp:5-10`, `BatteryAuthority.cpp:34-37`, `PowerTier.cpp:41-42`). So Trail02's SURVIVAL at 3.57–3.61 V (resting estimate about 17–21%) was voltage-driven, not SOC-driven.
  - **WO 3 evidence: published SOC ignores trust.** The webhook's `battery` field (`Generalized-Core-Counter.cpp:1995`, `:2022`), `device-data` `battery.soc` (`DeviceStatusPublisher.cpp:523`) and `device-status` `battery.soc` (`:246`) all read `PowerManager::soc()` = `CurrentReadings::get_stateOfCharge()` (`PowerManager.cpp:231-233`). That is the gauge SOC committed at `SensorManager.cpp:712`/`:768`, whatever its trust. Only `device-status` carries `socTrust` (`:259`).
  - **WO 3 evidence: the firmware never re-syncs a gauge that is merely wrong.** `fuelGauge.quickStart()` runs only at `SensorManager.cpp:588`. That needs a setup sample, a wake from low-power sleep, or the first sample after a power-management reset (`:490-494`), **and** a sample that looks suspicious: SOC outside 0–100, SOC ≤ 0 with vcell > 3.60 V, or SOC ≥ 100 with vcell < 4.15 V (`:89-120`, `:568-575`). A 12% or 98% reading at a normal voltage never qualifies. `BatteryHealth` is a trust signal only and never corrects (`:823-825`).
  - **WO 3 evidence: Trail02 discharge and recharge (S3 raw events, v38).**
    - **8 Oct 16:00:41Z:** the gauge stepped from 52.31% to 10.26%, then 2.6%, then 2.28% within 3 s, with vcell steady at 3.60–3.62 V. That is a gauge re-seed, not a drain.
    - **Not dark:** reports carried on at about 2.26% until 03:00:25Z on 9 Oct.
    - **9 Oct 03:40:30Z:** a status event shows `resetReason` 40 (power down), alert 0, `watchdogResetCount` 37.
    - **Hibernate:** `rtcBefore` puts it at about 03:46Z, for 22446 s.
    - **9 Oct 10:00:21Z:** an RTC wake on schedule (`hibernate_wake` `osReason` 30, `wakeReason` UNKNOWN, `rtcOk` 1). The first status shows `resetReason` 30, data 0, alert 0, `watchdogResetCount` 37 (no failsafe or watchdog resets). The status event has no clock-trust field. `device-status` at 00:00Z on 10 Oct shows `clock.trusted: true`.
    - **The recharge happened while it hibernated** (03:46–10:00Z, local night). The first report shows `battery` 6.0, `vc` 4.12, Charging, then Charged at 4.11–4.15 V. What charged it isn't visible in telemetry.
  - **WO 3 evidence: the SOC since recharge creeps toward vcell slowly.** The gauge SOC went 6.24% (10:05Z) → 8.34% (15:00Z) → 10.0% (16:00Z) → 11.81% (00:00Z on 10 Oct). That is about 0.4 %/h at first and about 0.23 %/h overnight, roughly +0.25 per report. Meanwhile vcell held at 4.11–4.14 V with `chargeState` DONE, so the resting estimate is about 94%. At 00:00Z `device-status` showed SOC 12.1, `socTrust` Untrusted and `tier` HEALTHY (voltage-driven). A colleague's serial capture (9.2 → 9.7%, vcell 4.111–4.113 V, `tier=H`) matches the 15:37–15:38Z reports on **9 Oct**. INF: if the creep stays linear, closing the gap of about 82 points takes about two weeks. Until then the webhook and `device-data` report about 10% for a full battery.
  - **WO 3 evidence: Dev-09 shows the same mechanism in the other direction.** On 9 Oct, ChargeDiag showed `soc=80.4` at `vcell=4.030` both before (13:00 SGT) and after (14:26 SGT) a reset-pin reset; the pin does not reset the gauge. After a full power-off (about 14:40–15:10 SGT), the first ChargeDiag showed `soc=98.1` at the **same** `vcell=4.030` (`BatteryHealth: trust=Suspect … restingEstimate=76.00 residual=22.13`), then reports of 98.45 → 97.34. INF: removing power re-seeds the MAX17043 from the cell voltage at power-up (under charge or load here), and the firmware leaves the result alone because it is not "suspicious".
- **CONSERVING threshold.** A device drops to CONSERVING below 70% and returns to HEALTHY at 75% (`BatteryBackoffPolicy.h:21–26`; corrected per the 6 Oct Codex Step 6 ownership report). That may be too high for solar sites going into shorter days. Tune it from field data on how often devices actually get that low. (Moved here from the Phase 4 list.)
- **Power-source misreads:** USB_ADAPTER and USB_HOST swapping on the same supply, as seen in Dev-09's and Dev-14's PowerDiag lines.
- **WO 2b (the sensor list) supplies the allowed `sensor.type` values.** WO-2026-10-08-002 (1c) narrowed `sensor.type` validation to 1 (PIR), the only type `SensorFactory` builds, and so hard-coded "PIR only" in `ConfigApply.cpp`. When 2b introduces the sensor list, config apply should take the allowed types from it.
- **WO 2b: a sensor that fails to start is silent.** If `SensorFactory::createSensor` returns null, `SensorManager::initializeFromConfig` logs an error and the device carries on with no sensor (`SensorManager.cpp:311-313`). Reports go out with zero counts and no alert is raised (WO-2026-10-08-002 Step 0 §3). Add an alert or a status flag with the sensor list.

**Phase 4: backlog**, in order: build provenance (vendored versus registry libraries; WO-2026-09-26-001), then the connect stall (WO-2026-09-25-005). The rest is grouped below.

**Clock owner (Step 5.5/6).**
- **Trust the AB1805 at boot** when `isRTCSet()` is true, the oscillator-fail flag is clear, and the time isn't earlier than the last persisted time. That removes the boot-time reconnect for clock sync and lets a missed close or session credit run in `setup()`. **Bench:** power-cycle with and without the RTC losing power. **Session credit across a restart (v37, WO-2026-10-04-001 item B):** PASS as designed. Example: an 11-minute session interrupted by a reset was credited 5 minutes (cap = last report + debounce). Worst case is under an hour of undercount with hourly occupied reports. Full recovery deferred to this work: a last-alive record in the AB1805's RAM.
- **AB1805 flags:** fix the sticky oscillator-fail flag, and clear SLST after reading it (`AB1805_RK.cpp:186` never clears it, so every wake after one deep power-down is labeled `DEEP_POWER_DOWN`; 4 Oct Codex report §C). This replaces the Dev-11 oscillator test from the old Phase 4 list; Dev-11 was retired on 29 Sep, so it needs another bench unit.
- **A late ALARM wake reports success:** apply the same on-time window as v37's item C (WO-2026-10-04-001).
- **`deepPowerDown()` and the PWGT configuration key:** see Phase 2.

**Upstream reports.**
- **AB1805_RK library.** Five defects, not yet written down elsewhere: the three wrong Osc. Status constants XTCAL, LKO2 and OMODE (fixed in the vendored copy by PR #41, `ab2e0fe`, "to be filed upstream"); `deepPowerDown()` sets PWGT in `REG_OSC_CTRL` without writing the configuration key first, so it never takes effect (`AB1805_RK.cpp:562`); and `resetConfig()` does the same for `REG_OSC_CTRL` and `REG_TRICKLE` (`:156–157`). Add a sixth: SLST is never cleared (`:186`).
- **Particle Device OS:** the `SystemSleepConfiguration` move-assignment leak (draft: `WO-2026-10-02-003-particle-bug-report-draft.md`). Issue link: not yet filed.

**Connectivity** (deferred from the 1 Oct investigation: `2026-10-01-connectivity-investigation-codex-report.md`, recorded in WO-2026-10-01-001's non-goals).
- **Connection attempts** (report #2): every Nth retry a full 11-minute attempt (Particle's guidance), and drop the "above 50% means always long" rule.
- **A cap on total awake time across retries** (report #4).
- **Slow modem teardown** (report #6): measure first, change nothing yet.

**After Step 6.**
- **Ledger content review:** what we send to the status and data ledgers, and why. The STATUS payload is 834 of 896 bytes (62 bytes of headroom), so decide what belongs there, what can be dropped, and what belongs in events instead. Supersedes the separate "ledger headroom" item.
- **`connecttime` is the previous connection's duration:** the payload is built in Report, before Connect (`State_Report.cpp:69`, `Generalized-Core-Counter.cpp:2028`). Rename it or document it in the payload review.
- **Widen the reporting-interval storage if a daily cadence is ever needed.** `reportingIntervalSec` is stored as `uint16_t` (`MyPersistentData.h:137`). WO-2026-10-08-001 caps it at 65535 s (about 18.2 h) in ConfigApply's validation.
- **All-or-nothing config apply (possible later WO).** `applyConfigurationFromLedger()` (`ConfigApply.cpp:132-139`) applies each section's valid fields, then combines the results. So a ledger update with one out-of-range value applies everything else and keeps only that field's old value. WO-2026-10-08-001 accepted this as existing behaviour. Validate every section first if all-or-nothing is ever wanted.
- **Compute the failsafe jitter once at boot.** `connectivityFailsafeJitterSec()` (`Generalized-Core-Counter.cpp:386-400`) builds a `String` from `System.deviceID()` on every call. Since WO-2026-10-08-001 moved the cadence rule below the cooldown check (`:2648`), long-cadence devices make that call on every loop pass while they wait out their extended threshold. Main already does the same for any device past 3 h in open hours. The value is a deterministic hash of the device ID, so compute it once at boot.
- **The cadence-shortening gap in the failsafe** (WO-2026-10-08-001, accepted edge). The threshold (cadence + 3 h when the cadence is ≥ 3 h) uses only the *current* cadence. If the battery recovers and the cadence shortens (for example from 12× to 1×) after age has passed 3 h, the failsafe can reset before Report gets its first chance on the new schedule. It isn't a regression against main, which resets at 3 h anyway. Closing it needs a stored "cadence since the last connection", which is new state.
- **CI:** run the host test suite (sh via zsh, py via python3) plus the WITH_ACK structural test on every PR via GitHub Actions, so a PR with failing tests can't be merged.
- **Renaming the webhook event:** its own WO, with a cut-over that never leaves a gap. No name may be a prefix of another.
- **Occupied courts reporting more often than hourly,** with mid-session reports carrying the session's minutes so far.
- **The reporting-state pilot (Phase 3),** starting with a voice walkthrough of `handleReportingState()`.
- **Muon M524 bring-up,** then the M635e and M404.
- **Alert 18 (state-machine thrash):** raised just before the tier-3 reset, so it may not be persisted. Fix with the ThrashGuard/persistence work (save state before any deliberate reset), then make 18 report-once like 19 and 42. (WO-2026-10-06-001 round 2: Stage 7 found the tier-3 loss, so 18 was dropped from v38. Until then it clears: never (planned).)

**Queued WOs** (in order; architect, 2026-10-09):
1. **The TimeDiag flood in CONNECTED.**
   - **Bug:** Idle's CONNECTED branch calls `logTimeDiag()` on every loop pass in open hours (`State_Idle.cpp:119-125`), unthrottled since `a95e284` (2026-06-06, v14).
   - **Field case:** Dev-09 on v40, 2026-10-09 14:27 SGT. It emitted about 100 lines/s, the forwarder carried about 1 line/s, and Chip powered it off.
   - **Evidence:** `docs/work-orders/2026-10-09-connected-idle-sleep-inventory.md` (PR #83).
   - **Next:** the Step 0 is in progress.
2. **WO-005: a config wait for inbound ledger syncs.** Chip's 2026-10-07 decision: one config wait per day, at the first connection after open hour plus the boot connection.
   - **Field evidence (Dev-09, v40, 2026-10-09):** `connectionMode: 0` was set in `device-settings` at 13:04:51 SGT. It did not land through five connections; at 14:00:44 and 14:10:49 the inbound sync failed with `-1001` about 3 s after `GateRelease … reason=ledger`. It landed only on the boot connection after a pin reset (14:27:00, `Config: Connection mode -> CONNECTED`).
   - **Follow-up, no code now (architect, 2026-10-14):** Particle (forum, 13 Oct 2026, rickkas7) plans to investigate a cloud-to-device sync notification API in their next sprint; no name or date yet. **When Particle's inbound-sync notification API ships, replace the 10 s timed hold with a wait on that signal and reconsider the once-a-day limit.**
3. **The modem standby latch.**
   - **Mechanism:** a teardown over 10 s marks the modem unstable and suppresses standby (`MODEM_UNSTABLE_SLOW_TEARDOWN_MS`, `State_Sleep.cpp:31`, `:887`). Recovery needs a teardown under 5 s (`MODEM_UNSTABLE_RECOVERY_TEARDOWN_MS`, `:32`, `:136`). With standby suppressed, every teardown is a full modem-off, so on a modem that takes about 19 s to power off the device never recovers until a reboot (RAM flag).
   - **Field case:** Dev-09 was latched from 2026-10-08 12:19 (after its breadcrumb-28 watchdog) through all 56 sleeps that day, and again on 2026-10-09 after a failed dial at 13:49.
   - **Cost:** every connection is a cold registration.
   - **Finding: on PCKL1 the latch re-trips within minutes, so v42's daily clear can't help it** (architect, 2026-10-15; PCKL1 daily-clear check, result: not evidenced).
     - **Re-trip after the reboot** (source: S3 `Ubidots-Sensor-Hook-v1/e00fce6865fe7770b80a014e/`, 2026-10-14 from 21:30Z and 2026-10-15 to 10:57Z, field `connecttime`):
       - warm before: 1 s at stamps 22:00:02Z and 22:10:51Z;
       - then 211 s (22:23:43Z) and 0 s (22:24:10Z);
       - then the breadcrumb-28 sleep watchdog (S3 `watchdog`, published 2026-10-14 23:00:48Z: `osReason 60, bc 28, stage sleep`; status 23:00:49Z: `watchdogResetCount 28`);
       - **warm, 2 s, at 23:00:33Z, the first connection after the reboot;**
       - **cold from 23:07:15Z** (13 s), then 11–17 s on every connection through 10:45:35Z on 15 Oct.
     - **Through the close:** the close report (stamp 01:59:59Z) took 12 s and the first report after the open (stamp 10:00:44Z) took 12 s. There were no `status` or `watchdog` events on 15 Oct up to 10:57Z, so no overnight reboot.
     - **INF:** the daily clear at the 02:00Z close re-allowed standby, if it ran; the first teardown after it re-tripped the latch. The data can't tell this apart from the clear not running: US devices have no serial. It also assumes the latch is what makes connections cold, and the 211 s connection at 22:23:43Z isn't explained.
     - **The contrast:** Court1, Court3 and MAFC-1 went from cold on v40 (100 %, 100 % and 56 % of connections ≥ 10 s) to warm (about 1 s) straight after the v42 OTA reboot, and stayed warm. Source: the v42 fleet health check (PR #102, `docs/work-orders/2026-10-15-v42-fleet-health-check-report.md`), window from each OTA (14 Oct 10:01–10:03Z) to 15 Oct 02:03Z; not checked since.
   - **A trigger seen on Dev-14, 15 Oct** (source: S3 `serialLog/e00fce688e592afaf23ac4fb/`, 06:11:39–06:11:42Z, v42, the first sleep after its failsafe reboot):
     - `06:11:39 0000031407 SLEEP: disconnect/modem-off exceeded budget (30001 ms, cloud=0 radioOn=0 standby=0) - raising alert 15` (`State_Sleep.cpp:838`);
     - then `06:11:40 MODEM_HEALTH: unstable reason=slow_teardown elapsed=30010` (`:112`);
     - then `06:11:42 MODEM_POLICY: standby temporarily disabled reason=unstable_modem` (`:124`).
     - **The context is the modem-off sleep loop** under "Observations to watch". Registration was failing at the time, so this teardown may not be typical of a healthy device.
   - **Queued investigation** (read-only, later; no code changes now):
     1. **Per device, how often modem-off overruns its budget.** Count `exceeded budget` and `MODEM_HEALTH: unstable` lines per device and per day from serial (Dev-09, Dev-14). US devices have no serial, so for them look for a connect-time proxy: warm-to-cold steps in the webhook `connecttime`.
     2. **Whether the budget is realistic.** Compare `disconnectBudgetMs` (`computeDisconnectBudgetMs()`, `State_Sleep.cpp:764`; bounds `DISCONNECT_BUDGET_MIN_SEC`/`MAX_SEC`, `:740–747`) and the 10 s slow-teardown threshold (`MODEM_UNSTABLE_SLOW_TEARDOWN_MS`, `:31`) with measured `modem-off complete elapsed=` times (verbose serial only), including the about 19 s power-off noted above.
     3. **Whether PCKL1's breadcrumb-28 sleep watchdogs coincide with slow modem teardown.** For example, the 211 s connection just before the 14 Oct 23:00Z watchdog. Line up `watchdog` events (`bc 28, stage sleep`) with the `connecttime` steps around them, across the fleet's breadcrumb-28 cases (PCKL1 14 Oct, PCKL2 12 Oct, MAFC-1 13 Oct; PR #102).
4. **WO 3a: publish a trusted battery value** (architect, 2026-10-10). In v41 if it's ready.
   - **Change:** when `socTrust` ≠ Trusted, the webhook `battery` field and both ledgers' `battery.soc` publish the voltage-based estimate the tier already uses, not the raw gauge SOC.
   - **Budget:** ≤ +10 expected.
   - **Field case:** Trail02 reported about 10% at 4.11–4.14 V with the charger showing DONE (the voltage-based estimate is about 94%; WO 3 evidence, PR #87).
5. **The downgrade check misses connected KEEP_ALIVE devices** (architect, 2026-10-11).
   - **Bug:** the battery check that sets the downgrade runs only on a report taken while the device is disconnected (`State_Report.cpp:168`, `:184`; the flag is set at `BatteryAuthorityCommand.cpp:80-83`). So a device that stays connected in a low tier never downgrades.
   - **Field case (11 Oct, v40 fleet health check, PR #89):** Court1 was CONSERVING at 3.97 V in KEEP_ALIVE with `lowBatteryMode: false`. Court3, in the same state, did downgrade.
   - **Same in v38 and v40.** Step 0 is history first.
6. **A general log-flood limiter.**
   - **Design (trimmed by the architect):** a `SerialLogHandler` subclass that mutes per-key repeats only: the key is the message with digits stripped, about 8 slots, one summary line per muted burst, ERROR exempt, and lines are dropped, not queued.
   - **Out of scope:** no global token bucket, and no status-ledger counter (the status payload is at about 847/896 bytes).
   - **Budget:** about **+30**, pending Chip's approval. History-first Step 0. Not yet dispatched.
7. **Small fix: alert 40's 3 h window appears to count closed hours** (architect, 2026-10-11).
   - **Mechanism:** at the first open report, the age of `lastHookResponse` spans the overnight close (about 8 h). That is more than `WEBHOOK_LONGTERM_ALERT40_SEC` (3 h, `ConnectivityPolicy.h:223`), so `raiseAlert(40)` runs (`State_Report.cpp:108-121`) on every non-hibernating device at every open. The first webhook response clears it (`Generalized-Core-Counter.cpp:2412-2416`). The only suppression is for an overnight hibernate wake at the open hour (`:1478-1485`), whose own comment calls the overnight gap "expected, not an error".
   - **Evidence (v40 fleet health check, pass 2):**
     - all eight non-hibernating US devices showed alert 40 on their 10 Oct OTA-boot status, each with `lastAlert` equal to its 06:00 open-report timestamp;
     - Court2's v38 OTA boot on 7 Oct showed the same;
     - Trail02, which hibernates, showed 0.
   - **Limit:** an ordinary morning can't show it. The report payload records the alert code before the raise (`State_Report.cpp:69`), `device-status` has no alert field, and `status` events are published only at boot.
   - **Why it matters:** only the 3 h cooldown, stamped by the raise itself (`MyPersistentData.cpp:913`), stops the 6 h ERROR_STATE escalation (`State_Report.cpp:136-147`) on that same pass.
8. **CONNECTED Idle: a no-progress THRASH false alarm on leaving Idle, and no reconnect after a cloud drop** (architect, 2026-10-13). After the current queue. One Step 0, history first; split if over +20.
   - **THRASH false alarm** (Dev-09, v41, CONNECTED, 13 Oct, forwarded serial):
     - `06:24:01 THRASH st=Sleeping noprog=1412s last=QUEUE_DRAIN tier=1` and `07:00:10 THRASH st=Reporting noprog=2169s last=STATE_TRANSITION tier=1`.
     - Idle has no ThrashGuard timeout (`ThrashGuard.cpp`, `timeoutForStateSec`: IDLE returns 0), but the no-progress clock keeps running. On leaving Idle, `thrashGuard.loop()` (`Generalized-Core-Counter.cpp:1651`) applies the new state's timeout before `publishStateTransition()` marks progress (`:2481`).
     - **Escalation risk (inferred, not seen):** two trips within 10 min give tier 2 (alert 18, forced disconnect and sleep); three within an hour give tier 3, a `System.reset()`.
   - **No reconnect after a cloud drop** (same device, 06:23:56–07:00):
     - The cloud socket closed. The Idle ceiling forced a teardown and sleep (`IDLE ceiling trip: mode=0 cloud=0`), and the CONNECTED+open abort returned it to Idle (`sleep-abort-open-hours`).
     - Idle's CONNECTED branch has no reconnect, so it stayed offline until the 07:00 scheduled report: about 30 min.
   - **Inventory:** `docs/work-orders/2026-10-09-connected-idle-sleep-inventory.md`, "Ceiling trip" (now observed) and open point 2.
9. **`sensor.setting1 == 0` invalidates the config for every sensor type** (architect, 2026-10-14; found by WO-2026-10-13-002 Step 0 round 2).
   - **Code:** `Config.cpp:94–101` treats setting1 as the occupancy debounce for all sensor types. A zero value logs `ConfigInvalid: sensor.setting1 (occupancy debounce) missing/zero` and marks the config invalid, which forces a connect at `State_Report.cpp:171`.
   - **Why it matters:** harmless while PIR is the only sensor. A future sensor that doesn't use setting1 (or uses 0 legitimately) would be rejected. Decide the per-type rule with the first new-sensor WO.
   - **New-sensor note (architect, 2026-10-14, from WO-2026-10-14-003):** on a closed boot (`_sensor` null), `SensorManager::activeSensor()` asks the factory for the configured `sensor.type`. An invalid type logs `Sensor type %d not yet implemented` once per closing report before falling back to PIR. Accepted for now; revisit when the factory gains a second sensor type.
   - **New-sensor note: field snapshot timing (architect, 2026-10-14, from the WO-2026-10-14-003 Stage 7 inference).** Since 2b-3, the sensor's fields are read in `reportFields()` just before `publishData()`'s main `snprintf` reads the common fields. Today nothing writes either set in between, so the output is unchanged. A future sensor whose values change concurrently (for example from an ISR or another thread) could produce a cross-field snapshot taken at slightly different moments. Check this when that sensor is added.

10. **Occupancy after the close while awake counts into the next day** (architect, 2026-10-16; found in the v43 Dev-09 soak, PR #106).
   - **Behavior:** the daily total is zeroed only when the close boundary is processed (`State_Report.cpp:46`, `:56-59` closes an open session at the boundary; `PIRSensor.cpp:15-22` zeroes `totalOccupiedSeconds` on a daily reset). Occupancy that is credited after that point, while the device is still awake, accrues in `closeOccupancySessionSafely()` (`State_Common.h:385-395`, the `set_totalOccupiedSeconds(newTotal)` write at `:391`) and counts into the next day's total.
   - **Field case:** Dev-09, v43, 15 Oct. Close report 13:59:59Z had `dailyoccupancy` 291. PIR activity at 14:01:55Z during a Singtel connect that kept the device awake until 14:22Z added 8 min, so the next-morning report (22:00:43Z) showed `dailyoccupancy` 8, not 0.
   - **Pre-existing:** the accrual in `State_Common.h` is unchanged from `ff1715c` to `71e346f` (v42 to v43). It doesn't hold v43. v42 simply had no after-close activity in its window.
   - **Not yet decided:** whether time after the boundary should be credited to the new day on purpose (it is the new day's occupancy) or whether the report path should split the session at the boundary. Step 0 is history first.

11. **A timer slot that falls inside a running connect attempt produces no report** (architect, 2026-10-16; found in the v43 Dev-14 soak, PR #106).
   - **Behavior:** a scheduled report is a stateless clock check, not a queued slot (`reportDueThisInterval()`, `State_Common.h:38-42`), and it runs only from Sleep and Idle (`State_Sleep.cpp:1766-1776`, `State_Idle.cpp:204`, `:217`). While the state machine is in Connect nothing checks it. When the connect times out (alert 31, radio off, `State_Connect.cpp:666-667`) the device sleeps to the next boundary (`State_Sleep.cpp:1075-1085`), so the slot is never revisited.
   - **Never built, not built-and-lost:** `publishData` is called only from `State_Report.cpp:69`, so no report exists for that slot. The queue library's eviction code was not read; no code that drops a queued report by slot was found.
   - **Field cases:** Dev-14 on v43, 16 Oct: Connect from 06:22:20Z, no 06:30Z report; the 07:00Z report was sent. Dev-14 on v42, 14 Oct: no 22:30Z report during the boot stall. No counts are lost (the occupancy total accrues regardless), only the slot's report.
   - **Not a v43 change.** Decide only if a missed slot matters to a customer; the cheap fix would be an overdue check on leaving Connect. Step 0 is history first.

12. **The failsafe's test-mode arms are unpinned** (architect, 2026-10-19; found by Codex's Stage 7 of P-2, WO-2026-10-19-001, PR #114).
   - **Gap:** nothing in the host suite pins the `CONNECTIVITY_FAILSAFE_TEST_MODE` arms of the connectivity failsafe (`src/power/ConnectivityFailsafe.cpp`, formerly `Generalized-Core-Counter.cpp:226–345` and `:2067–2246` at `3975e9c`). A 1 s change to the test-mode `jitterSec(stage)` arm passes all 8 related tests at `HEAD` (before the move: `connectivity_failsafe_open_hours`, `failsafe_cadence_rule`, `power_consumer_regression`, `connection_mode_downgrade`, `connectivity_failsafe_stage1_retired`, `connection_mode_downgrade_structural`, `clock_trust_standard_structural`, `battery_authority_commit_sites_structural`) and the P-2 move test.
   - **Not a P-2 defect:** the move is byte-identical and both builds are identical in size; the gap was there before.
   - **When:** pin both preprocessor arms (the timings, cadence and jitter constants, and the defer path) when the failsafe is next touched.
12a. **`BREADCRUMB_REPORT_LEDGER_DONE` is unpinned** (architect, 2026-10-19; found by Codex's Stage 7 of P-3, WO-2026-10-19-002, PR #116).
   - **Gap:** changing `setAppBreadcrumb(BREADCRUMB_REPORT_LEDGER_DONE)` in `publishData` (now `src/cloud/ReportPublisher.cpp:173`; one occurrence in `Generalized-Core-Counter.cpp` at `dd90842`) to another valid constant passes all 11 tests that anchor on the report path at `HEAD`, and the whole suite (86/86) after the move. The breadcrumb is forensic: it is what a watchdog reset reports as the last stage.
   - **Not a P-3 defect:** the move is byte-identical; the gap was there before.
   - **When:** pin the `BREADCRUMB_REPORT_*` sequence in `publishData` when the report path is next touched.
12b. **The AB1805 stale-alarm clear is unpinned** (architect, 2026-10-19; found by Codex's Stage 7 of P-4, WO-2026-10-19-003, PR #118).
   - **Gap:** inserting `return true;` at the top of `clearAb1805StaleAlarmInterrupts()` (now `src/time/HibernateEligibility.cpp:24–51`; formerly `State_Sleep.cpp:239–267`) bypasses the AB1805 status clear, the interrupt-mask clear and the timer-repeat disable, and the whole host suite still passes (86/86 at `HEAD` before the move, 87/87 after).
   - **Not a P-4 defect:** the move is byte-identical; the gap was there before. A future regression that leaves stale AB1805 alarm state uncleared before a Boron RTC-alarm hibernate would not be detected by the host suite.
   - **When:** pin the three register writes (and their order) when the hibernate path is next touched.
13. **Standard negative check for P-3 to P-6** (architect, 2026-10-19; from P-2 Stage 7). Every pure move's structural test must fail if the `retained` keyword appears in the new files (after stripping comments and string literals). P-2's `connectivity_failsafe_move_structural_test.py` is the pattern: Codex's probe (`retained uint8_t …Probe;` in the new `.cpp` and in the `.h`) must fail with a named message, and the Stage 6 dispatch and Stage 7 checks for P-3 to P-6 ask for it.

**Rollout and follow-ups** (not WOs).
- **v40 release step: repeat the 1c migration check.** Before releasing v40, read the resolved `sensor.type` for every device: the `default-settings` and every `device-settings` instance. Any value other than 1 would now be rejected at config apply, and a value already stored would stop that device counting after its next reboot. On 2026-10-08 the default was 1 and no device overrode it (WO-2026-10-08-002 Step 0 report, appendix).
- **Trail02 stays locked to v38** until the v40 fleet rollout, then joins it (architect, 2026-10-10).
- **Hibernate fleet-wide:** after v37's item A has a few days in the field. Trail02 woke on time on all three trial nights (4 Oct Codex report §C).
- **The Ubidots template:** add `"vc": "{{vc}}"` once every device runs v37.
- **v35's download case:** confirm at the next OTA to a v35+ device.
- **Alert 14's remaining uses:** review once older firmware has left the fleet (WO-2026-10-02-003, later cleanup).
- **Structural tests and `build-tmp/`:** `tests/thermal_coupling_structural_test.py:256` and `:264` scan the whole repo and skip only `.claude`, so a build copy of `src/` under `build-tmp/` makes them fail (Copilot and Claude Code, WO-2026-10-07-002). There is no shared scan helper; the other structural tests scan only `src/`. Skip `build-tmp` there too (a test change, not `src/`), then remove the temporary build-copy bullet from `AI_DEVELOPMENT_WORKFLOW.md` §2.

**Observations to watch** (no action yet).
- **PCKL3 restarted twice with reset reason 0,** 7 minutes after its v35 update (also seen on v25 on 28 Sep).
- **Dev-09 raised one stale alert 41** after its test setting was reverted: it applied its stale local copy before the reverted settings synced.
- **A missing occupancy start report after a restart** (three times). Check whether v37's item B changes it (4 Oct Codex report §B).
- **Duplicate deliveries:** with the fleet-ops agent (handover document, WO-2026-09-25-004). Moved here from the Phase 4 list.
- **A false alert 44 after a boot clock step** (Dev-14, PR #72 build, 2026-10-08). `ClockResync: sync advanced` at `0005171470`, then `GateFail: reason=ledger timeout=70000` and `raising alert 44` at `0005240897`–`0005240899`. The `LedgerSleepTimeout` line shows `pendingData=0 pendingStatus=0`, with `dataUpd=414217309 > dataSync=414210391` and `statusSync > statusUpd`. Log: `2026-10-07 17-12-25 Boron CDC Mode #1.log`. Cause not yet traced. **Not** the stale gate-timer cause fixed in WO-2026-10-07-004: that gate waited the full 70 s (`0005170895` → `0005240897`). The lead remains `dataUpd > dataSync` after the boot clock step.
  - **Related gate-timer gap (pre-existing):** the CONNECTED+open sleep abort (`State_Sleep.cpp:406-409`, from 2026-01 and 2026-06) leaves for Idle without resetting `cloudSyncStartMs`. If the mode in use becomes CONNECTED during a gate wait, a later sleep's gate starts with a stale timer and can time out at once, tearing down before its queued work drains. Found by WO-2026-10-07-004 Stage 7 round 2 (a CONCERN; deliberately not folded into that WO). The fix would be the same one line as WO-004's controller edit (`cloudSyncStartMs = 0;` before the transition). Not the Dev-14 alert-44 cause above.
- **A modem-off sleep loop after a restart** (Dev-14, v42, KEEP_ALIVE, 2026-10-15; architect, 2026-10-15). For 18.5 min after a restart at about 06:11:08Z, the device never slept.
  - **Source:** S3 `serialLog` for `e00fce688e592afaf23ac4fb`, 15 Oct 06:11:07–06:30:28Z (forwarded USB serial; timestamps are the forwarder's).
  - **The restart:** `06:11:07 0029423332 ConnSummary: fail elapsed=660002 last=CELLULAR_ACQUIRE … sig=na`. Then the serial port dropped, and the next line shows uptime restarted at `06:11:39 0000031407`. It was the connectivity failsafe: the queued status, published 11:43:20.824Z, reads `resetReason 140, resetReasonData 2, watchdogResetCount 126` (next bullet).
  - **The loop, 38 times, about every 30 s** (the first pass is `06:11:39`–`06:11:53`, uptime `0000031407`–`0000031431`; every later pass repeats it):
    - **First pass only:** the 30 s budget was missed and the modem was marked unstable: `MODEM_HEALTH: unstable reason=slow_teardown elapsed=30010` (`State_Sleep.cpp:112`). Standby was then suppressed: `MODEM_POLICY: standby temporarily disabled reason=unstable_modem` (`:124`).
    - **Gate blocked:** `SLEEP: pre-sleep gate blocked (cloud=0 radioOn=1 standby=0) - requesting modem off` (`:936`).
    - **Budget missed and alert 15 raised:** `SLEEP: disconnect/modem-off exceeded budget (30001 ms, cloud=0 radioOn=0 standby=0) - raising alert 15` (`:838`), then `Sleep->Error reason=sleep-disconnect-timeout`.
    - **The alert cleared again:** `Low-power mode: clearing alert 15 to avoid reset loop` (`State_Error.cpp:130`), then `Entering ERROR_STATE with alert=0, resetCount=1, resolution=0`.
    - **Back to sleep:** `Error->Idle reason=error-no-recovery` (`State_Error.cpp:150`), `Low-power idle: offline with 7 queued event(s) - sleeping` (`State_Idle.cpp:261`), then `Idle->Sleep`.
    - **How it ended:** the 38th budget miss was at `06:30:10 0001142210`. The scheduled report took over at `06:30:20` (`Idle->Report reason=report interval`), and its connection attempt failed at `06:41:11`. Normal 30 min sleeps followed: the next wake was at `07:11:14`, with uptime continuing.
  - **INF:** the budget line reports `radioOn=0`, but the non-standby gate waits for `Cellular.isOff()` (`State_Sleep.cpp:729`). The modem appears to have been in Device OS's in-between state, neither on nor off (the case WO-2026-10-04-001 item A2 describes), for the whole loop. That would keep `stillOn` true (`:836`). Unproved: no line logs `Cellular.isOff()` directly.
  - **Why it matters:**
    - in INTERMITTENT/KEEP_ALIVE, alert 15 is raised and cleared on every pass, so nothing escalates and nothing recovers the modem;
    - the device stays awake and drawing power until the next scheduled report;
    - each pass also asks for a full disconnect and radio-off again (`State_Error.cpp:124`).
  - **Same code in v43.**
    - **What Step 6 changed:** `State_Sleep.cpp` only from `:1091` on (the COUNTING defer and the wake-source setup), `State_Idle.cpp` only at `:252`, and nothing in `State_Error.cpp` (`git diff ff1715c 71e346f`).
    - **What it didn't:** the gate, the budget and the alert-15 lines cited here.
    - **Line numbers:** at main `71e346f`.
- **Firmware modem resets didn't recover registration; a power cycle did** (Dev-14, v42, KEEP_ALIVE, Singtel, 2026-10-15; architect, 2026-10-15).
  - **Sources:**
    - S3 `serialLog` for `e00fce688e592afaf23ac4fb`, 15 Oct 02:30–11:43:39Z (forwarded USB serial; the forwarder's timestamps);
    - S3 `status` for the same device, 15 Oct, published 11:43:20–11:43:38Z.
  - **The last good connection:** `02:30:09 0016165198 ConnSummary: ok elapsed=1418 … sig=80/25`.
  - **15 failed attempts in a row,** `03:41:07` to `11:14:17`. Each one was the same:
    - Device OS reset the modem at about 600 s (`[ncp.client] WARN: Resetting the modem due to the network registration timeout`, 15 times, 03:30–11:33Z);
    - then the 660 s budget ran out: `ConnSummary: fail elapsed=66000x last=CELLULAR_ACQUIRE cellMs=66000x … cloudRecoverStage=0 … sig=na`, with `q` rising to 16.
    - The first is `03:41:07 0020423078`; the last is `11:14:17 0018188559 … q=16`.
  - **A failsafe reset in the middle:** at about 06:11:08Z, after the 06:11:07 failure (uptime `0029423332`). Its queued status, published `11:43:20.824Z`, reads `resetReason 140, resetReasonData 2, appBreadcrumb 10, appBreadcrumbMs 29423342, watchdogResetCount 126`.
    - That is the connectivity failsafe (140/2), not a watchdog. `lastWatchdogBreadcrumb 28` / `lastWatchdogUptimeMs 1687188` are left over from an earlier watchdog, and the count is unchanged at 126.
    - **The reboot didn't recover registration either:** 9 more attempts failed after it, from 06:41:11 on.
  - **The power cycle and the recovery:**
    - **Chip power-cycled it at about 11:33Z** (an attempt starting `11:31:14` was cut off). The status published `11:43:36.744Z` reads `resetReason 40` (power down).
    - **Uptime restarted:** `11:33:41 0000004472 Using internal SIM card`.
    - **The first attempt after the power cycle registered at about 516 s:** `11:42:14 0000517819 CloudRecover: stage=1 action=particle-reconnect phaseElapsed=516526`, then `11:42:27 0000522351 ConnSummary: ok elapsed=520556 … cellMs=516526 … cloudMs=4030 … sig=78/33 q=18`, then `Failsafe: cleared reason=cloud-ok`.
    - **The v43 OTA followed at once.** The post-OTA boot registered in 6.8 s (`11:43:08 0000010749 ConnSummary: ok elapsed=9870 … cellMs=6812 … sig=80/33`).
  - **INF, unproved:** the modem or SIM was stuck in a state that a power-off clears and that neither Device OS's modem reset nor a `System.reset()` cleared (the failsafe reboot didn't help).
    - **Against it:** registration took 516 s even after the power cycle, so a network-side recovery at about 11:40Z can't be ruled out with this one case.
    - **What would help:** a second case, or a carrier-side check.
  - **Why it matters:** no firmware path short of a power cycle (for example a hard modem power-off via the PMIC or the modem's power pin) is known to clear this. That would be a recovery-plan item under Connectivity if it recurs.
  - **The connectivity-failsafe ladder** (architect, 2026-10-15; code at main `71e346f`, `Generalized-Core-Counter.cpp:2598–2685`):
    - **When it checks:** only in open hours (`:2599`); once the age since `max(openedAt, lastConnection)` reaches `CONNECTIVITY_FAILSAFE_STALE_SEC` = 3 h (`ConnectivityPolicy.h:114`); and never during a connect attempt still within budget (`:2648`).
    - **Stage 1 is retired** (`:2621–2625`). The first action is **stage 2**, a `System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE)`: reset 140, data 2, breadcrumb 10 (`:2671–2677`).
    - **Stage 3, an AB1805 deep power-down** (`ab1805.deepPowerDown()`, `:2681–2684`), follows only after `CONNECTIVITY_FAILSAFE_COOLDOWN_SEC` = 6 h (`ConnectivityPolicy.h:115`) plus a per-device, per-stage jitter of 0–30 min (`CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC`, `:116`; a hash of the device ID, `Generalized-Core-Counter.cpp:384–398`).
    - **Stage 2 and 3 are deferred** on battery when a downgrade is active or the tier is SURVIVAL (`:2654–2668`).
  - **On Dev-14, 15 Oct:**
    - **Stage 2 fired at about 06:11:08Z,** straight after the `06:11:07` failed attempt. The status published 11:43:20.824Z reads `resetReason 140, resetReasonData 2, appBreadcrumb 10`.
      - **INF:** the age base was the 03:00Z connection (the Particle API reads last heard 03:00:29Z), so the 3 h threshold passed at about 06:00Z during the attempt that started at about 05:55Z, and the stage fired when that attempt ended.
    - **Stage 2 didn't recover registration:** 9 more attempts failed, 06:41–11:14Z.
    - **Stage 3 would have come no earlier than 12:14:09Z:** 06:11:08Z + 6 h + Dev-14's stage-3 jitter of 181 s, computed from the hash at `:384–398`.
      - In practice it would have come at the first check after that, outside a connect attempt (so, with about 41 min cycles, by about 12:55Z), and only if Dev-14 was still in open hours and not deferred for battery.
    - **Chip's manual power cycle at about 11:33Z came first.** It was followed by registration at 11:42:27Z (see above), and `Failsafe: cleared reason=cloud-ok` at `11:42:28`.
  - **Queued check** (read-only, later): **the fleet recovery rate within 1 h** after stage-2 resets and after stage-3 deep power-downs.
    - **Source:** the S3 `status` history.
      - Stage 2 is `resetReason 140, resetReasonData 2`.
      - Stage 3 is identified from `failsafeStage 3` on the next status, together with its reset reason. Confirm which reason a deep power-down wake gives before counting.
    - **"Recovered":** a webhook report or a `status` with a fresh connection within 1 h of the reset.
    - **Per device and fleet-wide,** with the window stated.
  - **The decision it feeds** (no code now): **shorten the stage-3 delay, or skip stage 2.** On this evidence a power-off recovered registration and a reset didn't, but that is one case.
- **Inbound ledger config and INTERMITTENT:**
  - **Decided 2026-10-07 (Chip):** no per-connection dwell for inbound ledger syncs. The cost would be paid on every connect, and inbound config changes are rare.
  - **INTERMITTENT devices get one config wait per day:** the first connection after open hour, plus the boot connection. The gate holds a few seconds after outbound syncs, ending early on an input `onSync`.
  - **Consequence:** a config change may take up to a day to reach an INTERMITTENT device.
  - **Background:** Device OS 6.4.1 exposes no inbound-pending signal (WO-2026-10-07-005 evidence). The question has been asked on the Particle forum.
  - **Implementation is a later WO.**
- **Host tests that copy source:** `connection_mode_downgrade_test` (WO-2026-10-07-002) and `occupancy_report_by_mode_test` (WO-2026-10-07-004) extract code blocks from `src/` into the test rather than compiling the real files, so they can drift from the source. Switch them to compiling the real source the next time the test harness is touched.
- **Court3's daytime failsafe resets** (v36, 4 and 5 Oct; reset 140, data 2, `failsafeStage 2`): Court3 runs connectionMode 3 (KEEP_ALIVE, the product default). It reconnects through CONNECTING on every wake, which refreshes `lastConnection`. Its resets ended **real connectivity silences**; they were not healthy-device resets from the CONNECTED-online gap found in WO-2026-10-08-001 (Chip, 2026-10-08).
- **Failsafe flash churn at stage 0** (WO-2026-10-08-001 Step 0 addendum §5):
  - **Where:** at stage 0 the cooldown test (`Generalized-Core-Counter.cpp:2651`) doesn't apply. So while the low-battery block (`:2661-2664`) keeps firing, with age ≥ 3 h, Open, on battery with a downgrade or SURVIVAL, `persistConnectivityFailsafeState(..., now, false)` (`:2674`) runs `SystemConfig::flushNow()` (`:404-413`) on every loop pass.
  - **Bound:** about one flash write per second, since `setValue` dirties the file only when `now` changes. At stage ≥ 1 the 6 h cooldown bounds it.
  - **Not yet measured:** flash wear and how long the loop stays awake.
- **`reportingIntervalSec` truncation** (WO-2026-10-08-001 Step 0 addendum §4):
  - **The wrap:** `ConfigApply.cpp:265` accepts 300–86400 s, but the store is `uint16_t` (`MyPersistentData.h:137`). So 65536–86400 wraps; for example 86400 is stored as 20864.
  - **Rewritten on every apply:** the comparison at `ConfigApply.cpp:266` (`uint16_t` against `int`) never matches a wrapped value, so the value is rewritten on every config apply (INF).
  - **Ruling pending:** the wrap is on the 1b fix's input path, and the architect's ruling on capping the range at 65535 is pending. The `uint16_t intervalSec` locals in `State_Idle.cpp` and `State_Sleep.cpp` lose nothing, since the source is already `uint16_t`.

## Guardrails (`AI_DEVELOPMENT_WORKFLOW.md` §12)

1. **History first:** check `git log -S` before designing anything new. If the behavior existed before, restoring it is the default fix. For a restoration, Stage 7 verifies against "at least as good as the version restored", not an expanded fault model.
2. **A plain-language goal** at the top of every WO.
3. **A size budget in every dispatch.** Going over it means stopping and reporting.
4. **The two-round rule:** two rounds that add new mechanisms, or two rounds without VERIFIED, means stop and restate the goal with the user.
5. **Verify the binary, not the source,** when vendored libraries are involved. Local and cloud builds may use different library copies. Bench-test the build type the fleet will receive.
