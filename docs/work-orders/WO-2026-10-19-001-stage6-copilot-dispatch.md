AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-19, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:**
  - `src/Generalized-Core-Counter.cpp`: remove the moved functions (and `batteryTierShortName`, see below), their forward declarations at `:119–120`, and nothing else; add the include the file then needs;
  - **new** `src/power/ConnectivityFailsafe.{h,cpp}` (one pair);
  - `src/state/State_Common.h`: move the declarations at `:192`, `:247` and `:255` to the new header **only** if keeping them there is impossible (say so); otherwise leave them and have the new header include `State_Common.h` where needed;
  - `src/state/State_Connect.cpp` and `src/state/State_Sleep.cpp`: add the include only if the build needs it.
- **Tests:** re-point every test that anchors on text in `Generalized-Core-Counter.cpp` for a moved function so it reads the new file (never weaken it); add the structural test the WO lists; report each change.
- **Run** the host suite and local ARM release and bench builds.
- **Scratch directory:** **`build-tmp/wo20261019-001-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any change to a moved function body, to what any `#if CONNECTIVITY_FAILSAFE_TEST_MODE` arm contains, or to a comment or logic in them.
- `src/diagnostics/ConnectivityFailsafeTest.cpp` (rule 4), the `retained` definitions (including `retainedBoot`), `BuildProfile.h`, `FailsafeDeferReason` (`State_Common.h:63`).
- Any other `src/` change, `docs/`, `lib/`, `project.properties` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset, checkout or restore. Device or network access beyond the Particle compile. Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:**
- **`src/`:** **≤ +20 net non-move lines, HARD**: moved lines count 0, reported separately as gross; every new header line, include and namespace line counts. Over means STOP and report (§13 (b)).
- **Tests:** guideline +100; record the actual.

# Stage 6 dispatch: WO-2026-10-19-001 (Step 6 P-2: move the connectivity failsafe out of main)

**Goal, in plain language:** move the connectivity failsafe (260 function lines, about 274 with comments) from `Generalized-Core-Counter.cpp` into `src/power/ConnectivityFailsafe.{h,cpp}`, **unchanged**. This is the last-resort recovery (stage 2 reset, stage 3 deep power-down), so "unchanged" is checked strictly.

**Repository:** this worktree, branch `wo/2026-10-19-001-step6-p2-failsafe` (from main `3975e9c`, P-0 and P-1 merged). HEAD is at the commit that adds the WO and this dispatch. Don't switch branches. Main is Device OS 6.4.1; build with the 6.4.1 toolchain.

**Binding spec:** `docs/work-orders/WO-2026-10-19-001-step6-p2-failsafe-move.md`, including **Claude Code's citation re-check at `3975e9c`** (the ranges, the findings). Where this dispatch and the WO differ, the WO wins; report the difference.

## Do

1. Move the functions in the WO's re-check, byte-for-byte. Allowed edits: linkage keywords, declarations, includes. The helpers may stay internal to the new file; `clearConnectivityFailsafeRecovery`, `connectivityFailsafeSupervisor`, `failsafeDeferReasonName` and `claimFailsafeDeferLog` need external declarations. Their `State_Common.h` declarations (`:192`, `:247`, `:255`) may stay; `connectivityFailsafeSupervisor()` needs a declaration reachable from main (`:1280`).
2. **`batteryTierShortName`** (main `:211`, no header declaration, used only by the supervisor at `:2226`): move it with the failsafe, and say so in the report as a deviation from the WO's list.
3. The new file includes `state/StateMachine.h` for `state`, `ab1805` and `session`.
4. Re-point the tests the WO's re-check names (and any others a bare-name search finds): they extract function text from `Generalized-Core-Counter.cpp`; point them at the new file with the same assertions.

## Verify (all required)

- **Byte-identical bodies:** every moved range at `HEAD` against the new file, including both arms of each `#if CONNECTIVITY_FAILSAFE_TEST_MODE` block.
- **Both builds:** the release build (6.4.1 Boron, fresh `BUILD_PATH_BASE`) and the bench build with `EXTRA_CFLAGS="-DCONNECTIVITY_FAILSAFE_TEST_MODE=1"` (another fresh `BUILD_PATH_BASE`), each before (`git archive HEAD`) and after. Text/data/bss for all four; explain any text change over ±64 B by symbol with `nm -C -S`. If a fresh `BUILD_PATH_BASE` breaks the link step in your environment, say so and say what you did instead.
- **Retained check under the narrowed rule:** the whole `.backup` section before and after for the release build; `retainedBoot` must stay at `0x2003f420` and the signature word at `0x2003f494`; list any move and classify it.
- **Golden payload test** unchanged; **full suite** before and after, `N/N (sh via zsh, py via python3)`; the baseline at `HEAD` is 84. Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
- **Structural:** no failsafe function still defined in main; no duplicate definitions; `diagnostics/ConnectivityFailsafeTest.cpp` is byte-identical to `HEAD`.
- **Main `wc -l`** before (2280) and after.

## Implementation Report (required, as your final message)

- **Files:** new and changed, with net and gross `src/` lines and the arithmetic against +20; test lines added and removed; each re-pointed test and why it is not weaker.
- **Linkage and moved-with-the-code choices** (including `batteryTierShortName`), with file:line.
- **Results:** the byte-diff for both arms, the suite before and after, the four build sizes and the symbol-level text deltas, the full `.backup` table.
- **Confirmations:** you removed only your scratch directory; `ConnectivityFailsafeTest.cpp` unchanged.
- **Deviations** (or "none"), and **the model used**.
