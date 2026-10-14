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

## Stage 6: architect's rulings (2026-10-14)

1. **Test budget raised to +180 added lines,** with no trim. The actual is +178 added / −33 removed, including the new 32-line `tests/stubs/report_golden/Particle.h`.
2. **The `tests/watchdog_alert_code_test.sh` edit is accepted.** Its literal `"alerts":%i` check became two checks on the new argument form. Stage 7 confirms it still fails if `alerts` is renamed.
3. **The invalid-type log on a closed boot is accepted.** The factory's `Sensor type %d not yet implemented` is logged once per closing report when the configured type is invalid. It goes to the recovery plan's new-sensor notes.
4. **Dispatch correction:** the Stage 6 dispatch said `createSensor()` uses `new`. It doesn't: `createSensor(SensorType::PIR)` returns `&PIRSensor::instance()`, a function-local static (`SensorFactory.h:74`). So the fallback uses no heap.

## Closing record (2026-10-14)

**Result:** **VERIFIED** at Stage 7 (`WO-2026-10-14-003-stage7-verdict.md`), with two observations and one low inference. **Stage 8 approved** by the architect. **Release:** rides v43, with 2a, 2b-1 and 2b-2.

| Stage | Agent / model | Result |
|---|---|---|
| Citation re-check | Claude Code, at main `e149123` | structure as in round 2, with lines shifted by about −1. It also found the serial `Report:` log in `publishData()`, which was left unchanged as out of scope |
| Stage 6 | Copilot, `claude-sonnet-5.5`, high | net +10 `src/`; golden 175 rows unchanged plus 11 fallback vectors; 78/78 |
| Rulings | architect | test budget +180 added; the watchdog-test edit accepted; the invalid-type log accepted; the dispatch's `new` claim corrected (the factory returns a static) |
| Stage 7 | Codex, `gpt-5.6-sol`, high | VERIFIED; original rows re-recorded from clean HEAD with zero-byte diffs; 16 + 1 mutations caught |

Every model was probed first (§5). One round; no controller edits.

| Item | Budget | Actual |
|---|---|---|
| Commit (a): field writer (`ISensor.h` +3, `PIRSensor.h` +3, `PIRSensor.cpp` +13), not yet used, plus the lifecycle test's link change | — | **+19** `src/` |
| Commit (b): `SensorManager.{h,cpp}` +9; `publishData()` +23/−41 | — | **−9** `src/` |
| **WO `src/` total** | ≤ +20 (estimate +18) | **+10** (+51/−41) |
| Tests | stop past +150, **raised to +180 added** | **+178 / −33** (including the new 32-line `tests/stubs/report_golden/Particle.h`) |

- **ARM build:** text 151308 → **151220** (−88); data and bss unchanged. `publishData` 0x390 → 0x344. Added: `PIRSensor::reportFields` (0x48) and `SensorManager::activeSensor` (0x16). `PIRSensor::instance()` and `SensorFactory::createSensor` are now out of line, so `initializeFromConfig` shrank 0xe8 → 0x80. The `PIRSensor` vtable gained one slot and is now emitted strongly. `CSWTCH.*` renumbered.

**Why the golden harness was re-pointed.** Its extraction markers and its 175 original fixture rows are unchanged. The payload region now calls `SensorManager::activeSensor()->reportFields()`, so the harness had to:
- compile the real `src/sensors/PIRSensor.cpp`;
- extract `activeSensor()` verbatim from `SensorManager.cpp` (with a marker check);
- link both against new non-static fakes and a Particle stub (`tests/stubs/report_golden/Particle.h`).

That keeps it running the **real** code. It also gained 11 fallback vectors (closed boot with a valid type, invalid types 0/3/99, and a built sensor with a bad configured type), call-count checks showing `createSensor()` isn't called while the sensor exists, and mutations re-pointed to the new argument form.

**Other test changes:**
- `report_payload_fields_test.py` now reads the common format plus `PIRSensor.cpp`'s formats, and fails if a sensor field reappears in `publishData()`;
- `watchdog_alert_code_test.sh` checks the new `alerts` argument form (ruling 2; a rename still fails it);
- `pir_sensor_lifecycle_test.cpp` gains the `CurrentReadings` link definitions that `PIRSensor.cpp` now needs.

**Observations and notes:**
- the serial `Report:` log in `publishData()` still reads the PIR getters (out of scope; soak grading parses it);
- an invalid configured type logs once per closing report (ruling 3; recovery-plan new-sensor note);
- the snapshot-timing inference (recovery-plan new-sensor note).

**Bench:** none before merge. The v43 soak checks payloads end to end on Dev-09 and Dev-14.
