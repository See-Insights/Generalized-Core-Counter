NOT VERIFIED bf01b768e606aafd16d36ccc6224ac7a4975c2e9
1 Medium (src), 1 Low (docs)

| # | Result | Evidence |
|---|---|---|
| 1. Change | PASS | Only `src/Config.h:35` changed. Readers: `Config.cpp:147,151`; `MyPersistentData.cpp:555`; `cloud/DeviceStatusPublisher.cpp:644`; test readers at `debounce_default_test.sh:20,28-32` and `sleep_plan_equivalence_test.cpp:56`. No `lib/` readers. Stored 60000 remains 60000, including test devices. New/missing/invalid sensor storage initializes to 300000; zero uses the runtime fallback. `hasNonDefaultConfig()` would classify stored 60000 as non-default, but has no caller. |
| 2. 60000 hits | FAIL | No existing test falsely pins the compiled default: explicit-setting cases set 60000 themselves, while other hits are timing/fixture values. However, the Stage 6 classification is not complete for the literal command: 325 hits across 17 files. See Low finding below. |
| 3. New test | PASS | Compiles the real `src/Config.h`; exits 0 from repository root and `/tmp`; leaves no repository artifact. All requested mutations fail. The three greps are tied only by exact text within the relevant source file, not function scope or behavior; duplicate-text evasion is the documented structural-test limit. |
| 4. Docs | PASS | `docs/config/README.md:13,15`, `Config.h:35`, and `Config.cpp:145-151` agree. `docs/led-management-system.md:205` correctly identifies 60000 as test-device configuration. The only live-doc literal-60000 hits are those two files. The dated handover at `2026-10-22-exerciser-interface-answers.md:90,94,96` is historical and correctly left unchanged. |
| 5. Budget/build | PASS | `git diff --numstat 05f1439 -- src` = `1 1 src/Config.h`, net 0. Fresh Device OS 6.6.0 base and change builds succeeded. Both are text/data/bss `153108/1114/2240`; delta `0/0/0`. `nm` found `Config::occupancyDebounceMsForRuntime()` and `sensorConfigData::initialize()` in both ELF and objects. `objdump` showed base operands `60000` and change operands `0x493e0` (`300000`). |
| 6. Anything else | FAIL | Medium source finding: the zero-setting awake occupancy path still uses a separate 60-second fallback, inconsistent with the newly changed 300000-ms runtime fallback. |

Findings:

- **Medium, src — `src/state/State_Modes.cpp:80-103`.** With stored `setting1 == 0`, `debounceRemainingMs()` closes occupancy using the new 300000-ms fallback, but awake PIR handling arms/re-arms the LED for 60000 ms and logs a 60-second debounce. Before this change both paths agreed at 60 seconds; this change makes them inconsistent. Smallest fix: when `setting1` is zero, derive the millisecond value from `Config::occupancyDebounceMsForRuntime()` in both awake-event branches, and add a behavioral zero-setting test asserting a 300000-ms LED/debounce value.

- **Low, docs — `docs/work-orders/WO-2026-11-11-003-stage6-copilot-report.md:35-47`.** The claimed complete classification omits literal-command hits, including `debounce_default_test.sh:3`, `debounce_expiry_c1_test.sh:35`, `firmware_update_dwell_test.sh:435`, two `sleep_cloud_gate_transcript.txt` substring hits, 200 `sleep_execution_transcript.txt` hits, `pmic_fault_monitor_test.cpp:311`, and `sleep_execution_equivalence_test.sh:166,168`. These are unrelated superstring/comment values, not default-pinning tests. Smallest fix: add them to the unrelated classification.

Mutation results:

| Mutation | Result |
|---|---|
| Constant → `60000UL` | PASS: test failed, observed 60000 |
| Constant → `299999UL` | PASS: test failed, observed 299999 |
| `Config.cpp:147` → literal | PASS: invalid-config structural guard failed |
| `Config.cpp:151` → literal | PASS: zero-fallback structural guard failed |
| `MyPersistentData.cpp:555` → literal | PASS: initializer structural guard failed |

Model/reasoning: `gpt-5.6-sol`, medium.  
Interpreter: **120/120 (sh via zsh, py via python3)**.  
Cleanup: authorized scratch removed; worktree clean; HEAD rechecked unchanged.