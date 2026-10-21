# WO-2026-10-21-004: Sleep S-5: extract the pre-sleep cycle summary and the Wi-Fi-off guard (behavior-neutral, option D)

**Author:** architect, 2026-10-21 · **Status:** ready (Step 0 first)
**Branch:** `wo/2026-10-21-004-sleep-s5-preflight`, cut from the S-4 branch tip `e9475d7` (PR #132). The PR's base is `wo/2026-10-21-003-sleep-s4-plan`, retargeted as the earlier PRs merge.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:**
- `WO-2026-10-20-002-step0-addendum-report.md` (round 2) Q9 row 6 and sequence item 5;
- WO-2026-10-20-002 rulings 1–9;
- WO-2026-10-21-001's option D ruling;
- the one-build rule recorded in WO-2026-10-21-003.

**This WO cites `src/state/State_Sleep.cpp` at `e9475d7` (1,373 lines).** Re-check every range.

## Plain goal

Move the last block before the sleep call out of `handleSleepingState()` into two named functions in `State_Sleep.cpp`:
- the end-of-cycle summary, which takes the pre-sleep battery sample, runs `finalizeBeforeSleep`, checks the log-only invariants and writes the `CYCLE end` line;
- the Wi-Fi radio-off guard.

The guard's four statics go into one file-level struct. **No behavior change:** the same measurements, log lines, invariant warnings, radio requests, waits and returns.

## What changes (at `e9475d7`; re-check every range)

- **The end-of-cycle summary:** the brace block `:1021–1090`, from `measure.batteryState(PreSleep)` through the `CYCLE end` line.
- **The Wi-Fi-off guard:** `:1092–1146` (`#if Wiring_WiFi`), including its early return at `:1125`.
- **The guard's four statics:** `wifiOffGuardActive`, `wifiOffGuardStartMs`, `wifiOffGuardLastRetryMs` and `wifiOffGuardRetries` (`:945–950`). Their entry resets at `:956–961` stay where they are and only change to name the struct field.
- **Stays inline:** breadcrumb 24 (`:1006`) and `useNetworkStandby = useNetworkStandbyEffective` (`:1016`).

## Design (option D; do not widen)

1. **Two file-local functions** in the anonymous namespace:
   - the summary function takes `useNetworkStandby` and returns nothing;
   - the guard function takes `useNetworkStandby` and returns "wait" exactly where `:1125` returns today. It exists only under `Wiring_WiFi`.
2. **The four guard fields** become one file-static struct under `Wiring_WiFi`. Only the guard function and the entry reset write it, and a structural test checks that.
3. **Nothing else changes:** no new file or header, and no change to the S-1 to S-4 functions.
4. **Platforms.** The release target is Boron, where the guard compiles out. Wi-Fi stays for the P2 (Argon support is being dropped), so the harness exercises the Wi-Fi path on the host.

## Step 0 (Claude Code, read-only, §13 (c))

- **Re-check every range;** mark each holds, moved or wrong. Name every reader and writer of the four fields by bare name.
- **Size:** prototype and count the change as before.
  - **≤ +20:** dispatch Stage 6 without coming back.
  - **Over +20:** stop and report.
- **Fixture growth:** state the expected growth.
- **Ruling 8:** run the suite on the prototype, then list and re-anchor every test anchored on text or line order in the changed code, giving each one's mutation. Likely readers are the cycle-stats and invariant logging tests.
- **Report:** at most 150 lines, committed on the WO branch.

## Equivalence (freeze the output, not the code)

- **Extend the shared harness** (ruling 7). Record `tests/fixtures/sleep_preflight_transcript.txt` once from `e9475d7`, with that hash in its header. The harness extracts the new functions verbatim by marker (`COPY_MISMATCH`).
- **Builds:** record one build. A second build's scenarios are added only where a flag changes the code under test: `DEV_BUILD` (the awake-ceiling allowance) and `Wiring_WiFi` (the guard). Keep each of those to a small edge matrix.
- **Scenarios** (cover the edges, not a full cross-product):
  - **Standby:** on or off, with cloud and radio each on or off.
  - **Awake time:** under, at and over the ceiling.
  - **Connect duration:** at and over budget + 1000 ms, with an attempt or with none.
  - **Verbose:** on or off.
  - **Sentinels:** SoC out of range (the 0xFFFF sentinel) and an unknown charge state.
  - **Wi-Fi guard:**
    - radio already off;
    - radio on at the first pass;
    - retry at and just under `WIFI_OFF_GUARD_RETRY_MS`;
    - give-up at its limit;
    - the retry counter at 255.
- **Freeze, per pass:**
  - the `finalizeBeforeSleep` arguments;
  - the log lines, character for character;
  - the radio requests;
  - the guard fields after the pass;
  - the wait or continue result.

## Budget

- **`src/`: at most +20 net non-move lines, hard.** If Stage 6 goes over, stop and report; don't compress (§12.3).
- **Tests:** guideline +100. Record the actual and flag it at about twice the guideline. Record ruling 9's three numbers against `0c565fa`.

## Tests and checks

- The transcript matches exactly, the golden payload test is unchanged, and the suite reports `N/N`, run serially. The earlier fixtures are unchanged.
- Re-anchored tests each fail their mutation.
- **Mutations.** Each must fail the transcript or the named test aimed at it:
  1. Drop the 30 s slack from the awake ceiling.
  2. Swap the standby branches of the radio invariant.
  3. Retry the Wi-Fi guard one interval late.
  4. Drop the guard's give-up reset.
- **No `retained` keyword is added.**
- **`nm` retained check:** expect `.backup` unchanged. ELF delta explained by symbol.
- **Code layout:** as in S-4, report any same-size layout difference.

## Agents and models

- **Step 0:** Claude Code (controller), read-only, in its own session.
- **Stage 6:** Copilot, Sonnet tier, **medium**. It's two contiguous blocks with one small struct, the same pattern as S-3 and S-4.
- **Controller, before Stage 7 (§13 (g)):** regenerate the transcript and confirm it is byte-identical; reproduce the build, `nm`, the mutations and the suite, serially.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. It checks:
  - provenance and edge coverage, including the `DEV_BUILD` and Wi-Fi matrices;
  - the moved code is byte-identical apart from field access;
  - one owner;
  - the same returns and order;
  - the ruling-8 list is complete.

  No Argon build.
- **Stage 8:** pre-approved on VERIFIED, or on VERIFIED WITH CONCERNS when every concern is Low and already closed by another test, with net `src/` ≤ +20. Any High or Medium finding comes to the architect. At most two rounds.
- **PR:** marked **"do not merge until v45 is cut and #128, #130, #131 and #132 are merged"**.

## For the simplify series (record, don't do)

- **Stale comments at `:1008–1011`:** "NO AB1805 alarms" is wrong, because hibernate programs an AB1805 alarm, and "works reliably on Photon2" names a platform GCC doesn't target.
- **The alias at `:1016`:** `useNetworkStandby` only re-names `useNetworkStandbyEffective`.
- **The invariant block** is log-only, about 40 lines. It is judged in the necessity review on whether its warnings ever fire in the fleet logs.

## Closing record

- Budget versus actual.
- Ruling 9's numbers.
- The transcript hash and its provenance.
- ELF by symbol, and any layout difference.
- The retained outcome.
- The ruling-8 table.

## Stage 6 result and controller reproduction (21 Oct)

**Stage 6** (Copilot, Sonnet 5.5, medium; implementation commit `a06b09a` on `wip/s5-sleep-preflight`, no PR): option D as designed. `State_Sleep.cpp` 1,373 → 1,383 (**net +10**, within +20; +7 non-blank); only that file changed in `src/` (tree hash `2a96daf20a659b79cfbc5dc582ef1cc4e5e2f2c9bf79589dcc23166d76b9393c` → `88d823dedca396b814aa170c10928de40ead36fe2f44dffcc1eb78668bcab298`). The file is **identical** to Claude Code's pre-dispatch prototype (zero differing lines).
**Test complexity (WO-2026-10-20-002 ruling 9), against main `0c565fa` (Claude Code's count over all of `src/` and `tests/`):** `src/` 22,306 → 22,378 (**+72**; S-5 alone +10); test lines excluding fixtures 28,893 → 33,083 (**+4,190**; S-5 alone +871); fixture lines 834 → 18,890 (**+18,056**; S-5 alone +927); suite 91 → 101 (**+10**; S-5 alone +2). (Copilot counted `src/` as 22,366; its `find` differs from the method used for S-1 to S-4, so the recorded figure is Claude Code's.) The fixture is 927 lines against the Step 0 estimate of 450–700 (about 1.3–2.1×): 86 scenarios (59 release, 7 `DEV_BUILD`, 20 `Wiring_WiFi` with multi-pass guard coverage; 122 passes); test code +871 against the +100 guideline (Step 0 expected +600–850).
**Controller reproduction** (Claude Code, Device OS 6.4.1 Boron, fresh `BUILD_PATH_BASE`, `-frandom-seed=p6`, "before" is `git archive HEAD`):
- **Fixture:** `--regenerate` from the base (`e9475d7`) is **byte-identical** to the committed `tests/fixtures/sleep_preflight_transcript.txt`; the normal run matches in all three builds; the `COPY_MISMATCH` demonstration fires; the S-1 to S-4 fixtures are byte-identical to `HEAD`.
- **Suite 101/101** (sh via zsh, py via python3, serial; baseline 99); golden fixture unchanged; `git diff --check` has no hits; new test files clean.
- **Builds (Boron):** release `151348/1114/2212` and `-DDEV_BUILD=1` `151276/1114/2212`, both identical before and after, with **`.text` byte-identical** (the Wi-Fi guard compiles out on Boron and the summary inlines identically), identical symbol sets, static-init stubs 22/22, **`.backup` unchanged** (retained outcome A).
- **Mutations (git-backed copy):** the 30 s ceiling slack dropped, the standby branches of the radio invariant swapped, the Wi-Fi guard retried one interval late, the give-up reset dropped and the `+ 0.5f` rounding dropped each fail the transcript and the structural test; a stray write to a guard field fails the structural test (the transcript does not execute it); a `retained` probe fails the structural test.

## Stage 7 result (21 Oct)

**Stage 7** (Codex `gpt-5.6-sol`, high, on `a06b09a`; `WO-2026-10-21-004-stage7-verdict.md`): **VERIFIED WITH CONCERNS**, retained outcome A. Checks 1–5 PASS: the transcript regenerated byte-identical (86 scenarios, 122 passes: release 59, `DEV_BUILD` 7, `Wiring_WiFi` 20 multi-pass; Codex's own `soc <= 100 → < 100` and standby-invariant mutants fail it); the moved blocks are byte-identical (the summary's 68 interior lines including 6 blank lines; the guard's 50 lines including 4 blank lines, after exactly 22 `wifiGuard.` prefixes and one `return;` → `return true;`); one owner and placement (`State_Sleep.cpp:356–488`, between `cloudGatePass` and `SleepPlan`); the Wi-Fi guard compiles out on Boron (zero Wi-Fi-guard strings or symbols in either build pair) and matches the base on the host for all 20 scenarios; release `151348/1114/2212` and `DEV_BUILD=1` `151276/1114/2212` identical before and after with **`.text` byte-identical**, identical symbols, static-init stubs 22/22 and `.backup` unchanged. Checks 6 and 7 CONCERN, both **Low**:
- **C1 (Low):** `tests/sleep_cloud_gate_structural_test.py:139`, `tests/sleep_plan_structural_test.py:136` and `tests/sleep_preflight_structural_test.py:170` remove every `wifiGuard.` substring across the compared handler span, which is broader than S-5's four entry-reset lines: an unrelated `"sleep-abort-open-hours"` prefixed with `wifiGuard.` passes all three structural tests. The unchanged `config_window_hold_test.sh:79` rejects that mutant, and ordinary unrelated-code and gate-reset mutations fail as intended. A narrow test-maintenance loophole, not a defect in the code; restricting the normalization to the four reset lines would make the ruling-8 claim exact.
- **C2 (Low):** non-fixture test code is +871 lines, 8.71× the +100 guideline; the 927-line fixture (estimate 450–700) is proportionate (one broad release matrix plus 7 `DEV_BUILD` and 20 multi-pass Wi-Fi cases, not duplicate full matrices).
**Claude Code treated VERIFIED WITH CONCERNS as VERIFIED for the Stage 8 pre-approval** (both concerns are Low; C1 is closed for the demonstrated case by an unchanged test; C2 is a maintenance-cost concern, flagged each WO). Both are left for the architect.

## Closing record

| Item | Budget | Actual |
|---|---|---|
| `src/` net | ≤ +20, hard | **+10** (`State_Sleep.cpp` 1,373 → 1,383; +140/−130; +7 non-blank); only that file changed in `src/`; Claude Code's prototype was +10 and the file is identical to it |
| Tests (S-5 alone) | guideline +100 | **+871** non-fixture lines (+880/−9; 8.71×), a **927-line fixture** (estimate 450–700) |
| Suite | baseline 99 | **101/101** (sh via zsh, py via python3, serial); golden fixture sha256 `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637` unchanged |

- **Test complexity (ruling 9), against main `0c565fa` (cumulative; Claude Code's count, verified by Codex):** `src/` 22,306 → 22,378 (**+72**); test lines excluding fixtures 28,893 → 33,083 (**+4,190**); fixture lines 834 → 18,890 (**+18,056**); suite 91 → 101 (**+10**). S-5 alone: +10 / +871 / +927 / +2.
- **Design (option D):** `endOfCycleSummary(bool)` and, under `Wiring_WiFi`, a four-field `wifiGuard` struct with `wifiOffGuardPass(bool)`, in the anonymous namespace between `cloudGatePass` and `struct SleepPlan` (the one slot outside the S-2 to S-4 harness spans); the handler calls both where the blocks were. Breadcrumb 24, the stale comments and the `useNetworkStandby` alias stay inline.
- **Transcript provenance:** `tests/fixtures/sleep_preflight_transcript.txt`, base `e9475d7` (same `src/` as S-4's tip); 86 scenarios (59 release + 7 `DEV_BUILD` + 20 `Wiring_WiFi`), 122 passes; file sha256 `aa6ecd1e…`, body `e65eabcb…`. **Regenerated from the base by Claude Code and by Codex: byte-identical.** The S-1 to S-4 fixtures are unchanged. **One build** plus the two small flag matrices (the S-3 C2 ruling).
- **The properties no earlier test pinned** (the 30 s ceiling slack, the standby branches of the radio invariant, the guard's retry timing and give-up reset, the `+ 0.5f` rounding) **are now pinned by the transcript.**
- **ELF (6.4.1 Boron):** release and `DEV_BUILD=1` sizes identical before and after, **`.text` byte-identical** (no code-layout difference this time, unlike S-4), static-init stubs 22/22.
- **Retained outcome A:** the whole `.backup` section (`0x98` bytes, 25 retained symbols) byte-identical (sha256 `adfe530f…`); no `retained` keyword added.
- **Ruling-8 table (final):** re-anchored and mutation-checked: `occupancy_report_by_mode_test.sh` (teardown anchored at the handler's `SleepTeardown::run`; sleep check below the teardown fails), `sleep_cloud_gate_structural_test.py` and `sleep_plan_structural_test.py` (allowed differences extended by S-5's edits; an unrelated handler edit fails; **but the `wifiGuard.` normalization is broader than the four lines: C1**). Still binding: `power_consumer_regression_test.sh` (the `isCharging` mapping), `modem_teardown_confirmation_logging_test.py` (the standby=0 invariant text), `config_window_hold_test.sh`.
- **Mutations** (git-backed copies): the 30 s slack dropped, the standby branches swapped, the retry one interval late, the give-up reset dropped and the `+ 0.5f` rounding dropped fail the transcript and the structural test; a stray guard-field write and a `retained` probe fail the structural test.
- **Argon:** not built (support is being dropped; recovery plan).
- **Deviations:** the fixture is 927 lines against 450–700; the harness has a third `Wiring_WiFi` matrix compiled with the flag on the host; Copilot's own `src/` line count (22,366) differs from the S-1 to S-4 method (the recorded figure is Claude Code's); a few in-place python edits to test files.
- **Rounds:** Stage 6 one round; Stage 7 once (VERIFIED WITH CONCERNS, both Low). Ships in v46 with S-1 to S-4 and S-6. **The PR is marked "do not merge until v45 is cut and #128, #130, #131 and #132 are merged"** (release/v45 is cut: PR #135).

## For the architect

- **C1:** narrowing the `wifiGuard.` normalization to the four entry-reset lines in the three structural tests is a small test-only fix; left for the architect (or the next WO that touches those tests).
- **For the simplify series (record, don't do):** the end-of-cycle invariants (about 40 log-only lines) are judged in the necessity review on whether their warnings ever fire in the fleet logs; the stale comments at the working-configuration block and the `useNetworkStandby` alias are trivial removals.
