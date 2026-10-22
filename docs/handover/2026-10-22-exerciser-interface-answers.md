# Reply to the exerciser handover: how GCC sees a PIR (22 Oct 2026)

Sources are the GCC firmware at main `cf0edf7` (v45). Every claim cites the code.

## 0. The main recommendation: no new sensor type for V1

**The exerciser should look exactly like a PIR.** Run the counter with `sensor.type = 1` (PIR) and plug the exerciser into the PIR's place. Then the baseline measures the **real** PIR path: the ISR, the 500 ms filter, wake-on-motion, debounce, session crediting and the reports.

A new "exerciser" sensor type would test new code instead of the code we want to trust. Adding a type takes four edits (`SensorType` in `sensors/SensorFactory.h`, the `DEFINITIONS` table in `sensors/SensorDefinitions.h`, a `SensorFactory::createSensor` case and an `ISensor` class). That is worth doing only for V2/V3, and only if the counter has to talk to the exerciser over serial or read an analog output.

## 1. Sensor type

Answered by §0. The PIR driver is `sensors/PIRSensor.h`.

## 2. Electrical interface (`PIRSensor.h:41–50, 127, 136–166`; `device_pinout.h:25–26`)

**The interrupt input** is `intPin`, carrier D13 (S2/SCK):
- It is configured `INPUT_PULLDOWN` and attached on the **RISING** edge.
- It is active-high, so the exerciser idles **low** and pulses **high**.
- The wake-from-sleep source is the same pin and the same edge (`wakeSource()`, `:127`).

**Pulse width.** There is no software minimum. The ISR only sets a flag.
- Hold each pulse high for **100 ms to 1 s**, and return low between pulses. That gives sleep-wake detection margin and resembles a real PIR.
- **Pulses less than 500 ms apart after an accepted one are ignored** (`PIRSensor.h:76–80`).

**Sensor enable** is `disableModule`, carrier D12 (S0/MOSI). It is **active-LOW**: the firmware drives it **LOW = sensor on** and **HIGH = sensor off** (`:46, :144, :163`). That matches the exerciser's EN (tied low = on). The exerciser needs no new firmware: the counter already powers the "sensor" up and down at the right times.

**`ledPower`** (D11, active-low) drives the PIR's indicator LED. The exerciser can ignore it.

**Do not use D6.** It is the Boron's own deep-sleep EN (`device_pinout.h:37`).

**To confirm on the hardware** (I can't see it from firmware): that the carrier's JST-SR sensor cable carries `intPin` to exerciser **IO1** and `disableModule` to **EN**, and what the cable's power pin is. J2 pin 2 is "not connected" on the exerciser, but J3 carries V_IN.

## 3. Occupancy timings

**The debounce is also the timeout.** It comes from `sensor.setting1` in milliseconds, with a default of **60,000 ms** (`State_Common.h:318–321`, `Config.h:34`).
- A session starts on the first accepted pulse.
- **Every pulse restarts the timer.**
- The session ends when **no pulse has arrived for one debounce period**.

**Consequence for the pattern.** With the default 60 s, pulses "under 5 minutes apart" would split a cluster into several sessions. Keep in-cluster spacing **well under the debounce** (for example a pulse every 20–30 s), or set `setting1` to the debounce you want in the ledger.

**How minutes are credited:**
- The session is credited **when it closes**, as `close time − start time`, into `totalOccupiedSeconds` (`closeOccupancySessionSafely`, `State_Common.h:332+`).
- The close time is the moment the firmware notices the expiry. Asleep, that is the wake at the debounce timer, so a session runs from about the first pulse to the last pulse + debounce + wake latency.
- **An untrusted clock credits nothing:** the session is kept open and retried (`:341–349`).

**What is reported:**
- Each report carries `occupancy` (0/1, the current state) and `dailyoccupancy`, which is `totalOccupiedSeconds / 60`, cumulative minutes for today, truncated (`PIRSensor.cpp:26–29`).
- **There is no per-hour field.** Hourly minutes are the difference between consecutive hourly reports.
- A session still open at a report is not yet in the total. It lands in the hour it closes.
- **For a clean hourly baseline, make every session close well before the top of the hour.** With a cycle that divides the hour, keep the gap (no pulses) longer than the debounce plus a margin, and place it over the hour.

**Reporting:**
- Scheduled reports go out every reporting interval (hourly) at the boundary.
- In **INTERMITTENT_KEEP_ALIVE** and **CONNECTED** modes, each session start and end also triggers an immediate report and connect (`reportsOccupancyChangesNow()`, `State_Common.h:55–58`). That is 2 extra connects per session, so a 20-minute cycle adds about 6 per hour. INTERMITTENT records changes and reports them only on the schedule.

## 4. Sleep and wake

**Open hours** (default 06:00–22:00 local):
- The device naps in ULTRA_LOW_POWER between events.
- It wakes on the PIR (`intPin` RISING), on the button, or on a timer.
- The timer is the next reporting boundary, or the debounce expiry while occupied.
- The sensor stays powered, so the exerciser stays on and its pulses wake the counter.

**Closed hours:**
- At night entry the firmware calls `onEnterSleep()`, which takes `disableModule` HIGH and turns the exerciser **off**.
- It sleeps (Boron hibernate on the AB1805 alarm, or a ULP fallback) until the next open.
- On the first open-hours wake, `onExitSleep()` takes `disableModule` LOW and turns the exerciser **on**.

**EN pin:** `disableModule` (D12). The Boron already drives it; no firmware change is needed.

## 5. Daily reset and start-up hazards

**The exerciser powers up at the first open-hours wake, not at a clock boundary.** Its pattern phase is set by that wake time, and it drifts with the ATtiny's oscillator. Free-running is fine for daily totals. For hourly totals, either:
- make the exerciser cycle tolerant of phase (sessions close before the hour, §3); or
- have it align to the hour later, in V2, over serial.

**Start-up:**
- After EN goes low, hold IO1 **low for at least 5 s** before the first pulse, so a power-up glitch isn't counted as motion.
- Start each day with a gap, not a pulse.

**The daily reset at close** (default 22:00):
- A session open at close is credited up to the close.
- Then `dailyCount`, `occupied`, the start time and `totalOccupiedSeconds` are cleared (`PIRSensor.cpp:14–22`).
- The exerciser should be in a gap at close. Its power-off follows at night entry.

**A reset during a session** caps the credit for that session at the boot time (`State_Common.h:355–360`). That is the known occupancy-lost-across-reset issue, recovery plan item 15. The exerciser will measure it, which is useful.

## Suggested V1 pattern (default 60 s debounce, reporting hourly)

**One 20-minute cycle, three per hour, started after a 5 s quiet period at power-up:**
- **Cluster:** 10 pulses, 30 s apart. Each pulse is high for 200 ms, so the cluster spans 4.5 min.
- **Gap:** 15.5 min with no pulses. The session closes about 60 s after the last pulse.

**Expected credit:** about 4.5 min + 1 min debounce + wake latency, so **about 5.5 min per session**. That is about 16–17 min per hour and about 264 min per 16-hour day.

**The expected value is computed from the pattern and the rule, not from what the firmware reports.** The rule: a session runs from the first pulse to the last pulse + debounce. The first clean days measure precision (the day-to-day spread) and the firmware's error against that expected value. They do not set the baseline, because a baseline taken from today's reports would hide today's bugs.

**Scope:** this measures the firmware's handling of PIR pulses. It does not measure the PIR's own sensing of real people, which needs ground truth from a real site.
