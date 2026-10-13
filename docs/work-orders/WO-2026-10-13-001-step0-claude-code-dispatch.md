AGENT: Claude Code (a separate headless session: `claude -p`) · MODEL: claude-sonnet-5-5 (confirmed with a one-line probe on 2026-10-13, §5) · REASONING: medium (`--effort medium`)
AUTHORIZATION SCOPE:
- **Authorized:** read-only. Read, Grep and Glob, and read-only shell (`git log`, `git show`, `git diff`, `git blame`, `git grep`, `grep`, `sed -n`, `wc`, `ls`). Your final message is the report; Claude Code saves it to `docs/work-orders/WO-2026-10-13-001-step0-report.md`, then commits and pushes it on this branch.
- **Not authorized:** editing or writing files, state-changing git, builds, tests, device or network access.

# WO-2026-10-13-001 Step 0: Step 6 WO 2a, remove dead code

**Binding spec:** `docs/work-orders/WO-2026-10-13-001-step6-2a-dead-code.md`: the candidates, the scope, the budget, and Step 0 questions 1–5. Where this dispatch and the WO differ, the WO wins; report the difference.

**Plain goal:** delete code that nothing in a release build calls, with no change in device behavior.

**Base:** branch `wo/2026-10-13-001-step6-2a-dead-code`, the checked-out worktree (main `de8faab` plus the WO). Cite file:line at HEAD.

## Inputs prepared for you (you can't build)

- **Release ELF symbol tables,** from a fresh `BUILD_PATH_BASE` build of this HEAD (Boron, Device OS 6.4.1, default flags; text/data/bss 151460 / 1094 / 2220; version `v42-ConfigWindowLatch`):
  - `build-tmp/release-nm-demangled.txt`: `nm -C -S --size-sort`, demangled, with sizes.
  - `build-tmp/release-nm-raw.txt`: `nm -S`, mangled.
  - A symbol absent from both is not in the release build. Mind inlining: a function inlined at every call site has no symbol and can still be called. So the source search is the deciding test, and the ELF confirms it.
- **Test-mode and bench builds:** there is no ELF for these. Decide from source: find each build flag in `src/BuildProfile.h` and its `#if` guards. Say whether the flag is still live (can be set) or retired.

## Questions

Answer the WO's questions 1–5 exactly. In addition:

- **Q1 table:** columns *Candidate | Location (file:line range) | Lines | Callers in `src/` (with file:line, or "none") | In release ELF? (symbol, size) | In a live test/bench build? | Verdict (dead / used (where) / moved / keep for 2b)*. Give one row per symbol or block. A declaration in a header and its definition are separate rows, but are counted together in the totals.
- **Q2 (history):** for `sensorISR` and the PIR ISR count only. Use `git log -S` / `git log -L`, and check whether WO-2026-10-07-001 or any CHANGELOG entry names an interrupt-driven reading mode. Quote what you find.
- **Q3:** list each host test under `tests/` that references a candidate (by name or by byte-checked extraction) and what it would need: delete the reference, update a pin, or nothing.
- **Q4:** net lines for commit (a), the charge-cycle test alone, and commit (b), everything else, plus the total. Count deletions only. If any `src/` line would have to be added, say so (stop condition).
- **Q5:** end with **PROCEED** or **STOP**, with the reason.

## Report format (your final message only: Markdown, **≤ 150 lines**, nothing before or after it)

- **Header:** "WO-2026-10-13-001 Step 0", base commit, date 2026-10-13, "Model used: <the model you actually ran as>, reasoning: medium (set by `--effort medium`)".
- **The plain goal.**
- **Sections Q1–Q5.** Mark observation (OBS) and inference (INF). Every number states its source (the search, the file, or the `nm` file), per §13 (j).
- **"Open points".**
- **Closing:** a budget-versus-estimate table: *Item | Budget | Estimate (net) | Gross additions*, with the row "Step 0 report | 0 | — | 0".

Re-open every `src/` file:line you cite before finishing.
