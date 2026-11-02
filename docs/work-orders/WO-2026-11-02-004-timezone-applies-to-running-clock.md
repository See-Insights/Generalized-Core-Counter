# WO-2026-11-02-004: a timezone stored by ConfigApply must reach the running LocalTime (recovery item 25; v49 must-fix, gates wave 1)

**Author:** Claude Code, 2 Nov 2026, DRAFT for the architect's review · **Status:** DRAFT (no dispatch)
**Branch:** `wo/2026-11-02-004-timezone-running-clock`, from `main`
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-7; §5 (time and boundaries: Codex `gpt-5.6-sol`, xhigh); §13 (c), (j)
**Budget:** `src/` at most **+10 net**, hard. Stop and report rather than trim style.
**Agents (architect, 2 Nov):** Stage 6 Copilot standard model, medium. Stage 7 Codex `gpt-5.6-sol`, xhigh, a new output file per round, **at most two rounds**.

## Plain goal
A migrated device stores `UTC0` by default, applies `EST5EDT` from the ledger on its first connect, and then runs its day logic in UTC until something reboots it (ChurchSt-2: last report 22:00Z, silent until 06:01Z, an occupancy session counted at 04:29 Eastern). When `ConfigApply` stores a changed timezone, apply it to the running `LocalTime` as `setup()` does at boot.

## The defect (OBS, main `a7b69ce`; read, not run)
- `setup()` configures the converter once from the stored string: `Generalized-Core-Counter.cpp:927-932` (`LocalTime::instance().withConfig(LocalTimePosixTimezone(tz))`).
- `ConfigApply` stores a changed timezone and logs it, nothing more: `ConfigApply.cpp:253-255` (`SystemConfig::set_timeZoneStr(timezone)`, `Log.info("Config: Timezone -> %s")`, `changed = true`). `ConfigApply.cpp` does not include `LocalTimeRK.h`.
- Fresh default is `UTC0`: `Config.h:30`, `MyPersistentData.cpp:133`.
- Every reader takes the config live from `LocalTime::instance().getConfig()` (`Clock.cpp:69`, `ClockDiagnostics.cpp:49`, `DailyBoundary.cpp:11`), and `LocalTimeCache` refreshes when that config's signature changes (`LocalTimeCache.cpp:48-60`), so one `withConfig` call updates all of them.
- Field evidence: ChurchSt-2 and LAB-01 both `config.generation` `EF590054`; LAB-01 follows Eastern only because it rebooted after the apply (pilot record, 24 h checks).

## Change
At `ConfigApply.cpp:253-255`, after `set_timeZoneStr(timezone)`, call `LocalTime::instance().withConfig(LocalTimePosixTimezone(timezone))` (+1 line, +1 include of `LocalTimeRK.h`). **Option (preferred if it fits):** one function in `time/` (for example `Clock::applyTimeZone(const char *tz)`) used by both `setup()` and `ConfigApply`, so the two cannot drift: about +3 net (a definition and a declaration, minus the duplicated setup line). Estimate **+2 to +5**.

## Points Stage 7 must examine (time and boundaries)
1. **A mid-day zone change moves the local date.** `DailyBoundary::check` (`DailyBoundary.cpp:27-45`) and the day-total cleanup compare against a stored boundary. UTC to Eastern moves the local date back some hours of the day (at 02:00Z the UTC date is the 3rd, the Eastern date the 2nd). Walk both directions at a connect: can it fire a spurious daily cleanup, skip one, or lose or double a day total (`dailyoccupancy`)? What is `lastDailyCleanup` compared with?
2. **Open/close and the sleep plan after the switch:** `isWithinOpenHours`, the reporting window (`ReportingPolicy`), `Clock::openness()`, hibernate/sleep duration if computed before the apply in the same cycle.
3. **Occupancy session across the switch:** a session open at the apply (item 10's close-at-boundary logic, `State_Common.h:360-370`).
4. **Other things read once at boot** from the timezone or hours (a cached copy would have the same gap): list them, fix none in this WO.
5. **Invalid string:** `LocalTimePosixTimezone(tz)` with a malformed string (the string is already normalized and length-checked at `:249-251`); what does `withConfig` do on an invalid POSIX string, and does the stored setting then disagree with the running config?

## Tests
- Host: a seam that applies a ledger timezone and then reads the open/close decision and the local hour (UTC0 then EST5EDT at a fixed epoch: the local hour changes); `LocalTimeCache` returns the new local hour after the apply; same timezone applied twice is a no-op; a structural test that `ConfigApply` calls the apply function where it stores the zone.
- Mutations: remove the call; apply before storing; apply on the unchanged branch only.

## Field confirmation (item 25)
The v48 OTA after tonight's default decision reboots ChurchSt-2, which should then follow Eastern (hourly through 03:00Z, quiet until 11:00Z on 3 Nov). **Record the result as item 25's field confirmation** (the device has no remote reset). The fix itself is confirmed on a v49 device that applies a timezone change without rebooting.

## Waves
**Wave 1 moves to v49** (architect, 2 Nov).
