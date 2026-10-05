AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/state/State_Connect.cpp`, `src/state/State_Sleep.cpp` (item A only: the RSSI calls and the three gate decisions), `src/state/State_Common.h`, `src/Generalized-Core-Counter.cpp` (item B's boot step and item D's payload only), `src/time/HibernateCycle.cpp` (item C only), plus whatever B strictly needs to read `lastReport` and the debounce in the close step; run `./bump_version.sh v37-PreStep6Fixes "<one-line note>"`; add or update tests under `tests/` (including `tests/stubs/`); run the host suite and a local ARM release build. Your scratch directory is **`build-tmp/wo20261004-001-stage6/`**: keep every temporary file in it and remove **only it** when done / Not authorized: commits, pushes, merges, branch changes, stash, reset, checkout, flashing, device settings, AWS or network access beyond the Particle compile, any edit to `lib/`, `project.properties`, `docs/` or Device OS, any new persisted field, any change beyond this WO (no breadcrumbs, timers, states, or supervisor), and **deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`**.
**SIZE BUDGET (net `src/` code lines: nonblank, non-comment, including braces, declarations and includes): A ≤ 20, B ≤ 15, C ≤ 3, D ≤ 4; about 40 total. Don't compress code to meet it (`AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3). Any item over its own budget: STOP that item and report; don't continue past it.**

# Stage 6 dispatch — WO-2026-10-04-001 (v37-PreStep6Fixes)

**Goal, in plain language:** the device never waits on the modem in its main loop or goes to sleep with the modem half off; a restart in the middle of an occupancy session doesn't lose that session's minutes; an on-time hibernate wake is reported as a success; and every report carries the battery's cell voltage.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-04-001-pre-step6-fixes`, `HEAD` `79abe84` (v36 on main). Do not switch branches. Files under `docs/` are records; don't touch them, including the unrelated uncommitted and untracked files there.
**Binding spec:** `docs/work-orders/WO-2026-10-04-001-pre-step6-fixes.md`, including its fact-check corrections and Chip's three decisions (B-timing, B-anchor, C-rule). Where this dispatch and the WO differ, the WO wins; report the difference.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO, and report any correction to the spec as a deviation. If an item can't be done as approved within its budget, stop that item and report. Leave an uncommitted working-tree diff.

## What to implement (all file:line checked against `79abe84`; details in the WO)

| Item | Where | Change |
|---|---|---|
| A1 | `State_Connect.cpp` `:390`, `:451`, `:627`, `:693` (helper `:75`) | Keep **only** the `sampleConnectionSignal()` call at `:519` (`ConnSummary: ok`). Remove the discarded read at `:390`. Remove the call at `:451` (`ConnDiag`, during acquisition). `Connect: ok` (`:627`/`:637`) logs the values `:519` already read instead of reading again. Remove the call at `:693` (timeout), so `ConnSummary: fail` logs `sig=na`. Don't leave dead branches behind. |
| A2 | `State_Sleep.cpp` `:713`, `:811`, `:893` | Under `#if Wiring_Cellular`, the non-standby "modem on" test becomes `!Cellular.isOff()` instead of `Connectivity::isRadioPoweredOn()`. Standby, log-only uses, and `:753` are unchanged. Reuse the existing wait and its timeout. |
| B1 | `setup()` in `Generalized-Core-Counter.cpp` (after persistent data loads) | Clear `lastOccupancyEvent` to 0 at boot, so the existing 0-sentinel re-arms the debounce from this boot. |
| B2 | `closeOccupancySessionSafely()`, `State_Common.h:301–371` | Untrusted clock: keep the session open (don't clear `occupied`/start), credit nothing, re-arm the debounce (`set_lastOccupancyEvent(millis())`). Make sure the callers (`State_Idle.cpp:63`, `State_Modes.cpp:146`, `State_Sleep.cpp:1635`) don't signal an unoccupied transition, or loop on `REPORTING_STATE` or `OccAnom`, when the session stayed open. |
| B3 | same function, plus a RAM-only flag set in `setup()` when `occupied` is true at boot | While the flag is set, a trusted close limits `closeAt` to `min(closeAt, Time.now() − millis()/1000, max(occupancyStartTime, SystemConfig lastReport) + debounce)`, then clears the flag. The debounce is the same value the debounce checks use. **No new persisted or `retained` field.** |
| C | `HibernateCycle.cpp:96` | Also accept `AB1805::WakeReason::DEEP_POWER_DOWN` when `v.rtcReadOk` and `rtcBefore + requested ≤ rtcAtWake ≤ rtcBefore + requested + 60`. `ALARM` is unchanged. |
| D | `publishData()`, `Generalized-Core-Counter.cpp:1952–2032` | Add `"vc":%.2f` (unquoted) to both formats, from `SensorManager::instance().cachedBatteryVoltage(vc)`, with `vc` starting at `0.0f`. |
| Version | `./bump_version.sh v37-PreStep6Fixes "<note>"` | product 37 |

## Tests (outside the budget)

1. **A:**
   - a structural test that no `Cellular.RSSI()`/`WiFi.RSSI()` (directly or through `sampleConnectionSignal()`) is reachable in `State_Connect.cpp` before the `cloudConnected` branch or on the `elapsedMs > budgetMs` path;
   - a host check of the sleep gate's decision with the modem `isOn=false, isOff=false`: no sleep, and the existing wait/timeout path is taken;
   - mutations for both.
2. **B:** a host harness against the real `closeOccupancySessionSafely()` covering:
   - a mid-session restart with a trusted close: credited to the boot time when that's earlier than `anchor + debounce`, capped otherwise;
   - `lastReport` later than the start moves the anchor;
   - untrusted: open, nothing credited, the debounce re-armed, no repeated transition;
   - a long power-off (for example, 10 h): over-credit ≤ one debounce;
   - the boot-time `lastOccupancyEvent` clear;
   - mutations (drop the cap; drop the keep-open; drop the clear).
3. **C:** extend `tests/hibernate_wake_diagnostics_test`:
   - DPD on time → `ok` with real `actual`/`err`;
   - DPD late (+61 s), DPD early (a button, −N s) → `fail`;
   - `WATCHDOG`/`UNKNOWN` → `fail`;
   - mutations (drop the window bounds).
4. **D:** `tests/report_payload_fields_test.py` (or a new test): `vc` present in both formats, unquoted; no quoted numeric key; a worst-case length computation ≤ 255 for both formats. State the max before and after.
5. **Existing tests that pin changed text:** see the WO's list. Update only what they pin, and report each change. `publish_with_ack_structural_test.py`, `sleep_config_ownership_structural_test.py` and `ledger_no_retry_test` must pass **unchanged**.
6. Run each mutation on a copy, or restore byte-identically by rewriting in place (never `mv` a backup over the source).

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N`, before (v36: 62/62) and after.
2. Local ARM release build (boron, Device OS 6.4.1, after `make clean-user`, in your scratch directory): text/data/bss against v36's 150740 / 1090 / 2180. `strings` shows `v37-PreStep6Fixes`.
3. Net `src/` lines **per item** against its budget, and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change per item, with net line counts against each budget.
- Payload maximum before and after, for both formats.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261004-001-stage6/`.
- Deviations (or "none").
- The model and reasoning level actually used.
