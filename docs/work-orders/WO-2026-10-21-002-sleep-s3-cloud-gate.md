# WO-2026-10-21-002: Sleep S-3: extract the cloud-operations gate (behavior-neutral, option D)

**Author:** architect, 2026-10-21 · **Status:** ready (Step 0 first)
**Branch:** `wo/2026-10-21-002-sleep-s3-cloud-gate`, cut from the S-2 branch tip `8d9695e` (PR #130), which stacks it on S-1 and S-2. The PR's base is `wo/2026-10-21-001-sleep-s2-execution`, retargeted as the earlier PRs merge.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-20-002-step0-addendum-report.md` (round 2) Q9 row 2 and sequence item 3; WO-2026-10-20-002 rulings 1–8; WO-2026-10-21-001's option D ruling, which applies to S-3 to S-6. **This WO cites `src/state/State_Sleep.cpp` at `8d9695e` (1,348 lines).** Re-check every range.

## Plain goal

Move the cloud-operations gate (the bounded wait for the publish queue, ledgers, OTA check and webhook before disconnecting) out of `handleSleepingState()` into one named function in `State_Sleep.cpp`, with its state in one file-level struct. **No behavior change:** the same waits, budgets, alerts 43, 44, 42 and 40, log lines and resets, pass by pass.

## What changes (at `8d9695e`; re-check every range)

- **The gate block:** `:596–812`. It covers the queue-aware budget, the blocker mask, the progress and blocked logs, the timeout alerts and resets, the release log and `markServiceEnd`, and the not-connected reset.
- **The gate state, ten function statics:**
  - `operationsCompleteLogged`, `gateWasBlocking`, `lastGateStatusLogMs`, `lastGateBlockerMask`, `lastGateBlockerBeforeRelease` and `gateReleaseReasonKnown` (`:483–488`);
  - `cloudSyncStartMs` (`:513`);
  - `cloudSyncBudgetMs`, `cloudSyncMaxQueueDepth` and `lastQueueDepth` (`:544–546`).
- **Their resets stay where they are, unchanged:**
  - on entry, `:493–498` and `:561–563`;
  - on the CONNECTED+OPEN abort, `:525`;
  - on the firmware and occupancy priority exits, `:580` and `:591`.

  They change only to name the struct field.
- **No move:** the helpers `computeCloudSyncTimeoutMs`, `gateBlockerReasonLabel` and the `kGate*` constants (`:51–119`) are already file-local.

## Design (option D; do not widen)

1. One file-local function in the anonymous namespace, for example `cloudGatePass(bool useNetworkStandbyEffective)`, replaces `:596–812` at the same point.
2. **The ten statics become one file-static struct.** Only the gate function and the reset sites above write it, and a structural test checks that.
3. **Nothing else changes:** no new file or header, and no change to `SleepTeardown`, the S-2 functions or the order of the priority checks.

## Step 0 (Claude Code, read-only, §13 (c))

- **Re-check every range;** mark each holds, moved or wrong. Name every reader and writer of the ten fields by bare name.
- **Size:** prototype the change and count it the way the S-2 prototype was counted. **≤ +20:** dispatch Stage 6 without coming back to the architect. **Over +20:** stop and report.
- **Fixture growth:** state the expected growth (WO-2026-10-21-001 C1 ruling).
- **Ruling 8:** list every test anchored on text or line order in the changed code, run the suite on the prototype, and re-anchor whatever fails. Known to read gate text: the firmware-update ordering and dwell tests, and `sleep_breadcrumb_sequence_test.py`. If `firmware_update_dwell_test.sh` is touched, it skips block and trailing comments (S-1 C1).
- **Report:** at most 150 lines, committed on the WO branch.

## Equivalence (freeze the output, not the code)

- **Extend the shared harness** (ruling 7). Record `tests/fixtures/sleep_cloud_gate_transcript.txt` once from `8d9695e`, with that hash in its header. The harness extracts the new function verbatim by marker (`COPY_MISMATCH`).
- **Scenarios** (cover the edges, not a full cross-product):
  - each blocker alone and in combination (queue, ledger, webhook, update);
  - elapsed just under, at and over the budget;
  - the queue-depth growth that raises the budget;
  - the ledger-timeout floor;
  - release after blocking, and release with no block;
  - not connected;
  - the firmware and occupancy exits mid-gate.
- **Freeze, per pass:**
  - the alerts;
  - the log lines, character for character;
  - the ten fields after the pass;
  - `markServiceEnd`;
  - the transitions;
  - the `Particle.process` and watchdog calls.

## Budget

- **`src/`: at most +20 net non-move lines, hard.** If Stage 6 goes over, stop and report; don't compress (§12.3).
- **Tests:** guideline +100 (this extends the harness). Record the actual and flag it at about twice the guideline.

## Tests and checks

- The transcript matches exactly, the golden payload test is unchanged, and the suite reports `N/N`, run serially.
- Re-anchored tests each fail their mutation.
- **Mutations.** Each must fail the transcript or the named test aimed at it:
  1. Raise alert 44 before 43.
  2. Use the budget at the elapsed-equals-budget edge, off by one.
  3. Skip the `lastQueueDepth` progress update.
  4. Drop the `cloudSyncStartMs` reset on the occupancy exit.
- **Structural:** the ten fields are written only in the gate function and at the listed reset sites. No `retained` keyword is added.
- **`nm` retained check:** expect `.backup` unchanged. ELF delta explained by symbol.

## Agents and models

- **Step 0:** Claude Code (controller), read-only, in its own session.
- **Stage 6:** Copilot, Sonnet tier, **medium**. It's a single contiguous block becoming one function with one struct, and the harness pattern now exists twice.
- **Controller, before Stage 7 (§13 (g)):** regenerate the transcript and confirm it is byte-identical; reproduce the build, `nm`, the mutations and the suite, serially.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. It checks:
  - provenance and edge coverage;
  - the moved block is byte-identical apart from field access;
  - one owner;
  - the reset sites are unchanged;
  - the ruling-8 list is complete.

  Argon is not built: support is being dropped, per the recovery-plan item.
- **Stage 8:** pre-approved on VERIFIED, or on VERIFIED WITH CONCERNS when every concern is Low and already closed by another test, with net `src/` ≤ +20. Any High or Medium finding comes to the architect. At most two rounds.
- **PR:** marked **"do not merge until v45 is cut and #128 and #130 are merged"**.

## For the simplify series (record, don't do)

`gateBlockerReasonLabel()` is a 16-case switch spelling out every combination of four flags (`:81–119`, about 39 lines). Building the label from the four flags would be a candidate for the necessity review. It changes log text, so it needs its own WO.

## Closing record

- Budget versus actual.
- The transcript hash and its provenance.
- ELF by symbol.
- The retained outcome.
- The ruling-8 table.

## Stage 6 result and controller reproduction (21 Oct)

**Stage 6** (Copilot, Sonnet 5.5, medium; implementation commit `58c59c0` on `wip/s3-sleep-cloud-gate`, no PR): option D as designed. `State_Sleep.cpp` 1,348 → 1,356 (**net +8**, within +20); only that file changed in `src/` (tree hash `1a27b3d9…` → `ef585a79946cf8d0f389681aedc396b83e40cba9128943b779e8af98e966c395`). The file differs from Claude Code's pre-dispatch prototype in one blank line.
**Test complexity (WO-2026-10-20-002 ruling 9), against main `0c565fa`:** `src/` 22,306 → 22,351 (**+45**; S-3 alone +8); test lines excluding fixtures 28,893 → 31,545 (**+2,652**; S-3 alone +939); fixture lines 834 → 16,868 (**+16,034**; S-3 alone +5,471); suite 91 → 97 (**+6**; S-3 alone +2). **Flag:** the fixture is 5,471 lines against the Step 0 estimate of 1,500–2,500 (about 2.2×), because the harness records every scenario in both a gate-trace-on and a gate-trace-off build (78 + 78 scenarios, 952 passes); test code is about +939 against the +100 guideline.
**Controller reproduction** (Claude Code, Device OS 6.4.1 Boron, fresh `BUILD_PATH_BASE`, `-frandom-seed=p6`, "before" is `git archive HEAD`):
- **Fixture:** `--regenerate` from the base (`9578709`) is **byte-identical** to the committed `tests/fixtures/sleep_cloud_gate_transcript.txt`; the normal run matches in both builds; the `COPY_MISMATCH` demonstration fires; the S-1 and S-2 fixtures are byte-identical to `HEAD`.
- **Suite 97/97** (sh via zsh, py via python3, serial; baseline 95); golden fixture unchanged; `git diff --check` clean (no whitespace hits at all), new test files clean.
- **Build (Boron):** text/data/bss `151484/1094/2236 → 151348/1114/2212` (**−136 text, +20 data, −24 bss**): the ten function-local statics are replaced by one 24-byte `(anonymous namespace)::cloudGate`, which lands wholly in `.data` because two fields start non-zero (`lastGateBlockerMask`, `lastQueueDepth`); total RAM −4 B. `handleSleepingState` `0x1580` → `0x14f4`; `cloudGatePass` is inlined; static-init stubs 22 before and after. **`.backup` unchanged** (`0x98` bytes, 25 retained symbols at the same addresses; only the flash load address moved, `0xd9340` → `0xd92cc`): retained outcome A.
- **Mutations (git-backed copy):** alert 44 before 43, the budget edge off by one (`<=`), the `lastQueueDepth` update skipped, the occupancy-exit `cloudSyncStartMs` reset dropped and a stray gate-field write each fail the transcript and the structural test; a `retained` probe fails the structural test.

## Stage 7 result (21 Oct)

**Stage 7** (Codex `gpt-5.6-sol`, high, on `58c59c0`; `WO-2026-10-21-002-stage7-verdict.md`): **VERIFIED WITH CONCERNS**, retained outcome A. Checks 1–6 PASS (transcript regenerated byte-identical; moved block byte-identical apart from 57 `cloudGate.` prefixes and one `return true;`; one owner; same returns, order and effects; storage change shown constant-initialized with no guard or init stub; `.backup` unchanged), check 7 and check 8 CONCERN, both **Low**:
- **C1 (Low):** `firmware_update_dwell_test.sh:22–35` (`extract_braced_block`) still takes the first raw marker, so a harmless earlier *block comment* containing the exact guard marker would make the test fail to compile (a false failure, not a false pass). The comment-skipping for the order discovery (`:41–64`) works: both spoof mutations (block comment, trailing comment) are rejected. It cannot hide a guard-order regression: the clean test, its own movement mutant, both removal/spoof mutants and `firmware_update_wiring_structural_test.py` cover that property.
- **C2 (Low):** test volume. S-3 adds 939 non-fixture test lines against the +100 guideline and a 5,471-line fixture against the 1,500–2,500 estimate. Trace-on has 2,984 body lines and trace-off 2,480; after removing compile-time gate-trace log lines, the trace selector and the macro-sensitive event catalog, their 2,415-line projections are identical. Building trace-off is justified (it proves the `#if`-compiled path builds and the trace logs vanish), but replaying and freezing all 476 passes again adds no state or branch coverage beyond those log lines; a small trace-off matrix or a projected comparison would keep the assurance at less cost.
**Claude Code treated VERIFIED WITH CONCERNS as VERIFIED for the Stage 8 pre-approval** (every concern is Low; C1 cannot hide a regression because other tests cover the property; C2 is a maintenance-cost concern). Both are left for the architect; C2 bears on ruling 9 (test complexity) and is listed under "For the architect" below.

## Closing record

| Item | Budget | Actual |
|---|---|---|
| `src/` net | ≤ +20, hard | **+8** (`State_Sleep.cpp` 1,348 → 1,356; +247/−239; +6 non-blank); only that file changed in `src/`; Claude Code's prototype was +8 |
| Tests (S-3 alone) | guideline +100 | **+939** non-fixture lines (+981/−42) and a **5,471-line fixture** (Step 0 estimate 1,500–2,500, about 2.2×); flagged |
| Suite | baseline 95 | **97/97** (sh via zsh, py via python3, serial); golden fixture sha256 `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637` unchanged |

- **Test complexity (ruling 9), against main `0c565fa` (cumulative):** `src/` 22,306 → 22,351 (**+45**); test lines excluding fixtures 28,893 → 31,545 (**+2,652**); fixture lines 834 → 16,868 (**+16,034**); suite 91 → 97 (**+6**). Verified independently by Codex.
- **Design (option D):** `cloudGatePass(bool useNetworkStandbyEffective)` and the ten-field `cloudGate` struct in `State_Sleep.cpp`'s anonymous namespace, before `struct WakeContext` (execution order; keeps the S-2 harness's extraction span untouched). The reset sites stay in the handler and only name the struct field.
- **Transcript provenance:** `tests/fixtures/sleep_cloud_gate_transcript.txt`, base `9578709` (its `src/` equals S-2's tip `8d9695e`); 156 scenarios (78 with gate trace on, 78 off), 952 passes; file sha256 `6e6d9401dd4b3c2827b91c53b7bf0b42c566937d3ff232b9caa32bd13a35eeff`, body `bf143623…`. **Regenerated from the base by Claude Code and by Codex: byte-identical.** The S-1 and S-2 fixtures are unchanged.
- **ELF (6.4.1 Boron):** text/data/bss `151484/1094/2236 → 151348/1114/2212` (**−136 text, +20 data, −24 bss**; RAM −4 B): the ten statics become one 24-byte `.data` object (two fields start non-zero: `lastGateBlockerMask = 0xff`, `lastQueueDepth = 0xffff`); `handleSleepingState` `0x1580` → `0x14f4`; `cloudGatePass` inlined; static-init stubs 22 before and after.
- **Retained outcome A:** the whole `.backup` section (`0x98` bytes, 25 retained symbols) byte-identical; only the flash load address moved (`0xd9340 → 0xd92cc`). No `retained` keyword added.
- **Ruling-8 table (final):** re-anchored and mutation-checked: `config_window_hold_test.sh` (dropping the abort's reset fails), `firmware_update_dwell_test.sh` (guard below `cloudGatePass(` fails; now skips block and trailing comments; C1 above), `occupancy_report_by_mode_test.{sh,cpp}` (occupancy exit below the gate fails seven checks), `sleep_teardown_structural_test.py` (the lost-timestamp guard below the gate fails). Still binding: `sleep_execution_equivalence_test.sh` (S-2 harness; the gate sits before `struct WakeContext`), `firmware_update_wiring_structural_test.py`, `sleep_breadcrumb_sequence_test.py`, `loop_stage_sleep_prep_exclusion_test.py` and the rest. A token, log and alert scan found no other test that reads the gate text.
- **Mutations** (git-backed copies): alert 44 before 43, the budget edge off by one, the `lastQueueDepth` update skipped, the occupancy-exit `cloudSyncStartMs` reset dropped and a stray gate-field write fail the transcript and the structural test; a `retained` probe fails the structural test; Codex's own update/webhook-priority and not-connected-release-reset mutants fail the transcript.
- **Argon:** not built (support is being dropped; recovery plan).
- **Deviations:** a sibling runner (`sleep_cloud_gate_equivalence_test`) reuses the shared fakes instead of extending the S-1/S-2 runner; a gate-trace-off build is added to the harness; the harness hoists the base's ten function statics to file scope with a mechanical regex so it can read them; additive edits to three shared stub headers.
- **Rounds:** Stage 6 one round; Stage 7 once (VERIFIED WITH CONCERNS, both Low). Ships in v46 with S-1, S-2 and S-4 to S-6. **The PR is marked "do not merge until v45 is cut and #128 and #130 are merged".**

## For the architect

- **C2 / ruling 9:** the series is above the baseline on every measure (tests +2,652, fixtures +16,034, suite +6, `src/` +45). The S-3 fixture alone came in at 2.2× its Step 0 estimate because every scenario is frozen twice. A projected trace-off comparison (or a small trace-off matrix) would cut about 2,500 fixture lines now; whether to do that here or leave it to the end-of-series shrink is the architect's call.
- **For the simplify series (record, don't do):** `gateBlockerReasonLabel()` (16-case switch, about 39 lines) is a candidate for the necessity review; the `CLOUD_OPS`/gate-trace logging is a candidate for the same review.
