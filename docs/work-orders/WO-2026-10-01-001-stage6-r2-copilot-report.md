<!-- Copilot claude-opus-5 (medium) Stage 6 round 2 Implementation Report, extracted verbatim from its --share transcript by Claude Code, 2026-10-01. Claude Code checked: B, C, D src/ lines byte-identical to round 1; archive file count unchanged (120584). -->

# Implementation Report — WO-2026-10-01-001 item A, round 2

## Files and lines changed (item A only)

| File | Change |
|---|---|
| `src/Generalized-Core-Counter.cpp` | Replaced round-1 state (`firmwareUpdateBeginPending`, `firmwareUpdateLastEvent`) with `volatile bool firmwareUpdateInProgress` + kept `firmwareUpdateLastActivityMs`; handler now sets on begin(0), clears on complete(1)/failed(−1), stamps `millis()` on begin/progress(2); **removed** the loop-level begin block. `System.on(firmware_update, firmwareUpdateHandler)` kept. |
| `src/state/State_Sleep.cpp` | `extern volatile bool firmwareUpdateInProgress;` + the gate check inside `handleSleepingState()` (line ~479): `if (!disconnectRequested && firmwareUpdateInProgress) { cloudSyncStartMs = 0; transitionTo(FIRMWARE_UPDATE_STATE, "firmware update in progress"); return; }` — placed after the `enteredState`/latch-safety resets and **before** the cloud-ops gate and all three teardown requests (now 768 / 783,787 / 909,917,927). |
| `src/state/State_Connect.cpp` | Externs switched to `firmwareUpdateInProgress`/`firmwareUpdateLastActivityMs`; **removed** the complete/failed→Idle block; added flag-clear exit → `SLEEPING_STATE` ("firmware-update-not-in-progress"). Kept: restarting 5-min no-progress exit, `markProgress("OTA_PROGRESS")` on each new activity, button override, config-load block, 3-line webhook-ack hold. `State_Connect.cpp:651–652` entry unchanged. |
| `src/ThrashGuard.cpp` | Unchanged from round 1 (330 s). |
| `src/power/ConnectivityPolicy.h` | Unchanged from round 1 (comments only). |

**Per-cycle static reset (reported per dispatch §3):** leaving mid-gate requires clearing `cloudSyncStartMs`, the one gate static the `enteredState` block does not reset; it uses the handler's existing `cloudSyncStartMs = 0;` reset pattern so a later return to sleep starts its gate cleanly.

## Item A line count (nonblank, non-comment, vs v30 `9e10778`)

`Generalized-Core-Counter.cpp` +10 · `State_Sleep.cpp` +6 · `State_Connect.cpp` +11 (externs +2; `handleFirmwareUpdateState()` 36→45) · `ThrashGuard.cpp` 0 · `ConnectivityPolicy.h` 0 → **net +27** against the ~30 budget (round 1 was 29). B, C, D hunks verified byte-identical to round 1.

## Tests

Rewrote `tests/firmware_update_dwell_test.sh` (compiled extraction of the real handler, the real sleep-gate check, and the real dwell handler) and `tests/firmware_update_wiring_structural_test.py` to the round-2 design, including all three Stage 7 round-1 reproductions (begin between loop passes → no teardown; no re-entry after the download ends; stale terminal event does not cut short a later dwell).

| Mutation | Dwell (zsh) | Structural (py) |
|---|---|---|
| Sleep check moved after first teardown | pass | **FAIL (caught)** |
| Sleep check removed | **FAIL (caught)** | **FAIL (caught)** |
| Fixed, non-restarting 5-min timer | **FAIL (caught)** | pass |
| `markProgress` removed | **FAIL (caught)** | **FAIL (caught)** |
| Update-state ThrashGuard timeout 180 s | pass | **FAIL (caught)** |

All five caught; all three sources restored and verified by SHA-256.

## Verification

1. **Host suite: 51/51** — 25 `tests/*.sh` via **zsh**, 26 bare `tests/*.py` via **python3**. `tests/publish_with_ack_structural_test.py` unchanged and green.
2. **ARM build** (boron, Device OS 6.4.1, release, after a successful `make clean-user` via buildscripts 1.17.2): **text 150420 / data 1090 / bss 2204** — vs v30 150164/1090/2196 (**+256 / 0 / +8**), vs round 1 150444/1090/2212 (**−24 / 0 / −8**). `strings` found `v31-ConnectivityFixes`, `firmware update in progress`, `firmware-update-not-in-progress`; no `pdiag`. `target/` was backed up and **restored byte-for-byte** afterwards (`arm-none-eabi-size` re-reads 150444/1090/2212).
3. **Linkage:** `nm` → `firmwareUpdateHandler` at `0x000b53bc`, `handleFirmwareUpdateState()` at `0x000c09d0`, `firmwareUpdateInProgress` at `0x2003d8c0`. `setup()` passes handler pointer `0x000b53bd` with event mask **256** to `SystemClass::on`. `handleSleepingState()` at `0xc26f4` loads `0x2003d8c0`, compares, and branches into the transition — the check is in the image.
4. Scratch: **only `build-tmp/wo20261001-stage6-r2/` was created and deleted.** `build-tmp/`, `build-tmp/connectivity-archive/` and the round-1 transcript are intact. No commits, pushes, branch changes, flashing, or network/device access.

## Deviations

1. **`tests/loop_stage_sleep_prep_exclusion_test.py` edited** (outside the two named item-A test files). It pins an exact `transitionTo(` count inside `handleSleepingState()`; item A's required check makes it 16, not 15. Changed `EXPECTED_TRANSITION_CALLS` 15→16 and its comment/messages only. Without it the suite is 50/51. No other legacy test was touched (`tests/nightly_heap_guard_flush_test.sh` is byte-identical to round 1).
2. **`configLoadedInUpdateMode = false;` not carried into the new exit.** Round 1's terminal block set it; the state-entry block already resets it, and Stage 7 round 1 flagged it as redundant. Dropped (−1 line).

Otherwise none.

## Model / reasoning

**claude-opus-5, reasoning medium** — as dispatched.

---

