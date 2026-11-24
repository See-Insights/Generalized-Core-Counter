AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-20, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized (read-only, two repositories):**
- **GCC:** this worktree, branch `wo/2026-10-20-004-fleet-convergence-step0`, main `a3e012d`.
- **Visitation-Counters (v50/v54), read-only:** `/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/3aa500de-487b-4335-ad8d-861927a20849/scratchpad/vc`, a clone of See-Insights/Visitation-Counters at **`377cfd3` ("v54 - v50 rebuilt for deviceOS@6.6.0")**, with full history (v50 is `9d11f4a`, "v50 - Fixed for backoff"; v48 is `a5a899f`). The files are write-protected; do not try to change them. Use `git -C /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/3aa500de-487b-4335-ad8d-861927a20849/scratchpad/vc show/log/blame` for history. Its source is mainly `src/Visitation-Counters.ino` plus `src/*.h`/`*.cpp` and `lib/`.
- Read-only host commands (`rg`, `git`, `sed -n`, `awk`, `wc`). The fleet-operations docs at `/Users/chipmc/Documents/Maker/AWS/particle-fleet-operations/docs` may be **read** (for what the back end and fleet tools consume, question 3 and 4).

**Not authorized:**
- Any change to any file in either repository; any commit, push, stash, reset, checkout or restore.
- **No build or compile of any kind, no device, network or AWS access.**
- Proposing changes to v50/v54; GCC changes appear only as a sized plan in the answers.

# Step 0 dispatch: WO-2026-10-20-004 (one firmware for occupancy, trail and car counters)

**Binding spec:** `docs/work-orders/WO-2026-10-20-004-fleet-convergence-step0.md`: the plain goal, the eight questions and their stop rule (item 8). Where this dispatch and the WO differ, the WO wins; report the difference. Standing: the sleep split (S-1 to S-6, WO-2026-10-20-002) runs in parallel, so item 7 must mark what can run alongside it and what must wait; the 2b-7/2b-8 items are the deferred Step 6 WOs (the report request; delivering `sensor.setting1–4` and `samplingMode` to the sensor) and appear in the GCC recovery plan (`docs/RECOVERY_PLAN_2026-09-26.md`) and the Step 6 work orders under `docs/work-orders/`.

## Rules for the report

1. **Every `file:line` states its repo and commit:** `GCC State_Sleep.cpp:1279` (at `a3e012d`) or `VC Visitation-Counters.ino:410` (at `377cfd3`). Re-read each line; do not cite from memory. Claude Code checks each one in both trees before the architect sees the report; a mismatch is returned.
2. **Observation versus inference**, and where the two repos disagree, show both sides with their lines. For v50 behavior that changed over time, say which version (v48, v50, v54) the line is from; v54 is v50 on Device OS 6.6.0, so confirm from `git diff 9d11f4a 377cfd3 --stat` that no application behavior differs, and say so.
3. **History claims need a commit** (hash, subject); "no evidence found" when none. Do not guess why something exists.
4. **Do not assume.** What the back end (Ubidots, the fleet tools) consumes and how legacy devices are configured in the field may not be in either repo or the docs; say "cannot tell from the trees" and name who or what would answer (a Particle product setting, a console query, Chip), rather than inventing. Device counts and product membership in the WO are the architect's figures; treat them as given.
5. Answer the WO's eight questions in order, numbered the same. Question 7: a table of WOs (title, what changes, net non-move `src/` lines expected, equivalence or golden check, dependency, and **alongside / after the sleep split**). Hard rule on each: ≤ +20 net `src/` lines, behavior-neutral for existing occupancy devices; a change the numbers cannot meet goes into the stop answer (item 8).
6. **Length: ≤ 150 lines.** Begin with the model, reasoning level, both commits read (`git rev-parse --short HEAD` in GCC; `377cfd3` for VC).

## Deliver

The report as your final message (the controller saves it as `docs/work-orders/WO-2026-10-20-004-step0-report.md`).
