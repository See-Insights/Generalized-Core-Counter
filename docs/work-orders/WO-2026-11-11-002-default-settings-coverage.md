# WO-2026-11-11-002: The product default ledger is complete, and a host test keeps it that way

**Author:** architect, 11 Nov 2026 · **Status:** READY to dispatch. It's docs and tests only, so it can merge any time; it isn't held for RC1 (no `src/` change).
**Branch:** `wo/2026-11-11-002-default-settings-coverage` from `main`
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5–8; §4 (SHA rule; at most two Stage 7 rounds). The local suite equals CI's discovery, and the test must exit 0 when run bare.
**Agents:**
- Stage 6: **Copilot** (standard model, medium effort). It's one JSON file and one Python test.
- Stage 7: **Codex `gpt-5.6-sol` at medium**. This is test tooling, not device logic.

**Budget:** `src/` **0 lines** (any `src/` change means stop and report). Test at most about 80 lines.
**Inputs:** the architect's analysis of 11 Nov:
- the merge rule at `src/cloud/ConfigApply.cpp:29–68` (`main` `e8b15ad`): device wins, then default; if both are absent, the **stored value is kept**;
- the compiled defaults at `src/MyPersistentData.cpp:128–149`, `src/Config.h:31–39` and `src/power/ChargeInhibitPolicy.h:24–27`.

## Plain goal
Today you can't tell from the ledgers whether a setting such as `enableHibernateSleep` is in force. When neither ledger names a field, the device keeps whatever it stored last. **Rule (Chip, 11 Nov):** the product default ledger lists **every** field the firmware reads, with its value. The device ledger holds only differences. A host test fails whenever the repo's default file and the firmware's list of read fields disagree.

## The change
1. **Replace** `default-settings-v3.23.json` (stale: `setting1` 60000, Singapore timezone) with **`docs/config/default-settings-42131.json`**, holding exactly this content. It's the source of truth Chip pastes into the console.
```json
{
  "messaging": { "serial": false, "verboseMode": false, "verboseTimeoutMin": 60 },
  "sensor":    { "type": 1, "setting1": 300000, "setting2": 0, "setting3": 0, "setting4": 0 },
  "timing":    { "closeHour": 22, "openHour": 6, "reportingIntervalSec": 3600,
                 "timezone": "EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00", "connectAttemptBudgetSec": 300 },
  "modes":     { "sensorMode": 1, "connectionMode": 3, "reportingMode": 1, "samplingMode": 0,
                 "cloudDisconnectBudgetSec": 15, "modemOffBudgetSec": 30, "enableHibernateSleep": false },
  "reporting": { "webhook": { "name": "Ubidots-Sensor-Hook-v1", "enabled": true, "timeoutMs": 20000 } },
  "power":     { "thermalChargeInhibit": { "armHighC": 37.0, "armLowC": 0.0, "releaseHighC": 35.0, "releaseLowC": 3.0 } }
}
```
   Grep for references to the old file name and update them.
2. **New host test** `tests/default_settings_coverage_test.py` (python3, standard library only):
   - From `src/cloud/ConfigApply.cpp`, collect every `(section path, key)` the firmware reads. The section comes from `getTopLevelMap(defaults, "<s>", <var>)` or `getNestedMap(defaults, "<a>", "<b>", <var>)`. The key comes from each `getMerged{Int,Bool,String,Float}Value(<var>, …, "<key>", …)`. Dedupe: `webhook.enabled` is read twice.
   - Collect every leaf key path in the JSON file.
   - **Fail** on any key the firmware reads that the file lacks ("incomplete default"). Also fail on any key in the file the firmware doesn't read ("dead field", e.g. the old `messaging.disconnectedMode`). Print both lists.
   - **Fail** if the parser finds fewer than 25 distinct keys, so a regex that silently matches nothing can't pass. Today's count is **27**: 3 messaging, 5 sensor, 5 timing, 7 modes, 3 webhook, 4 thermal. Step 0 confirms it.
3. **Out of scope:** checking the live console ledger against the file (that needs a Particle API read, which belongs to the fleet-ops thread), and checking values. The test checks which keys are present, not what they're set to.

## Step 0 (read-only, inside Stage 6)
- Re-cite the merge helpers and every `getMerged*Value` call on the dispatch base. List the section and key pairs.
- Check for any key read some other way than `getMerged*Value` (e.g. `has()` or `get()` direct). If there is one, the test must include it, or stop and report.

## Tests and mutations (each must make the new test fail)
- Delete `modes.enableHibernateSleep` from the JSON.
- Add `messaging.disconnectedMode` back to the JSON.
- Add a scratch `getMergedIntValue(defaultModes, deviceModes, "newKey", x)` to a **copy** of `ConfigApply.cpp` (use the back-up, mutate, restore rule; never `git checkout`).
- Break the regex so it matches nothing (the floor of 25 catches it).
- The full suite stays green, and the test exits 0 when run bare.

## Standing rules this WO records
- **Any WO that adds, removes or renames a ledger field** updates `docs/config/default-settings-42131.json` in the same PR. This test enforces it.
- **The HIBERNATE flip:** after v50.1 soaks, `enableHibernateSleep` becomes `true`, in the console **and** in this file, in the same change.

## Closing record
Record budget against actual, and the Stage 7 SHA and verdict.
