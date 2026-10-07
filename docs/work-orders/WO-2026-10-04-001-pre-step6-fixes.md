# WO-2026-10-04-001: four small fixes before Step 6 (v37)

**Goal, in plain language:** the device never waits on the modem in its main loop or goes to sleep with the modem half off; a restart in the middle of an occupancy session doesn't lose that session's minutes; an on-time hibernate wake is reported as a success; and every report carries the battery's cell voltage.

**Status:** **Stopped at USER GATE 2.**

- **A, C, D:** VERIFIED in Stage 7 round 1.
- **B:** VERIFIED WITH NOTES in the round-2 re-check, after Claude Code's narrow edit.
- **The only note:** a documentation correction to the Known-limitation fact check, now applied.
- **Checks:** 65/65 tests; build 150844 / 1090 / 2196.

Nothing has been committed.

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`: Stage 5 decisions → Stage 6 (Copilot) → Stage 7 (Codex), stopping at USER GATE 2; §5 model routing; §12 guardrails (plain goal, a size budget per item with no compressed code, the two-round rule, facts checked against their source, budget versus actual in the closing record).

**Branch:** `wo/2026-10-04-001-pre-step6-fixes`, from `main` at `79abe84` (v36 merged, PR #60). Every file:line below was checked against `79abe84` on 2026-10-05; corrections to the opening dispatch are marked **(fact check)**.

**Version:** `v37-PreStep6Fixes`, product version 37. It includes v36.

**Spec source:** `docs/work-orders/2026-10-04-pre-step6-known-issues-codex-report.md` (Codex `gpt-6-astra`, high, read-only, 4 Oct), as cut down by Chip.

**Total budget:** about 40 net `src/` lines (nonblank, non-comment, including braces, declarations and includes). Any item over its own budget means stop, report, and ask Chip before going further.

## Record: what reset reason 60 is

Reset reason 60 is the **MCU watchdog, 60 s**: `AWAKE_WATCHDOG_TIMEOUT_MS = 60000UL` at `src/Generalized-Core-Counter.cpp:167`, set up at `:907–916` (`Watchdog.init(awakeWatchdogConfig)`). It is **not** the AB1805's 124 s watchdog, which resets the MCU through the reset pin and shows up as `PIN_RESET` (confirmed separately by `HibernateCycle::classifyWake()`, `src/time/HibernateCycle.cpp:63–73`). Device OS maps the MCU watchdog flag to reason 60 (`hal/src/nRF52840/core_hal.c`, Device OS 6.4.1).

## Items

### A. Nothing in the main loop waits on the modem. Budget ≤ 20

`Cellular.RSSI()` is a synchronous modem transaction: it takes the modem-client lock and runs AT commands (Device OS `cellular_hal.cpp:321`, `sara_ncp_client.cpp:776`). Today it runs during connection attempts and after timeouts, even when trace output is compiled out.

**(1) Read signal strength once, after the cloud connects.** The RSSI sites on `79abe84`, in `src/state/State_Connect.cpp`:

| Site | When it runs | v37 |
|---|---|---|
| `:390`: connect start | before `Particle.connect()`; the result is discarded (`(void)`) | **remove** |
| `:451` → helper `:75`: `ConnDiag`, every 30 s | during acquisition; the log line is compiled only with `ENABLE_CONNECT_TRACE`, the read is not | **remove** the call |
| `:519` → helper `:75`: `ConnSummary: ok` | after `Particle.connected()` | **keep**: the one read |
| `:627`: `Connect: ok` | after `Particle.connected()`, a second read | **remove**; log the values `:519` already read |
| `:693` → helper `:75`: `ConnSummary: fail` | on the timeout path | **remove** the call; the line logs `sig=na` (the branch v31 added) |

**(fact check)** The opening dispatch listed `:75, :390, :627`. `:75` is the helper `sampleConnectionSignal()`, which three call sites use (`:451`, `:519`, `:693`). Only the `:519` call survives. The WiFi branches (`:82`, `:637`) follow the same rule. `SensorManager::getSignalStrength()` (`src/sensors/SensorManager.cpp:1250`) also calls `Cellular.RSSI()`, but nothing calls it (declared at `SensorManager.h:196`, no caller in `src/`); the linker drops it. Leave it.

**(2) Call `System.sleep()` only once `Cellular.isOff()` is true** (non-standby sleep). The sleep gate in `src/state/State_Sleep.cpp` uses `Connectivity::isRadioPoweredOn()` (`src/power/Connectivity.h:12`, which is `Cellular.isOn()`) as "the modem is on", so "not on" passes as "off". Device OS has in-between states where both `isOn()` and `isOff()` are false (`system_network_manager.cpp:1045`), and `System.sleep()` then waits up to 120 s for the modem to turn off (`system_sleep.cpp:79`). That's longer than the 60 s MCU watchdog. On a cellular build, change the three gate decisions to `!Cellular.isOff()`:

- `:713`, `stillOn` for non-standby;
- `:811`, "modem-off complete";
- `:893`, `sleepPreconditionsSatisfied()` for non-standby.

Use the existing teardown wait and its timeout (`:845–853` and `:940–960`: alert 15 → `ERROR_STATE`). Add no new timer, state or breadcrumb. **Standby sleep is unchanged**: it keeps the modem on by design (`ulpConfig.network(... INACTIVE_STANDBY)`, `:1359`). That matches Codex's "the existing bounded non-standby teardown gate". The log-only uses (`:691`, `:1137`, `:1230`, `:1253`) and the `:753` early request are unchanged. If the modem is in between states when the gate is reached, `:893`'s "cloud=0 radioOn=1" branch already requests power-off and waits.

All sleeps go through this gate: hibernate (`:1152`), ULP (`:1405`) and the STOP fallbacks (`:1457`, `:1472`).

**History:** `ef5be17` (2026-03-15) added the modem-off precondition. v37 corrects its definition of "off"; it adds no new mechanism.

### B. A restart during a session doesn't lose the session's minutes. Budget ≤ 15

Persistence already exists in `/usr/current.dat` (`src/MyPersistentData.h:694–697`): `occupied`, `occupancyStartTime` (epoch), `lastOccupancyEvent` (**millis**), `totalOccupiedSeconds`. Session start is recorded at `src/state/State_Modes.cpp:65–66` and on PIR wake at `State_Sleep.cpp:1689–1707`.

**(1) The debounce uses the previous boot's `millis()`.** `State_Idle.cpp:53–58` and `State_Modes.cpp:134–140` compute `millis() - lastOccupancyEvent`, and after a restart that value is the previous boot's `millis()`. **Fix:** at boot, clear the persisted `lastOccupancyEvent` to 0, so the existing `lastEvent == 0` re-arm (`State_Idle.cpp:54`, `State_Modes.cpp:135`) starts the debounce from this boot.

**(2) The close step clears the session when the clock isn't trusted.** `closeOccupancySessionSafely()` (`src/state/State_Common.h:301–371`) treats `!Clock::isTrusted()` as invalid, but still clears `occupied` and `occupancyStartTime` (`:368–369`). **Fix:** when the clock isn't trusted, keep the session open, credit nothing, and re-arm the debounce (`set_lastOccupancyEvent(millis())`) so the close is retried one debounce later. A close that didn't happen must not be signaled as an unoccupied transition: no repeated `REPORTING_STATE` transition or `OccAnom` line on every pass.

**(3) Credit a session that was open across a restart.**

- **(fact check)** The opening dispatch said "on boot, if a session was open and the clock is trusted". The clock is **never** trusted during `setup()`: `isClockTrusted()` (`src/time/Clock.cpp:477–479`) needs a cloud time sync observed during the current boot (`observedTimeSyncedLastMs()`). **Decision B-timing (Chip, 2026-10-05):** the credit runs at the **first trusted close** of that session during the boot.
- **Mechanism, no new persisted field:** a RAM-only flag (not `retained`, not persisted) is set in `setup()` when `occupied` is true at boot. While it's set, `closeOccupancySessionSafely()` limits `closeAt` to `min(closeAt, bootEpoch, anchor + debounce)` and then clears the flag.
  - `bootEpoch = Time.now() − millis()/1000` at the time of the close. That is valid within one boot: Device OS adds slept time back into `millis()` after STOP/ULP sleep (`hal/src/nRF52840/sleep_hal.cpp:1170–1181`). A hibernate is a new boot, and the flag is set again.
  - `debounce` is the same value the debounce check uses (`sensorSetting1`, else `Config::occupancyDebounceMsForRuntime()`).
- **The anchor. (fact check)** `current.dat` has no epoch later than the session start in occupancy mode: `lastCountTime` is written only in counting mode (`State_Modes.cpp:37`, `State_Sleep.cpp:1662`), and `lastAlertTime` is unrelated. **Decision B-anchor (Chip, 2026-10-05):** `anchor = max(occupancyStartTime, sysStatus lastReport)`. `lastReport` is set on every report (`src/state/State_Report.cpp:79`), so a value later than the start lies inside a session that was still open at the restart. This is existing storage (`/usr/sysStatus.dat`).
- **Over-credit is bounded:** the session was open at `anchor`, and the credit ends no later than `anchor + debounce`, so it can't over-credit by more than one debounce period, however long the device was off.
- **Known limitations, accepted:**
  - a session longer than one debounce period since the last report is under-credited by the rest of that stretch;
  - motion after the restart, before the first trusted close, is folded into the capped close, and a later PIR starts a new session;
  - the daily-boundary close (`State_Report.cpp:59`) is capped the same way.

Stop and report if this needs a new persisted field.

**Known limitation (Chip, 2026-10-05):** if the clock loses trust partway through a boot (24 h after the last sync), a session that started during that boot stays open until trust returns and can be over-credited. Bounded in practice by the failsafe's 3-hour reset during open hours, which goes through the capped boot path. Revisit with the Clock-owner work (trusting the AB1805 at boot).

- **(fact check, Claude Code, 2026-10-05)** The 3-hour reset is the **connectivity failsafe**: stage 2 calls `System.reset()` when there has been no connection for 3 h during open hours (`CONNECTIVITY_FAILSAFE_STALE_SEC`, `ConnectivityPolicy.h:114`; `Generalized-Core-Counter.cpp:2617–2667`).
- It acts only when `Clock::openness() == Open` (`:2611`), which needs a trusted clock and valid configuration (`Clock.cpp:567`). Trust lasts 24 h after the last sync (`ClockTrust.h:42`). The 3-hour age starts at the later of `lastConnection` and today's opening, not at the last sync. So a reset before trust expires is likely but **conditional on all of the failsafe's gates** (corrected per Stage 7 round 2, check 9).
- It doesn't hold when the failsafe defers or stops (`Generalized-Core-Counter.cpp:2562` onward):
  - the low-battery block (`:2641–2656`: low-battery mode or SURVIVAL tier, without external power);
  - stage 3 already reached (`:2622`);
  - closed or unknown openness (an untrusted clock or invalid configuration);
  - DISCONNECTED mode, a firmware update in progress, or updates pending;
  - invalid time, no `lastConnection`, or the 3-hour age not yet reached;
  - recovery cooldown or jitter, or a connection attempt still within its budget.
- The alert-40 webhook reset (`State_Error.cpp:71`) is not this bound: it also needs a trusted clock (`:58`).


### C. An on-time hibernate wake is reported as a success. Budget ≤ 3

- The library labels any wake `DEEP_POWER_DOWN` while `REG_SLEEP_CTRL_SLST` is set. That check comes before the alarm flag, and the library never clears SLST (`lib/AB1805_RK/src/AB1805_RK.cpp:186`). The app doesn't either.
- So after one `ab1805.deepPowerDown()` (Dev-09's failsafe stage 3 on 1 Oct at 12:43:05Z), every later wake carries that label, **button wakes included**.
- The gate accepts only `ALARM`: `in.wakeReasonIsAlarm = (wakeReason == AB1805::WakeReason::ALARM);` at **`src/time/HibernateCycle.cpp:96`**. **(fact check)** Codex cited `:94`, which is the `GateInputs` line just above. Anything else fails the gate (`:105`), and `HibernateWakeDiagnostics.h:134–138` zeroes `actual`/`err`.

**Decision C-rule (Chip, 2026-10-05):** also accept `DEEP_POWER_DOWN`, but only when the RTC shows the wake **0 to +60 s** after the requested time: `rtcReadOk`, and `rtcBefore + requested ≤ rtcAtWake ≤ rtcBefore + requested + 60`. With this rule:
- an early wake, such as a button press, still fails;
- a late wake still fails;
- a wake with any other label still fails;
- `ALARM` handling is unchanged.

**(fact check)** The gate has no lateness bound today, so a late `ALARM` wake already reports `ok`. That's out of scope here; it's on the recovery-plan backlog. Trail02's `UNKNOWN` wakes still fail by design.

**Disagreement preserved:** Codex recommended not whitelisting `DEEP_POWER_DOWN`/`UNKNOWN` and reporting wake timing independently of the label (25–45 lines). Chip chose the narrower on-time window.

### D. Each report shows the cell voltage. Budget ≤ 4

Add `"vc"` (cell voltage, volts, `%.2f`, an **unquoted JSON number**) to both report formats in `publishData()` (`src/Generalized-Core-Counter.cpp:1952`): occupancy (`:2000–2001`) and counting (`:2018–2019`). Source: `SensorManager::cachedBatteryVoltage(float&)` (`src/sensors/SensorManager.cpp:248`). It writes the value only for a plausible sample (above 2.5 V, below 5 V, not NaN). Otherwise `vc` is `0.00`, the same fallback `battery` uses (`:1980–1984`). No NaN can reach the payload.

**Buffer:** `char data[256]` (`:1960`). Worst case by field type (uint32 `%lu` fields at 10 digits; `battery` ≤ `100.00`; `temp` taken as ≥ `-100.00`; `key1` `Disconnected`): occupancy **237 → 247** bytes, counting **224 → 234** (`,"vc":4.99` adds 10). Both fit with the NUL. Stage 6 and Stage 7 recompute these independently.

## Not in scope

- Adding breadcrumbs, a supervisor, or a new timer or state (A).
- AB1805 RAM heartbeat or any new persisted field (B).
- Clearing SLST, a lateness bound for `ALARM`, or timing reports independent of the label (C).
- Battery trust or tier changes (D is data only).
- Settings or webhook changes. The Ubidots template gets `"vc": "{{vc}}"` only after every device runs v37.

## Tests (outside the budget)

These tests pin code that v37 touches. Update one only where it pins changed text, and report each change:
- `tests/connection_signal_validity_test.sh`
- `tests/modem_teardown_confirmation_logging_test.py`
- `tests/hibernate_wake_diagnostics_test.sh` (`:125–126` pins the `:96` line) and `.cpp`
- `tests/report_payload_fields_test.py`
- `tests/hourly_while_occupied_test.sh`
- `tests/daily_cleanup_boundary_test.py`
- `tests/clock_trust_standard_structural_test.py`
- `tests/awake_cycle_counters_test.sh`
- `tests/nightly_heap_guard_flush_test.sh`

The WITH_ACK (`publish_with_ack_structural_test.py`), sleep-configuration (`sleep_config_ownership_structural_test.py`) and ledger (`ledger_no_retry_test`) tests must stay green and unchanged.

## Acceptance (Stage 7, narrow; each item against its goal)

- **A:**
  - no `Cellular.RSSI()` (or `WiFi.RSSI()`) call is reachable before `Particle.connected()` or on a timeout path (a structural test);
  - `System.sleep()` is never reached on the non-standby path with `Cellular.isOff()` false (a host check covering `isOn=false, isOff=false`);
  - mutations for both.
- **B:**
  - a restart mid-session credits up to the boot time, capped at `anchor + debounce`;
  - an untrusted clock leaves the session open, credits nothing, and causes no report storm;
  - the debounce no longer uses the previous boot's `millis()`;
  - a host check of a long power-off shows over-credit ≤ one debounce period;
  - mutations.
- **C:**
  - an on-time wake labeled `DEEP_POWER_DOWN` reports `ok` with real `actual`/`err`;
  - a late wake, an early wake (button) or a wrong label still reports `fail`;
  - mutations.
- **D:** `vc` is a JSON number; no new key is quoted; the worst-case payload fits in 256 bytes.
- **Everywhere:**
  - the suite passes (sh via zsh, py via python3; v36: 62/62);
  - a clean local Boron release build: text/data/bss against v36's 150740 / 1090 / 2180; `strings` shows `v37-PreStep6Fixes`, product 37;
  - budget versus actual per item.

## Bench (Chip, after USER GATE 2)

- **A:** Dev-14, which has had several sleep-stage watchdog resets a night: fewer or none over two nights. In serial, signal is logged only after a successful connection.
- **B:** start a session on Dev-09, reset it mid-session (`particle usb reset`), and check that the next close shows the minutes up to the reset, capped at `anchor + debounce`.
- **C:** the morning `hibernate_wake` events on Dev-09 and Dev-14 report success.
- **D:** `vc` arrives as a number in AWS.

### Bench results

- **2026-10-05, about 07:10Z: OTA lock.** Chip locked Dev-09 (`e00fce68399ee6244a963935`) and Dev-14 (`e00fce688e592afaf23ac4fb`) to product version 37. They update over the air at their next connection. The release image is `v37-PreStep6Fixes-boron-6.4.1.bin`, built from PR #62's head `85bd316`: SHA-256 `fd376d8b…2bcad7`, 151,938 bytes, 150844 / 1090 / 2196.
- **Particle API at 07:11Z:** both devices online and still running firmware 36, with target and lock both at 37. Dev-09 was last heard at 07:00:44Z, Dev-14 at 07:07:30Z.
- **Particle API at 08:04Z: both devices on v37.** Dev-14 and Dev-09 each report `firmware_version` 37 and are online; no update target remains pending. Dev-14 was last heard at 08:03:06Z, Dev-09 at 08:02:28Z. The A/C bench nights start tonight (SGT).
- **D, first look, about 08:15Z: PENDING. No report generated by v37 has arrived yet.** (Claude Code, read-only, from the raw S3 objects under `particle-events/2026-10-05/`.)
  - **The two reports labeled firmware 37 have no `vc`:** Dev-09's published at 08:02:14Z, and Dev-14's at 08:02:31Z and 08:02:32Z.
  - **v36 generated them, before the update:**
    - their payload `timestamp` is 08:00:04Z, from the 08:00 wake;
    - each device's boot `status` (reset reason 70, an OTA update) published right after, at 08:02:15Z (Dev-09) and 08:02:34Z (Dev-14);
    - that `status` reports `v37-PreStep6Fixes`.
  - **So:** the reports were queued under v36, and the device sent them after rebooting into v37. Particle stamps `fw_version` from the firmware running at publish time.
  - **Both devices run `v37-PreStep6Fixes`,** confirmed from the device's own status.
  - **Next check:** the first report v37 itself generates, expected at the 09:00Z wake.
- **D, 2026-10-05 at 09:24Z: PASS.** Every report generated by v37 carries `vc` as an unquoted JSON number, and `key1` is still the only string-valued field. All payloads are under 256 bytes. (Claude Code, read-only, raw S3 objects.)

| Device | Report generated → published | `vc` | Battery | Payload |
|---|---|---|---|---|
| Dev-09 | 09:00:02Z → 09:00:13Z | `4.03` | 80.42 | 195 bytes |
| Dev-14 | 08:30:02Z → 09:06:50Z | `3.77` | 50.78 | 199 bytes |
| Dev-14 | 09:00:01Z → 09:06:52Z | `3.81` | 50.78 | 199 bytes |

The Ubidots template change (`"vc": "{{vc}}"`) waits until every device runs v37.

- **B, Dev-09, reset-button test, 2026-10-05: PASS, at the report's one-minute resolution.** (Claude Code, read-only: serial forwarder and raw S3 objects.)
  - **Before the reset:**
    - session opened at 11:30:09Z (`Occ: state=1 reason=pir-wake led=300s`, so the debounce is 300 s);
    - occupancy report at 11:30:18Z (`Report: occ=1 totalMin=10`).
  - **The reset:** Chip pressed it at about 11:41:18Z. Boot was at 11:41:17Z, derived from `millis` 31395 at 11:41:48Z. Status reset reason 20 (pin reset).
  - **Clock:** untrusted until a cloud time sync at 12:00:30Z (`ClockResync: sync advanced`). The 12:00Z report still showed `occ=1 dailyoccupancy=10`, so the session stayed open with nothing credited, as specified.
  - **The close:** at the 12:09Z wake (the first debounce wake after trust). The report stamped 12:09:43Z shows **`occ=0 dailyoccupancy=15`**.
  - **Expected:** anchor `max(11:30:09, 11:30:18)` + 300 s = 11:35:18Z, earlier than the 11:41:17Z boot, so about 309 s credited: 10 → 15 minutes. ✓
  - **Without the cap** (credit through to the 12:09 close) it would have shown about 49. v36 would have thrown the session away and stayed at 10.
  - **Not captured:** the exact `Occ: state=0 … session=` line. The close ran before the serial forwarder reattached at 12:09:46Z.
- **A, observed on Dev-09 after the reset (the gate working as designed):**
  - **11:41:30Z:** `[ncp.client] ERROR: Failed to power off`, 13 s after boot.
  - **11:41:48Z:** the gate waited its 30 s budget with the modem neither on nor off (`radioOn=0`), then raised alert 15 (`disconnect/modem-off exceeded budget`) and marked the modem unstable, which disabled standby.
  - **Then:** Error → Idle (`error-no-recovery`; the existing alert 19 kept precedence), then Idle → Sleep. The gate requested modem-off again, and the device slept normally (next serial at the 12:00Z wake).
  - **Why it matters:** this is the in-between state item A targets. v36 would have called `System.sleep()` here and risked the 60 s MCU watchdog.
- **A, observed on Dev-14:**
  - **12:08–12:13Z:** a failed 300 s attempt (DNS `-170`, cloud recovery stage 2 exhausted).
  - **The failure summary:** `ConnSummary: fail … sig=na`. No signal read on the timeout path. ✓
  - **Signal elsewhere:** on both devices, signal was logged only on `ConnSummary: ok`/`Connect: ok`.
- **B verdict (Chip, 2026-10-05): PASS as designed.** Example: an 11-minute session interrupted by a reset was credited 5 minutes (cap = last report + debounce). Worst case is under an hour of undercount with hourly occupied reports. Full recovery deferred to the Clock-owner work (a last-alive record in the AB1805's RAM).
- **A verdict on the Dev-09 case (Chip, 2026-10-05):** a textbook example. The modem got stuck neither on nor off right after the reset, and v37 waited its 30 s, raised alert 15, and slept normally, where v36 would have called `System.sleep()` into a stuck modem and risked the 60 s watchdog. That's direct evidence that A works, ahead of the two-night count.
- **Still open:**
  - B: Dev-14's close, at its next successful report;
  - C: tomorrow morning's `hibernate_wake` on both devices;
  - A: the two nights of watchdog counts on Dev-14.
- **Dev-14 power, context for the A and C nights (not a v37 issue):**
  - **Symptom:** Dev-14 hadn't charged since 3 Oct.
    - Serial `ChargeDiag`: 2 Oct reached `chg=DONE` (SOC 68 → 77%); 3 Oct had one `FAST` reading, then only `OFF`.
    - SOC fell 77% → 47.7% by 12:03Z on 5 Oct (cell 3.756 V, so the SOC is genuinely low).
    - Power-good flapped 0/1 every day since at least 30 Sep. The source reads were mostly BATTERY, mixed with VIN/USB_HOST/USB_ADAPTER.
    - The charge LED flashed rapidly. The charger fault register read `0x00`.
  - **Not firmware:** Dev-09 ran the same firmware (v35/v36/v37) over the same days at `src=VIN pg=1 chg=DONE`.
  - **Fix (Chip, about 13:15Z on 5 Oct):** swapped the USB cable. The charge LED is now solid amber (charging).
  - **Bench context:** Dev-14 entered the A/C nights in the CRITICAL tier (INTERMITTENT mode). The first `ChargeDiag` after the swap is to be confirmed at its next wake.
- **Dev-14 after the cable swap, watched 13:10–14:10Z (Claude Code):**
  - **Cloud:** not reached since 12:03:57Z (Particle `last_heard`).
    - 13:25Z wake: a 660 s attempt failed in `CELLULAR_ACQUIRE` (a modem reset on the registration timeout).
    - The 14:00Z attempt (the 22:00 SGT close) was still registering at 14:10Z.
  - **Charging:** no `ChargeDiag` line since the swap. It isn't logged at the points a failed attempt passes through.
    - **To check:** the SOC and charge state at the first successful connection (tomorrow morning at the latest), against 47.7% at 12:03Z.
  - **B on Dev-14: inconclusive.** At 13:30:17Z it logged `Occ: state=0 reason=debounce session=300s total=1766s`.
    - 300 s fits the cap. It equally fits a single trigger at the 13:25Z wake plus the 300 s debounce.
    - Serial was missing from 12:14 to 13:24Z, so it can't be shown which session closed where. Dev-09 is the B evidence.
- **Bench check at 2026-10-05 23:01Z / 6 Oct 07:01 SGT (Claude Code, `claude-opus-5-5`, read-only).** AWS was unavailable: the `particle-admin` SSO token had expired. Particle's API and ledgers were used instead.
  - **Dev-14 charging after the cable swap: PASS.**
    - **Source:** `device-status` ledger, updated 22:09:31Z: `chargeState DONE`, `soc 69.8`, `vcell 4.02`, `power.source USB_HOST`, `socTrust Trusted`, tier CONSERVING.
    - **Before the swap:** 47.7% and 3.756 V (12:03Z `ChargeDiag`), so it charged fully overnight.
    - **Not checked:** `pg` isn't in the ledger. It needs the serial log (AWS).
  - **D: PASS for the 5 Oct v37 reports; later reports PENDING.**
    - **Evidence:** every report generated by v37 through 12:10Z carried `vc` unquoted, 3.75–4.03 V (Dev-09 at 09:00, 12:00 and 12:09Z; Dev-14 from 08:30Z on). They're in the tables above.
    - **Still to check:** reports after 12:10Z (both devices) and this morning's. They need AWS.
  - **B on Dev-14: still inconclusive.** Nothing new without AWS.
    - Dev-14's `device-data` ledger at 22:09:26Z shows `occupied false`, `totalOccupiedSec 0`: the daily reset after the 22:00 SGT close.
  - **Morning wake (context for C and the closes):** both devices restarted from hibernate (`startup.reason power-management`, `resetCount 0`).
    - Dev-14 at 22:00:07Z (06:00:07 SGT); clock synced 22:09:22Z.
    - Dev-09 at 22:00:42Z (06:00:42 SGT); clock synced 22:00:54Z.
    - Dev-14 was last heard at 22:10:02Z, after a 561 s connection (`connection.elapsedMs 561269`).
    - C's on-time margin and the two closes need the `hibernate_wake` events and reports in AWS.
- **B, Dev-14: superseded by the entry above (inconclusive).** No successful report since that failure, so its close's credit isn't visible yet. Its 12:00:16Z report showed `occ=1 resets=2`. The serial forwarder didn't capture whether that session started before or after the reset.

### Bench results, morning of 6 Oct SGT (Claude Code, `claude-opus-5-5`, read-only; AWS archive pulled 23:10Z on 5 Oct)

| Item | Result | Evidence (device, UTC, source) |
|---|---|---|
| **C: hibernate wake** | **PASS** | Dev-09 `hibernate_wake` 22:00:56Z: `result ok`, `wakeReason DEEP_POWER_DOWN`, req 28798 s, actual 28799 s, **err +1 s**. Before v37, this label reported `fail`. Dev-14 22:09:35Z: `result ok`, `wakeReason ALARM`, req 28140 s, actual 28141 s, **err +1 s**. |
| **Closes, 22:00 SGT on 5 Oct** | **PASS** | Dev-09 close report stamped 13:59:59Z (`dailyoccupancy 36`), published 14:00:21Z, then the morning report at 22:00:42Z with 0. Dev-14 close stamped 13:59:59Z (29) and morning report 22:00:07Z with 0. Dev-14's close was delivered late, at 22:09:33Z: its 14:00Z connection failed (poor coverage), but the stamp is on time. |
| **D: `vc`** | **PASS** | All 35 v37-generated reports (Dev-09 19, Dev-14 16) carry `vc` unquoted, 3.75–4.11 V. `key1` is the only string field. The only reports without `vc` are the three v36-generated ones delivered after the OTA (08:02Z). |
| **A: signal only after connecting** | **PASS** | Serial since 08:02:30Z. Dev-09: `sig=` values only on `ConnSummary: ok` (18) and `Connect: ok` (15). Dev-14: values only on `ConnSummary: ok` (4) and `Connect: ok` (3); every `ConnSummary: fail` (4) logs `sig=na`. |
| **A: Dev-14 sleep-stage watchdog resets** | **night 1: 0; PENDING night 2** | No `watchdog` event and no reason-60 `status` on 5 Oct since v37 (status resets: 70 OTA 08:02, 20 pin 12:03, 30 hibernate wake 22:09). Baseline below. |
| **B on Dev-14** | **NOT EXERCISED** (inference) | The reboot's first trusted close (report stamped 12:08:35Z) took `dailyoccupancy` from 10 to 19, about 480–600 s, more than the 300 s cap a session open across the restart would get. So that session began after the reboot (occupancy report 12:00:16Z). The 13:30:17Z `session=300s` close was the fresh 13:25:16Z session. Dev-09 is B's evidence. |

**A baseline: Dev-14 reason-60, breadcrumb-28 (`stage sleep`) watchdog events, by UTC day published.** Events publish at the next connection, so the day is approximate.

| Day | 29 Sep | 30 Sep | 1 Oct | 2 Oct | 3 Oct | 4 Oct | 5 Oct (v37 from 08:02) |
|---|---|---|---|---|---|---|---|
| Sleep-stage (bc 28) | 1 | 3 | 0 | 9 | 2 | 1 | **0** |
| Other reason-60 | — | — | — | bc 4 ×1, bc 18 ×4 | — | bc 4 ×1 | 0 |

Power affected the baseline. Power-good flapped on every day from 30 Sep. 3–4 Oct are the under-powered days (no charging after 2 Oct, faulty cable). The cable was replaced at about 13:15Z on 5 Oct.

**Dev-14 charging after the cable swap: PASS.**
- 22:09:26Z `ChargeDiag: chg=DONE(3) pg=1 vcell=4.021 soc=69.8 src=USB_HOST`.
- 22:30:09Z `chg=FAST(2) pg=1 vcell=4.115 soc=70.0 src=USB_ADAPTER`.
- Reported battery 46.5% (13:00Z) → 53.8% (13:59:59Z close) → 65.4% (22:00:07Z wake).

**Dev-14 update, 23:14Z on 5 Oct (07:14 SGT on 6 Oct; Claude Code, read-only):**
- **Connections:** two full 660 s attempts failed in `CELLULAR_ACQUIRE` (ended 22:41:05Z and 23:09:39Z), then it connected at 23:12:01Z through cloud-recovery stage 1.
- **No resets:** no reset, `watchdog` or `status` event during the episode. The 2 Oct comparison is corrected in the entry below.
- **Signal:** both failures logged `sig=na`; the success logged `sig=83/29`.
- **Charging:** 23:12:03Z `ChargeDiag: chg=DONE(3) pg=1 vcell=4.021 soc=75.5 src=USB_HOST`. The tier is back to HEALTHY, so keep-alive mode is restored (`Sleep: … mode=IKA tier=H`).
- **Reports:** three queued reports were delivered, all with numeric `vc` (4.11, 4.02, 4.02 V); battery 69.95 → 73.07 → 75.2%.
- **Occupancy:** a 548 s session closed at 23:07:47Z, and a new one opened at 23:08:02Z (`dailyoccupancy 9`).

**A, bench record (Claude Code, 6 Oct about 23:30Z; every figure checked against Dev-14's and Dev-09's serial and S3 events).**
- **Sleep stage (reason 60, breadcrumb 28), per night on v37, against the baseline above:** night 1 (5→6 Oct) = **0**. Night 2: PENDING.
- **Connection stage (criterion):** zero connection-stage watchdog resets (reason 60, breadcrumb 18) across all failed-connection episodes on v37 devices during the bench, with the number of episodes and failed attempts recorded. A period with no failed connections doesn't count toward this.
  - **So far: 2 episodes, 5 failed attempts, 0 resets of any kind** (Dev-14). Dev-09 has had no failed connections (18 `ConnSummary: ok`), so it doesn't count.
  - **Episode 1, Dev-14:** failed at 12:13:37Z (300 s budget), 13:36:17Z and 14:11:05Z (660 s, the 22:00 SGT close; then the hibernate night). Each failure was `CELLULAR_ACQUIRE` with `sig=na`.
    - **Connected:** 22:09:26Z (561 s, `sig=67/6`). `chg=DONE vcell=4.021 soc=69.8`; tier CONSERVING (`device-status` ledger, 22:09:31Z); mode → `INTERMITTENT_KEEP_ALIVE`.
    - **Reports delivered:** 7 queued reports, `vc` 3.79–4.11.
  - **Episode 2, Dev-14:** started 22:30:19Z; failed at 22:41:05Z and 23:09:39Z (660 s each, `sig=na`).
    - **Connected:** 23:12:02Z (`sig=83/29`), `chg=DONE vcell=4.021 soc=75.5`, then `mode=IKA tier=H`.
    - **Reports delivered:** 3, `vc` 4.11, 4.02, 4.02.
    - **Timing:** about 42 min end to end, 22 min of it acquiring.
  - **No reset during either episode.** The only resets on v37 were 70 (OTA), 20 (the pin reset) and 30 (hibernate wakes).
  - **This supports A but doesn't confirm it.**
- **No comparable connection-stage baseline:**
  - Dev-14's 2 Oct breadcrumb-18 events ran v31/v33, not v36, and are likely two resets published four times.
  - The v35/v36 days (3–4 Oct) have no breadcrumb-18 resets, but their failed-connection counts weren't measured.

### v37 acceptance, 6 Oct 23:5xZ (7 Oct about 07:50 SGT; Claude Code, `claude-opus-5-5`, read-only: S3 archive 1–7 Oct, serial, Particle API and ledgers)

| Criterion | Result | Evidence (device, UTC, source) |
|---|---|---|
| **A, sleep stage:** Dev-14 reason 60 + breadcrumb 28, per night (pass: 0–1) | **PASS: night 1 = 0, night 2 = 0** | No `watchdog` event, and no reason-60 `status`, since v37. Night 2's only reset is 30 (the hibernate wake, 22:01Z on 6 Oct). Baseline: 29 Sep 1, 30 Sep 3, 1 Oct 0, 2 Oct 9, 3 Oct 2, 4 Oct 1. |
| **A, connection stage:** breadcrumb 18 across failed connections (pass: none) | **PASS** | New since the last record: **1 episode, 1 failed attempt** (Dev-14, 23:17–23:28:32Z on 5 Oct, `CELLULAR_ACQUIRE`, `sig=na`), connected 23:30:14Z, no reset. Since v37 in total: 3 episodes, 6 failed attempts, **0** breadcrumb-18 resets. Dev-09: 24 connections, 0 failures. |
| **A, gate** (observed) | as designed | Dev-14 13:11:56Z on 6 Oct: `modem-off exceeded budget (30002 ms … radioOn=1)`, alert 15 raised, `ERROR_STATE` with no recovery action, then sleep; no reset. Alert 19 masks the 15; that's v38's fix. |
| **C:** bench morning `hibernate_wake` | **PASS** | Dev-09 22:01Z on 6 Oct: `ok`, `DEEP_POWER_DOWN`, req 28798 s, actual 28799 s (**+1 s**). Dev-14 22:00Z: `ok`, `ALARM`, 28798 / 28799 (**+1 s**). The night before was the same on both (+1 s). |
| **Soak move to v37** | — | Trail02 at 00:00Z on 6 Oct (status reason 70); PCKL1 and PCKL2 at 00:01Z on 6 Oct (reason 70). |
| **Soak (a) nothing lost** | **PASS** | Every hourly stamp is present on all three. Trail02 *delivers* in 4-hour batches; that's the tier backoff, the same under v36 (4–5 Oct). |
| **Soak (b) cyc − slp = 1** | **PASS** | 0 exceptions (Trail02 14 reports, PCKL1 39, PCKL2 44). |
| **Soak (c) closes and morning 0** | **PASS** | PCKL1 and PCKL2 closed at 21:59:59 EDT on 5 Oct (`dailyoccupancy` 621 / 371); Trail02 at 22:59:59 EDT (16). All three reported 0 at about 06:00 on 6 Oct. 6 Oct's US closes are after this check (02:00Z on 7 Oct). |
| **Soak memory** | PASS | Free-heap trend per wake cycle: PCKL1 −0.3 B, PCKL2 −0.6 B. Trail02: not measurable (11 cycles since its hibernate wake). |
| **Soak resets** | **PASS** | Only 70 (OTA) and Trail02's 30 (hibernate wake). No watchdog event. |
| **Soak `vc`** | **PASS** | All 97 v37 reports carry a number: Trail02 3.69–3.78, PCKL1 4.01–4.19, PCKL2 4.03–4.19. |
| **Trail02 `hibernate_wake`** | **on time; label `UNKNOWN`, so `fail` by design** | 10:01Z on 6 Oct: `fail`, `UNKNOWN`, gate `wake_reason`. RTC 03:00:51 → 10:00:51Z against req 25199 s: **+1 s**. It was identical under v35 (4 Oct) and v36 (5 Oct). v37's C accepts `DEEP_POWER_DOWN` only (this WO, item C fact check). |
| **B in the soak** | not exercised | No reset during a session on Trail02, PCKL1 or PCKL2 since v37. Dev-09 is B's evidence. |

**Verdict (Claude Code's assessment for Chip): PASS, so merge PR #62.** Every v37 criterion is met. The one exception is Trail02's `UNKNOWN` wake label: the wake is on time, it happened identically before v37, and it's outside item C's designed scope.

## Rollback

Revert the v37 commit and flash v36 (`v36-HourRules`). No persisted format changes.

## Approval record

- [x] Stage 5: Chip and the architect, 2026-10-04, opening dispatch: goal, items A–D, budgets (A ≤ 20, B ≤ 15, C ≤ 3, D ≤ 4; about 40 total), version v37 / product 37, Stage 7 checks, bench. Routing: one Copilot round (`claude-opus-5`, medium) and one narrow Stage 7 (Codex, `gpt-6-astra`, high). Not authorized: commits, merging, flashing, settings or webhook changes.
- [x] Fact check (Claude Code, `claude-opus-5-5`, 2026-10-05, against `79abe84`). The corrections are marked above:
  - A's RSSI sites;
  - the gate line in C (`:96`, not `:94`);
  - the clock is never trusted in `setup()`;
  - `current.dat` has no later epoch in occupancy mode;
  - DPD is sticky and the gate has no lateness bound.
- [x] Decisions (Chip, 2026-10-05):
  - **B-timing:** credit at the first trusted close;
  - **B-anchor:** `max(start, lastReport)`;
  - **C-rule:** DPD within a 0 to +60 s window.
- [x] Stage 6: Copilot (`claude-opus-5`, medium), 2026-10-05, one round. Report: `WO-2026-10-04-001-stage6-copilot-report.md`.
  - **Net `src/` lines:** A −69 (≤ 20), B +14 (≤ 15), C +3 (≤ 3), D +4 (≤ 4); −48 in all. Nothing compressed, as far as Claude Code's review saw.
  - **Tests:** 62/62 → 65/65; three new tests (`connect_no_modem_wait_structural_test.py`, `sleep_gate_modem_off_test.sh`, `occupancy_session_restart_test.sh`); 18/18 mutations caught.
  - **Build:** local 150788 / 1090 / 2188 (v36 rebuilt: 150740 / 1090 / 2180). bss +8 is the RAM-only `occupancySessionCrossedBoot`.
  - **Deviations:**
    1. `make clean-user` isn't a target in 6.4.1 `main`; Copilot used a fresh tree and build path instead;
    2. its payload maxima (occupancy 228 → 238, counting 217 → 227, with realistic widths) differ from the WO's type-worst figures (237 → 247, 224 → 234); both fit;
    3. a shared `occupancyDebounceMs()` replaces two copies in `State_Idle.cpp`/`State_Modes.cpp`;
    4. a new `stillOpen` field on `OccupancyCloseResult` guards the three `reportNow` transitions.
- [x] Claude Code's checks (2026-10-05):
  - **Suite:** 65/65 (sh via zsh, py via python3).
  - **Particle cloud compile:** boron 6.4.1 succeeded, flash 152014 / RAM 3282; `strings` finds `v37-PreStep6Fixes`.
  - **Tree:** `docs/` unchanged by Stage 6.
  - **Review note for Stage 7:** B2 keeps a session open whenever the clock is untrusted, not only at boot. Trust expires 24 h after the last sync (`ClockTrust.h:42`). A session whose debounce runs out while trust has lapsed during that boot is credited up to the first trusted close. Stage 7 bounds this.
- [x] Stage 7: Codex (`gpt-6-astra`, high), 2026-10-05. Verdict: `WO-2026-10-04-001-stage7-verdict.md`. **NOT VERIFIED.**
  - **Passed:** A1, A2, C, D, test intent, suite (65/65, sh via zsh, py via python3), build (150788 / 1090 / 2188; `nm` linkage confirmed), budget.
  - **B (blocking):** `State_Common.h:341` reads `lastReport` when the session closes. A report sent after the boot, before the clock is trusted (`State_Report.cpp:82`), moves the anchor forward. Reproduced with a 300 s debounce and a 10 h power-off: 40,000 s credited, against 4,300 s without that report. That's 36,000 s of over-credit.
  - **Proposed fix (not applied):** snapshot `max(start, lastReport)` at boot into a RAM-only `occupancySessionBootAnchor` (0 = no session open across a restart), replacing the bool. B goes to +15, within budget. In Codex's scratch check it brought the over-credit down to 300 s. The restart test needs a matching update and a regression case for a report after the boot.
  - **Mid-boot trust lapse (Claude Code's review note):** VERIFIED WITH NOTES. A session that started during the current boot, whose debounce expires after trust has lapsed, is credited until the first trusted close (36,000 s excess in a 10 h probe). v36 discarded such a session (credited 0). The 86,400 s guard still applies. Codex judges this an acceptable, disclosed limitation of B2; Chip to decide.
  - **Payload (Codex, all `%lu` at 10 digits):** occupancy 237 → 247, counting 224 → 234 (excluding the NUL). That matches the WO.
  - **Tree:** byte-identical afterwards (124,126 files); Claude Code confirmed `src/` and `tests/` against its pre-Stage-7 snapshot.
- [x] **Narrow edit (Claude Code, pre-authorized by Chip, 2026-10-05), the B fix from Stage 7:**
  - **Source:** `SessionState::occupancySessionBootAnchor` (`time_t`, RAM-only, 0 = no session open across a restart) replaces the bool. `setup()` captures `max(occupancyStartTime, lastReport)` once. `closeOccupancySessionSafely()` uses that snapshot and clears it.
  - **Test:** `tests/occupancy_session_restart_test.sh` pins and fake follow the rename. New case `testReportAfterRestartBeforeTrustDoesNotMoveTheCap` is Codex's reproduction: 300 s debounce, report at 5,000, 10 h off, a report after the restart at 41,050 while untrusted. Credited 4,300 s, over-credit 300 s.
  - **Mutation:** reading `lastReport` at close time again fails the new case. Source restored byte-identically by rewriting in place.
  - **Budget:** B +15 (≤ 15); total −47 net `src/` lines.
  - **Suite:** 65/65 (sh via zsh, py via python3).
  - **Cloud compile:** boron 6.4.1, flash 152070 / RAM 3290 (+8 RAM: the `time_t` field).
- [x] **Stage 7 round 2, narrow re-check of B only:** Codex (`gpt-6-astra`, high), 2026-10-05. Verdict: `WO-2026-10-04-001-stage7-round2-verdict.md`. **B: VERIFIED WITH NOTES.**
  - **Checks 1–8, 10 and 11:** PASS.
    - Codex's reproduction credits 4,300 s with or without the report after the restart: 300 s of over-credit.
    - 10-hour and 10-day outages are each capped at one debounce.
    - The three callers stay quiet when the clock is untrusted.
    - The anchor is in `.bss`, so it's RAM-only.
    - The daily-boundary close is correct.
    - Both regression mutations fail the new case.
  - **Check 9 (report only):** the Known-limitation fact check listed too few exceptions. Claude Code applied Codex's documentation correction above; it changes no `src/` lines.
  - **Suite and build:** 65/65 (35 sh via zsh, 30 py via python3). Local build 150844 / 1090 / 2196 (+56 text and +8 bss over round 1). `nm` finds `setup` and `closeOccupancySessionSafely`.
  - **Budget:** B +15 (setup +3, SessionState +1, State_Common.h +17, Idle −3, Modes −3).
  - **Tree:** byte-identical afterwards (124,128 files); Claude Code confirmed `src/` and `tests/` against its own snapshot.
- [ ] USER GATE 2: Chip.

## Budget versus actual (closing record)

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| A | ≤ 20 | — | −69 (A2: 0) | `connect_no_modem_wait_structural_test.py`, `sleep_gate_modem_off_test.sh` (new) |
| B | ≤ 15 | — | +15 after the narrow edit (Stage 6: +14) | `occupancy_session_restart_test.sh` (new; restart regression case added in the narrow edit) |
| C | ≤ 3 | — | +3 | `hibernate_wake_diagnostics_test` .sh/.cpp (updated) |
| D | ≤ 4 | — | +4 | `report_payload_fields_test.py` (updated) |
