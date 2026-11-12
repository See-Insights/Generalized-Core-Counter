# WO-2026-11-11-003 Stage 6 report (Copilot)

**Model/level:** claude-sonnet-5.5. The dispatch asked for low effort; I cannot observe the level actually applied.
**Interpreter:** zsh 5.9 (arm64-apple-darwin26.0), Python 3.14.6. Device OS 6.6.0 (`project.properties`), boron, gcc-arm 10.2.1, buildscripts 1.17.2.
**State:** changes are uncommitted. Nothing was committed, pushed or flashed.

## Baseline (before editing)
- Tests: **119/119 (sh via zsh, py via python3)**.
- Base build (`git archive HEAD`, fresh `BUILD_PATH_BASE`): text 153108 / data 1114 / bss 2240.
  The make run ends with `flash-all Error 2` after the ELF and size steps, because no device is attached. It is not a build failure.

## Step 0: readers of `DEFAULT_OCCUPANCY_DEBOUNCE_MS` (`git grep`, src/lib/tests; `docs/html` is generated and excluded)
| Where | Use |
|---|---|
| `src/Config.h:35` | definition (60000UL before, 300000UL now) |
| `src/Config.cpp:147` | `occupancyDebounceMsForRuntime()`: fallback when `!isValid(false)` |
| `src/Config.cpp:151` | same function: fallback when stored `setting1 == 0` |
| `src/MyPersistentData.cpp:555` | `sensorConfigData::initialize()`: initial `setting1` for a new or reset device (function at 549-558) |
| `src/cloud/DeviceStatusPublisher.cpp:644` | `Cloud::hasNonDefaultConfig()` |
| `tests/sleep_plan_equivalence_test.cpp:56` | test stub returns the constant |

- **`DeviceStatusPublisher.cpp:644`:** `hasNonDefaultConfig()` returns true if `sensorSetting1 != DEFAULT`, among other fields.
  `git grep` finds only its definition (`DeviceStatusPublisher.cpp:641`) and its declaration (`Cloud.h:323`), so nothing in `src/` calls it.
  The change alters no published field today.
  If it were called, a device holding 60000 would now count as non-default. That is the intended consequence of the ruling.
- No reader treats a stored 60000 as "default" by value. All readers compare against or fall back to the constant.
  A stored 60000 stays 60000. The default only applies to new, reset, invalid-config or zero cases.
- No unexpected behaviour, so no stop.

## Classification of `git grep -n 60000 -- tests`
- **Default debounce:** none. No test pinned the default, and the suite stayed 119/119 with the constant at 300000.
- **Fixture recording the default:** none.
  `sleep_plan_equivalence_test.cpp:56` uses the constant by name, and `fixtures/sleep_plan_transcript.txt` has no 60000.
- **Explicit `setting1` test input (not the compiled default):**
  - `debounce_expiry_c1_test.sh:42-52` sets `set1 = 60000` itself and expects 60000.
  - `exerciser_baseline_test.py:66,108` pass `--debounce-ms 60000` explicitly.
  - `debounce_awake_pir_test.sh:77,78,80` (round 2 numbering; the "explicit" checks of the awake first, awake re-arm and sleep first blocks) set `setting1 = 60000` as an explicit input and expect 60000 back. Not the compiled default. Round 3 rewrote the file; the same explicit input is now the `60000` entry of the matrix.
- **Unrelated** (durations, timestamps, LED remaining, awake ms, cloud-gate times):
  - `clock_trust_test.cpp:71`
  - `conn_phase_modem_off_test.sh:320`
  - `firmware_update_dwell_test.sh:408`
  - `occupancy_report_by_mode_test.cpp:468,549,728,732,755,768`
  - `sleep_cloud_gate_equivalence_test.cpp:222`
  - `sleep_wake_equivalence_test.cpp:300`
  - `untrusted_piece_c2_test.sh:211`
  - `webhook_timeout_range_test.py:4` (a comment)
  - Added in round 2 (superstrings, arithmetic or comments; none pins the default):
    `debounce_default_test.sh:3` (comment), `debounce_expiry_c1_test.sh:35,52` (`3600000` fake, `60000 - 396`),
    `firmware_update_dwell_test.sh:435` (`600000`), `pmic_fault_monitor_test.cpp:311` (`3600000UL`),
    `sleep_execution_equivalence_test.sh:166,168` (`dur=3600000`),
    `sleep_cloud_gate_transcript.txt` (2 substring hits), `sleep_execution_transcript.txt` (200 substring hits)
  - fixtures `sleep_cloud_gate_transcript.txt:2079,4699`, `sleep_preflight_transcript.txt` (`awake=60000`, many lines) and `sleep_wake_transcript.txt` (`led=1/60000`, 8 lines)

## Failing-before evidence
Nothing pinned the default, so no existing test failed after the change.
I added `tests/debounce_default_test.sh`. It compiles the real `Config.h` on the host with a stub `Particle.h` and requires 300000.
It also greps that `Config.cpp` (both fallbacks) and `MyPersistentData.cpp` still use the constant.
With the constant at 60000 it printed `FAIL: default debounce is 60000, expected 300000` (rc=1).
At 300000 it prints `PASS` (rc=0).

## `src/` net
`git diff --numstat -- src`: `1 1 src/Config.h`. That is +1/-1, net 0, and nothing else in `src/` changed.

## Mutation
I backed up `Config.h` with `cp`, set the constant to 60000, and ran the test. `tests/debounce_default_test.sh` failed (above).
I restored with `cp`, and it passed again.

## Verification
- Full suite after editing: **120/120 (sh via zsh, py via python3)**. That is 119 plus the new test.
- Change build (fresh `BUILD_PATH_BASE`, `git archive HEAD` plus the modified `Config.h`): text 153108 / data 1114 / bss 2240.
  It is identical to the base, delta 0/0/0. The constant is a literal operand either way.
- I claim nothing about hardware.

## Docs
**Changed:**
- `docs/config/README.md`: the setting1 row already said "production ledger value 300000; 60000 on test devices only, Chip's rule".
  The trailing line is now "The firmware's compiled default and fallback when `setting1` is missing or zero is 300000 (`Config.h:35`, `Config.cpp:145-151`)".
  Both citations were verified on this head: `Config.h:35` is the constant, and `Config.cpp:145-151` is the body of `occupancyDebounceMsForRuntime`.
- `docs/led-management-system.md:205`: now reads "setting1 = 60000ms = 60 seconds, a test-device value; the compiled default is 300000ms".

**Left unchanged:**
- `docs/reference/alert-codes.md`: no 60000 and no debounce default cited, so nothing to change.
- `docs/html/*` (generated; cites the old line numbers).
- `docs/work-orders/*` (historical records).
- `docs/handover/2026-10-22-exerciser-interface-answers.md:90,96`: "default 60 s debounce" in a dated handover. It is historical and its text does not literally match 60000, so I left it. Flag it if it should be live.

## Deviations / open questions
- I added one new test file; the WO allowed "new or existing".
- **The new test's greps are structural.** They check the source text of the readers rather than running them.
- **Scratch:** my first base copy sat under `build-tmp/` and tripped `thermal_coupling_structural_test.py`, which scans for source files.
  I deleted my own copy (and the later `new/` copy). `build-tmp/wo-2026-11-11-003-stage6/` now holds only my logs, objs and test work files.
- `hasNonDefaultConfig()` has no caller. The architect may want to know that the dead function is the only place the default's value could have mattered for publishing.
