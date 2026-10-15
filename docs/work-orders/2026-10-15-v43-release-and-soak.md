# v43 release and bench soak (Step 6 2a + 2b)

**Author:** architect, 2026-10-15 · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §13 (g), (h), (i), (j)
**Agent:** Claude Code. Release PR: Claude Code itself (no model dispatch). Soak grading needs S3, the Particle API and the forwarder logs, so it goes to a **Sonnet subagent, medium**. Report the model actually used.
**Budget:** 0 `src/` lines beyond the version bump.
**Start:** only after #103 (2b-6) is merged.

## Plain goal

Ship the Step 6 restructuring (2a dead code #94; 2b-1 to 2b-6: #97, #98, #99, #100, #101, #103) as v43. **v43 is meant to behave exactly like v42.** The soak's job is to show that on real hardware: same reports, same wakes, same resets, same heap.

## 1. Release PR (Claude Code)

- From main after #103; confirm all seven PRs above are merged and that no other `src/` change has landed since v42 (`git diff --stat <v42 tag or ff1715c>..HEAD -- src/`; list anything unexpected and stop).
- `bump_version.sh` to `v43-SensorShape` (or the closest name under the convention); product version 43. **The version string is final before the soak**, so the soaked `.bin` is the one released (the v41/v42 lesson).
- CHANGELOG, plain language: no intended behavior change. Code restructured: about 460 lines of dead code removed; the sensor now supplies its own report fields, wake pin and hourly/daily reset; a golden test pins the report payload byte for byte.
- Fresh `BUILD_PATH_BASE` build; `strings` shows the version; record the `.bin` size (versus v42's 152558 bytes) and sha256; tests `N/N (sh via zsh, py via python3)`.
- **Before Chip uploads:** check the production devices' `device-settings` ledgers for `sensorMode`. If any runs COUNTING, say so in the PR and stop; I'll add a short COUNTING step on Dev-09.
- Open the PR for Chip; don't merge.

## 2. Bench soak — Chip does the device steps

Chip uploads the PR head's `.bin` as product v43 (Particle's size and sha256 must match the PR's, §13 (j)), then OTAs it to **Dev-09 and Dev-14 only**, both locked to v43 and both left in KEEP_ALIVE. Each device is graded on its own 24 h window from its first v43 boot (reset 70). No config changes during the soak.

**Checks, per device, each against the same device's previous 24 h on v42:**
1. **Payload unchanged end to end** (S3 webhook): the same keys in the same order with the same number formats as a v42 report from the same device; no new, missing or renamed field; `battery`, `alerts` and `resets` in their usual places.
2. **PIR wakes unchanged** (serial): every motion wake from sleep is followed by occupancy handling, and every occupancy start and end is reported immediately (≤ 20 s, as on v41/v42).
3. **Timer wakes unchanged** (serial and S3): scheduled reports in every open-hour slot; none missing; `cyc − slp = 1`.
4. **Daily reset unchanged** (S3 webhook): the close report stamped close − 1 s, then the next morning's daily totals are 0. `resetCount` is cleared at the close as before.
5. **Hourly reset:** only runs in COUNTING mode, and both bench devices run OCCUPANCY, so it isn't exercised here (host-tested in 2b-6). Checked before the soak, in §1.
6. **Standard:** no unexpected resets; `watchdogResetCount` unchanged; heap (`fh`, serial `heap=`) in the v42 range; no alerts except the known post-boot 44; no `GateFail`; one daily config wait per device per day.

Any breadcrumb-28 watchdog, a payload difference or a missing report is a FINDING and holds the fleet rollout.

## 3. Grading

PASS or FINDING per device, with log lines, ≤ 60 lines in chat; every number names its source and window (§13 (j)). Add the result to this file and commit on a docs branch with a PR.

## 4. Fleet steps (Chip, only if both pass)

1. Set v43 as the product default (the soaked `.bin` is already product v43; recheck size and sha256).
2. Dev-09 and Dev-14 stay locked for the next soak.
3. Run the usual fleet health check about 24 h after the default changes.

## Result

(to be filled in by the grader)
