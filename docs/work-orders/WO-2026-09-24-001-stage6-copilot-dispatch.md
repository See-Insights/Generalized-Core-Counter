# Stage 6 dispatch — WO-2026-09-24-001 (Revised): Restore daily count reset

**To:** GitHub Copilot (Implementer), standard tier, local `copilot` CLI against the working tree.
**Repository:** `Generalized-Core-Counter`
**Branch:** `wo/2026-09-24-001-daily-cleanup-boundary-fix`, created from post-Step-5 `main` at `8163295`. Check it out with a clean working tree before starting.
**Spec:** `docs/work-orders/WO-2026-09-24-001-daily-cleanup-boundary-fix.md`. This is the only source of truth. Read all of it before changing anything. Stage 5 decisions 1–6 in its approval record are binding.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Read its "GitHub Copilot — Implementer" section and its "Temporary build and test artifacts" section first. In summary:

- Work only from this approved WO. Do not expand scope or change the architecture. If the WO cannot be implemented as written, **stop and report back**. Do not improvise.
- **Do not commit, push, merge, or release.** Leave a complete, uncommitted working-tree diff.
- Temporary artifacts must use visible, descriptive names, sit under a gitignored path, and be removed when you finish, including on failure paths. No dot-prefixed files.

## What to implement

Replace the day-boundary check in `handleReportingState()` (`src/state/State_Report.cpp`) with the block in the WO's "Fix" section. Keep its sequence exactly, because the ordering is load-bearing:

trusted-clock gate → normalize `close` (`open == close` → 24) → compute `boundary` (today's local close; if `now` is before it, yesterday's) → due if `lastDailyCleanup < boundary || lastDailyCleanup > now` → if occupied, close the session **at `boundary`** → `publishData()` → reset daily counts → if `close == 24` and was occupied, start a new session at `boundary` → `setLastDailyCleanup(now)`.

Meet every item under the WO's "Requirements", in particular:

- `localTodayAt(h)` computes the boundary live from `Time.now()` plus the current timezone offset (via `LocalTimeRK` conversion of the current instant). It must never read `LocalTimeCache`. The cache refreshes only once per minute and can return the wrong calendar date right at a boundary (the midnight-straddle race, a separate WO). `localTodayAt(24)` returns the next day's local midnight.
- `closeSessionAt(t)` credits `max(0, t - sessionStart)` and never counts time past `t`.
- `lastDailyCleanup` is the only boundary state. Zero counts as due.
- `lastReport` and the hourly `hourlyCount` reset are not touched.
- Always-open is handled only by the `close = 24` normalization. There is no separate branch, and the `openHour == closeHour` convention in `Clock.cpp` stays as it is.
- Always-open is represented internally by normalizing open == close to close = 24 inside the boundary calculation only. `closeHour = 24` is not a valid stored or config value in this WO. Config validation, `Clock.cpp`'s open-hours check, and stored data are unchanged. Adopting 0/24 as the user-facing always-open convention is a separate follow-up WO (config validation, cloud settings, migration of open == close devices).

### Names (post-Step-5 facades are now on `main`)

The WO's name-mapping table lists pre-Step-5 symbols. Use the facade forms:

| WO placeholder | Use |
|---|---|
| `openHour` / `closeHour` | `SystemConfig::get_openTime()` / `SystemConfig::get_closeTime()` |
| `lastDailyCleanup()` / `setLastDailyCleanup()` | `SystemConfig::get_lastDailyCleanup()` / `SystemConfig::set_lastDailyCleanup()` (confirmed present, `src/persist/SystemConfig.h:140-141`) |
| `isOccupied()` | `CurrentReadings::get_occupied()` |
| `startSessionAt(t)` | `CurrentReadings::set_occupied(true)` + `CurrentReadings::set_occupancyStartTime(t)` |
| `resetDailyCounts()` | the existing `dailyCleanup()` path (→ `resetEverything()`) |
| `publishData()` | `publishData()`, unchanged, `void` |

### The one new helper: `closeSessionAt(t)`

**Extend** `closeOccupancySessionSafely(path)` in `src/state/State_Common.h` (now at line 272) with a boundary-aware parameter, so it credits up to `t` instead of `Time.now()`. Do not create a sibling copy: the existing helper already handles `start == 0`, future-dated starts and the 86400-second sanity clamp. Existing callers (`State_Idle.cpp`, `State_Modes.cpp`, `State_Sleep.cpp`) must keep their current behavior.

### Explicitly dropped. Do not add any of these, and do not leave TODOs for them

- `sensorOff()` or any PIR interrupt management
- Any change to `publishData()`'s return type or signature, or a publish success/failure gate
- Deferring cleanup while occupied, or rescuing sessions beyond the close-at-boundary credit
- `PublishQueue` overflow handling, the `LocalTimeCache` midnight-straddle race, DST edge cases, backfilling backend data
- Retiring the `open == close` convention (that is WO-2026-09-24-004)

## Acceptance criteria (from the WO, all seven)

1. Daily counts reset exactly once per boundary: at park close for park-hours sensors, at midnight for 24-hour sensors.
2. While the clock is untrusted, nothing in this block runs or stamps any state.
3. If trust returns after a missed boundary, cleanup runs once on the first trusted loop.
4. A session open at close is credited only up to the boundary.
5. A `lastDailyCleanup` later than `now` triggers cleanup instead of blocking it.
6. Month-end rollover (31 → 1) works, because nothing compares day-of-month values.
7. Hourly reporting and `hourlyCount` behavior are unchanged.

Add or update host tests so that each criterion is exercised. Check existing tests that reference `dailyCleanup()` or the old year/month/day comparison (e.g. `tests/clock_resync_wiring_test.py`). Update a test only where the WO's change makes its assertion obsolete, never to force a pass. Report every test you changed and why.

## Verification commands (run all and report results)

1. Full host suite: every `tests/*.sh` plus every bare `tests/*.py`. Report the pass count against the current baseline of 42/42.
2. Local ARM-toolchain build (boron). Report flash/RAM against the `main` baseline.
3. Cloud compile: `particle compile boron . --target 6.4.1`. Report flash/RAM against `main`.

## Implementation Report (required at the end)

- Files changed
- Behavior changed
- Tests added or updated, and why
- Commands run and their results (including build sizes)
- Known limitations
- Deviations from the WO or unresolved concerns (or "none")

Stage 7 (Codex) will review the diff against this WO only, checking acceptance criteria 1-7.
