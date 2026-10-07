## Implementation Report — WO-2026-10-04-001 (v37-PreStep6Fixes)

**Branch** `wo/2026-10-04-001-pre-step6-fixes` @ `79abe84`. **Nothing committed, pushed, merged or released** — the diff is left in the working tree.

### Items, changes, net `src/` lines vs budget

| Item | Change | Net | Budget |
|---|---|---|---|
| **A1** | `State_Connect.cpp`: removed 3 of 4 `Cellular.RSSI()` sites (connect-start discard, 30 s `ConnDiag`, timeout path). `Connect: ok` now reuses the `summarySig*` values already sampled by `ConnSummary: ok`; three platform `#if` variants collapsed to one valid/na pair. | **−69** | ≤20 ✅ |
| **A2** | `State_Sleep.cpp`: three non-standby gate decisions (`stillOn` :713, modem-off-complete log :815, `sleepPreconditionsSatisfied()` :897) now use `Cellular.isOff()` instead of `isRadioPoweredOn()`. Standby path and log-only uses unchanged. | **0** | — |
| **B** | RAM-only `occupancySessionCrossedBoot` in `SessionState`; sampled+cleared in `setup()`; `State_Common.h` gains `stillOpen`, shared `occupancyDebounceMs()`, an untrusted-clock early return (keep session open, re-arm debounce, no OccAnom) and the cross-boot cap `min(closeAt, bootEpoch, max(start,lastReport)+debounce)`; three `reportNow` transitions guarded with `&& !closeResult.stillOpen`. | **+14** | ≤15 ✅ |
| **C** | `HibernateCycle.cpp`: `DEEP_POWER_DOWN` accepted as alarm only when `rtcReadOk` and wake is 0…+60 s after the requested time. | **+3** | ≤3 ✅ |
| **D** | `publishData()`: cached `vc` added as unquoted `"vc":%.2f` to both payload formats. | **+4** | ≤4 ✅ |

Total net `src/`: **−48**.

### Payload sizes (worst case, incl. NUL, buffer 256)
- Occupancy **228 → 238**; counting **217 → 227**. `,"vc":5.00` is exactly 10 bytes; `cachedBatteryVoltage()` only writes `2.5 < v < 5.0` and non-NaN, so `%.2f` can't exceed 4 chars.
- *Deviation:* the WO predicted 237→247 / 224→234. The gap is the WO's more pessimistic 10-digit `%lu` assumption for `fh`/`lfb`/`dailyoccupancy`. Both fit 256 either way.

### Tests
- **New:** `tests/connect_no_modem_wait_structural_test.py` (A1 reachability), `tests/sleep_gate_modem_off_test.sh` (A2, fake modem with independent `isOn`/`isOff`, incl. alert-15/ERROR_STATE timeout and standby-unchanged), `tests/occupancy_session_restart_test.sh` (B, extracts real code verbatim, 10 cases).
- **Updated:** `hibernate_wake_diagnostics_test.sh` (1 pin changed, 3 added), `hibernate_wake_diagnostics_test.cpp` (mirror + 4 tests), `report_payload_fields_test.py` (`vc` worst case, unquoted-`%.2f`, JSON-float parse, 2 provenance checks).
- **Suite: 65/65 PASS** (was 62/62 at v36), re-run after the version bump and after scratch deletion. Interpreters: `zsh` for `.sh`, `python3` for `.py`.

### Mutations
18 mutations across A1, A2, B1–B3, B-anchor, C, D — **18/18 caught**.

### Build
`make -s PLATFORM=boron …` (Device OS 6.4.1, fresh `BUILD_PATH_BASE`):

| | text | data | bss |
|---|---|---|---|
| v36 (rebuilt on-machine from `79abe84`) | 150740 | 1090 | 2180 |
| v37 | **150788** | **1090** | **2188** |
| Δ | +48 | 0 | +8 |

+8 bss is the `occupancySessionCrossedBoot` bool plus struct padding in `SessionState`. `strings` confirms `v37-PreStep6Fixes` present and **no `pdiag`** string (v26 release rule).

### Version
`./bump_version.sh v37-PreStep6Fixes` → product 37, `FIRMWARE_VERSION = "v37-PreStep6Fixes"`, CHANGELOG/README/Doxyfile updated.

### Deviations
1. `make clean-user` is **not a valid target** in 6.4.1's `main` makefile; cleanliness achieved via a fresh rsync'd tree + fresh `BUILD_PATH_BASE`.
2. Payload maxima differ from the WO's figures (see above).
3. Item B added a shared `occupancyDebounceMs()` and refactored the two existing call sites in `State_Idle.cpp`/`State_Modes.cpp` — de-duplication required to stay inside the 15-line budget.
4. Item B added a **new** `stillOpen` field rather than reusing `valid`, so the other invalid cases (zero/future start, >86400 s) keep their existing clear-and-transition behaviour.

### Cleanup
Deleted **only** `build-tmp/wo20261004-001-stage6/`. `build-tmp/connectivity-archive/`, `build-tmp/WO-2026-10-01-001-stage6-copilot-transcript.md` and `target/` untouched. The modified `docs/work-orders/WO-2026-09-24-004-…md` and the four untracked `docs/work-orders/*.md` files were pre-existing and not touched by me.

**Model:** Claude Opus 5, medium reasoning.
