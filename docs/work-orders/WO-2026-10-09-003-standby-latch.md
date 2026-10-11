# WO-2026-10-09-003: the modem standby latch clears once a day

**Author:** architect (Claude in the app), 2026-10-10 · **Status:** Step 0 accepted; ready for Stage 6
**Branch:** `wo/2026-10-09-003-standby-latch` (Step 0 at `fbd53b0`) · **Base for Stage 6:** current main, merged into the branch first
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.1–§12.4; §13 (b), (d), (g), (j)

## Plain goal

A device whose modem standby was suppressed after a slow teardown gets standby back without a reboot, unless its modem is still failing.

## Why

- Once tripped, the latch can't clear: with standby suppressed every teardown is a full modem-off, and recovery needs one of 5 s or less (`State_Sleep.cpp:32`, `:134-138`), while Dev-09's take 9–22 s. Step 0 found no `MODEM_HEALTH: recovered` anywhere in the record.
- The bench devices hibernate nightly, which resets RAM and ends the latch. **The US fleet doesn't hibernate,** so there a latch lasts until the next reboot, possibly days, forcing a cold registration on every KEEP_ALIVE reconnect in open hours.
- Evidence: `WO-2026-10-09-003-step0-report.md` (on the WO branch).

## The fix: Step 0's option (b)

Inside the existing `if (due)` block in `State_Report.cpp`, after `SystemConfig::set_lastDailyCleanup(now);` (`:82` on main `d161909`), clear the four RAM `session` fields (`StateMachine.h:62-65`) when `session.modemUnstable` is set, and log one line:

`MODEM_POLICY: standby re-allowed at daily close; night modem-off decides`

The night's full modem-off is the verdict: a still-slow modem re-trips through the existing check (`State_Sleep.cpp:887-888`), and a healthy one has standby back the next morning.

## Architect rulings (2026-10-10)

1. **Option (b) approved.** It reuses the existing daily boundary; no new persisted field, timer, schedule or RAM field.
2. **The trip rule stays as it is** (Step 0 open point 1). Whether a slow-but-working modem should keep standby is a separate, larger question (Codex 2026-10-01 report #6), logged for later, not in this WO.
3. **No dead-band change** (option d). One change per WO.
4. **Accepted edges, no extra code:**
   - an always-slow modem (like Dev-09's) re-trips every night, so nothing changes for it;
   - with an untrusted clock, `due` is false and nothing clears (today's behaviour);
   - if a night has no modem-off to measure, standby returns without a verdict; the two-connect-timeout trip remains the protection.
5. **Don't touch** `maybeClearModemUnstable()`, the trip thresholds, or anything in `State_Sleep.cpp`.

## Budget

≤ +20 net `src/` lines; expected about +8. Over +20 means stop and report. Moved lines counted separately.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **medium** reasoning. A small insertion at an identified line.
- **Stage 7:** Codex, `gpt-5.6-sol`, **medium** reasoning. Independent check of a small change.
- Claude Code probes each model with a one-line prompt first (§5) and records the IDs.

## Tests (host)

- A latched device reaching the daily boundary clears all four fields and logs the line once.
- A device that isn't latched logs nothing at the boundary.
- `due` false (no boundary, or untrusted clock): no clear.
- After a clear, a teardown over 10 s re-trips (existing path unchanged).
- Test code copied from `src/` is byte-checked (§13 g); each mutation is caught by its own check (for example: clear outside `if (due)`; one field not cleared; clear unconditionally).
- Tests reported as `N/N (sh via zsh, py via python3)`.

## Stage 7 checks

Standard: linkage, fresh `BUILD_PATH_BASE` build, binary contains the new path (`nm`/disassembly), tests, retirement (no other new writer of the four fields). Plus: confirm the clear can't run in open hours before the night's verdict, and that `State_Sleep.cpp` is unchanged.

## Bench and release

- **No laptop bench before merge.** The bench devices hibernate, which already clears the latch, so they can't show the fix.
- **Release:** v41, with the TimeDiag fix (#85) and the config window (#86).
- **Field check after v41:** a US device that logs `MODEM_POLICY: standby temporarily disabled` should show `standby=1/1` again the morning after a clean night. Only visible where serial is forwarded; otherwise from status timing.

## Rules

Agents may open the PR; only Chip merges. Two-round rule applies. Records committed and pushed per stage.

## Closing record

(To be added: rounds and models, budget versus actual, controller edits, accepted edges, release.)
