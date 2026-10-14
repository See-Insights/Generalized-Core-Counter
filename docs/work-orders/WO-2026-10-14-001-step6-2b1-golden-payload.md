# WO-2026-10-14-001: Step 6 2b-1 — golden payload test (test-only)

**Author:** architect, 2026-10-14 · **Status:** ready for Stage 6
**Branch:** `wo/2026-10-14-001-step6-2b1-golden-payload`, from current main (2a merged in #94)
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (g), (j)

## Context: 2b cut chosen (architect, 2026-10-14)

Option A from `WO-2026-10-13-002-step0-round2-report.md` (Q4): six WOs — **2b-1** golden test, **2b-2** `setup()` merge and `sensorDetect` removal, **2b-3** sensor supplies its fields with the `activeSensor()` fallback, **2b-4** wake pin and edge from the sensor, **2b-5a** reset hook (not wired), **2b-6** wire the reset. Deferred to the first new-sensor WO, where they will have a real consumer: report request and kind (2b-7), settings 1–4 and `samplingMode` delivery (2b-8a/b). Builder extraction dropped. Power leftovers 6c/6d go to a separate power WO.
Rulings on the round-2 open points: keep the alerts/resets order swap per mode exactly; pin MEASUREMENT's counting-format output; pin `temp` NaN/Inf rendering as it is today; `Config.cpp:94–101` (setting1 == 0 invalidates config) goes to the recovery plan, not 2b; PIR-wake block stays in `State_Sleep.cpp`.

## Plain goal

Pin today's webhook payload bytes, event name and publish flags with a host test that runs the **real** `publishData()` code through an observer of the actual queue call, so every later 2b WO can prove its output is byte-for-byte unchanged.

## Spec

As in the round-2 report, Q3 ("The golden test (2b-1)"): markers `static const char *batteryContext[7] = {` through the `PublishQueuePosix::instance().publish(...)` line, `EXTRACT_FAILED` if a marker is missing or not unique; observer fake asserting one call, name, bytes, length and flags `0x9`/`0`; every controlled input with a call counter; the ~90-vector matrix; host caveats (≤ 32-bit `%lu` inputs; expected bytes recorded from unchanged code and committed, never regenerated). Delete the stale mirror in `watchdog_alert_code_test.cpp:100–135`; supersede `daily_cleanup_boundary_test.py:236–245`.

## Budget

`src/` **0** lines (test-only). Tests about +300 / −40; stop and report if the test grows past +400.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **high** reasoning. *Why:* an extraction harness with many controlled inputs; exactness matters.
- **Stage 7:** Codex `gpt-5.6-sol`, **medium**. Checks: the test passes on unchanged code; mutating any single format specifier, key, the alerts/resets swap, the flags or the event name fails it; no `src/` diff.

## Bench

None (test-only).

## Closing record

Budget versus actual (test lines), vector count, mutations caught.
