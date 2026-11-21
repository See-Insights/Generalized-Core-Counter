# WO-2026-11-19-001 v2.1 Stage 6 report (one connect decision, with a hold after failure)

**Agent:** GitHub Copilot CLI 1.0.112. **Model:** `claude-fable-5.2`, as dispatched, not picked by Auto. **Reasoning:** the session does not show me its reasoning-effort setting, so I can't confirm "high". No sub-agents were used.
**Interpreters:** sh via zsh 5.9, py via python3 (Python 3.15.2); host C++ is Apple clang 14.0.3. Firmware: arm-none-eabi-gcc 10.2.1, Device OS 6.6.0.
**Branch / base:** `wo/2026-11-19-001-connect-decision` at `e2ffe7d` (base `eb398b8`). All changes are **uncommitted**. Step 0 was not repeated.

## Result in short
- The WO's "Approved architecture" is implemented as written. **Net `src/` = 0 lines** (target ≤ 0, cap +10).
- Suite: **125/125 (sh via zsh, py via python3)**. Baseline was 122/122.
- **One thing needs a ruling:** the WO's `src/` changes broke eight existing tests beyond the two tests and the stub that Step 0 named. I updated them, narrowly, to keep the suite green (see "Deviations", item 1). Nothing was weakened beyond the WO's own deltas, but this is wider than the dispatch's stated test scope.

## Baseline
- As checked out: **117/122**. This worktree hangs off a shallow clone (2 commits). Five equivalence tests extract a "base" block from an older commit and fail without it: `sleep_cloud_gate`, `sleep_execution`, `sleep_plan`, `sleep_preflight` and `sleep_wake` `_equivalence_test.sh`. CI uses `fetch-depth: 0`.
- As CI runs it: **122/122 (sh via zsh, py via python3)**. To get history without touching the shared clone, I made a bare clone of the public origin inside my scratch directory and exported `GIT_ALTERNATE_OBJECT_DIRECTORIES` for the test run only. All figures below were measured that way.

## Failing-before evidence (tests written first, `src/` untouched)
| Test | Result before the implementation |
|---|---|
| `connect_decision_test.sh` (new) | exit 1: `no such file or directory: 'src/power/ConnectDecision.cpp'` |
| `connect_hold_wiring_test.sh` (new) | exit 1: `EXTRACT_FAILED: decision block start x0` |
| `standby_latch_daily_clear_test.sh` (updated) | exit 1: `FAIL unlatched:countCleared: session.modemConnectTimeoutCount == 0` |

The first two can only fail at build or extract time, because the module didn't exist. The third is a run-time failure of ruling 4's check against the old `clearAtDailyClose()`.

## What changed in `src/`
`git diff --numstat -- src` (the two new files are marked intent-to-add so that this command counts them):

| File | + | − | What |
|---|---|---|---|
| `power/ConnectDecision.h` (new) | 27 | 0 | `Inputs`, `Result`, `holdMs()`, `decide()` |
| `power/ConnectDecision.cpp` (new) | 35 | 0 | the hold, then rules 2–8 in the chain's order |
| `power/ConnectivityPolicy.h` | 6 | 0 | `HOLD_BASE_MS` (1 h), `HOLD_MAX_MS` (6 h) |
| `power/ModemHealth.cpp` | 1 | 1 | `clearAtDailyClose()` zeroes the count unconditionally |
| `state/SleepTeardown.cpp` | 0 | 4 | the four session writes |
| `state/StateMachine.h` | 2 | 5 | four fields out, `lastConnectFailMs` in, one comment line |
| `state/State_Connect.cpp` | 1 | 0 | `session.lastConnectFailMs = millis();` beside `noteConnectTimeout()` |
| `state/State_Report.cpp` | 16 | 78 | defer constant and block out; chain replaced by "build Inputs, `decide`, act" |
| **Total** | **88** | **88** | **net 0** |

- R0, R1, the battery commit, `returnToSleepAfterReport` and R1b are where they were, in today's order. `connect_hold_wiring_test.sh` pins that order.
- On a hold, Report logs once per pass: `CONNECT_HOLD: remaining=<s> failures=<n> trigger=<name>`.
- Nothing is persisted. No payload, ledger or layout change. No other `src/` or `lib/` file was edited.

## Tests
**New**
- `connect_decision_test.{sh,cpp}`: the real `ConnectDecision.cpp` on the host.
  - Characterization at `failureCount = 0`: 28 named rows for map rows R2–R5, R7, R9 and R10 with the "Outcome today" strings, and all 32 trigger combinations × 4 modes against a transcription of the old chain (`State_Report.cpp:220–282` at `eb398b8`, without the defer arms). R1 and R1b are not inputs; `occupancy_report_by_mode_test.sh` drives those two exits.
  - Hold: 1, 2, 4, 6, 6 h for failures 1–5, and 6 h for every count up to 255. Each held trigger waits inside the hold and gives its own CONNECT at and after it. The service request connects during a hold. A count of 0 never holds.
  - Purity: the module builds against an empty `Particle.h` stub and names no Device OS object, global or log.
- `connect_hold_wiring_test.{sh,cpp}`: the real block of `handleReportingState()`, extracted byte-for-byte and run with the real `decide()`. It checks each input, the hold log line, the `millis()` wrap, the order of the kept exits, that `lastConnectFailMs` is written once (beside `noteConnectTimeout()`), and that the defer and its four fields are gone from `src/`.
- `connect_hold_base_delta.py`: a helper for the five Sleep structural tests (item 1 below). CI runs every `tests/*.py`, so it counts as one entry; run directly, it checks itself.

**Suite count:** 122 + 2 new tests + 1 helper = **125/125**.

## Mutations
The WO's three, run in place on `src/power/ConnectDecision.cpp` with a `cp` backup and a `cp` restore (`cmp` clean after each; no `git checkout` or `git restore`):

| # | Mutation | Aimed check | Result |
|---|---|---|---|
| 1 | Drop the hold rule (`if (false)`) | `hold:` rows | caught: 245 of 245 failures are `hold:` |
| 2a | Remove the doubling | `seq:` | caught: `seq: failure 2 holds 2 h` (254 `seq:` failures) |
| 2b | Remove the cap | `seq:` | caught: `seq: failure 4 holds 6 h` (252 `seq:` failures) |
| 3 | Exempt `occupancyChange` from the hold | `hold: occupancy change waits` | caught: 80 of 80 failures are that check |

The same four also run on private copies inside `connect_decision_test.sh` on every suite run, with two more (service request held; daily close moved below keep-alive). `connect_hold_wiring_test.sh` adds eight wiring mutants. All are caught by the check aimed at them.

## Build sizes (local toolchain, fresh `BUILD_PATH_BASE` each)
| Build | text | data | bss |
|---|---|---|---|
| `eb398b8` | 151124 | 1114 | 2264 |
| Candidate | 151116 | 1114 | 2248 |
| Difference | −8 | 0 | −16 |

- No warning from a file this WO changed. **Correction (Stage 7 r1, Low):** the build does emit two inherited warnings from files byte-identical to the base: `CONNECTIVITY_FAILSAFE_TEST_MODE=0` at `src/diagnostics/ConnectivityFailsafeTest.cpp:18` and undefined `MUON_HAS_TMP112` at `src/sensors/SensorManager.cpp:978`. They exist at `eb398b8` too. `nm` on the candidate ELF shows `ConnectDecision::decide` and `ConnectDecision::holdMs`; the object is `src/power/ConnectDecision.o`.
- The build wrote nothing into `~/.particle/toolchains` (no file there changed during it).

## Deviations from the WO and the dispatch
1. **Tests changed beyond the dispatch's named set (needs a ruling).** The dispatch authorises "the tests and stub that name the deleted fields". Eight more existing tests pin code the WO changes, and failed after the `src/` edits:
   - `modem_health_equivalence_test.sh` and its fixture. Ruling 4 changes six frozen rows (`dailyClose`, latch clear, count 2: now `count=0`). I edited those six rows by hand, recorded it in the fixture header, and updated one mutation anchor. I added a mutant for "clears only when latched".
   - `daily_cleanup_boundary_test.py`. Its regex required the old `else if (due)` branch. It now checks that Report hands `due` in as `closeDue` and that the close rule precedes keep-alive and cadence in `ConnectDecision.cpp`.
   - `occupancy_report_by_mode_test.{sh,cpp}`. It compiled the chain's first two branches and modelled the rest. It now compiles the real decision block with the real `ConnectDecision.cpp`. No expectation changed; all 23 of its mutations are still caught.
   - `sleep_cloud_gate`, `sleep_execution`, `sleep_plan`, `sleep_preflight` and `sleep_wake` `_structural_test.py`. With history present, each requires "no new `src/` file" and "`SleepTeardown.cpp` identical to its base commit". They now allow exactly the two new files and the four deleted lines (applied to the base text only). I checked that a stray edit to `SleepTeardown.cpp` and a stray new file under `src/` still fail all five.
   - In the named set, `sleep_teardown_equivalence_test` needed more than the dump change. The base block still writes the four fields, and the deleted `= millis()` was one counted read. The harness now takes those writes in a base-only struct. The regenerated fixture equals the old one with the `sess=` column removed (1065 lines) and one `millis()` read less on the 288 completion passes; I verified that by script.
2. **`Result` has a fourth field, `trigger`.** The WO's log line needs a trigger name and its `Result` has no place for one. The name is the existing outcome string of the rule that would have connected, or `none`.
3. **A held pass with nothing due answers `"connect hold"`**, not `"not aligned"`, and logs the hold line with `trigger=none`. That is the WO's rule order read literally (the hold is rule 1). Both answers go to IDLE.
4. **Log level:** `CONNECT_HOLD` is `Log.info`. The WO gives no level; the two deleted defer lines were `Log.warn`.
5. **One trace block kept.** The WO says to delete the trace blocks "inside the chain only" and to leave `returnToSleepAfterReport` (`:211–216`) unchanged, so its trace line and `tierName` stay. Step 0 expected `tierName` to go. Removing them would save about 9 lines.
6. **Types:** the module's times and `lastConnectFailMs` are `uint32_t`, so the host and the device compute the same wrap.
7. **Index state.** I ran `git add -N` on the two new `src/` files so that `git diff --numstat -- src` counts them. `git reset` is blocked in this session, so I couldn't undo it. No content is staged. The new files under `tests/` are untracked.

## Gaps seen outside the WO
- **Step 0's test inventory was incomplete.** It was read from another worktree and found two tests and a stub. Ten tests were affected (item 1).
- **Shallow worktrees hide checks.** Without history, five equivalence tests fail and the five Sleep structural tests skip their base comparison silently. A targeted rerun without history gave the same five failures as the baseline and no new one, so a plain run here should read 120/125; I did not run the full suite that way.
- **The trace flag now does little in Report.** `ENABLE_CONNECT_DECISION_TRACE` still enables one line in `State_Report.cpp` and three in `State_Connect.cpp`. `BuildProfile.h:53` still calls it "connection decision rationale logs". I did not edit `BuildProfile.h` (not in scope).
- **Two tests leave files outside their temp dir:** see "Scratch". They ignore `TMPDIR` and never clean up.
- **Workflow §3 build path:** it names `buildtools/1.1.1/bin`, but the binaries are in `buildtools/1.1.1/` here.
- **Not verified:** nothing was flashed or bench-tested. The equivalence of the old chain and `decide()` rests on the characterization test and on Stage 7's line-by-line check, as the WO says.

## Scratch
- `build-tmp/wo-2026-11-19-001-stage6/` held the suite runner, logs, backups, the bare history clone and the two build trees. I removed that directory at the end, and `tests/__pycache__/`, which my runs created.
- **Left in place, not mine to name:** `build-tmp/wo-2026-11-03-004-stage6/c1_test/` and `build-tmp/wo-2026-11-03-006-stage6-r2/c2_test/`. `build-tmp/` didn't exist when I started; two existing tests (`debounce_expiry_c1_test.sh`, `untrusted_piece_c2_test.sh`) hard-code those paths and leave their build files there on every suite run. They are gitignored. I did not delete them because the dispatch limits me to my own scratch directory.
- To rerun the suite with history in this worktree, clone the origin bare and export `GIT_ALTERNATE_OBJECT_DIRECTORIES=<clone>/objects`.

## Closing-record figures
| Item | Budget | Actual net `src/` lines | Tests |
|---|---|---|---|
| Connect decision + hold | ≤ 0 (cap +10) | **0** (+88 / −88) | 125/125; 2 new tests, 1 helper, 10 existing tests updated |
