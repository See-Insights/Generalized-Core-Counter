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
