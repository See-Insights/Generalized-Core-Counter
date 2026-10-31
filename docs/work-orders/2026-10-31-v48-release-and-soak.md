# v48 release and bench soak (occupancy ends at the close)

**Author:** Claude Code, 31 Oct 2026, from the v47 WO (`2026-10-27-v47-release-and-soak.md`) and WO-2026-10-29-002; the architect may amend · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §3 ("Release gate: the Particle cloud compile"), §13 (g), (h), (i), (j)
**Agents:** release PR, Claude Code itself; soak and baseline runs, a **Sonnet subagent, medium**, read-only. Report the model used.
**Budget:** 0 `src/` lines beyond the version string.
**Start:** v47 is the product default (23:01Z 28 Oct) and its fleet health check passed (PR #167). #166 (item 10) is merged.

## Plain goal

Ship #166 as **v48-OccupancyEndsAtClose** on Device OS 6.6.0. The one behavior change: on a normal-hours site, a session open at the close is credited to the close and ended by it, and nothing detected between the close and the next opening is counted. **What the soak must show:** the first report each morning reads `dailyoccupancy` **0** with no carry-in (v47 carried +5 to +7 min into the next day on Dev-09 and Dev-14, 28 Oct), and the day's last session before 22:00 is still credited to the close.

v48 is also the gate for the main waves of the legacy-occupancy migration (WO-2026-10-29-001, PR #165): the pilot devices stay on v47 and the waves wait for the v48 pass.

## 1. Release PR (branch `release/v48`)

- `src/` differs from v47 (`f624c15`) only by #166 (`State_Common.h`, `State_Modes.cpp`, net +16) and the version string. Release commit: `FirmwareVersion.h` (48, `v48-OccupancyEndsAtClose`), `CHANGELOG.md`, `README.md`, `Doxyfile`.
- **Suite:** run with zsh for the `.sh` tests and python3 for the `.py` tests, serial. Result: 107/107 on the release head.
- **Local build (§3, mandatory per change), 31 Oct:** Boron, Device OS 6.6.0, fresh `BUILD_PATH_BASE`, `EXTRA_CFLAGS=-frandom-seed=$$@`, built from a `git archive` of the release head (`release/v48`, before this WO edit). `app.bin` **152,722 B** (v47: 152,610 B, +112), **sha256 `2aa44eb2f303e6c07b13e040283a97c77bcd9e3b8d85a559dc9a21035fd60da8`**, text/data/bss 151604/1114/2200 (v47: 151492/1114/2200). `.backup` section **152 B**, as v47; `src/` has no `retained` change since v47 (`git diff f624c15 origin/main -- src` has no `retained` line). `strings`: `v48-OccupancyEndsAtClose` once. `particle binary inspect`: CRC ok (7e59bca4), compiled for boron, product version 48, depends on system module 1 at version 6600. The file: `/Users/chipmc/Downloads/v48-OccupancyEndsAtClose-boron.bin`. The sha256 is not reproducible across builds (v47 WO), so upload this file and compare Particle's size and sha256 with the numbers above, §13 (j).
- **Release gate, Particle cloud compile (§3), 31 Oct: not completed; the service failed.**
  - The full tree: `Compile failed: Compile input not allowed` (a different error from before; it listed 368 files including `tests/`, `docs/` and `tools/`).
  - Only `src/`, `lib/` and `project.properties`: `Compile failed: Compiler timed out or encountered an error` after 2 min with no diagnostics, as on the v46 and v47 heads. **No flash or RAM numbers.**
  - **Per the gate: recorded; release from the verified local build.** Retry once more before the upload and add the numbers here if it passes.

## 2. Roles and plan

- **Dev-09** (e00fce68399ee6244a963935) and **Dev-14** (e00fce688e592afaf23ac4fb) are the soak devices, locked to v48 by Chip. Dev-09's `timing.reportingIntervalSec` stays 3600. Dev-14 keeps the bad-antenna rules from the v47 WO §2 (judge its sleep path and payload; exclude its connect metrics; a failsafe stage-2/3 reset after the 22:00 SGT close is the designed #151 behavior).
- **Anchor** every job on each device's actual first v48 boot (status reset 70, `v48-OccupancyEndsAtClose`; exclude the OTA-interrupted session).
- Pilot devices (Vaughan-002, OCCUPANCY-LAB-01, ChurchSt-2) are not part of this soak and stay on v47.
- Not set as the product default during the soak.
- Chip uploads v48 (checking size and sha256 against the release PR), then locks the two devices.

## 3. Checks, per device (as the v47 soak §3)

1. Payload unchanged (same keys and order as v47); the standard checks (resets, `watchdogResetCount`, `fh`, alerts); "missing" = a report v47 would have sent.
2. First boot: reset 70 once (the OTA) and nothing else; `dailyoccupancy` continuity across the OTA.
3. Sleep returns: every cycle sleeps (`cyc` − `slp` = 1); no breadcrumb-28 watchdog; reset types as v47.
4. Failsafe: `failsafeStage` and `failsafeCount`; counts only.

## 4. The item 10 test: pass criteria

Use the baseline script's per-category report (`--dates <SGT day> --tz SGT-8 --open 6 --close 22 --debounce-ms 300000`, #163) plus the hook increments.
- **PASS needs all of:**
  1. On each full v48 day, the **first report after the 06:00 open reads `dailyoccupancy` 0** on both devices, with no post-close carry-in in the script's per-report increments.
  2. The **last session before 22:00 is credited to the close** (about 9.5 min if it was a full session, less if cut by the close).
  3. **Normal sessions at about 9.55 min,** within the v45-v47 spread (9.54-9.58).
  4. **The standard health checks (§3) are clean:** resets (only the one reset 70 at the OTA, and reset types as v47), breadcrumb-28 watchdogs (none on Dev-09; Dev-14 stays on the item 14 watch), item 18 gaps (a gap of 3 h or more outside the overnight close is recorded; hook gaps of 4 h or more are the normal 7-8 h night gap ending in a normal report), payload (same keys and order as v47), and missing reports (none not explained by a failed connect).
- **FINDING (holds the rollout):** any non-zero first-of-day `dailyoccupancy`; any session credited after the close; normal sessions outside the v45-v47 spread; a payload difference; a breadcrumb-28 watchdog on Dev-09; an item 18 gap on Dev-09; a missing report not explained by a failed connect.
- **Not testable remotely:** the 0-24 and Unknown-openness cases (host tests cover them, Stage 7 checked them).

## 5. Default (if both pass)

Set v48 as the product default (recheck size and sha256), merge the release PR, run the fleet health check about 24 h later (the v47 report as the model). Then the migration's main waves may start (WO-2026-10-29-001 §10); items 19 and 15 follow in v49.

## 6. Schedule

Dates are set when Chip uploads v48. Chip has no stable connection until further notice, so every step is remote and anchored on the first v48 boot:

| When | Step | Who |
|---|---|---|
| on Chip's go | Upload v48; lock Dev-09 and Dev-14 | **Chip** |
| the next 22:00Z wake | OTA (reset 70) | automatic |
| first full v48 SGT day, 14:30Z | v48 day 1 and the first-boot check | Claude Code (Sonnet subagent) |
| 24 h after each first boot | 24 h grading | Claude Code (Sonnet subagent) |
| the day after | day 2 and the decision | Claude Code, then **Chip** |

All reads need `AWS_PROFILE=particle-admin`; an expired SSO token stops a job.

## Result

(to be filled in by the grader)
