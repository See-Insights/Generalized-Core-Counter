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
- **Duplicated power-source codes and PMIC fault masks** (WO-2026-09-29-002).
- **Boron versus M-SoM build selection** (WO-2026-09-29-002).
- **Connection-mode tug-of-war** (2026-10-03, found while explaining Trail02's 2-hour delivery spacing on v34):
  - `ConfigApply.cpp:445–455` restores keep-alive on every config apply;
  - `BatteryAuthorityCommand.cpp:80–84` re-applies intermittent in CONSERVING or worse;
  - the mode flips, and is persisted, on every connection.

  **Fix:** separate the configured mode from the battery downgrade. The effective mode is derived, and neither overwrites the other.
- **The failsafe's allowance for deliberate spacing.** Its only allowance today is the low-battery block (`Generalized-Core-Counter.cpp:2657–2672`), which works only because the tug-of-war above sets the low-battery flag. **Goal:** the failsafe counts only the time when the device was expected to connect.
- **Battery trust and tier.** The gauge is compared with a resting-voltage table (`kOcvKnots`, `BatteryHealth.cpp:19–32`) using a voltage sampled while awake, so healthy devices were marked untrusted and dropped to CRITICAL: Court3 at 82%, PCKL1 at 77% (the 4 Oct Codex report's ledger snapshot: 79.8% and 76.5%, both Untrusted/CRITICAL), and Dev-14 showing CRITICAL at 65%. Decide only after a week of v37's `vc` data (WO-2026-10-04-001 item D). If that isn't enough, add the fuller battery-decision snapshot proposed in the 4 Oct Codex report §D (raw and accepted SOC, vcell, charge state, radio state, sample age, source, resulting tier).
- **WO 3 (battery): which sensor modes a low tier downgrades.**
  - **Today's rule:** the connection-mode downgrade applies only to OCCUPANCY devices configured for KEEP_ALIVE (`PowerManager::downgradeActive()` from v39; the `:83` write on v38). A COUNTING device in a low tier stays in its configured mode.
  - **Decision for WO 3:** whether low tiers should downgrade every sensor mode.
  - **Live data, 2026-10-08:** the only device below HEALTHY is Trail02. It runs OCCUPANCY (`sensorMode` 1) in KEEP_ALIVE (`connectionMode` 3), from the product defaults, at SURVIVAL on v38, **so it is downgraded**.
  - **Its flag flips on v38.** `device-status` showed `battery.lowBatteryMode: true` at 03:00:16Z (the AWS fleet-ops copy) and `false` at 03:00:22Z (Particle's current version, written after that connection's config apply cleared the flag). That is the flip-flop WO-2026-10-07-002 fixed in v39.
  - **Consequence for checks:** on v38, a single ledger read can show either value. Fleet checks should judge a downgrade from tier and modes, not from the flag alone.
  - **WO 3 evidence (Trail02, `device-status` 2026-10-08 03:00Z):** SURVIVAL at **53.8% SOC marked Untrusted, with vcell 3.61 V** (`vcellState` Known). Also `power.profile: UsbBench` while `power.source: BATTERY`, a profile and source mismatch (see "Power-source misreads").
  - **WO 3 evidence: SOC against vcell divergence (Trail02, on v38, discharging).** From 2026-10-08 03:00Z to 2026-10-09 03:00Z, SOC fell from **53.8% to 2.2%** (Untrusted, then Suspect) while vcell only went from **3.61 V to 3.57 V**. The tier stayed SURVIVAL throughout. This is a case for the tier following voltage when SOC is untrusted.
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

**Rollout and follow-ups** (not WOs).
- **v40 release step: repeat the 1c migration check.** Before releasing v40, read the resolved `sensor.type` for every device: the `default-settings` and every `device-settings` instance. Any value other than 1 would now be rejected at config apply, and a value already stored would stop that device counting after its next reboot. On 2026-10-08 the default was 1 and no device overrode it (WO-2026-10-08-002 Step 0 report, appendix).
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
