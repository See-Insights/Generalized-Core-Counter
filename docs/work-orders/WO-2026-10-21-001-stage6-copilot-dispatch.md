AGENT: Copilot · MODEL: claude-sonnet-5.5 (probed 2026-10-20, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:**
  - `src/state/State_Sleep.cpp` **only** in `src/`: option D (below).
  - **Tests:** extend the S-1 harness (`tests/sleep_teardown_equivalence_test.{cpp,sh}`, `tests/stubs/sleep_teardown/**`) or add a sibling runner that reuses its fakes, and add the new fixture `tests/fixtures/sleep_execution_transcript.txt`; add the structural test; re-anchor the three tests named below; widen `clock_owner_structural_test.py`; report each change.
- **Run** the host suite (serially) and the local ARM release build.
- **Scratch directory:** `build-tmp/wo20261021-001-stage6/`. Remove **only it** when you finish.

**Not authorized:**
- **Any new file or header under `src/`; any change to `SleepTeardown.{h,cpp}`, to `drainSerialBeforeSleep` (it stays where it is), to any include, or to any other `src/` file.**
- Any change to a sleep configuration, breadcrumb, watchdog call, log line (text, level, condition), reset, transition, wake classification or the order of any of them.
- Moving the plan and heap guard (`:707–733`), the preflight and Wi-Fi guard (`:735–877`), or anything from `:1113` on; removing hibernate, the STOP fallbacks or any check (ruling 4).
- `docs/`, `lib/`, `project.properties`, Device OS.
- Commits, pushes, merges, branch changes, stash, reset, checkout or restore. Device or network access beyond the Particle compile. Deleting anything you didn't create, including `build-tmp/`.

**SIZE BUDGET:**
- **`src/`: at most +20 net lines, HARD** (Claude Code's prototype of this exact design is **+17**, so you have 3 lines of headroom). A forward declaration, an extra helper, a new file or an extra wrapper line is a STOP: report, do not compress (§12.3).
- **Tests:** guideline +175 (new scenarios and a fixture on the existing harness); record the actual and flag it at about twice that.

# Stage 6 dispatch: WO-2026-10-21-001 (Sleep S-2, option D: in-file phase functions and the standby-return owner)

**Goal, in plain language:** make the sleep-execution phases of `handleSleepingState()` named, file-local functions with one owner for the three standby-return fields, in the same file (no new module). **No behaviour change:** the same sleep configurations, breadcrumbs, watchdog order, log lines, resets, transitions and wake classification, pass by pass.

**Repository:** this worktree, branch `wo/2026-10-21-001-sleep-s2-execution` (cut from the S-1 branch tip `0cb5622`; stacked on PR #128). HEAD is the commit that adds this dispatch. Don't switch branches. Main is Device OS 6.4.1; build with the 6.4.1 toolchain. The base `src/` tree hash is `7fc57df7a302712ea3f26ff37758ce455aaa6203808a7092d11636f5d06d190a`-family (S-1's verified candidate plus its later records; compute yours with `find src -type f | LC_ALL=C sort | xargs shasum -a 256 | shasum -a 256` before you start and report it).

**Binding spec:** `docs/work-orders/WO-2026-10-21-001-sleep-s2-execution.md`, **including the architect's ruling "option D"**, which supersedes its design items 1–3, the `SleepExecution.{h,cpp}` file and the S-2a/S-2b split. Also `WO-2026-10-21-001-step0-report.md` (range re-check, readers and writers, the ruling-8 table) and `WO-2026-10-21-001-step0-addendum-report.md` (the D prototype and its test results). **`WO-2026-10-21-001-optionD-prototype.diff` is Claude Code's uncompiled sketch of this change (+17); it is a reference, not a patch to apply blindly: you may follow it exactly, but you own compiling and verifying it.** Where this dispatch and the WO differ, the WO's ruling wins; report the difference.

## Do (option D)

1. **In `State_Sleep.cpp`'s anonymous namespace** (before its closing `} // namespace`, now `:121`), add at most **three functions and the structs**:
   - `struct WakeContext { bool pirWake, buttonWake, timerWake; const char *wakeReasonLabel; };`
   - **one file-static struct** holding the three standby-return fields (`preserveAfterNormalReturn`, `forceModemOff`, `error`), always defined (so S-1's by-reference call needs no non-cellular dummies, which go);
   - `effectiveStandby(requested)`: the policy `:181–201` (the comment and the `useNetworkStandbyRequested` computation `:172–180` stay in the handler);
   - `hibernateAttempt(overnightFallbackSleep, wakeInSeconds)`: `:639–705`, the whole `#if PLATFORM_ID == PLATFORM_BORON` block;
   - `sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)`: `:879–1111`; returns `true` when all sleep attempts failed (the caller returns, exactly as the `return;` at `:1031` did), otherwise fills `wake` (the only four locals read after `:1111`).
   Moved lines stay **byte-identical** apart from the three field renames (`standbyReturn.…`), the `return;` → `return true;`, the added `wake = {…};` and `return false;`.
2. **The handler** calls them where the blocks were and acts on the results exactly as the inline code did; it unpacks `pirWake`, `buttonWake`, `timerWake` and `wakeReasonLabel` from `wake` (the later code reads those four names, unchanged). `SleepTeardown::run(...)` keeps its by-reference parameters; the call passes `standbyReturn.forceModemOff, standbyReturn.error`.
3. **Only `effectiveStandby`, `sleepAndResume` and that call write the three fields** (a structural test checks it); no `retained` anywhere new.

## Verify (all required)

- **Equivalence (extend the S-1 harness; ruling 7):** the same runner and fakes under `tests/stubs/sleep_teardown/`; add fakes only for what the moved code calls. **Record `tests/fixtures/sleep_execution_transcript.txt` once from the base** (`git show <base>:src/state/State_Sleep.cpp`, base = the commit recorded in the fixture header, default `1e6d34f`, never by line number); a generator mode regenerates it. **Extract the three functions verbatim by marker text from the *current* file and from the base, and fail with `COPY_MISMATCH` on any drift** between the extracted copy and what the harness compiled (§13 (g)); extract the handler's call-site lines by marker too. **Scenarios** (edges, not a full cross-product; keep the fixture compact): hibernate: not eligible; wake pin held low; stale-interrupt clear fails; alarm programming fails; `System.sleep` returns. ULP: returns OK on the button, the sensor pin and the timer; returns an error and STOP-with-GPIO succeeds; that fails too and timer-only STOP succeeds; all fail (the reset and the ERROR transition). Each of those with standby on and off, cellular and Wi-Fi, and from each prior standby-return state (including a pending forced-off); serial connected or not. **Freeze, per pass:** the configuration fields (mode, wake pins and edges, network flag, duration); the breadcrumb order; the watchdog stop, pause, resume and restore order; the `AwakeCycles` and `HibernateCycle` calls; the three fields after the pass; log lines character for character; resets and transitions; the returned `WakeContext`. Show the transcript is not trivial (passes, distinct outcomes). **The S-1 teardown transcript must still match byte for byte** (`tests/fixtures/sleep_teardown_transcript.txt`, unchanged).
- **Structural test (new):** the three fields are declared once, in one struct in the anonymous namespace; only `effectiveStandby`, `sleepAndResume` and the teardown call arguments mention them (comment-stripped); `SleepTeardown.{h,cpp}` unchanged; no new file under `src/`; no `retained` added; every `SystemSleepConfiguration` is still a fresh local; the moved blocks are byte-identical to the base apart from the listed edits.
- **Re-anchor (ruling 8), each with the mutation it must fail:** `occupancy_report_by_mode_test.sh` (extract `bool pirWake = …` from `sleepAndResume`; order the handler's `sleepAndResume(` call against the sleep-or-suppress decisions; mutation: move the occupancy check below that call), `sensor_wake_source_test.sh` (extract from `sleepAndResume`; the unpack line must not match; mutation: change `wakePin == sensorWake.pin`), `loop_stage_sleep_prep_exclusion_test.py` (15 `transitionTo(` in `handleSleepingState()` and exactly 1 in `sleepAndResume`; mutation: add a transition to either). **Widen `clock_owner_structural_test.py` (test-only, architect):** scan all of `src/` for a direct `Clock::isPlausibleEpoch(`, not only `State_Sleep.cpp`; mutation: a direct call in another `src/` file must fail it. If `firmware_update_dwell_test.sh` is touched, make it skip block and trailing comments (S-1 C1); otherwise leave it.
- **Mutations (each in a scratch copy; each must fail the transcript or a named test, or be recorded as unpinned at the base):** (1) on a ULP error skip STOP-with-GPIO and go straight to timer-only; (2) leave `forceModemOff` unset after a standby sleep error; (3) drop `HibernateCycle::abandon()` after a returned hibernate; (4) move `thrashGuard.markProgress("WAKE_FROM_SLEEP")` after the serial re-wait; plus a write to a standby-return field outside the allowed functions (structural) and a `retained` probe. Scratch copies have no `.git`; judge them on the tests that run the code, and note that a few structural tests need history.
- **Golden payload test** unchanged; **full suite** before and after, `N/N (sh via zsh, py via python3)`, serial; baseline **93**.
- **Local ARM release build** (6.4.1 Boron, fresh `BUILD_PATH_BASE`, `-frandom-seed=p6`; "before" from `git archive HEAD`): text/data/bss before and after; explain any change over ±64 B by symbol with `nm -C -S`; list any new static-initialization stub; **retained check (narrowed rule):** the whole `.backup` section before and after; `retainedBoot` at `0x2003f420`, the hibernate fields `0x2003f480/488/48c/490`, the signature word `0x2003f494`. Expect text change near zero (code moves within one file) and a small BSS change at most (the always-defined struct).
- `git diff --check` clean on new lines; `wc -l State_Sleep.cpp` (1,331 before).

## Implementation Report (≤ 150 lines, your final message)

Files changed with net and gross `src/` lines and the arithmetic against +20 (the net should be near +17); test lines added and removed; each re-anchored test and its mutation; the transcript (scenario and pass counts, distinct outcomes, fixture header, old-versus-new result, `COPY_MISMATCH` demonstrated); the byte-diff of the moved blocks; the suite before and after; the mutations; the build sizes and symbol deltas; the `.backup` table; the `src/` hash before and after; scratch removal; deviations (or "none"); the model used.
