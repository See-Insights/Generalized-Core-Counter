AGENT: Codex · MODEL: gpt-5.6-sol (probed 2026-10-20, §5) · REASONING: high
AUTHORIZATION SCOPE: as in `WO-2026-10-20-003-stage7-codex-dispatch.md`: review only; scratch under `build-tmp/wo20261020-003-recheck/` or `$TMPDIR`, removing only what you create; mutate only in scratch copies (`git worktree add --detach`); write the verdict to `docs/work-orders/WO-2026-10-20-003-stage7-recheck-verdict.md` and give it in full as your final message; no commit/push/stash/reset/checkout/restore, no device/network/AWS, **no ARM build** (the `src/` tree is unchanged).

# Stage 7 recheck dispatch: WO-2026-10-20-003 (Sleep S-1), limited to check 8 and the F1 mutation

**Branch:** `wip/s1-sleep-teardown`, follow-up commit `8de7ddc` on top of the reviewed `658b7cf` (`8de7ddc` changes only `tests/firmware_update_wiring_structural_test.py` and `tests/firmware_update_dwell_test.sh`). Your first verdict: `docs/work-orders/WO-2026-10-20-003-stage7-verdict.md` (checks 1–7 and 9 are not re-run unless `src/` changed).
**Checks (PASS / FAIL, with evidence):**
1. **`src/` unchanged:** the `src/` tree at `8de7ddc` is byte-identical to `658b7cf` (`git diff 658b7cf 8de7ddc --stat -- src` empty; tree hash `7fc57df7a302712ea3f26ff37758ce455aaa6203808a7092d11636f5d06d190a`), so checks 1–7 and 9 stand. Net `src/` +20 (hard ceiling).
2. **F1 closed:** your mutation (the firmware-update guard moved from `State_Sleep.cpp:269` to below the `SleepTeardown::run(` call at `:504`) now fails the full suite, and which test(s) catch it and with what message; also the guard moved to just above the later `Connectivity::request*` calls. Try **two more of your own** that break the same property (for example the guard inside the `run()` result gate, or the guard placed before the `enter()` call).
3. **Check 8 again, for the two re-edited tests:** diff each against `658b7cf`: the structural test keeps every earlier assertion; the dwell test's derived order uses the real text (is the derivation itself robust: what does it do if the guard, the gate or `run(` is missing or appears in a comment?) and its runtime assertions are unchanged. Neither is weaker than before S-1 (compare with `7176048`'s pre-move versions for the same property).
4. **Suite:** `N/N (sh via zsh, py via python3)`, serial; baseline 93; golden payload fixture unchanged.

**Verdict:** VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; model, reasoning, interpreter line; a table of checks 1–4; findings with file:line and severity; confirmations. ≤ 80 lines. **No further round after this one** (architect).
