AGENT: Codex · MODEL: gpt-5.6-sol (probed 2026-10-20, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the Stage 6 change: `git diff e6d3e69 febcf0b` (HEAD is febcf0b plus this dispatch commit), branch `wip/s6-sleep-wake-and-occupancy`; the base is `e6d3e69` (WO branch tip `54f21d6`, which only adds docs).
- Run the host suite (**one suite at a time**) and local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261022-001-stage7/`** or `$TMPDIR`; remove only what you create.
- Mutate only in scratch copies made with `git worktree add --detach` (a copy without `.git` makes the harness fail for any mutant).
- Write your verdict to `docs/work-orders/WO-2026-10-22-001-stage7-verdict.md` (the only file you may add).

**Not authorized:** changes outside your scratch locations and the verdict file; commits, pushes, stash, reset, checkout or restore in the review worktree; device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-22-001 (Sleep S-6: wake-event processing to `State_Modes.cpp`, post-wake routing to one function)

**Goal:** `handleSleepingState()`'s wake-event processing (`State_Sleep.cpp:1217–1324` at base) is now `bool processSleepWakeEvent(bool pirWake)` in `State_Modes.cpp`; the post-wake routing is `static void postWakeRouting(...)` in `State_Sleep.cpp`. **No behaviour change; net `src/` +12 (limit +20).**

**Binding spec:** `docs/work-orders/WO-2026-10-22-001-sleep-s6-wake-and-occupancy.md`, `…-step0-report.md`, `…-stage6-copilot-dispatch.md`. Copilot's report is not in the repo; treat every claim as unverified. Claude Code's reproduction (unverified by you): suite 103/103 (baseline 101), serial; `src/` +5/+115/+115−223 = **+12**, byte-identical to the prototype; fixture regenerates identically from `e6d3e69`; `COPY_MISMATCH` shown; the four WO mutations fail the transcript and the structural test; ARM text/data/bss `151348/1114/2212 → 151340/1114/2212`, `.backup` identical, `processSleepWakeEvent` 0x250 out of line, `handleSleepingState` 0x12a0.

## Checks (each: PASS / FAIL / CONCERN, with evidence)

1. **Moved code is verbatim.** Diff `processSleepWakeEvent` against base `:1217–1324` and `postWakeRouting` against `:1165–1382`: only `return;`→`return true;`, the final `return false;`, and the 6a call. Any change to a log string, transition, breadcrumb, `BatteryAuthority` call, LED call, write or order is a FAIL. Check the three `State_Modes.cpp` includes are the minimum, and that the whitespace `git diff --check` flags (`State_Modes.cpp` 176, 225, 253) is carried from base.
2. **Call site and ordering.** The handler calls `postWakeRouting(...)` right after `sleepAndResume`; the removed unpack lines had no side effects; `processSleepWakeEvent` is called at the same point (after open-hours re-enable and low-battery refresh, before the timer/overdue routing); all paths return as before (read each `return`).
3. **Visibility and ODR.** `processSleepWakeEvent` declared once in `StateHandlers.h`, defined once, non-static, no name collision; `postWakeRouting` is `static`; `State_Modes.cpp` globals it touches (`session`, `thrashGuard`, `transitionTo`, `logOccupiedEvent` etc.) resolve to the same objects as in `State_Sleep.cpp`.
4. **Transcript quality.** Does `sleep_wake_equivalence_test` pin what it claims: the harness compiles the real extracted code, not a model; `COPY_MISMATCH` works; scenarios cover the four WO mutations and the timer-suppress condition; the fixture header base is `e6d3e69`; the base region really is the base. Run **two mutations of your own** that the author did not list.
5. **Re-anchors.** For each of the ten re-anchored tests (and the three narrowed `wifiGuard.` normalizations): is it any weaker than before? Run one mutation per test. State whether the S-2 harness edit still pins the handler's call site.
6. **Structural test** (`sleep_wake_structural_test.py`): can it pass on a wrong tree (placement, ownership, `retained`)?
7. **ELF and retained.** Your own Boron 6.4.1 build, fresh `BUILD_PATH_BASE`, `-frandom-seed=p6`, before and after: sizes, symbol accounting (`nm -C -S`), no new static-init stub, the whole `.backup` before and after, no `retained` in any added line. Outcome A/B/C.
8. **Budget.** `src/` net ≤ +20; tests against the +100 guideline (flag, don't fail); full suite `N/N (sh via zsh, py via python3)` serial; golden payload test passes with its fixture unchanged.

## Verdict (write it to the file above, then end with a short summary)

Header: VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED, model and reasoning, interpreter line; a table of checks 1–8 and, on its own line, the retained outcome; findings with file:line and severity (observation separate from inference); budget versus actual as one row; confirmations that the tree matches the start and you removed only your own scratch files.
