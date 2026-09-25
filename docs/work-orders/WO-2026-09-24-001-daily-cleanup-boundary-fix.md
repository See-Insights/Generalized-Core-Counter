# WO-2026-09-24-001 (Revised): Restore daily count reset

**Status:** **Stage 5 APPROVED** (2026-09-24). Stage 4 closed. This revision,
authored by Chip, **replaces the prior WO text** - including the §4a
cleanup/publish reordering and the round-3 "lossless rollover" scope. Ready
for Stage 6 (Copilot) once Step 5 merges, then Stage 7 (Codex diff review),
Stage 8 (commit).

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`.

**Sequencing:** Start only after Step 5 (WO-2026-09-23-001) is merged to
`main`. Branch from post-Step-5 `main` and use the Step 5 facade accessors
for `lastDailyCleanup` (`SystemConfig::get_lastDailyCleanup()` /
`SystemConfig::set_lastDailyCleanup()`).

**Model tiers:** Copilot standard tier for Stage 6 (the logic is fully
specified below). Codex standard high-reasoning tier for Stage 7.

## Change of direction

This WO replaces the prior §4a work (the cleanup/publish reordering) and the
round-3 "lossless rollover" scope. Three investigation rounds kept expanding
what "lossless" meant: publish-result contracts, rescuing active sessions,
payload side effects. The actual fix is about 15 lines. **The complexity came
from over-specifying, not from the problem.**

Two facts close the round-3 findings:

1. **Publish durability is already solved.** `publishData()` writes to
   `PublishQueue`, which persists across coverage gaps. A publish
   success/failure gate is not needed - which retires round 3's finding 1
   and the proposed `publishData()` signature change.
2. **Occupancy is only counted during open hours.** A session open at close
   is credited up to close, then ended. There is no deferral and no
   after-the-fact rescue - which retires round 3's finding 2 and the
   proposed occupancy-deferral rule.

**Do not reintroduce any of the dropped scope.** Any finding outside this WO
gets filed separately and does not block it.

## Problem

`State_Report.cpp` gates the daily reset on `Clock::isTrusted()` but stamps
`lastReport` regardless. When the boundary check first fires while the clock
is untrusted, the boundary crossing is erased permanently, so
`dailyoccupancy` never resets.

Confirmed on Dev-09, Dev-11, and Dev-14; production affected since
2026-09-21.

### Supporting evidence (retained from the prior investigation)

Fresh, live telemetry across real SGT day boundaries - four crossings, three
independent devices, zero resets:

| Device | Before boundary | After boundary | Result |
|---|---|---|---|
| Dev-14 | Sep 21 14:02 UTC: `dailyoccupancy=74` | Sep 21 22:01 UTC: `74` | No reset |
| Dev-14 | Sep 22 13:36 UTC: `dailyoccupancy=173` | Sep 22 22:02 UTC: `173` | No reset |
| Dev-09 | Sep 22 14:00 UTC: `dailyoccupancy=322` | Sep 23 01:50 UTC: `322` | No reset |
| Dev-11 | Sep 22 13:35 UTC: `dailyoccupancy=458` | Sep 23 01:23 UTC: `467` | Still climbing - no reset |

Root cause, confirmed by direct code reading on `main`: the day-boundary
check is gated inside `if (Clock::isTrusted())` while
`sysStatus.set_lastReport(now)` runs unconditionally outside it - so an
untrusted boundary report consumes the crossing without resetting anything,
and no later report can detect the miss.

Onset correlates with WO-2026-09-19-001 (Step 3b), which converted this gate
to `Clock::isTrusted()`.

## Fix

Replace the existing day-boundary check with the following. Names below are
placeholders; see "Name mapping" for the real functions and config fields.

Revised by Stage 5 decision 8 (2026-09-25). There is one publish per report,
and the report that triggers the cleanup is the day's closing record.

```cpp
time_t now = Time.now();

// Boundary test: evaluated first, with no writes, so the report knows
// whether it is the closing record. Untrusted: not due, nothing stamped.
bool due = false;
time_t boundary = 0;
int close = 0;
if (Clock::isTrusted()) {
    // Normalize once - no separate always-open branch.
    close = (openHour == closeHour) ? 24 : closeHour;  // legacy always-open → midnight
    boundary = localTodayAt(close);
    if (now < boundary) boundary -= 86400;      // before today's close → use yesterday's
    time_t last = lastDailyCleanup();
    due = (last < boundary || last > now);      // due, or stamp is in the future (clock jumped back)
}

// Closing-record preparation (only when due): credit an open session up to
// the boundary so the closing report includes it.
bool wasOccupied = false;
time_t sessionStart = 0;
if (due) {
    wasOccupied = isOccupied();                  // capture BEFORE close: resetDailyCounts() clears occupancy
    sessionStart = occupancyStartTime();         // capture BEFORE close: closeSessionAt() zeroes it
    if (wasOccupied) closeSessionAt(boundary);   // credit up to close, not beyond
}

// (1) The normal report publish - the only publish. When due, it is the
//     day's closing record, stamped at boundary - 1.
publishData(due ? boundary - 1 : 0);             // 0 = no override (existing timestamp)

// (2)-(4) Writes, in order, only when due.
if (due) {
    resetDailyCounts();                                                          // (3) reset
    if (close == 24 && wasOccupied) startSessionAt(max(boundary, sessionStart)); // remainder counts toward new day
    setLastDailyCleanup(now);                                                    // (4) stamp
}
```

Order of effects: (1) the normal report publish, (2) the boundary decision
it carries, (3) reset, (4) stamp `lastDailyCleanup`. The boundary test itself
is evaluated before the publish because the publish must know whether it is
the closing record (for the stamp), and because an open session must be
credited up to the boundary before it is reported; otherwise the reset would
zero the credited time without it ever being reported. The test writes
nothing.

### Requirements

- `localTodayAt(h)` computes the boundary live from `Time.now()` plus the
  current timezone offset (via `LocalTimeRK` conversion of the current
  instant). It must never read `LocalTimeCache`. The cache refreshes only
  once per minute and can return the wrong calendar date right at a boundary
  (the midnight-straddle race, a separate WO). **`localTodayAt(24)` returns
  the next day's local midnight.**
- `closeSessionAt(t)` credits `max(0, t - sessionStart)`. It must never count
  time past `t`.
- `lastDailyCleanup` is the only boundary state. Its zero value (never
  cleaned) counts as due.
- `lastReport` and the hourly `hourlyCount` reset are not touched.
- There is exactly one `publishData()` call per report. The separate in-block
  snapshot publish is removed (decision 8).
- `publishData()` takes an optional stamp override
  (`publishData(time_t stampOverride = 0)`). A nonzero override replaces the
  occupancy-mode payload's `timestamp`; zero keeps today's behavior. Only the
  report that triggers the cleanup passes `boundary - 1`, in both the on-time
  and catch-up cases. The counting-mode payload (which stamps
  `endOfPrevHourStampSec`) is unchanged.
- The always-open case is handled by the `close = 24` normalization above,
  not by a separate branch. Note this does **not** retire the
  `openHour == closeHour` convention itself - that remains in
  `Clock.cpp:21-23` and is blocked on a config audit (filed separately; see
  "Two pre-implementation checks").
- Always-open is represented internally by normalizing open == close to
  close = 24 inside the boundary calculation only. `closeHour = 24` is not a
  valid stored or config value in this WO. Config validation, `Clock.cpp`'s
  open-hours check, and stored data are unchanged. Adopting 0/24 as the
  user-facing always-open convention is a separate follow-up WO (config
  validation, cloud settings, migration of open == close devices).

### Name mapping (Stage 3 work, to remove ambiguity before Stage 6)

Verified against the current tree. Post-Step-5, `sysStatus.` becomes
`SystemConfig::` and `current.` becomes `CurrentReadings::` per the facade
split - the mappings below name the pre-Step-5 symbols for traceability.

| Placeholder | Real symbol | Notes |
|---|---|---|
| `openHour` / `closeHour` | `sysStatus.get_openTime()` / `get_closeTime()` | `uint8_t` hour-of-day |
| `close == 24` (always-open) | normalized from `openTime == closeTime` | Existing convention: `Clock.cpp:21-23` treats `openHour == closeHour` as always-open. Normalized inline per Stage 5 decision 3 - no separate branch |
| `lastDailyCleanup()` / `setLastDailyCleanup()` | `sysStatus.get_lastDailyCleanup()` / `set_lastDailyCleanup()` | Already exists; currently write-only, no reader makes a decision from it |
| `isOccupied()` | `current.get_occupied()` | |
| `occupancyStartTime()` | `current.get_occupancyStartTime()` | Added with Stage 5 decision 7 |
| `startSessionAt(t)` | `current.set_occupied(true)` + `current.set_occupancyStartTime(t)` | Mirrors `State_Modes.cpp:65` / `State_Sleep.cpp:1669`, but with an explicit time instead of `Time.now()` |
| `publishData()` | `publishData(time_t stampOverride = 0)` | Still `void`. Decision 8 adds the optional stamp override (occupancy payload only); the report path's single call passes `boundary - 1` when due, `0` otherwise |

**One placeholder needs a small new helper:**

**`closeSessionAt(t)`** - the existing `closeOccupancySessionSafely(path)`
(`State_Common.h:271`) credits up to `Time.now()`, not an arbitrary
boundary, and always ends with `set_occupied(false)` /
`set_occupancyStartTime(0)`. Crediting *up to `boundary`* needs a
boundary-aware parameter on that helper. Extend the existing helper rather
than duplicating it - it already handles `start == 0`, future-dated starts,
and the 86400-second sanity clamp, and a sibling copy would have to
re-implement all three.

**`sensorOff()` is dropped** (Stage 5 decision 1). The open-hours check at
`Clock.cpp:17` already stops counting at close, so the step does nothing in
this WO. PIR interrupt management belongs to Step 6's `SensorManager` split
if it is needed at all. No placeholder, no TODO.

Also note for Stage 6: `resetDailyCounts()` maps to the existing
`dailyCleanup()` → `current.resetEverything()` path, which **also clears
occupancy state** (`occupied`, `occupancyStartTime`, `lastOccupancyEvent`,
`totalOccupiedSeconds`) - which is why the `startSessionAt(boundary)` step
must come *after* it in the sequence above. That ordering is load-bearing,
not incidental.

### `dailyCleanup()` → `resetEverything()` write audit (Stage 5 decision 2)

Complete list of everything the cleanup path writes, verified by direct
reading of `Generalized-Core-Counter.cpp:2701-2722` and
`MyPersistentData.cpp:689-707`:

**`dailyCleanup()` itself:**

| Action | Kind | Notes |
|---|---|---|
| `publishDiagnosticSafe("Daily Cleanup", "Running", PRIVATE)` | Publish | Only when `Particle.connected()`. No persisted write |
| `requestClockResync("daily-cleanup")` | Resync request | Only when connected. Persisted `lastTimeSync` is stamped later by `checkClockResync()` on confirmed success - **not** here |
| `Log.info("Running Daily Cleanup")` | Log | |

**`current.resetEverything()`:**

| Field written | Store | Category |
|---|---|---|
| `lastCountTime` ← `isClockTrusted() ? Time.now() : 0` | `current` | Telemetry (documented write-only field, no consumers) |
| `resetCount` ← `0` | **`sysStatus`** | Boot reset counter - **cross-store write** |
| `hourlyCount` ← `0` | `current` | Hourly count |
| `dailyCount` ← `0` | `current` | Daily count |
| `occupied` ← `false` | `current` | Occupancy state |
| `lastOccupancyEvent` ← `0` | `current` | Occupancy state |
| `occupancyStartTime` ← `0` | `current` | Occupancy state |
| `totalOccupiedSeconds` ← `0` | `current` | Occupancy state |

**Verdict against the three prohibitions: all clear - no change needed.**

- `lastReport` - **not touched** ✓ (acceptance criterion 7 holds)
- `lastDailyCleanup` - **not touched** ✓ (this WO's only boundary state
  stays exclusively under the new block's control)
- Persisted config (open/close hours, timezone, reporting interval, sensor
  settings) - **not touched** ✓

Clearing `hourlyCount` at the boundary is fine, as noted at Stage 5: the
block publishes before resetting.

**Two pre-existing behaviors recorded for visibility, neither prohibited nor
changed by this WO:**

1. `resetEverything()` writes `sysStatus.set_resetCount(0)` - a
   `currentStatusData` method reaching across into `sysStatus`. It is not
   `lastReport`, not `lastDailyCleanup`, and not config, so it is permitted
   - but it means the daily boundary also zeroes the boot-reset counter.
2. `requestClockResync("daily-cleanup")` fires on every cleanup when
   connected. This WO changes *when* cleanup fires, so it changes when that
   request fires - behavior unchanged, timing follows the corrected
   boundary.

### Two pre-implementation checks (requested at Stage 5)

**1. Does the open-hours check already exclude the closing hour? YES - no
field behavior change, nothing to disclose.**

`Clock.cpp:14-25` is the single implementation, used by all three call sites
(`Clock.cpp:82`, `:560`, `:606`) - there is no divergent second copy:

```cpp
if (openHour < closeHour) {
  return (hour >= openHour) && (hour < closeHour);   // closes AT closeHour
} else if (openHour > closeHour) {
  return (hour >= openHour) || (hour < closeHour);   // overnight window
} else {
  return true;                                        // openHour == closeHour: always open
}
```

The close side is `hour < closeHour`, i.e. the park is already closed once
`hour == closeHour`. Deployed parks are **not** counting an extra hour past
close. No correction is needed and this WO introduces no field behavior
change on that account.

**2. Which deployed devices use `openTime == closeTime`? INCONCLUSIVE from
fleet telemetry - do not treat as "none."**

Audited all 12 devices in `DeviceCurrentState`:

| Devices | Open/close visibility | Result |
|---|---|---|
| Dev-09, Dev-11, Dev-14 | Settings ledger populated | `open=6 close=22` - not always-open |
| 8 production devices (ToM-MCP-*, Morrisville-Tennis-*) | `deviceSettingsLedgerData` is `null` | **Unknown** |
| SAMIT-TRAIL02 | Partial: `closeHour=23`, **no `openHour`** | **Unknown - and a live candidate:** if its on-device `openTime` is also 23, it is in always-open mode today |

Why telemetry cannot close this question: the settings ledger reflects the
*cloud-pushed override*, not the effective on-device value (which lives in
persisted `sysStatus` / compiled defaults). The device-**status** ledger
publishes no `timing` block at all - confirmed against the payload builder -
so effective open/close hours are not observable from the fleet side.
`Cloud::hasNonDefaultConfig()` (`DeviceStatusPublisher.cpp:645`) is a
composite boolean spanning timezone, reporting interval, sensor settings and
open/close, so it cannot isolate the question either.

**Conclusion:** the `openHour == closeHour` always-open convention cannot be
retired as "a one-line cleanup in `Clock.cpp:21-23`" on current evidence -
that would require confirming no deployed device relies on it, which this
data does not support. Both follow-ups are now filed separately per Stage 5
decision 4 and neither blocks this WO:

- **WO-2026-09-24-003** - publish effective open/close hours in the
  device-status payload (closes the visibility gap that made this question
  unanswerable).
- **WO-2026-09-24-004** - retire the `open == close` convention. Blocked on
  the config audit; unblocking path recorded there.

This WO **normalizes** the sentinel inline (`close = 24`) rather than
retiring it, so the daily-reset fix works for always-open devices regardless
of whether WO-2026-09-24-004 ever lands.

## Explicitly out of scope

Not part of this WO:

- Changing the return value of `publishData()`
- Handling `PublishQueue` overflow (pre-existing)
- Deferring cleanup while occupied
- The `LocalTimeCache` midnight-straddle race (has its own WO)
- Backfilling backend `dailyoccupancy` data since 2026-09-21. **Decided
  2026-09-25: no backfill.** History before the update stays marked as
  corrupted; counts are correct from the update onward (see "Deployment
  effect").
- DST edge cases

## Acceptance criteria

1. Daily counts reset exactly once per boundary, at `parkClose` for
   park-hours sensors and at midnight for 24-hour sensors.
2. While the clock is untrusted, nothing in this block runs or stamps any
   state.
3. If trust returns after a missed boundary, cleanup runs once on the first
   trusted loop.
4. A session that is open at close is credited only up to the boundary.
5. A `lastDailyCleanup` value later than `now` triggers cleanup instead of
   blocking it.
6. Month-end rollover (31 → 1) works, because nothing compares day-of-month
   values.
7. The hourly reporting and `hourlyCount` behavior is unchanged.
8. The report that triggers the cleanup is published before the reset, and
   there is no second publish at cleanup (decision 8).
9. Only the report that triggers the cleanup has its occupancy payload
   timestamp set to `boundary - 1`, in both the on-time and catch-up cases.
   Every other report, and the counting-mode payload, is stamped as before
   (decision 8).

## Bench validation

- **Normal close:** the reset happens at `parkClose`, and the final publish
  shows the full day's counts.
- **Occupied at close:** the session is credited up to close, and the counts
  are zero afterward.
- **Untrusted across the boundary, then trusted:** a single catch-up reset
  occurs.
- **24-hour sensor:** the reset happens at midnight.

### Bench results, 2026-09-25 (Dev-14, pre-decision-8 build)

Build: the Stage 7-verified WO build (decisions 1–7), which still had the
in-block snapshot publish that decision 8 removes. Times UTC, with SGT in
parentheses.

- **Morning catch-up (the one-time post-flash correction).** Flashed at
  06:13Z (14:13 SGT). The cleanup ran at the **06:13:28Z** report: the
  serial `last=1790316808` is 06:13:28Z. The 06:13:44Z webhook (payload
  06:13:28Z, `dailyoccupancy=493`) was that report's in-block snapshot. The
  post-reset 0 report from the same report was lost when a reset at
  06:15:32Z cleared the RAM queue before it was sent. The next boot's first
  report (payload 06:15:35Z, 0, `resets=1`) was delivered at 06:18:24Z.
  (This corrects Claude Code's earlier account, which placed the cleanup on
  the 06:18 boot.)
- **On-time close.** `closeHour` set to 15 at 06:15:30Z. At 07:00:03Z
  (15:00:03 SGT) the report ran at close: USB serial shows
  `Daily boundary reached (boundary=1790319600 last=1790316808 now=1790319603 close=15)`,
  then `Report: … totalMin=9` (snapshot) and `Report: … totalMin=0`, and the
  DATA ledger was stamped 15:00:03. `lastDailyCleanup` is now 15:00:03 SGT,
  so the next morning's first report does not run a cleanup. The cleanup
  logic behaved as specified.
- **Both 15:00 webhooks were lost in transit** (accepted into the queue,
  queue drained, never received by Particle's integrations or AWS). That
  loss is not caused by this WO; it is tracked as **WO-2026-09-25-001**.

## Deployment effect (accepted 2026-09-25)

On devices affected by the 2026-09-21 bug, `lastDailyCleanup` has not been
stamped since the bug began, so the first trusted report after the flash is
due and the daily counts reset once, mid-day. This is accepted: holding off
until the next boundary would keep the corrupted accumulated counts for the
rest of that day. The day's counts are published before the reset, so
nothing is lost.

**Release notes line:** "On the day of the update, affected devices will
show a mid-day dailyoccupancy reset (a one-time correction of the 9/21 bug)."

## Stage 7 instructions (Codex)

Review the diff against this spec only and confirm acceptance criteria 1
through 9. Anything outside scope gets filed as a separate item and does not
block this WO.

## Approval record

- [x] Codex Stage 4 — three rounds, 2026-09-24, GPT-6 Astra
      `--reasoning-effort xhigh`. **Closed.** Rounds 1-2 produced the
      `lastDailyCleanup` marker and zero-handling decisions, both retained
      above. Round 3 correctly falsified the prior §4a "lossless" design;
      that design has since been dropped entirely in favor of this revision,
      which retires both round-3 findings on the two facts in "Change of
      direction."
- [x] Chip approval (Stage 5) — 2026-09-24. Four decisions applied to this
      document: (1) `sensorOff()` dropped entirely, no placeholder;
      (2) `dailyCleanup()`/`resetEverything()` write audit completed and
      recorded - all clear, no change needed; (3) always-open normalized
      inline via `close = 24`, no separate branch; (4) the two config
      follow-ups filed as WO-2026-09-24-003 and WO-2026-09-24-004.
      Two further decisions added 2026-09-25, before Stage 6 dispatch:
      (5) `localTodayAt(h)` computes live local time from `Time.now()` plus
      the current timezone offset, never from `LocalTimeCache` (see
      Requirements); (6) `close = 24` is an internal normalization only -
      `closeHour = 24` is not a valid stored or config value in this WO, and
      adopting 0/24 as a config value is a follow-up WO (see Requirements).
      One further decision added 2026-09-25, after Stage 6 and before
      Stage 7: (7) Occupancy is captured before the session close. The
      always-open restart uses that captured value, because
      `resetEverything()` clears occupancy. The restarted session starts at
      `max(boundary, originalSessionStart)`, so it never credits time before
      the person actually arrived. (Stage 6 implemented the capture without
      reporting it as a spec correction; the `max()` rule was applied by
      Claude Code as a pre-authorized one-liner, with test coverage, before
      Stage 7. The Fix pseudocode above is corrected to match.)
      Decision 8 (2026-09-25): restore the user's specified order: (1) the
      normal report publish, (2) then the boundary test, (3) then reset,
      (4) then stamp lastDailyCleanup. Remove the separate in-block snapshot
      publishData() (it came from the architect's sketch, not the user's
      spec). The report that triggers the cleanup is the day's closing
      record: stamp its payload at boundary − 1 via an optional stamp
      override on publishData(), so Ubidots, which plots the payload
      timestamp, attributes it to the day it closes, in both the on-time and
      catch-up cases. The counting-mode payload is unchanged.
      (Implementation note, Claude Code: the boundary *test* is evaluated
      before the publish, with no writes, because the publish needs its
      result for the stamp and an open session must be credited up to the
      boundary before it is reported. All writes follow the order above. See
      the Fix section.)
- [x] Copilot implementation (Stage 6) — 2026-09-25, `copilot` CLI 1.0.88,
      model `gpt-5.5` (reasoning effort not set; CLI default), dispatched
      from `WO-2026-09-24-001-stage6-copilot-dispatch.md` on `8e7a72f`.
      Implemented the WO with the `wasOccupied` capture (recorded as
      decision 7). Local ARM build not run by Copilot (toolchain outside its
      path allowance); run by Claude Code instead.
- [x] Codex diff review (Stage 7) — 2026-09-25, **VERIFIED** after one
      test-only fix. Review `gpt-6-astra` at reasoning high: NOT VERIFIED,
      mutation (f) survived. Test fix authorized by Chip and applied by
      Claude Code. Narrow re-verification `gpt-6-astra` at reasoning xhigh:
      VERIFIED. Both verdicts, word for word, are in
      `WO-2026-09-24-001-stage7-verdict.md`. Final: suite 43/43 (sh via zsh,
      py via python3); local ARM 150520 / 1090 / 2444; cloud 151698 / 3530.
- [x] Copilot implementation of decision 8 (Stage 6, round 2) — 2026-09-25,
      `copilot` CLI 1.0.88, `gpt-5.5` at reasoning medium, dispatched from
      `WO-2026-09-24-001-stage6-round2-copilot-dispatch.md`. No deviations
      reported. The default argument lives in `State_Common.h` only.
- [x] Codex narrow review of decision 8 (Stage 7, round 2) — 2026-09-25,
      **VERIFIED, no findings**. `gpt-6-astra` at reasoning ultra. All 12
      mutations caught: round-1 (a)–(h) and decision-8 (i)–(iv). Suite 43/43
      (sh via zsh, py via python3); local ARM 150576 / 1090 / 2444; cloud
      151754 / 3530. Verdict appended, word for word, to
      `WO-2026-09-24-001-stage7-verdict.md`.
- [ ] Chip final gate / commit (Stage 8)
