AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-17, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-17-001-step6-p0-retained-layout`).
- Run the host suite and the tests; run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261017-001-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies.
- Write your verdict to the file named on the command line.

**Not authorized:**
- Changes outside your scratch locations (and the verdict file).
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-17-001 (Step 6 P-0: pin the retained layout)

**Goal, in plain language:** put the nine unvalidated retained variables in `Generalized-Core-Counter.cpp` into one `retained` struct with a magic and a version, checked before anything reads them, so that no later code move can shift them and a layout change resets them cleanly. No other behaviour change.

**Binding spec:** `docs/work-orders/WO-2026-10-17-001-step6-p0-pin-retained-layout.md`, including Claude Code's citation re-check at `5a28885` and its Stage 6 observations. **Architect's rulings (17 Oct):** a shifted signature word (a one-time full retained clear) is acceptable; the STOP case is only an unvalidated symbol moving while the signature stays.

**Inputs:** `docs/work-orders/WO-2026-10-17-001-stage6-copilot-dispatch.md`, and Copilot's Stage 6 report (not in the repo; the figures below are Claude Code's reproduction of it). Treat every claim as unverified.

**What changed** (uncommitted):
- **`src/`:** `Generalized-Core-Counter.cpp` +10/−9 and new `src/persist/RetainedBoot.h` +18, net **+19** (hard limit +20).
- **Tests:** new `retained_boot_test.cpp`, `retained_boot_test.sh`, `retained_boot_structural_test.py`; `serial_settle_test.py` +11/−1. About 216 lines added against a +100 guideline.
- **Claude Code's reproduction (6.4.1 Boron, `HEAD` against the working tree, fresh `BUILD_PATH_BASE`s):** suite 83/83 (sh via zsh, py via python3); text 151396 → 151468 (+72); `retainedBoot` is one `.backup` symbol at `0x2003f420`, size 32; `thrashTripCount` `0x2003f43c → 0x2003f440`, `lastThrashReason` `0x2003f43e → 0x2003f442`, `thrashResetCount` `0x2003f440 → 0x2003f444`; `__backup_sram_signature` stays at `0x2003f494`; `.backup` end stays at `0x2003f498`.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **The member list is complete** for main's unvalidated retained variables: the nine in the WO, in the order and with the types the struct uses, with no internal padding that changes the layout between builds. Classify every other `retained` definition in `src/` (the WO's re-check lists them) as validated (magic and version, sentinel, guarded) or not; name any unvalidated one that the WO's out-of-scope list does not cover.
2. **The check precedes every read.** `validateRetainedBoot(retainedBoot);` is the first statement of `setup()`, ahead of `ensureRetainedLoopForensicsInitialized()`. Search by bare and qualified names for any other reader before `setup()` runs (static constructors, `STARTUP()`, ISRs, other files); the WO's re-check says there is none, so confirm it.
3. **The reset values equal today's initializers** (`0`, `false`, `BREADCRUMB_NONE`). Confirm `BREADCRUMB_NONE`'s value against the enum, not by assumption.
4. **No boot-storm path can trip on the post-OTA boot.** From the reset state, the trip condition at `:873` is false for every reset reason, clock trusted or not; a genuine storm still trips afterwards. Check Copilot's host test proves this rather than assuming it, and trace the `:818` and `:849–873` logic yourself.
5. **State which outcome the build produces,** from the architect's rulings:
   - **(A) the signature word shifts** → Device OS clears all retained RAM once: acceptable;
   - **(B) the signature stays and an unvalidated symbol moves** → the STOP case.
   Reproduce the `nm` comparison for the whole `.backup` section. The reproduction shows (B) for the three ThrashGuard counters only. **Decide:** are they unvalidated? What reads them (`ThrashGuard.cpp:112–113`, `:151`, `cloud/Particle_Functions.cpp:52–53`, and anything else by bare name)? What exact values would they read once after the OTA (old bytes at `0x2003f440`, `0x2003f442`, `0x2003f444` in the HEAD layout: say what occupies them)? Does any decision, alert, tier or reset depend on them? Then give your outcome as (A), (B), or (B) with a consequence class (report-only or decision-bearing). Also say whether a smaller layout, such as one that keeps the trio at its old addresses, is possible without widening the WO, and what it would cost.
6. **The +72 text is explained by symbol** (`nm -C -S` for both builds; Copilot claims `setup` +0x3c, `setAppBreadcrumb` +4, `refreshRetainedLoopForensics` +4, the rest `CSWTCH` renames and padding). Confirm there are no new symbols beyond `retainedBoot` and no logic change beyond the design.
7. **The tests hold up.**
   - `retained_boot_test` (wrong magic, wrong version, garbage fill, values kept; all 256 reasons) fails under these mutations, in scratch copies: the magic check removed; the version check removed; a member reset to a wrong initial value; the `bootInProgress` reset removed.
   - `retained_boot_structural_test.py` fails when the check is moved below `ensureRetainedLoopForensicsInitialized()`.
   - `serial_settle_test.py`: the edit allows only `validateRetainedBoot(retainedBoot);` ahead of the serial wait; every other assertion is unchanged (diff it).
8. **Golden, suite and budget:** the golden payload test passes with the fixture unchanged; the full suite `N/N (sh via zsh, py via python3)` (baseline 81, now 83); net `src/` ≤ +20 (hard); tests about 216 against the +100 guideline (flag the overrun, don't fail on it).

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–8,** and, on its own line, **the outcome from check 5 (A, B-report-only, or B-decision-bearing)**.
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.
