# WO-2026-11-02-004: a changed timezone stored by ConfigApply triggers a planned reset (recovery item 25; v49 must-fix, gates wave 1; design: planned reset)

**Author:** Claude Code, 2 Nov 2026, DRAFT for the architect's review · **Status:** DRAFT for review; dispatch tomorrow (3 Nov) with the GitHub Actions WO (-003) and the spike
**Branch:** `wo/2026-11-02-004-timezone-planned-reset`, from `main`
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-7; §5 (time and boundaries: Codex `gpt-5.6-sol`, xhigh); §13 (c), (j)
**Budget:** `src/` at most **+10 net**, hard. Stop and report rather than trim style.
**Agents (architect, 2 Nov):** Stage 6 Copilot standard model, medium. Stage 7 Codex `gpt-5.6-sol`, xhigh (time and boundaries, §5), a new output file per round, **at most two rounds**.

## Plain goal
A migrated device stores `UTC0` by default, applies `EST5EDT` from the ledger on its first connect, and then runs its day logic in UTC until something reboots it (ChurchSt-2: last report 22:00Z, silent until 06:01Z, an occupancy session counted at 04:29 Eastern). When `ConfigApply` stores a changed timezone, reboot once through the deliberate-reset path so `setup()` configures `LocalTime` from it.

## The defect (OBS, main `a7b69ce`; read, not run)
- `setup()` configures the converter once from the stored string: `Generalized-Core-Counter.cpp:927-932` (`LocalTime::instance().withConfig(LocalTimePosixTimezone(tz))`).
- `ConfigApply` stores a changed timezone and logs it, nothing more: `ConfigApply.cpp:253-255` (`SystemConfig::set_timeZoneStr(timezone)`, `Log.info("Config: Timezone -> %s")`, `changed = true`). `ConfigApply.cpp` does not include `LocalTimeRK.h`.
- Fresh default is `UTC0`: `Config.h:30`, `MyPersistentData.cpp:133`.
- Every reader takes the config live from `LocalTime::instance().getConfig()` (`Clock.cpp:69`, `ClockDiagnostics.cpp:49`, `DailyBoundary.cpp:11`), and `LocalTimeCache` refreshes when that config's signature changes (`LocalTimeCache.cpp:48-60`), so one `withConfig` call updates all of them.
- Field evidence: ChurchSt-2 and LAB-01 both `config.generation` `EF590054`; LAB-01 follows Eastern only because it rebooted after the apply (pilot record, 24 h checks).

## Change (architect design ruling, 2 Nov: a planned reset, not a live re-apply)
When `ConfigApply` stores a changed timezone, **reset the device through the existing deliberate-reset path with a new reset cause**; the boot path then reads the stored timezone in `setup()`. Rationale: the boot path is field-proven (LAB-01's 15:30Z reset applied Eastern correctly), and a reboot re-reads every boot-only setting at once, so no live re-apply and no list of boot-only settings to chase.
- **New cause:** `RESET_CAUSE_TIMEZONE_CHANGE = 8` in `ResetCause.h` (+1); the reset surfaces as reset 140, data 8 (`ResetCause.h` header comment; the structural test `reset_cause_structural_test.py` counts the sites and values, extend it).
- **Where:** `ConfigApply.cpp`. Record that the timezone was stored (`ConfigApply.cpp:253-255`, a file-local flag), and in `applyConfigurationFromLedger` (`:123-171`), in the success branch after `validateStoredData()`, if the flag is set: **`SystemConfig::flushNow()` then `System.reset(RESET_CAUSE_TIMEZONE_CHANGE)`**, the same pairing the failsafe uses (`ConnectivityFailsafe.cpp:86,149,306`). The comment at `ConfigApply.cpp:144-146` says storage is normally flushed lazily by `SystemConfig::loop()`; a reset right after `set_timeZoneStr` without the forced flush would lose the value and loop (the ledger would re-apply on every boot).
- **Not in this WO:** malformed strings (architect: out of scope; the existing normalization and length check stay), any live re-apply.
- Estimate **+8 to +10** `src/` (enum +1, flag +2, reset block +4 to +5, comment). Budget at most **+10**.

## Stage 7 checks (two, architect ruling)
(a) **No reboot loop:** the reset happens only on a real change (the existing `strcmp` against the stored value, `:252`), the value is stored **and flushed** before the reset, and after the reboot the same ledger value finds no change. Walk: first connect with `UTC0` stored and `EST5EDT` in the ledger (one reset, then none); ledger equals stored (none); the apply called from `setup()` before the converter is configured (`Config.cpp` path) — does it reset needlessly?; a failed section (`success` false) must not reset; a device-level override that flips the zone back and forth (each real change resets once).
(b) **Daily totals and an open session survive,** as on failsafe and hibernate resets: `dailyoccupancy`, the open-session state and the publish queue after `System.reset(8)`; compare with the failsafe stage-2 path.

## Tests
- Host: the apply path with a changed zone flags the reset and flushes before it (order); unchanged zone does not; a failed apply does not; the flag is not set when only other sections change. `reset_cause_structural_test.py`: code 8 declared, distinct and non-zero, site passes it.
- Mutations: reset without the flush; reset on every apply; reset when the zone is unchanged; reset on a failed apply; cause 0 or a reused code.

## Field confirmation (item 25)
The v48 OTA after tonight's default decision reboots ChurchSt-2, which should then follow Eastern (hourly through 03:00Z, quiet until 11:00Z on 3 Nov). **Record the result as item 25's field confirmation** (the device has no remote reset). The fix itself is confirmed on a v49 device that applies a timezone change without rebooting.

## Waves
**Wave 1 moves to v49** (architect, 2 Nov).
