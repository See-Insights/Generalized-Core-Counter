## A. Answer in five lines

1. Occupancy is credited exactly once, at close, as wall-clock `closeAt - occupancyStartTime`; hourly reports only read the accumulated total.
2. A :00 report neither closes/reopens the session nor changes its start, last-PIR timestamp, or LED deadline.
3. After that report/connect, the low-power sleep plan schedules a fresh full 300-second sleep instead of the remaining time to the existing debounce deadline.
4. The expired LED/debounce is therefore not noticed until that sleep returns; close then uses the late wall-clock time, making both `session=` and credited seconds late by the same amount.
5. Thus the arithmetic is correct for the firmware’s artificially prolonged session, but the 12–13 minutes are not evidence of 12–13 minutes of actual bench occupancy.

## B. Questions 1–6

### 1. Where credit is computed

The only production increment of `totalOccupiedSeconds` is in `closeOccupancySessionSafely()`:

- It reads the persisted epoch `occupancyStartTime`, not `lastOccupancyEvent`, at `src/state/State_Common.h:350-377`.
- It computes `sessionSeconds = closeAt - effectiveStart` and adds that once to the previous daily total at `src/state/State_Common.h:385-395`.
- `occupancyStartTime` is an epoch; `lastOccupancyEvent` is a `millis()` value used for debounce eligibility: `src/MyPersistentData.h:693-697`.
- The normal awake debounce test is `millis() - lastOccupancyEvent`: `src/state/State_Modes.cpp:132-147` and `src/state/State_Idle.cpp:48-61`.
- The sleep-wake close instead tests whether the retained LED deadline has expired: `src/state/State_Modes.cpp:165-174`.
- The configured debounce controls when close is attempted, but is not separately added to the total; it is already part of the start-to-late-close wall span.

All callers capable of adding credit are:

- General mode-handler close: `src/state/State_Modes.cpp:135-160`.
- Idle-state close: `src/state/State_Idle.cpp:48-73`.
- Post-sleep LED/debounce close: `src/state/State_Modes.cpp:165-183`.
- Daily-boundary close through the boundary epoch: `src/state/State_Report.cpp:54-61`.

There is no hourly-report increment or state-change “flush” increment. `publishData()` merely renders the current total divided by 60 at `src/sensors/PIRSensor.cpp:25-30`; its serial `totalMin` does the same at `src/cloud/ReportPublisher.cpp:191-197`. Daily cleanup is the only report path that closes an open session, and only when the daily boundary is due (`src/state/State_Report.cpp:40-61`).

After a valid close, `occupied` becomes false and the start is cleared at `src/state/State_Common.h:418-420`. In IKA/CONNECTED mode the close latches an occupancy-change report and transitions to Report (`src/state/State_Modes.cpp:173-182` or `src/state/State_Idle.cpp:63-73`); that subsequent payload is the observed `occ=0` report. Other connection modes wait for a later scheduled report.

### 2. What an hourly report/connect does to an open session

A representative :53 cycle is:

1. A PIR GPIO wake is classified at `src/state/State_Sleep.cpp:976-982`, then passed to `processSleepWakeEvent()` at `src/state/State_Sleep.cpp:1049`.
2. The first PIR sets `occupied=true`, stores `Time.now()` as the session start, sets `lastOccupancyEvent=millis()`, and arms the LED deadline for the configured debounce: `src/state/State_Modes.cpp:226-250`.
3. Later PIR wakes leave the start unchanged but re-arm both the LED deadline and `lastOccupancyEvent`: `src/state/State_Modes.cpp:262-271`.
4. After every PIR wake, low-power operation returns to sleep when no report is due: `src/state/State_Sleep.cpp:1074-1103`.
5. Sleep is allowed while occupied; Idle explicitly says this at `src/state/State_Idle.cpp:241-267`. PIR GPIO or the timer can wake it (`src/state/State_Sleep.cpp:755-776`).
6. Near :57:30, the last test pulse re-arms the 300-second deadline. The sleep plan also considers the nearer hourly boundary, so the :00 boundary wakes it before debounce expiry: `src/state/State_Sleep.cpp:569-590`, `:607-616`.
7. On the :00 wake, the LED is not expired yet, so the pre-routing close at `src/state/State_Modes.cpp:165-184` does nothing. The timer route enters Reporting at `src/state/State_Sleep.cpp:1058-1071`.
8. Reporting serializes `occ=1` and the unchanged daily total, sets `lastReport`, and does not touch the session: `src/state/State_Report.cpp:67-95`.
9. Because the device is occupied in low-power mode, Report records that it should return to sleep (`src/state/State_Report.cpp:208-216`) and IKA requests the connect (`src/state/State_Report.cpp:239-250`).
10. A successful connect transitions to Sleeping via `return-to-sleep-after-report`: `src/state/State_Connect.cpp:593-604`.
11. The next sleep plan sees `occupied=true` and requests a complete 300 seconds, not the roughly 150 seconds still remaining from the final PIR: `src/state/State_Sleep.cpp:607-616`.
12. When this sleep returns, the retained LED deadline is already expired. The sleep-wake close runs before ordinary timer routing, logs `Occ: state=0`, and enters Reporting: `src/state/State_Modes.cpp:165-183`, `src/state/State_Sleep.cpp:1049-1051`.
13. The next Reporting pass emits the `occ=0` payload containing the whole newly added session span.

The hourly report does not call `signalLED()`, change `lastOccupancyEvent`, or rewrite `occupancyStartTime`. The retained absolute LED deadline itself also remains unchanged (`src/device_pinout.cpp:94-113`). What is restarted is the timer wake requested by the sleep plan.

### 3. Mechanism for the extra 165–240 seconds

Let:

- `L` = final PIR time,
- `D` = 300-second debounce,
- `S` = entry into post-report sleep,
- `δ` = wake/serial processing delay before close.

Without the hourly interruption, close is approximately `L + D`. After the report, the code closes at approximately `S + D + δ`. Therefore:

`extra ≈ S - L + δ`

For a last pulse around :57:30 and post-report sleep shortly after :00, the base excess is about 150 seconds. Report, connect, cloud-gate/teardown, sleep-entry, wake latency, and enabled serial re-enumeration add the remaining variation. The serial wake path can itself wait before processing close at `src/state/State_Sleep.cpp:936-955`.

The connect is therefore contributory elapsed time, but its reported 1–6 seconds cannot account for 165–240 seconds. The dominant term is the already elapsed time since the last PIR being followed by another complete debounce-length sleep.

### 4. Does extra length equal extra credit?

Yes, at second resolution. The late close time is passed into the one close calculation, so every extra second in `session=` is an extra second added to `totalOccupiedSeconds`: `src/state/State_Common.h:377-395`.

There is no addition at the mid-session hourly report. A 734–810-second session contributes 734–810 seconds once, rather than a normal approximately 570 seconds. The hook exposes `floor(totalOccupiedSeconds / 60)`, so differences between successive integer-minute totals can vary by one minute depending on the pre-existing remainder. That explains the 12/13-minute presentation without double counting.

### 5. Bench and site dependence

This is not specific to the exerciser, but its phase makes it deterministic:

- The 20-minute cadence matters only because every third session starts near :53 and is still open at :00.
- A different phase that places :00 between sessions makes the effect absent, as on Dev-14’s observed run.
- With ten pulses 30 seconds apart, the final pulse is 270 seconds after start; adding the configured 300-second debounce gives the normal 570-second session.
- A shorter debounce that expires before :00 removes the overlap. A longer debounce makes report overlap more likely; sufficiently long sessions can cross more than one report boundary.
- Once a particular report interrupts an already-open debounce, the excess is principally the age of the last PIR when post-report sleep begins, not another independent addition of the configured debounce.
- Low-power modes expose the mechanism because they sleep with an open session. CONNECTED mode stays awake during open hours, where the `millis()` debounce checks continue running, so this sleep-reset mechanism is absent (`src/state/State_Sleep.cpp:1166-1170`).
- In IKA, start/end changes report immediately (`src/state/State_Common.h:45-58`); in INTERMITTENT/DISCONNECTED, the same late credit can occur but may not be exposed until the next scheduled report.
- A real occupant has the same exposure if their last motion precedes the hourly report and no later PIR wakes the device. A genuine PIR after the report legitimately resets the debounce and changes the expected close.

### 6. Smallest confirming read-only measurement

One direct-USB serial cycle is sufficient. Capture:

1. The final `Occ: state=1 reason=pir-wake reset=1 led=300s` before :00.
2. The :00 `Report: occ=1 ...` and `ConnSummary: ok elapsed=...`.
3. The following existing `Sleep: ULP ... reason=debounce dur=300s occ=1` from `src/state/State_Sleep.cpp:794-807`.
4. The later `WakeReturn: elapsed=... reason=TIMER` and `Occ: state=0 ... session=...` from `src/state/State_Sleep.cpp:957-982` and `src/state/State_Common.h:290-298`.

The decisive observation is a new `dur=300s` sleep after the hourly report even though the last PIR was already roughly 2½–4 minutes old. A close approximately one full sleep later confirms this mechanism and distinguishes it from a report-side session restart or double addition.

The hook fields alone show the symptom, not the sleep decision. Also, the existing `Wake: ... led=` field is unsuitable for measuring remaining debounce: `signalLEDTimeRemaining()` already returns seconds (`src/device_pinout.cpp:124-138`), but `logWakeSummary()` divides it by 1000 again (`src/state/State_Sleep.cpp:34-40`).

## C. Ranked mechanisms

1. **Strongly supported: post-report sleep schedules a fresh full debounce duration.** Directly shown by `src/state/State_Sleep.cpp:607-616`; close cannot run while blocked in `System.sleep()` and uses current wall time afterward.
2. **Secondary: report/connect/gate/teardown and post-wake serial delays add variance.** They move `S` or `δ`; they explain some spread but not the core ~150-second extension.
3. **Possible only if motion actually occurs: PIR during report/connect resets debounce.** PIR is interrupt-latched (`src/sensors/PIRSensor.cpp:7-12`) and sensor handling runs after every state-handler iteration, including Connecting (`src/Generalized-Core-Counter.cpp:1224-1234`). The supplied bench pattern has no scheduled PIR at :00, so this is not the systematic explanation.
4. **Ruled out: report directly re-arms LED/debounce.** Reporting contains no write to the LED deadline or `lastOccupancyEvent`.
5. **Ruled out: report restarts the session start.** `occupancyStartTime` is written only on an unoccupied-to-occupied transition; an already occupied PIR and an hourly report leave it unchanged.
6. **Ruled out: duplicate credit at report and close.** Only the close helper increments the total.
7. **Ruled out as the main cause: close must wait for Report/Connect state completion.** The centralized occupancy handler still runs every main-loop iteration in those states; only actual sleep prevents evaluation.

## D. Not determined from code alone

The code establishes the causal path but cannot apportion each observed 165–240-second excess among final-PIR phase, report work, connect, queue/webhook gate, teardown, actual sleep overshoot, USB re-enumeration, or an unexpected PIR. The serial sequence above supplies those timestamps.

No files were edited.