# VERIFIED WITH CONCERNS — gpt-5.6-sol (standard tier) · reasoning: high

**Test interpreters:** 79/79 — 48 `.sh` via `zsh`, 31 `.py` via `python3`.

| Check | Verdict | Evidence |
|---|---|---|
| 1. Six `intPin` sites | PASS | HEAD `1365/1426/1457/1465/1520/1581` map to `sensorWake.pin` at current `1367/1428/1459/1467/1522/1583`. Edge-bearing sites use `sensorWake.edge`; `PIRSensor.h:127` resolves these to `{intPin, RISING}`. Zero `intPin` tokens remain in `State_Sleep.cpp`. |
| 2. PIR source/ELF equivalence | PASS | Both configs remain button/FALLING then PIR/RISING. STOP `.duration((uint32_t)wakeInSeconds * 1000UL)` remains immediately after the GPIO calls; no other chain element changed. Fresh Boron builds: HEAD **151220/1094/2212**, candidate **151308/1094/2212**—**+88 text**. `nm`: `handleSleepingState()` `0x1c80→0x1cac` (+`0x2c`); new `PIRSensor::wakeSource()` `0x24`; PIR vtable `0x40→0x44`; same-sized `CSWTCH` renames `211→215`, `214→218`, `287→283`, `289→285`, `292→288`. Named growth is 84 bytes; the remaining 4 bytes are linker alignment/padding. |
|  |  | Disassembly: HEAD ULP/STOP calls pass button with `r2=2` (FALLING), then `intPin` with `r2=1` (RISING). Candidate makes the same first call, guards against `0xff`, then passes the returned pin/edge. `PIRSensor::wakeSource()` loads `intPin` and emits edge `1`. STOP still calls `duration()` after the second GPIO call. |
| 3. Closed boot/failure paths | PASS | `SensorManager.cpp:317–320` returns `_sensor`, configured sensor, or PIR fallback. Failed hardware initialization leaves the PIR pointer installed (`:305–308`); factory failure sets null and subsequently falls back to PIR. All ULP/STOP entry paths therefore retain `{intPin,RISING}` for today’s configuration. Hibernate remains intentionally out of scope. |
| 4. Timer wake | PASS | `wakePin` is `pin_t`; `pirWake = sensorCanWake && wakePin == sensorWake.pin`. Device OS confirms `pin_t` is `uint16_t`, `PIN_INVALID=0xff`, and non-GPIO `wakeupPin()` is `0xffff`. Tests prove timer+PIR false, timer+no-source false, and PIR GPIO true. |
| 5. `pin_t` and narrowing | CONCERN | The required stored-pin `uint8_t` mutant fails, but at `sensor_wake_source_test.cpp:48`—the `0xffff` round-trip assertion—not at the functional timer decision. Stored-only narrowing leaves all timer/PIR behavior assertions green because `sensorCanWake` prevents the false match. This enforces the type rule, but not through the originally claimed wrong-wake mechanism. |
|  |  | Manual mutations: narrowing only `wakePin`, or both fields, still leaves functional `pirWake` false, but makes the earlier unguarded no-source timer label say “PIR”; `sensor_wake_source_test.cpp:50` catches this. No tested narrowing produces a wrong functional wake branch while the guard remains. |
| 6. Read once | PASS | Exactly one `activeSensor()` call exists in `State_Sleep.cpp`, at `:1362`, after all early-return gates and before ULP configuration. The initialized local dominates every use, including STOP fallback; no repeated lookup or stale/uninitialized wake-source use exists. |
| 7. Deviations | CONCERN | `State_Sleep.cpp:1428` labels solely by equality, without `sensorCanWake`. On current hardware, a no-source `0xff` wake is unreachable: real GPIOs are below 39, while every non-GPIO wake returns `0xffff`. It is log-only. Dropped mutants remain covered: default-real-pin by test line 46, narrowed `wakePin` by line 50, and hard-wired `intPin` by the zero-token/other-pin checks. |
| 8. Re-pointed tests | CONCERN | Own scratch mutations: removing the ULP sensor GPIO made the structural test fail with expected `[BUTTON, intPin]` versus actual `[BUTTON]`; substituting FALLING also failed. Restoring the `intPin` `pirWake` comparison made occupancy extraction fail. However, `sleep_config_leak_test.sh` passed both the missing-GPIO and wrong-edge mutations: it preserves its leak-ownership intent but does not independently enforce wake parity. |
| 9. Golden/suite/budget | PASS | Golden script/vectors are unchanged and passed; only its stub gained required enum/`PIN_INVALID` definitions. Full suite **79/79**. `git diff --check` passed. HEAD remains `05aa05b`; branch and origin are 0 commits apart. |

## Findings

- **LOW — test scope. Observation:** `sleep_config_leak_test.sh:69–82` derives its expected wake-source count from the mutated source, and `sleep_config_leak_test.cpp:235` compares against that derived count. Wrong or missing sensor GPIOs therefore survive. **Inference:** semantic parity remains covered by the structural test, but the leak test itself does not meet the requested wrong/missing-source mutation claim.

- **LOW — diagnostic only. Observation:** `State_Sleep.cpp:1428` can label a narrowed no-source timer value as “PIR,” while guarded `pirWake` at `:1583` remains false. **Inference:** unreachable with Device OS’s real `0xffff` timer result and current GPIO range; no functional wake-handling defect.

- **LOW — mutation semantics. Observation:** stored-pin narrowing is detected by the width assertion, while its timer-decision assertions remain green. **Inference:** the test adequately protects the mandatory `pin_t` invariant, but Copilot’s behavioral explanation for stored-only narrowing is incorrect.

## Budget versus actual

| Budget | Actual |
|---|---|
| `src/` net ≤ +20; tests ≤ +150 added; one change unit | `src/` **+22/−9 = net +13**; tests **+150/−13** exactly at the addition cap. One cohesive uncommitted patch; no commit was made because Stage 7 prohibited commits. |

The original worktree matched the start snapshot byte-for-byte before cleanup. I removed only `build-tmp/wo20261014-004-stage7/`, which I created; the pre-existing Stage 6 artifacts and Codex run log remain untouched.