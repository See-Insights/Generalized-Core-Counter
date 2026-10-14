# WO-2026-10-14-003: Step 6 2b-3 — the sensor supplies its own report fields

**Author:** architect, 2026-10-14 · **Status:** ready (citation re-check first)
**Branch:** `wo/2026-10-14-003-step6-2b3-sensor-fields`, from main after the 2b-2 PR merges
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-13-002-step0-round2-report.md`, Q2 "2b-3" (citations at `25dbc69`; re-check at the new base, they will have shifted after 2a/2b-2).

## Plain goal

`publishData()` stops knowing which sensor fields exist: the active sensor writes its own part of the report (for PIR: `occupancy`/`dailyoccupancy` or `hourly`/`daily`), and the common part (battery, vc, key1, temp, alerts, resets, connecttime, fh, lfb, cyc, slp, timestamp) stays central. **All 175 golden vectors stay byte-identical.**

## Design (from the round-2 report; do not widen)

- `ISensor::reportFields(char* out, size_t len, uint8_t sensorMode)` (exact signature per Step 0) with a default that writes nothing; `PIRSensor` implements it, integer-only output, ≤ 40 bytes.
- `SensorManager::activeSensor()`: `_sensor` if built; otherwise the factory for `SystemConfig::SensorSettings::get_sensorType()`; otherwise PIR — so the closing report after a closed boot (`_sensor == nullptr`) still produces today's bytes.
- `publishData()` keeps **one** `snprintf` into the same 256-byte buffer: the sensor prefix is a `%s` argument, and the alerts/resets key names are arguments so each mode keeps its current order. Getter evaluation order unchanged.
- No heap use in the report path (the golden test's allocation counter must stay at zero).

## Not in scope

Device-data ledger, report request (2b-7), settings (2b-8), any change to field names, order or formats, MEASUREMENT behavior.

## Budget

Net `src/` **≤ +20** (estimate +18). Two commits: (a) `ISensor` + `PIRSensor` (field writer, unused); (b) `SensorManager::activeSensor()` + `publishData()` switch-over. Tests separately; stop past +150 test lines.

## Tests

- The 2b-1 golden test passes **unchanged** after (a) and after (b). Re-point it only if the extraction markers must move; any fixture change is a stop-and-report.
- New vectors for the fallback: `_sensor == nullptr` with a valid type and with an invalid type; call-count asserts showing each getter is read exactly as before.
- Re-point `report_payload_fields_test.py` and retire `daily_cleanup_boundary_test.py:236–245`'s remainder if still present (per the round-2 report).

## Agents and models

- **Citation re-check:** Claude Code at the new base; stop if any cited structure differs from the round-2 report.
- **Stage 6:** Copilot, Sonnet tier, **high**. *Why:* this is the payload path; exact formatting and evaluation order matter.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. Checks: golden unchanged at both commits; fallback vectors; zero heap; no remaining PIR knowledge in `publishData()` (search for `occupancy`, `hourly`, `daily` keys there); ELF size change explained; `createSensor()` not called per report when `_sensor` exists.

## Bench

None before merge; the v43 soak checks payloads end to end on Dev-09/Dev-14.

## Closing record

Budget versus actual per commit; any golden-test re-pointing and why.
