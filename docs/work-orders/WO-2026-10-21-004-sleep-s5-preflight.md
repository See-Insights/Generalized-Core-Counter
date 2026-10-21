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
