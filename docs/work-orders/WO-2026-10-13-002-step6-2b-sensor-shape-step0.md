# WO-2026-10-13-002: Step 6 WO 2b — one sensor shape (Step 0 only)

**Author:** architect, 2026-10-13 · **Status:** Step 0 (read-only); no implementation until the architect splits it
**Branch:** `wo/2026-10-13-002-step6-2b-sensor-shape`, from `wo/2026-10-13-001-step6-2a-dead-code` (`f666d40`) so the dead code is already gone
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 2–3; §12.1 (history first); §12.2–12.4; §13 (b), (c)

## Plain goal

Every sensor fits one small shape, and the hourly report is built from what the sensors supply, so a new sensor (temperature/humidity on the Muon, a counter, PM2.5) can be added without touching the report, sleep or connect code. **The webhook JSON for today's PIR occupancy and counting modes stays byte-for-byte identical.**

## The agreed design (architect and Chip, 2026-10-07; do not reopen)

- **Six operations per sensor:** start, sample, wake sources, ask for a report (and which kind), add its fields, reset daily totals.
- **Compile-time list** of sensors built into an image; the **ledger** picks which runs (`sensor.type`, `sensorMode`), with settings from `sensor.setting1–4` and the reading mode from `samplingMode`.
- **One periodic report** per reporting period carrying every sensor's fields; the event name comes from the ledger (`reporting.webhook.name`), not the sensor. Other report kinds (urgent alert, status) keep working and are out of scope.
- **Fields grouped as in device-data** (battery, environment, occupancy, system). Building device-data from the same fields is **after** Step 6, not here.
- No runtime registry, no plug-in framework.

## What exists today (verify, don't assume)

`src/sensors/ISensor.h` already declares a virtual interface (`setup`, `loop`, `getData`, `getSensorType`, `isReady`, `reset`, `initializeHardware`, `onSleep`, `onWake`, `usesInterrupt`, `isHealthy`, `lastErrorCode`); `PIRSensor` implements it; `SensorFactory.h` builds it; `publishData()` (`Generalized-Core-Counter.cpp:1965` on the 2a branch) builds the payload centrally. 2a's hand-off list: `ISensor::setup()`, `sensorDetect`, legacy `SystemConfig::get_sensorType()`, `PowerCapabilities::hasFuelGauge`, the `PowerDiagnostics` labels.

## Step 0 questions (report ≤ 150 lines on the WO branch, committed and pushed)

1. **Map** each of the six operations to today's code: where it happens now (file:line), and which `ISensor` method already covers it, partly or fully.
2. **Payload:** list every field `publishData()` writes for occupancy and counting modes, its source, and which of them are sensor fields versus common fields (battery, alerts, resets, connecttime, fh, lfb, cyc, slp, vc, timestamp).
3. **History first:** did an earlier version build the payload from the sensor, or have a fuller `ISensor`? If so, restoring it is the default.
4. **Smallest path:** propose a sequence of WOs, **each ≤ +20 net `src/` lines** (moves counted separately), each behavior-neutral with byte-identical JSON proved by a golden-output host test. Likely order: (i) golden-output test of today's payloads, test-only; (ii) sensor supplies its own fields; (iii) daily-total reset through the sensor; (iv) settings from `setting1–4`; (v) the 2a leftovers. Adjust as the code dictates.
5. **Stop and report** if any step can't be made behavior-neutral, needs a new persisted field, or the whole of 2b looks bigger than about 6 small WOs — then say what to cut.

## Agents and models

- **Step 0:** Claude Code in its own session, Sonnet tier, **high** reasoning, after a probe. *Why:* it maps interfaces and proposes a sequence; it needs care, not new design.
- **Stage 4 (independent check of the proposed sequence):** Codex `gpt-5.6-sol`, high, before the architect accepts it.

## Budget

Step 0: 0 `src/` lines. The eventual WOs: ≤ +20 net each, stated per WO when the architect accepts the sequence.
