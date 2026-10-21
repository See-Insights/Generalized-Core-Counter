# WO-2026-10-21-001: Sleep S-2: extract sleep execution and the standby-return owner (behavior-neutral)

**Author:** architect, 2026-10-21 · **Status:** ready (Step 0 first)
**Branch:** `wo/2026-10-21-001-sleep-s2-execution`, cut from the S-1 branch tip `0cb5622` (PR #128), so S-2 stacks on S-1. The PR's base is `wo/2026-10-20-003-sleep-s1-teardown`; after #128 merges it is retargeted to main.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-20-002-step0-report.md` (round 1) sequence item 2 and the standby-return state paragraph; `WO-2026-10-20-002-step0-addendum-report.md` (round 2) Q9 rows 5, 7 and 8 and sequence item 2; WO-2026-10-20-002 rulings 1–8. Those reports cite `a3e012d`. **This WO cites `src/state/State_Sleep.cpp` at `0cb5622` (after S-1, 1,331 lines).** Re-check every range.

## Plain goal

Move the part of `handleSleepingState()` that actually puts the device to sleep and brings it back into its own file, `state/SleepExecution.{h,cpp}`. That covers the hibernate attempt, the ULP call with its two STOP fallbacks, and the common wake restore. The three standby-return fields go with it, so they have one owner. **No behavior change:** the same sleep configurations, breadcrumbs, watchdog order, log lines, resets, transitions and wake classification, pass by pass. This is the second of six sleep WOs.

## What moves (at `0cb5622`; re-check every range)

- **Serial drain helper:** `drainSerialBeforeSleep()` with its comment, `:43–49`. Its only callers are in the moved ranges (`:676, 937, 989, 1004`).
- **The standby-return fields:** `preserveStandbyAfterNormalReturn`, `forceModemOffAfterStandbyFailure` and `standbySleepFailureError`, with the non-cellular dummy locals S-1 added, `:152–158`.
- **The effective-standby policy:** `:172–201`. It reads and conditionally clears `preserveStandbyAfterNormalReturn` and produces `useNetworkStandbyRequested` and `useNetworkStandbyEffective`.
- **The Boron hibernate attempt:** `:639–705`. It runs from the `#if PLATFORM_ID == PLATFORM_BORON` line to its `#endif`, and covers the AB1805 alarm, the fresh hibernate configuration, breadcrumbs 24–28, `HibernateCycle::armForSleep` and `abandon`, and the fall-through warnings.
- **The ULP configuration, call cascade and common resume:** `:879–1111`. It covers the sensor wake source, the fresh ULP configuration, the watchdog pause, the ULP call, the standby-return writes `:947–968`, the STOP-with-GPIO and timer-only STOP fallbacks, the all-attempts-failed reset with its ERROR transition (ending `:1031`), the watchdog restore, the serial re-wait, `WakeReturn` and the wake classification `:1100–1103`, through `logPowerState("post-wake")`.
- **Size:** about 340 lines leave the sleep file.

**Stays in `State_Sleep.cpp`:**
- `logWakeSummary` (S-6).
- The plan and heap guard, `:707–733` (S-4).
- The preflight and Wi-Fi guard, `:735–877` (S-5).
- Everything from `:1113` on (S-6).

## Design (do not widen)

1. **One owner.** `SleepExecution` owns the three standby-return fields as one file-static struct in `SleepExecution.cpp`. No new globals, no `retained` and no framework. **No other file writes them.** `SleepTeardown` reads and clears the pending forced-off error through one `SleepExecution` call, replacing S-1's by-reference parameters and the non-cellular dummies.
2. **Interface, at most four entry points:**
   - **The policy:** takes `requested` and returns `effective`, with today's reads and clears.
   - **The hibernate attempt:** returns only when hibernate did not happen, exactly as today's fall-through.
   - **Sleep and resume:** takes `wakeInSeconds`, `useNetworkStandby` and `sleepReason`, and returns a small `WakeContext` (`pirWake`, `buttonWake`, `timerWake`, `wakeReasonLabel`, which are the only four locals read after `:1111`). It also returns a "handled" result for the all-attempts-failed path, so the caller returns exactly as today.
   - **The forced-off query or clear,** for the teardown.
3. **The call sites.** `handleSleepingState()` calls these where the blocks were, and acts on the results exactly as the inline code did.
4. **Ownership stays where it is.** `ModemHealth` stays the only writer of the latch fields, and `HibernateCycle` stays the owner of its retained fields.
5. **Fresh local configurations.** The fresh-local rule for `SystemSleepConfiguration` (the leak fix `1c74ac8`) is kept in every moved call.

## Step 0 (Claude Code, read-only, §13 (c))

- Re-check every range above, marking each holds, moved or wrong. Name every reader and writer of the three fields and of `drainSerialBeforeSleep` by bare name.
- **Size estimate with its reasoning,** counting non-move lines only: the header, the struct, the parameters, the `WakeContext`, the call sites, and the `SleepTeardown` signature change, less the removed dummies.
- **Pre-ruled split.** If the estimate is over +20, S-2 is split before dispatch, and Step 0 ends PROCEED on the split, not STOP:
  - **S-2a:** everything above except the hibernate attempt. Hibernate stays in `State_Sleep.cpp` and calls the drain helper through the header.
  - **S-2b:** the hibernate move, as its own small WO.
  - Report which cut applies.
- **Tests anchored on text or line order (ruling 8).** List every test that reads the moved code by marker, regex or position. Already known to name the moved text:
  - `sleep_breadcrumb_sequence_test.py`
  - `sleep_config_leak_test.cpp`
  - `sleep_config_ownership_structural_test.py`
  - `sleep_gate_modem_off_test.sh`
  - `awake_cycle_counters_test.sh`
  - `occupancy_report_by_mode_test.{cpp,sh}`

  36 test files name `State_Sleep.cpp`, so find them all. For each one, say whether it gets re-anchored or still binds. The evidence is a mutation of the property it guards.
- **History (§12.1):** confirm with `git log -S` that nothing here is a restoration.
- **Report:** at most 150 lines, committed on the WO branch.

## Equivalence (freeze the output, not the code)

- **Extend the S-1 harness (ruling 7):** the same runner and fakes under `tests/stubs/sleep_teardown/`. Add fakes only for what the moved code calls. Don't build a new harness.
- **Record the fixture from the base:** `tests/fixtures/sleep_execution_transcript.txt`, recorded once from `0cb5622` with that hash in its header. A generator mode regenerates it from the base.
- **Scenarios.** Cover the edges below, not a full cross-product, and keep the fixture compact:
  - **Hibernate:** not eligible; wake pin held low; stale-interrupt clear fails; alarm programming fails; `System.sleep` returns.
  - **ULP:** returns OK on the button, the sensor pin and the timer; returns an error, after which STOP-with-GPIO succeeds; that fails too and timer-only STOP succeeds; all fail, giving the reset and the ERROR transition.
  - **Each of those** with standby on and off, on cellular and Wi-Fi, and from each prior standby-return state.
  - **Serial:** connected or not.
- **Freeze, per pass:**
  - the configuration fields (mode, wake pins and edges, network flag, duration);
  - the breadcrumb order;
  - the watchdog stop, pause, resume and restore order;
  - the `AwakeCycles` and `HibernateCycle` calls;
  - the three fields after the pass;
  - the log lines, character for character;
  - resets and transitions;
  - the returned `WakeContext`.

## Budget

- **`src/`: at most +20 net non-move lines, hard** (round 1 estimated +18; S-1 estimated +14 and landed at +20). Step 0's split rule applies first. If Stage 6 still goes over, stop and report; don't compress (§12.3).
- **Tests:** guideline +175 (new fakes on the existing harness). Record the actual and flag it at about twice the guideline.

## Tests and checks

- The transcript matches exactly, the golden payload test is unchanged, and the suite reports `N/N`, run serially.
- Re-pointed tests are not weakened; each re-anchored test fails its mutation. If `firmware_update_dwell_test.sh` is touched, make it skip block and trailing comments (S-1 C1).
- **Structural:** none of the three fields is defined or written outside `SleepExecution.cpp`; no `retained` appears in the new files (negative checks for both); every `SystemSleepConfiguration` is still a fresh local.
- **Mutations.** Each must fail the transcript or the named test aimed at it:
  1. On a ULP error, skip STOP-with-GPIO and go straight to timer-only.
  2. Leave `forceModemOffAfterStandbyFailure` unset after a standby sleep error.
  3. Drop `HibernateCycle::abandon()` after a returned hibernate.
  4. Move `markProgress("WAKE_FROM_SLEEP")` after the serial re-wait.
- **`nm` retained check** (narrowed rule): expect `.backup` unchanged. ELF delta explained by symbol.
- **Whitespace:** carried byte-for-byte is waived; new lines must be clean.

## Agents and models

- **Step 0:** Claude Code (controller), read-only, in its own session. No build.
- **Stage 6:** Copilot, Sonnet tier, **high**. It's a stateful owner with a teardown interface change, plus sleep fakes on the harness.
- **Controller, before Stage 7 (§13 (g)):** regenerate the transcript from the base and confirm it is byte-identical; reproduce the build, `nm`, the mutations and the suite, serially. The baseline ELF can be reused if the base `src` tree hash matches S-1's verified candidate.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. It checks:
  - the transcript's provenance and edge coverage;
  - every moved range byte-identical apart from field access;
  - one owner for the three fields;
  - the same returns, resets and transitions at the call sites;
  - the ruling-8 list complete.
- **Stage 8:** pre-approved on VERIFIED, or on VERIFIED WITH CONCERNS when every concern is Low and already closed by another test, with net `src/` ≤ +20. Any High or Medium finding comes to the architect. At most two rounds.
- **PR:** marked **"do not merge until v45 is cut and #128 is merged"**.

## Out of scope (recovery plan, not here)

- **Duplicate helper:** `drainSerialBeforeSleep()` is defined twice, in anonymous namespaces in `Generalized-Core-Counter.cpp:180–186` and `State_Sleep.cpp:43–49`. Record it as a simplify-series candidate; S-2 only moves the sleep copy.
- **No removals in the split:** no removal of hibernate, the STOP fallbacks or the heap guard (ruling 4; the v45 soak gathers that evidence).

## Bench

None. Ships in v46 with S-1 and S-3 to S-6.

## Closing record

- Budget versus actual (non-move `src/`, moved lines, sleep `wc -l`, tests).
- The cut taken (S-2, or S-2a with S-2b).
- The transcript hash and its provenance.
- ELF by symbol.
- The retained outcome.
- The ruling-8 table.

## Architect's ruling on Step 0 (21 Oct): option D (Chip approved)

Step 0 (`02514d5`) found that every new module carries a fixed cost of about 35 non-move lines. Its counts were S-2 at +65 and S-2a at +59. Across S-2 to S-6, that is roughly +150 to +200 lines of wiring. **That cost is not accepted.** The split exists to give readable phases and per-phase transcripts, and neither needs a file per phase.

**This ruling supersedes:**
- Design items 1–3;
- the `state/SleepExecution.{h,cpp}` file;
- the pre-ruled S-2a/S-2b split.

**Option D:**

1. **No new file or header.** The phases become file-local functions in `State_Sleep.cpp`'s anonymous namespace: the effective-standby policy (`:181–201`, per Step 0's adjustment), the hibernate attempt and sleep-and-resume. Sleep-and-resume returns the four-field `WakeContext`, plus "handled" for the all-attempts-failed path. Add at most three functions and that struct.
2. **The three standby-return fields** become one file-static struct in the same anonymous namespace. Only those functions and the policy write the fields, and a structural test checks that.
3. **S-1's interface is unchanged.** `SleepTeardown` keeps its by-reference parameters, and `drainSerialBeforeSleep` stays where it is.
4. **The transcript plan is unchanged** (it extends the S-1 harness). The harness extracts the new functions verbatim by marker, with `COPY_MISMATCH` on any drift (§13 (g)).
5. **Ruling-8 table.** Re-run it on the D prototype and re-anchor whatever fails. `clock_owner_structural_test.py` is widened to scan all of `src/` in this WO, as a test-only change, because it is the same silent-weakening risk as S-1's F1.

**Budget: at most +20 net non-move `src/` lines, hard.**

**Step 0 addendum (Claude Code):**
- Prototype D and count it the way the first prototype was counted.
- **≤ +20:** dispatch Stage 6 (Copilot, Sonnet, high) without coming back to the architect.
- **Over +20:** stop and report.

Stage 7 and the Stage 8 pre-approval are unchanged.

**S-3 to S-6:** these use the same pattern, in-file functions with no new module. Each Step 0 prototypes and counts the change before dispatch. Round 1's estimates are not used.

## Chip's direction for after the split (21 Oct)

Once S-6 lands, the simplify series starts with a **necessity review of every check on the sleep path**. That covers each safe-to-sleep check, guard, fallback, wait and retry, and asks the same questions of each:
- What does it protect against?
- Is there evidence it fires (v45/v46 soak and fleet counts, incidents)?
- Is it an edge case?
- What does it cost in lines?
- Should it be kept, simplified or removed?

**The goal is a measurable reduction in `src/` lines**, in small removal WOs, each with evidence. The candidates already named:
- the dead precondition-timeout arm (recovery plan item 17);
- the duplicate `drainSerialBeforeSleep()`;
- hibernate, the heap guard and the STOP fallbacks, judged against the v45 counts (ruling 4).

## Stage 6 result and controller reproduction (21 Oct)

**Stage 6** (Copilot, Sonnet 5.5, high; implementation commit `e721330` on `wip/s2-sleep-execution`, no PR): option D as ruled. `State_Sleep.cpp` 1,331 → 1,348 (**net +17**, within +20); only `src/state/State_Sleep.cpp` changed in `src/` (the `src/` tree hash `7fc57df7…`-family → `1a27b3d912d354481eb0bcd83c05c287c8ed75daea5728dd2e38b878632b5100`); `SleepTeardown.*` untouched, no new `src/` file. The file differs from Claude Code's pre-dispatch prototype in one blank line. Tests: about +884 lines excluding the generated fixture (5,810 lines) and +52/−10 in edits: far above the +175 guideline, flagged.
**Controller reproduction** (Claude Code, Device OS 6.4.1 Boron, fresh `BUILD_PATH_BASE`, `-frandom-seed=p6`, "before" is `git archive HEAD`):
- **Fixture:** `--regenerate` from the base `1e6d34f` is **byte-identical** to the committed `tests/fixtures/sleep_execution_transcript.txt` (137 scenarios, 157 passes); the normal run matches in both directions; the `COPY_MISMATCH` demonstration fires; the S-1 teardown fixture is byte-identical to `HEAD`.
- **Suite 95/95** (sh via zsh, py via python3, serial; baseline 93); the golden fixture is unchanged. `git diff --check`: five hits, all on trailing-whitespace lines carried byte-for-byte from the base (waived); new test files clean.
- **Build (Boron):** text/data/bss `151532/1094/2228 → 151484/1094/2236` (**−48 text, +8 bss**): the three handler-local statics (1, 1, 4 B) are replaced by the 8-byte `(anonymous namespace)::standbyReturn`; `handleSleepingState` 0x15b0 → 0x1580; no new symbol (the new functions are inlined) and no new static-init stub (22 before and after). **`.backup` unchanged** (`0x98` bytes, 25 retained symbols at the same addresses; only the flash load address moved, `0xd9370 → 0xd9340`): retained outcome A.
- **Mutations (git-backed copy; a `.git`-less copy makes the harness fail for any mutant and proves nothing):** `System.sleep(stopConfig)` neutralized, `forceModemOff` not set after a standby error, `HibernateCycle::abandon()` dropped, `WAKE_FROM_SLEEP` progress moved after the serial re-wait, `sleepAndResume` returning `false` after the all-attempts-failed transition, the `Cellular.isOff()` term dropped from the policy, a stray write to a standby-return field, and a static `ulpConfig` each fail the transcript (the first seven) and the structural test (all eight).

## Stage 7 result and architect's rulings (21 Oct)

**Stage 7** (Codex `gpt-5.6-sol`, high, on `e721330`; `WO-2026-10-21-001-stage7-verdict.md`): **NOT VERIFIED on F1 alone**, retained outcome A. Checks 1 (transcript regenerated byte-identical, 137 scenarios / 157 passes, `COPY_MISMATCH` real, two own mutations caught), 2 (moved bytes: the hibernate body has no byte difference, the other two differ only by the listed edits), 3 (one owner), 4 (returns, resets, transitions), 6 (ELF and retained) and 7 (re-anchored tests, property mutations each caught) PASS; check 8 CONCERN (test volume, below). **F1 (Medium, pre-existing):** the exact Argon build fails at `PmicFaultMonitor.cpp:70` (`invalid use of incomplete type 'class PMIC'`: Device OS 6.4.1 defines no `PMIC` on Argon) **identically on the parent and the target**; Claude Code's own Argon builds of both trees fail the same way. With a scratch-only two-line PMIC guard applied to both sides the Argon builds pass (`137520/1058/2124 → 137576/1058/2132`) and the target `State_Sleep.o` has no hibernate helper symbol, so the changed non-cellular path compiles and `hibernateAttempt` compiles away off Boron.
**Rulings (architect, 21 Oct):**
- **The Argon exact-build check is waived for S-2: "waived by the architect: pre-existing on the parent"** (§12.4 exception; not caused by S-2; the isolation build passes).
- **Stage 8 proceeds.** The PR's base is `wo/2026-10-20-003-sleep-s1-teardown` and it is marked **"do not merge until v45 is cut and #128 is merged"**.
- **Argon support is dropped, as a removal WO, not a guard** (Chip's decision): recovery plan, the bullet next to the Boron versus M-SoM portability item, and item 17; scheduled with the simplify series after the split; the Wi-Fi paths stay (the P2 needs them).
- **C1 (test volume) is accepted.** S-3 onwards state their expected fixture growth in Step 0.

## Closing record

| Item | Budget | Actual |
|---|---|---|
| `src/` net | ≤ +20, hard (option D) | **+17** (`State_Sleep.cpp` 1,331 → 1,348; +364/−347; +13 non-blank); only that file changed in `src/`; Claude Code's pre-dispatch prototype was +17 |
| Tests | guideline +175 | about **+884 lines excluding a 5,810-line generated fixture**, +52/−10 in edited tests; accepted (C1) |
| Suite | baseline 93 | **95/95** (sh via zsh, py via python3, serial); golden fixture sha256 `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637` unchanged |

- **The design (option D):** no new file or header; `WakeContext`, one file-static `standbyReturn` struct for the three standby-return fields, and `effectiveStandby`, `hibernateAttempt`, `sleepAndResume` in `State_Sleep.cpp`'s anonymous namespace; `SleepTeardown` unchanged (its by-reference call now passes the struct members; the non-cellular dummies are gone).
- **Transcript provenance:** `tests/fixtures/sleep_execution_transcript.txt`, base `1e6d34f` (its `src/` equals S-1's tip `0cb5622`); 137 scenarios (cellular 113 + Wi-Fi 24), 157 passes; file sha256 `aca33dd1cb5defe9c22a8f6e55f47bc6b328caf28b39118c3decfb52cc4d5aac`, body `14bcf926…`. **Regenerated from the base by Claude Code and by Codex: byte-identical.** The S-1 teardown fixture is unchanged.
- **ELF (6.4.1 Boron):** text/data/bss `151532/1094/2228 → 151484/1094/2236` (**−48 text, +8 bss**: the 8-byte `standbyReturn` replaces three statics of 1+1+4 B); `handleSleepingState` `0x15b0 → 0x1580`; the three new functions are inlined (no symbols); static-init stubs 22 before and after.
- **Retained outcome A:** the whole `.backup` section (`0x98` bytes, 25 retained symbols) byte-identical; only the flash load address moved (`0xd9370 → 0xd9340`). No `retained` keyword added.
- **Ruling-8 table (final):** re-anchored and mutation-checked: `occupancy_report_by_mode_test.sh` (the pending-occupancy guard moved below `sleepAndResume` fails), `sensor_wake_source_test.sh` (`== sensorWake.pin` → `!=` fails), `loop_stage_sleep_prep_exclusion_test.py` (a helper transition fails; 15 in the handler and 1 in `sleepAndResume`), `clock_owner_structural_test.py` widened to all of `src/` (a direct call in another file fails). Still binding, each caught by its property mutation: `sleep_breadcrumb_sequence_test.py` (breadcrumb 28), `sleep_config_ownership_structural_test.py` (`ulpConfig` static), `sleep_gate_modem_off_test.sh` (standby network flag), `awake_cycle_counters_test.sh` (`recordSleepReturn`), `modem_teardown_confirmation_logging_test.py` (`sleep-attempts-failed-reset` log), `clock_hibernate_helpers_move_structural_test.py` (hibernate stays in the file, so its include stays). The scanners over all of `src/` (`modem_health_owner`, `hibernate_cycle_single_owner`) bind to any new file. `firmware_update_dwell_test.sh` was not touched, so the C1 comment fix does not apply.
- **Mutations** (Claude Code, git-backed copy): `System.sleep(stopConfig)` neutralized, `forceModemOff` unset after a standby error, `HibernateCycle::abandon()` dropped, `WAKE_FROM_SLEEP` progress moved after the serial re-wait, `sleepAndResume` returning `false` after the all-attempts-failed transition, the `Cellular.isOff()` term dropped from the policy, a stray write to a standby-return field and a static `ulpConfig`: all caught by the transcript and the structural test (a `.git`-less copy makes the harness fail for any mutant and proves nothing).
- **Argon: waived by the architect: pre-existing on the parent** (above).
- **Deviations:** additive edits to three `sleep_teardown` stub headers; some test edits made by script; five `git diff --check` trailing-whitespace hits, all on lines carried byte-for-byte from the base (waived).
- **Rounds:** Stage 6 one round; Stage 7 once (NOT VERIFIED on a pre-existing Argon failure, waived). Ships in v46 with S-1 and S-3 to S-6. **The PR is marked "do not merge until v45 is cut and #128 is merged".**
