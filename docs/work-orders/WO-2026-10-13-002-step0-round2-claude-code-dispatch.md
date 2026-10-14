AGENT: Claude Code (a separate headless session: `claude -p`) · MODEL: claude-sonnet-5-5 (confirmed with a one-line probe on 2026-10-14, §5) · REASONING: high (`--effort high`)
AUTHORIZATION SCOPE:
- **Authorized:** read-only. Read, Grep and Glob, and read-only shell (`git log`, `git show`, `git diff`, `git blame`, `git grep`, `grep`, `sed -n`, `wc`, `ls`). Your final message is the report; Claude Code saves it to `docs/work-orders/WO-2026-10-13-002-step0-round2-report.md`, then commits and pushes it on this branch.
- **Not authorized:** editing or writing files, state-changing git, builds, tests, device or network access.

# WO-2026-10-13-002 Step 0, round 2: re-split the 2b sequence

**Why a second round:** the Stage 4 check found the round-1 sequence **NOT SOUND** (`docs/work-orders/WO-2026-10-13-002-stage4-verdict.md`):
- its cuts left the agreed design unmet;
- the wake work is larger than one WO;
- the golden test boundary was wrong;
- reset reuse, the null-sensor case and fragment formatting need care;
- the corrected work exceeds about six small WOs.

The architect asked for 2b Step 0 to continue from main.

**Binding spec:** `docs/work-orders/WO-2026-10-13-002-step6-2b-sensor-shape-step0.md`. "The agreed design" is settled; **don't reopen it**, and the stop rule (Q5) applies.

**Binding inputs:**
- the Stage 4 verdict: its facts, findings and recommended sequence. Treat them as checked, but re-verify every file:line you rely on at HEAD;
- the round-1 report, `WO-2026-10-13-002-step0-report.md`, for what still holds.

**Base:** branch `wo/2026-10-13-002-step6-2b-sensor-shape`, now on main `7a5f5e7` with WO 2a merged (#94). Cite file:line at HEAD.

## Method rules (`AI_DEVELOPMENT_WORKFLOW.md` §13)

- **Searches use the bare name as well as the qualified one** (§13 (c)).
- **Every number states its source** (§13 (j)). No explanation without evidence.
- **Mark OBS and INF.**

## Questions

1. **Corrections.** For each Stage 4 finding, say whether it holds at HEAD (AGREE / DISAGREE, with evidence), and what it changes in the sequence.
2. **The full sequence, without cuts.** Re-split all of 2b into WOs of **≤ +20 net `src/`** each (moves counted separately). Cover every one of the six operations, including the report request and its kind, settings 1–4, and `samplingMode`. Each WO must be behavior-neutral, with byte-identical JSON proved by the golden test. For each WO give:
   - the goal and files;
   - the net estimate, **with its arithmetic** (lines added and removed per file);
   - moves;
   - the golden test or parity test;
   - its dependencies;
   - the risk named by Stage 4 that it handles (null sensor, daily-close order, ISR-pending, getter order and truncation, init and ISR parity).
3. **The golden test (2b-1).** Specify it concretely:
   - the extraction boundary, through the real queue call;
   - the observer fake (event name, exact payload bytes, flags);
   - every input to control, including Stage 4's additions;
   - the vector matrix;
   - which existing structural tests it supersedes or must coexist with.
4. **Size verdict.** Count the WOs. If more than about six, **say what to cut or defer** to bring 2b to about six, and what each cut leaves unmet against the plain goal ("a new sensor without touching report, sleep or connect code"). Offer at most two options, with a recommendation.
5. **End with PROCEED or STOP** and the reason.

## Report format (your final message only: Markdown, **≤ 150 lines**, nothing before or after it)

- **Header:** "WO-2026-10-13-002 Step 0 round 2", base commit, date 2026-10-14, "Model used: <the model you actually ran as>, reasoning: high (set by `--effort high`)".
- **Sections Q1–Q5.**
- **"Open points".**
- **Closing:** a table *WO | Goal | Est. net `src/` | Moves | Test | Depends on | Risk handled*.

Re-open every `src/` file:line you cite before finishing.
