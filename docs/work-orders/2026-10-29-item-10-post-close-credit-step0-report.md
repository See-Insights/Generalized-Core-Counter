## A. Answer in five lines

1. `dailyoccupancy` is only `totalOccupiedSeconds / 60`; the sole normal increment is `closeOccupancySessionSafely()`, while the close cleanup resets it to zero.
2. After cleanup, any PIR session closed while the device remains awake adds to that zeroed counter, so it survives as the next reporting-day total; the opening wake performs no second reset.
3. A session spanning close is not handled uniformly: if cleanup sees it open, it credits only through the boundary and ends it; if it closes before delayed cleanup, its entire duration enters the old counter and is then reset.
4. History shows boundary splitting was deliberately implemented only for always-open `0/24` operation; normal park-hours occupancy was specified to end at close because occupancy was intended to count only during open hours.
5. Thus the observed credit is mechanically attributable to the next reporting day, but it was not the documented park-hours design intent; choosing to ratify it is treatment (c), not a statement of current intent.

## B. Findings

### 1. Increment, reset, and boundary order

`dailyoccupancy` is not separate storage: the PIR report renders `totalOccupiedSeconds / 60` at `src/sensors/PIRSensor.cpp:25-29`.

The only production addition is:

- `previousTotal + sessionSeconds`, then `set_totalOccupiedSeconds(newTotal)`: `src/state/State_Common.h:361-395`.
- Normal duration is trusted wall-clock `closeAt - occupancyStartTime`: `src/state/State_Common.h:333-340,350-378`.
- The four production callers are:
  - Idle debounce: `src/state/State_Idle.cpp:45-64`.
  - Central awake debounce: `src/state/State_Modes.cpp:127-150`.
  - Post-sleep debounce: `src/state/State_Modes.cpp:165-175`.
  - Daily boundary close: `src/state/State_Report.cpp:54-61`.

Zeroing paths are:

- Normal daily cleanup: `dailyCleanup()` calls the central non-sensor reset, then the PIR daily reset: `src/state/State_Report.cpp:306-328`.
- PIR daily reset clears `dailyCount`, `occupied`, `lastOccupancyEvent`, `occupancyStartTime`, and `totalOccupiedSeconds`: `src/sensors/PIRSensor.cpp:14-22`.
- Full initialization/factory-invalid-store reset clears the same occupancy fields: `src/MyPersistentData.cpp:642-660,721-726`.
- Validation independently clears an impossible total above 24 hours: `src/MyPersistentData.cpp:674-680`.
- No normal close zeroes the accumulated total; it clears only `occupied` and `occupancyStartTime`: `src/state/State_Common.h:400-420`.

Boundary order is explicitly: determine due → close an open session at `boundary` → publish the closing record → daily reset → restart only when `close == 24` → stamp cleanup: `src/state/State_Report.cpp:37-87`. The boundary query requires a trusted clock and uses the most recent configured local close: `src/time/DailyBoundary.cpp:26-45`.

Consequences of ordering:

- Cleanup first, later PIR close: the new session adds to zero and becomes next-day occupancy.
- Session still open when cleanup runs: it is credited only to `boundary`, then ended and reset for normal close hours.
- Session already closed after the boundary but before cleanup: the ordinary close used `Time.now()`, so its complete span is in the old total; cleanup publishes that total and erases it.
- PIR starts after the boundary but before delayed cleanup: cleanup closes it with `closeAt=boundary`; because start is later than `closeAt`, the helper clamps start back to the boundary and credits zero, then resets it: `src/state/State_Common.h:369-377`.
- A pending PIR interrupt survives the daily PIR reset (`resetTotals()` deliberately does not clear `_motionDetected`), so it can be consumed later in the same main-loop pass and immediately open a new session: `src/sensors/PIRSensor.h:114-120,132-133`; this is pinned by `tests/pir_sensor_reset_totals_test.cpp:63-68`.

### 2. Awake-after-close and sleep behavior

Every main-loop state handler runs before centralized sensor servicing: `src/Generalized-Core-Counter.cpp:1124-1160,1224-1234`. Therefore PIR sampling and `updateOccupancyState()` continue while awake in Idle, Sleeping/preparation, Reporting, Connecting, FirmwareUpdate, and Error, provided the sensor remains ready.

The interrupt-driven PIR is checked every loop and its 500 ms hardware-event debounce is applied there: `src/sensors/SensorManager.cpp:317-330`; `src/sensors/PIRSensor.h:66-86`. The occupancy debounce runs at the end of every `handleOccupancyMode()` call: `src/state/State_Modes.cpp:63-116`.

Long awake paths include:

- Connect: normal/deep budgets are 5/11 minutes; CloudRecover stages occur at 60/120 seconds: `src/state/State_Connect.cpp:202-226,249-285,379-405`.
- Report followed by Connect, including the forced daily-close connect: `src/state/State_Report.cpp:218-250`.
- Sleep’s publish/ledger/webhook gate and modem teardown: `src/state/State_Sleep.cpp:1177-1241`.
- Firmware update and Error reset-delay states.
- The connectivity failsafe itself does not keep the device awake: it defers to an in-budget Connect, then stage 2 resets or stage 3 powers down: `src/power/ConnectivityFailsafe.cpp:278-314`.

A short open-hours ULP sleep can cross the close with a session still open: the wake is aligned just after the reporting boundary, and occupancy may shorten it to the remaining debounce: `src/state/State_Sleep.cpp:551-590,607-620`.

It should not enter the long overnight sleep with an unprocessed close: the Closed branch first redirects to Reporting when cleanup is due, before powering down the sensor or computing the next-open sleep: `src/state/State_Sleep.cpp:491-520`. This is pinned by `tests/close_before_night_sleep_test.py:105-152`.

After cleanup, a newly opened post-close session can delay hibernate by one debounce sleep. Closed sleep powers the PIR down, but the occupied branch shortens the requested duration; Boron hibernate then fails its 900-second minimum and falls back to ULP: `src/state/State_Sleep.cpp:512-520,607-620,689-756`; `src/time/HibernateEligibility.cpp:54-73`. Once debounce closes, the next pass can take the full night sleep.

### 3. Which day receives credit; opening-wake persistence

For a pre-close start and post-close end:

- If the boundary report sees it open, only start→boundary is credited to the closing day; boundary→end is discarded for normal park hours.
- If the ordinary debounce closes it before delayed boundary processing, start→actual close is credited to the closing report and then erased.
- Always-open `0/24` is different: after reset, the session is restarted at `max(boundary, originalSessionStart)`: `src/state/State_Report.cpp:77-82`; valid `0/24` hours are defined at `src/Config.h:40-55`.

For a session starting and ending after cleanup, the full trusted epoch span is added to the zeroed counter, hence the next reporting day. Local date is not consulted during the close.

The occupancy deadline is millis-based, but credited duration is epoch-based. `occupancyStartTime` and the total are fields in persistent `/usr/current.dat`: `src/MyPersistentData.h:674-680`. Across a reset, setup captures a RAM-only boot anchor and clears only the stale millis timestamp: `src/Generalized-Core-Counter.cpp:493-505`; `src/state/StateMachine.h:45-70`. The first trusted close is capped at the earliest of explicit close, boot epoch, and anchor plus one debounce: `src/state/State_Common.h:352-360`.

At the 22:00Z opening wake for a 06:00 SGT opening, nothing zeroes occupancy totals. Setup loads the persistent record, captures the anchor if occupied, and clears only `lastOccupancyEvent`. Since the prior close already stamped `lastDailyCleanup`, the opening report is not another daily reset. Therefore a carried-in value such as five minutes can survive and appear in the first morning report.

### 4. Existing test coverage relevant to treatments

- Boundary calculation, close/publish/reset order, and always-open restart: `tests/daily_cleanup_boundary_test.py:127-224`.
- Explicit `closeAt`, trusted-clock behavior, cross-boot cap, and item 15 cases: `tests/occupancy_session_restart_test.sh:288-394,473-527`.
- Exact daily PIR reset fields and pending-ISR survival: `tests/pir_sensor_reset_totals_test.cpp:44-68`.
- Full versus sensor-owned reset wiring: `tests/reset_wiring_test.cpp:100-119`.
- Sleep plan and occupied debounce behavior: `tests/sleep_plan_equivalence_test.sh:4-21`, fixture `tests/fixtures/sleep_plan_transcript.txt`.
- PIR wake/close routing: `tests/sleep_wake_equivalence_test.sh:4-14`, fixture `tests/fixtures/sleep_wake_transcript.txt`.
- Close-before-night-sleep invariant: `tests/close_before_night_sleep_test.py:105-152`.
- Queue publishes retain `WITH_ACK`: `tests/publish_with_ack_structural_test.py:1-30`.

There is no combined test covering “daily cleanup, then PIR start/end while still awake” or “ordinary session closes after boundary before delayed cleanup.”

### 5. Orphan `occ=0`

An `occ=0` report does not prove that an `occ=1` report was built:

- In INTERMITTENT or DISCONNECTED mode, occupancy changes intentionally do not report immediately: `src/state/State_Common.h:44-58`; `src/state/State_Modes.cpp:83-93,146-160`.
- In KEEP_ALIVE/CONNECTED mode, a start during Connect merely sets the boolean flag; only Idle immediately transitions to Reporting: `src/state/State_Modes.cpp:83-93`.
- If the session closes before Report executes, the same boolean remains set, but `publishData()` snapshots current state—now zero. Report consumes the flag once at `src/state/State_Report.cpp:72-75`; payload fields are read later at `src/cloud/ReportPublisher.cpp:95-117`.
- Thus start and end can coalesce into one `occ=0` report without any dropped queue event.

If serial contains `Report: occ=1 ... ok=1`, then an occ=1 payload was built and accepted by the local queue: `src/cloud/ReportPublisher.cpp:122-137,191-197`. The queue retries failed acknowledged cloud publishes rather than deleting them: `lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:343-389`. A missing downstream hook can still be a cloud-integration/webhook issue; the code alone cannot distinguish that from absent field evidence.

This does not affect credit: `totalOccupiedSeconds` is updated locally before any report or delivery outcome.

### 6. Smallest read-only bench measurement

Use one controlled run with the existing firmware and logs—no instrumentation:

1. Force/observe a Connect spanning close.
2. Start one PIR session just before close and let it end after close but before cleanup.
3. After `Daily boundary reached ...`, run one complete post-close session.
4. Capture existing lines: `Occ: state=1`, `Occ: state=0 ... session=... total=...`, `Daily boundary reached`, `Report: occ=... totalMin=...`, `StateReq`, and `Sleep: HIBERNATE`/`Sleep: ULP`.
5. Read the first opening report.

This distinguishes: split/discard by zero post-boundary credit; ignore by absence of post-close `Occ` lines; deliberate new-day accounting by preserving both the crossing tail and later session; current behavior by losing/misassigning the crossing tail while retaining only sessions credited after cleanup.

## C. Candidate treatments

| Treatment | Required code surface | Hibernate / item 15 risk | Size |
|---|---|---|---|
| (a) Boundary-aware split/discard | Teach `closeOccupancySessionSafely()` or all three ordinary-close callers to clamp known-Closed sessions at `DailyBoundary`; adjust `State_Report.cpp`. Extend daily-boundary and restart/cap tests. | Hibernate generally improves because sessions end at close. Medium item-15 risk: this changes the helper containing the boot cap and explicit `closeAt` ordering. | Medium |
| (b) Ignore PIR after close | Gate centralized occupancy processing at `Generalized-Core-Counter.cpp:1224-1234`, or power down/gate the PIR from the close cleanup through opening; preserve closure of any pre-close session. Update sensor lifecycle, sleep-wake fixtures, and close-before-sleep tests. | Lowest direct boot-cap impact if existing sessions still close at boundary. Must ensure sensor re-enable at opening and not suppress the final pre-close debounce. Favors prompt hibernate. | Small–medium |
| (c) Deliberately credit the new day | Preserve/restart the after-boundary portion through daily reset and handle sessions that close before delayed cleanup. This likely needs day attribution or a pending-new-day accumulator, touching `State_Common.h`, `State_Report.cpp`, PIR reset semantics, and possibly persistent layout. | Highest item-15 interaction: new cross-reset attribution must coexist with the boot cap. Keeping occupancy open can defer hibernate; closing and separately booking the tail avoids that but needs more state. | Large |
| (d) Record current behavior | No firmware change; document the reporting-day definition and bench/exerciser consequence, and make analysis use the closing record rather than the last post-close hook. | Existing debounce delay to hibernate and item-15 behavior remain unchanged. | Extra-small/docs |

History evidence: `git log -S` identifies `88721ff` as the original accumulator/helper, `599038e` as the boundary-aware close, `897f858` as the PIR reset hook, and `bbed065` as the boot-cap change. `git log -L` shows `599038e` introduced the exact close→publish→reset→conditional-restart sequence. The approved rationale explicitly says normal occupancy ends at close, while only always-open operation restarts the remainder: `docs/work-orders/WO-2026-09-24-001-daily-cleanup-boundary-fix.md:19-36,97-115,324-350`.

## D. What code alone cannot show

- Whether either field device was actually in Connect, sleep teardown, or another state for every PIR pulse.
- Whether Dev-14’s missing `occ=1` was never built, locally rejected, cloud-published but integration-lost, or omitted from the available evidence.
- Exact session timing when serial forwarding drops lines.
- Whether the real-site product policy should exclude closed-hours presence or treat close as the start of the next reporting day.
- Whether changing attribution would materially increase hibernate delay under real, rather than exerciser, traffic.

The source-only conclusion is firm: the five-minute carry can survive the morning wake, and no opening-wake reset removes it.