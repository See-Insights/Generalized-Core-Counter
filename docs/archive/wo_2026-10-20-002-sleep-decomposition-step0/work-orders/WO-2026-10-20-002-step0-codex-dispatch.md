AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-20, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized (read-only):**
- Read the tree at main `a3e012d` (this worktree, branch `wo/2026-10-20-002-sleep-decomposition-step0`) and its git history (`git log`, `git show`, `git blame`, `git grep`).
- Run read-only host commands (`rg`, `git`, `sed -n`, `awk`, `wc`); you may compile or run an existing host test **only** to see what it reaches, in `$TMPDIR`.

**Not authorized:**
- Any change to any file in the repository, any commit, push, stash, reset, checkout or restore.
- **No ARM build, no Particle compile, no device, network or AWS access.**
- Proposing logging or behavior changes (item 5 is an answer only).

# Step 0 dispatch: WO-2026-10-20-002 (decompose `handleSleepingState`)

**Binding spec:** `docs/work-orders/WO-2026-10-20-002-sleep-state-decomposition-step0.md`: the plain goal, the eight questions, the standing rulings (PIR-wake block stays in `State_Sleep.cpp`; no behavior fixes in this series; `ModemHealth` stays the only writer of the latch fields). Where this dispatch and the WO differ, the WO wins; report the difference.

**Sources named by the WO:** `docs/architecture-review-2026-09-03.md`; `docs/work-orders/WO-2026-10-15-003-step0-report.md` (Q7 and open points); `docs/RECOVERY_PLAN_2026-09-26.md` item 14 (breadcrumb-28 sleep stall), the standby latch on PCKL1, and the Dev-14 post-failsafe sleep loop (find them by searching the plan).

## Rules for the report

1. **Every `file:line` is at `a3e012d`.** Claude Code checks each against the tree before the architect sees the report (§13 (j)); a citation that does not match is returned. Cite a range as `State_Sleep.cpp:179–260`. Do not cite from memory or from the WO's numbers (they were taken earlier); re-read the line.
2. **Separate observation from inference** in each answer: what the code says (with the line), then what you conclude.
3. **History claims need a commit** (`git log -S`/`-G`/`--follow` evidence): hash, subject, and what changed. Say "no evidence found" when you find none; do not guess a reason for a merge-back.
4. Answer the eight questions in the WO's order, numbered the same. For question 6, one block per WO: title, what moves or changes, sleep-file lines removed, net non-move `src/` lines expected, the equivalence transcript it would record (inputs and what is frozen), expected ELF change, and what it depends on.
5. **Length: ≤ 150 lines.**
6. Stop and say so, as the WO's item 8 requires, if the split cannot be made behavior-neutral, needs a persisted field, or needs more than six WOs.

## Deliver

Write the report as your final message (the controller saves it as `docs/work-orders/WO-2026-10-20-002-step0-report.md`). Begin with: the model and reasoning level used, and the commit you read (`git rev-parse --short HEAD`).
