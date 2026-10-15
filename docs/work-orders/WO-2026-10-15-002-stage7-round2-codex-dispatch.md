AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-15, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Review this worktree (branch `wo/2026-10-15-002-step6-2b6-wire-reset`, uncommitted).
- Run `tests/reset_wiring_test.sh` and the host suite.
- Mutate only in scratch copies under `$TMPDIR` or `build-tmp/wo20261015-002-stage7r2/`, and remove **only** what you create.

**Not authorized:**
- Any change outside your scratch locations.
- ARM builds (see below).
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.

# Stage 7 round 2: WO-2026-10-15-002 (2b-6), check 5 only

**Why round 2:** round 1 (`docs/work-orders/WO-2026-10-15-002-stage7-verdict.md`) was NOT VERIFIED on check 5 alone. `reset_wiring_test.sh` counted whole regex regions, not boundary markers, so a duplicated hourly start marker passed silently, and the other duplicates failed only as compiler errors. Every other check passed.

**The round-2 change (test only):** `tests/reset_wiring_test.sh` gains a `marker()` check (11 added lines). Before extraction, each of the five start markers, and the hourly block's end marker, must occur **exactly once** at the start of a line. Otherwise the test exits with `EXTRACT_FAILED: <start|end> marker for <name> found <N> times (expected 1): '<marker>'`. Sonnet made the fix, as a subagent at medium reasoning.

**Controller's reproduction** (Claude Code, before this dispatch), using scratch copies with `RESET_WIRING_SRC_DIR`:
- for each of the six markers, both **delete** and **duplicate** exited 1 with the named message: 12 of 12;
- the unmodified test passes;
- the suite is 81/81.

**No ARM build:** `src/` is unchanged from round 1's candidate. The working-tree `src/` tree hash is `845d28701360406b720b2fa295a0f98a096786f7`, equal to round 1's candidate (`845d28701360406b720b2fa295a0f98a096786f7`); HEAD's `src` is `1f67937ba00074dffcb8b0698673eb66bdac4ed5`. Re-derive the working-tree hash yourself (a temporary index: `read-tree HEAD`, `add -A src`, `write-tree`, then `rev-parse <tree>:src`) and **record both hashes**. Round 1's ELF results stand.

## Check (PASS / FAIL / CONCERN, with evidence)

**5. `reset_wiring_test` extracts by unique markers that fail loudly.**
- For each of `dailyCleanup()`, the hourly block (start and end), `currentStatusData::resetEverything(bool)`, the `CurrentReadings` wrapper and `activeSensor()`: in scratch copies, **delete** the marker and **duplicate** it. Show a non-zero exit with the **named** `EXTRACT_FAILED` message each time, not a compiler error.
- Confirm the wrapper marker can't also count the `currentStatusData::` line, given the line-start anchor.
- Note that the script exits early when `RESET_WIRING_SRC_DIR` is set (`:56`). Confirm the marker checks still run in that mode, and that the normal run still performs the full behavioural checks.
- Confirm there's no remaining extraction that relies on line numbers or a silent fallback.
- Re-run the unmodified test, and the full suite: `N/N (sh via zsh, py via python3)`, expected 81/81.
- Re-derive the `src/` tree hash and confirm it matches round 1's candidate.

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **Check 5:** the result with evidence, plus the two `src/` tree hashes.
- **Findings** (if any), with file:line and severity.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.

Limit: 60 lines.
