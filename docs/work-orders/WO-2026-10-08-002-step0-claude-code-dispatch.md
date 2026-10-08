AGENT: Claude Code (a separate headless session: `claude -p`) · MODEL: claude-sonnet-5-5 (confirmed with a one-line probe on 2026-10-08, §5) · REASONING: high (`--effort high`)
AUTHORIZATION SCOPE:
- **Authorized:** read-only. Read, Grep and Glob, and read-only shell (`git log`, `git show`, `git diff`, `git blame`, `grep`, `sed -n`, `wc`, `ls`). Your final message is the report; Claude Code saves it to `docs/work-orders/WO-2026-10-08-002-step0-report.md`, then commits and pushes it.
- **Not authorized:** editing or writing any file, git commands that change state, builds, tests, Particle/AWS/device commands, and network access.

# WO-2026-10-08-002 Step 0: `sensor.type` from the ledgers reaches sensor creation (Step 6 WO 1c)

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §3 Stages 2 and 4, §12.1 (**history first**), §12.3 (budget).

**Plain goal:** the `sensor.type` set in the ledgers is the value sensor creation uses, so a configured sensor type is never silently ignored.

**Base:** main at **`8f82e1b`** (v39). This is the checked-out worktree, branch `wo/2026-10-08-002-sensor-type`, with a clean tree. Cite file:line at that commit. WO 1b (PR #77, unmerged) touches `ConfigApply.cpp:265` and `State_Report.cpp`. Say if your proposal would conflict with it. 1b and 1c ship together as v40.

**Budget:** 0 `src/` lines for Step 0. Give an estimate for the eventual fix. The architect hasn't set a cap yet; flag anything over +20.

## What is already known (verify; don't trust)

`docs/work-orders/2026-10-07-step6-extensibility-evidence.md` §6 (WO-2026-10-07-001, base `41ad238`):
- The ledger's `sensor.type` is written by `ConfigApply.cpp:351-357` into `SystemConfig::SensorSettings::sensorType`, the `/usr/sensor.dat` store (`sensorConfigData`, `MyPersistentData.h:512`).
- Sensor creation reads `SystemConfig::get_sensorType()`, which is `sysStatus.sensorType` (`MyPersistentData.h:135`, `.cpp:1044`), at `SensorManager.cpp:307` and `Main:1127`.
- `sysStatus.sensorType` is written only by its default (1, `MyPersistentData.cpp:136`).
- `SensorFactory` implements PIR only (`SensorFactory.h:73-74`; others return null at `:85-87`).
- `initializeFromConfig` runs only at boot, wake and idle-enable, never after a config apply.

## Questions

1. **Is it still real at `8f82e1b`?** Re-open each citation above and mark it HOLDS, MOVED TO file:line, or WRONG. Then list every reader and writer of **both** sensor-type fields (`sysStatus.sensorType` and `SensorSettings::sensorType`), with file:line and what each one does with the value. Include `sensorISR` (`Main` around `:2696-2703`), the status and data payloads, the config hash, `Config::isValid`, and any test stubs.
2. **History (§12.1).**
   - When and why did the two fields appear? Did sensor creation ever read the ledger-written field, and which commit changed that?
   - Search with `git log -S`/`-G` on `sensorType`, `set_sensorType`, `SensorSettings`, `sensor.dat`, `sensorConfig`, `initializeFromConfig`, `SensorFactory` and `"type"` in ConfigApply. List the searches you ran.
   - If an earlier version had the ledger value reach creation, **restoring it is the default fix**. Say exactly what to restore.
3. **What the field effect would be.** Today only PIR exists.
   - If `sensor.type` reached creation, what would happen for type 1 (PIR), for 0 (the legacy pressure sensor), and for an unknown type? Would a null sensor crash, idle safely, or fall back?
   - Should an unsupported type be **rejected at config apply** (like the reporting-interval cap in WO-2026-10-08-001) rather than accepted? Give the options; don't decide.
   - When should a type change take effect: at the next `initializeFromConfig` (boot, wake, idle-enable), or right after config apply? Say what exists today for re-creating the sensor.
4. **The smallest fix.** Propose the one smallest change that meets the plain goal using existing state. Candidates:
   - make creation read `SensorSettings::sensorType` and retire or alias `sysStatus.sensorType`;
   - make config apply write the field creation reads;
   - make the facade `SystemConfig::get_sensorType()` forward to the sensor store.

   For each:
   - say whether persisted layout changes (`tests/persisted_layout_preservation_test.py` must pass unchanged);
   - say what happens to devices in the field whose two fields currently differ (migration);
   - give the file:line changes and a net `src/` estimate.

   Pick one.
5. **Interactions.** Confirm the fix doesn't change occupancy reporting (WO-004), the failsafe (WO 1b), or the payload schema. If a payload field reports the sensor type, say which value it reports today and after the fix.
6. **Stop conditions.** End with **STOP** or **PROCEED** and the reason. STOP if:
   - it isn't real (the citations are wrong);
   - the fix needs a persisted-layout change or a new state, timer or field;
   - the estimate is over +20, in which case propose a split.

## Out of scope

Any code change, test run, build or device operation. Implementing other sensor types. Any design beyond the one proposal.

## Report format (your final message, Markdown only, **≤ 150 lines**, nothing before or after it)

- **Header:** "WO-2026-10-08-002 Step 0", base `8f82e1b`, date 2026-10-08, "Model used: <the model you actually ran as>, reasoning: high (set by `--effort high`)".
- **The plain goal.**
- **Sections 1–6.** Use tables. Mark observation (OBS) and inference (INF).
- **"Open points".**
- **Closing:** a budget-versus-actual table with the row "Step 0 report | 0 | — | 0 | none run"; the model used; and the citations worked out from diffs rather than re-opened.

Before finishing, re-open every `src/` file:line you cite.
