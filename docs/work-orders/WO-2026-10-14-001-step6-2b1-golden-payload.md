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

## Stage 6 deviations: architect's ruling (2026-10-14)

- **Accepted:** Copilot's deviations. These are removing the two alert-in-payload tests along with the stale mirror, replacing the stamp-override structural check with golden vectors, and recording the fixture once more with its first vectors byte-identical.
- **Exception: add the SOC, battery-state, vc and event-name edge vectors in occupancy mode too.** 2b-3 merges the two format strings, so each edge case must be pinned in both layouts.
- **Test budget raised to +450.** It's a test-only change, so it stays within this round, with no new round.

## Closing record (2026-10-14)

**Result:** **VERIFIED WITH CONCERNS** at Stage 7 (`WO-2026-10-14-001-stage7-verdict.md`), with two low, documentation-only findings. **Stage 8 approved** by the architect.

| Stage | Agent / model | Result |
|---|---|---|
| Step 0 | (2b round 2, `WO-2026-10-13-002-step0-round2-report.md` Q3) | the spec |
| Stage 6 | Copilot, `claude-sonnet-5.5`, high | 129 vectors; 9 built-in mutants caught; 77/77 |
| Stage 6 addendum (same round, architect's ruling) | Copilot, `claude-sonnet-5.5`, high | +46 occupancy edge vectors (175 total); the original rows byte-identical |
| Stage 7 | Codex, `gpt-5.6-sol`, medium | VERIFIED WITH CONCERNS; re-recorded all 175 rows from clean HEAD, byte-identical; no mutant survived |

Every model was probed first (§5). One round. The Stage 7 run on the pre-addendum tree was stopped by Claude Code and re-run on the final tree.

| Item | Budget | Actual |
|---|---|---|
| `src/` / `lib/` | 0 | **0** |
| Test lines | about +300 / −40; stop past +400, **raised to +450** (architect) | **+436 / −71**, plus 2 lines in `tests/README.md` |

- **Vectors: 175**, recorded from unchanged `src/`. The fixture is never regenerated.
  - mode {0,1,2} × `stampOverride` × (min,sec);
  - alert codes in both layouts;
  - SOC × battery state, vc, and event name, **in both layouts**;
  - temp, including NaN/Inf, in both layouts;
  - widest-possible values, and a truncation sweep crossing 255/256 bytes;
  - the queue returning false.

  Each vector also asserts one publish call, flags `0x9`/`0`, the event-name pointer, length ≤ 255, and zero heap use.
- **Mutations caught:**
  - **Copilot (built into the `.sh`):** `battery` and `temp` specifiers, a renamed key, the `alerts`/`resets` swap undone, keys swapped across layouts, `PRIVATE` dropped, the event name changed, buffer 256 → 512, and a duplicated marker (`EXTRACT_FAILED`).
  - **Codex (by hand):** the vc, timestamp and `%lu` specifiers, keys in both layouts, the swap in both layouts, `WITH_ACK` dropped, the event-name source, and the occupancy divisor. **No mutant survived.**
- **Host-pinned:** the fixture was recorded on **Darwin arm64, Apple clang 14** (14.0.3). NaN/Inf text and the 64-bit host `unsigned long` are host-dependent. The fixture's recording-HEAD note (`74bff9d`) is kept as history; the extracted source is identical at the merged HEAD.
- **Superseded:** the stale payload mirror and its two tests in `watchdog_alert_code_test.cpp`, and the stamp-override split checks in `daily_cleanup_boundary_test.py`. Both are now pinned by golden vectors. `tests/README.md` was updated (Stage 7 finding 1).
- **Architect's rulings pinned:** the `alerts`/`resets` swap per mode, MEASUREMENT's counting format, and `temp` NaN/Inf rendering.
- **Bench:** none (test-only).
