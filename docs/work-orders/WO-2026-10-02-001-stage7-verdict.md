**Overall: NOT VERIFIED.** A, B, D, E, the suite, builds, linkage, identity, and budgets pass. **C fails its diagnostics-only requirement.** No lasting edits were made.

**C — FAIL / NOT VERIFIED**

`MODEM_OFF` changes recovery timing indirectly through the [phase-change block](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Connect.cpp:368>). That block resets `phaseStartMs`; `phaseElapsedMs` subsequently feeds the [cloud-recovery decisions](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Connect.cpp:407>).

A compiled extraction of the actual classifier, phase accounting, and recovery blocks reproduced this:

| Identical input sequence | v31 behavior | Uncommitted change |
|---|---|---|
| Modem off at 0 s; powered but not ready at 50 s; cloud acquisition at 70 s | Recovery stage 1 fires at 70 s | Does not fire: elapsed value is only 20 s |

Both versions account for **70,000 ms** in `connPhaseCellMs`. Classification and accounting therefore pass; decision independence fails.

Every phase reader was traced: labels/logging, elapsed-time accumulation, phase-change detection and timer resets, `cloudAcquireElapsedMs`, recovery thresholds, and success/timeout summaries. The shared timer is the consequential dependency.

**Smallest fix:** treat `MODEM_OFF` and `CELLULAR_ACQUIRE` as equivalent when detecting timing boundaries, while updating the raw diagnostic phase separately. A scratch-only extraction verified that this restores existing recovery timing and accounting. Moving the existing `lastConnPhase` assignments keeps C at **+7 net lines**. Add the demonstrated transition sequence to C’s test.

| Check | Verdict | Evidence |
|---|---|---|
| **A: three-hour recovery** | **PASS — VERIFIED WITH NOTES** | Compiled extraction of the real supervisor and `Clock::openness()`, with device/time stubs. No reset at 3 h − 1 s; reset **code 2** at 3 h, using both opening-based and later-connection-based ages. |
| **A: morning and clock gates** | **PASS — VERIFIED** | No reset at 06:00 after either 22:00 close or Trail02’s 23:00 close. Closed hours, invalid time, untrusted clock, and Unknown openness do not act. |
| **A: clearing and escalation** | **PASS — VERIFIED** | Successful-connection setter/clear pair and clear function are unchanged. Clearing resets recovery state and restarts connection age. Stage 3 waits 6 h + jitter; tested both jitter endpoints. Late stage 2 defers stage 3 until the next opening + 3 h. |
| **A: mutations and test mode** | **PASS — VERIFIED** | Restoring 12 h fails the three-hour test; removing the opening base fails morning-wake testing. Mutated scratch files were restored byte-identically in place. Test timings remain **300 / 900 / 0 / 60 seconds**; prediction age, stage, and cooldown calculations follow the new rule. |
| **B: stage 1 retired** | **PASS — VERIFIED** | No supervisor radio-reset path remains. Stages 0 and persisted 1 progress to 2; no new assignment creates stage 1. Breadcrumb 9 is removed and reserved. |
| **D: memory fields** | **PASS — VERIFIED** | Both real format strings were rendered using compiled `snprintf` and JSON-parsed. `fh`/`lfb` are numbers. `runtime_info_t.size` is set before `HAL_Core_Runtime_Info`. |
| **E: sleep counters** | **PASS — VERIFIED** | `cyc` initializes to 1; all four sleep returns update it. `slp` increments only on success. Mixed success/failure tests preserve `cyc ≥ slp`. ARM disassembly confirms all four calls; counters occupy ordinary `.data`/`.bss`, not retained storage. |

**Deviation rulings:** Keeping alert **45 is correct**: the unchanged clear path still consumes legacy persisted alerts.

The **24-hour configuration consequence is reachable**: existing validators accept equal hours throughout 0–23. With `open=close=6`, stale recovery first becomes eligible at 09:00; with equal hours **21, 22, or 23**, three hours never accumulate before the daily base advances, disabling this failsafe. This is recorded as the approved rule’s consequence, with no redesign proposed. Local evidence does not establish a currently deployed equal-hours device.

Payload sizes are conservative worst-case bounds, **including NUL**:

| Format | Before | After | Buffer |
|---|---:|---:|---:|
| Occupancy | 169 | **228** | 256 |
| Counting | 158 | **217** | 256 |

All four new fields parse as unquoted numbers; **`key1` remains the only string-valued key**.

**Suite and toolchain — PASS / VERIFIED:** **56/56**, comprising 28 shell tests through **zsh** and 28 Python tests through **python3**. `publish_with_ack_structural_test.py` is unchanged and green. The daily-cleanup test changes only accommodate the helper rename/export and preserve its intent.

Both local Boron / Device OS 6.4.1 builds passed:

| Build | Text | Data | BSS | `.bin` bytes |
|---|---:|---:|---:|---:|
| Release | **150612** | **1090** | **2204** | 151706 |
| Failsafe test mode | **152180** | **1090** | **2204** | 153274 |

`nm` confirms user-object references to the runtime-info, opening-boundary, openness, and modem-power paths. Sleep-call relocations confirm counter updates. Firmware identity is **v32-RecoveryVisibility**, with product version **32**, also verified in the ELF.

Using nonblank, non-comment source lines, including braces, declarations, and includes:

| Item | Net lines | Budget |
|---|---:|---:|
| A | +6 | ≤8 |
| B retirement | −13 | Negative |
| B build fix | +1 net; 2 lines touched | 2-line fix |
| C | +7 | ≤8 |
| D | +5 | ≤10 |
| E | +12 | ≤12 |
| **Total** | **+18** | About 45 |

**Preservation confirmed:** after deleting only `build-tmp/wo20261002-stage7/`, the final inventory/hash comparison found **0 added, 0 removed, 0 changed entries** across 125,048 original entries. The working tree is byte-identical, including all **120,584 archive files**. Nothing outside the authorized scratch directory was created or deleted.

**Model:** gpt-6-astra. **Reasoning:** high.