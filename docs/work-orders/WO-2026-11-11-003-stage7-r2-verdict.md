NOT VERIFIED 975ef58f4c0c8276a1cc635179f2ce4918a47586
2 Medium (src), 1 Medium (test), 1 Low (src), 1 Low (docs)

Round 2 of 2, the last.

| # | Result | Evidence |
|---|---|---|
| 1 | FAIL | Zero correctly uses 300000 ms in all four PIR blocks, but `setting1=1..999` regresses to a zero-ms awake LED window. A separate 60-second fallback remains at `src/state/State_Sleep.cpp:612`. |
| 2 | FAIL | Low source defect: `src/state/State_Modes.cpp:78-79` logs the pre-fallback value—0 seconds for zero—immediately before applying 300 seconds. |
| 3 | FAIL | Real blocks are extracted and all named mutations are caught, but the fake runtime function and missing sub-second boundary cases let the source regression pass. |
| 4 | PASS | The `+9/-1` structural-test edit maps back exactly the two intended replacements. An unrelated `Log.info` added to `State_Modes.cpp` was rejected. |
| 5 | FAIL | Every round-1 omitted hit is now classified, but the three new explicit-setting hits in `debounce_awake_pir_test.sh:77-80` are unclassified. |
| 6 | PASS | Budget, host suite, two fresh Device OS builds, `nm`, and `objdump` passed. |
| 7 | PASS | No additional regression found beyond the findings below. |

Value walk:

| `setting1` | Awake first/re-arm | Sleep first/re-arm | Expiry |
|---|---:|---:|---:|
| `0` | 300000 ms | 300000 ms | 300000 ms |
| `1..999`, valid | **0 ms** | exact 1..999 ms | exact 1..999 ms |
| `1000` | 1000 ms | 1000 ms | 1000 ms |
| `60000` | 60000 ms | 60000 ms | 60000 ms |
| `300000` | 300000 ms | 300000 ms | 300000 ms |

For `1..999`, `occupancyDebounceMsForRuntime()` returns the nonzero stored value, which is divided by 1000 again and remains zero. Before Amendment 1, awake behavior was 60000 ms; it is now 0 ms, not 300000 ms. With `isValid(false)==false` and `setting1==0`, all paths use 300000 ms. For nonzero stored values, these call sites continue using the stored value without consulting validity.

Findings:

- **Medium, src — `src/state/State_Modes.cpp:77-83,97-101`.** A valid `setting1` of 1–999 ms now arms/re-arms the awake LED for zero milliseconds, while sleep and `debounceRemainingMs()` use the configured milliseconds. Smallest fix: distinguish `setting1Raw==0` from seconds truncating to zero in both awake branches, preserving the pre-amendment nonzero behavior or explicitly using one millisecond value end-to-end.

- **Medium, src — `src/state/State_Sleep.cpp:611-618`.** With `setting1==0`, an already-expired occupied sleep-plan path still substitutes 60 seconds and can cap a longer scheduled wake at 60 seconds rather than the runtime 300-second fallback. This is the only additional hard-coded debounce found; other grep hits are unrelated minute/hour arithmetic. Smallest fix: derive this value from `Config::occupancyDebounceMsForRuntime()` and add the zero-setting sleep-plan case.

- **Medium, test — `tests/debounce_awake_pir_test.sh:48,74-81`.** The fake `occupancyDebounceMsForRuntime()` always returns 300000, unlike the real function for nonzero settings, and the matrix skips 1 and 999. Thus the test passes despite the source regression. Smallest fix: make the fake reproduce runtime semantics and test 0, 1, 999, 1000, 60000, and 300000 in both awake branches, plus invalid-config behavior.

- **Low, src — `src/state/State_Modes.cpp:78-82`.** The info log states `setting1=0 => debounce=0 sec`, although the applied fallback is 300 seconds. A later `Occ:` log is correct, but the contradictory mapping is still misleading. Smallest fix: log after fallback resolution or label the first value as pre-fallback.

- **Low, docs — `docs/work-orders/WO-2026-11-11-003-stage6-copilot-report.md:30-51`; `...stage6-r2-copilot-report.md:28-32,48-52`.** The classification omits `debounce_awake_pir_test.sh:77-80`, and “Nonzero `setting1` is unchanged” is false for 1–999 ms. Smallest fix: classify those explicit-setting hits and record the sub-second behavior accurately.

Mutation results:

| Mutation | Result |
|---|---|
| Literal `60` in awake first branch | Caught, exit 1 |
| Literal `60` in awake re-arm branch | Caught, exit 1 |
| Literal `60000` in awake first branch | Caught, exit 1 |
| Sleep first fallback removed | Caught, exit 1 |
| Constant restored to `60000UL` | Caught, exit 1 |
| Unrelated line added for structural test | Caught, exit 1 |

The new test exits 0 from both repository root and `/tmp`, leaves no repository artifact, uses unique marker starts, and fails loudly with `EXTRACT_FAILED` if a marker is reworded. Deliberately duplicating suitable marker text remains a documented structural-test limitation, not a finding.

Interpreter: **121/121 (sh via zsh, py via python3)**.

`src/` budget:

- `Config.h`: +1/-1, net 0
- `State_Modes.cpp`: +2/-4, net −2
- Total: +3/-5, net −2; within budget

Fresh Device OS 6.6.0 builds:

| Revision | text | data | bss |
|---|---:|---:|---:|
| `975ef58f4c0c8276a1cc635179f2ce4918a47586` | 153028 | 1114 | 2240 |
| `05f1439` | 153108 | 1114 | 2240 |
| Delta | −80 | 0 | 0 |

`nm` found `Config::occupancyDebounceMsForRuntime()`, `sensorConfigData::initialize()`, and `handleOccupancyMode()` in the reviewed ELF and relevant objects. `objdump` showed reviewed `0x493e0` (300000) versus base `0xea60` (60000).

Model/reasoning: `gpt-5.6-sol`, medium. Scratch removed; worktree unchanged; HEAD rechecked.