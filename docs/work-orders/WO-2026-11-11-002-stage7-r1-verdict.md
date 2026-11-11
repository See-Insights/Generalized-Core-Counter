UNDER REVIEW: wo/2026-11-11-002-default-settings-coverage @ 5b87f994248d89773b1c19a770744e84a7d9fac6
NOT VERIFIED

| # | Verdict | Evidence |
|---|---|---|
| 1 | PASS | `src/cloud/ConfigApply.cpp:184-630`; exactly 27 distinct firmware paths, matching the 27 JSON leaves. `reporting.webhook.enabled` at `:562` and `:584` is deduplicated. |
| 2 | FAIL | Current syntax is parsed correctly, but plausible constant/helper/other-file reads can escape detection; `tests/default_settings_coverage_test.py:16,20-31`. |
| 3 | FAIL | Every matching file is checked and no-file/missing/dead/valid/nested cases behave correctly. Invalid JSON exits 1 but does not name the offending defaults file; `tests/default_settings_coverage_test.py:44-46`. |
| 4 | PASS | All required mutations failed. Two additional surviving mutations expose one accidental gap and one constructed limit, detailed below. |
| 5 | FAIL | Structure, per-product values, type 1, debounce meaning, and unused settings 2–4 are supported. However, “60000 for testing only” is false at this commit; `docs/config/README.md:13`. |
| 6 | PASS | Root `default-settings-v3.23.json` is deleted. No live CI, script, guide, or README references it. Historical references remain as listed below. |
| 7 | PASS | `src/`: 0 changed lines; test: 79 lines; README: 15 lines. Host suite: **119/119**—68 `.sh` via zsh, 51 `.py` via python3. No Device OS build required. |

Key inventory

- `messaging` (`ConfigApply.cpp:184`): `serial`, `verboseMode`, `verboseTimeoutMin` (`:197,205,213`)
- `sensor` (`:335`): `type`, `setting1`, `setting2`, `setting3`, `setting4` (`:350,363,379,387,395`)
- `timing` (`:235`): `timezone`, `reportingIntervalSec`, `openHour`, `closeHour`, `connectAttemptBudgetSec` (`:249,264,282,283,304`)
- `modes` (`:413`): `sensorMode`, `connectionMode`, `reportingMode`, `samplingMode`, `cloudDisconnectBudgetSec`, `modemOffBudgetSec`, `enableHibernateSleep` (`:430,445,463,479,493,506,518`)
- `reporting.webhook` (`:536`): `name`, `enabled`, `timeoutMs` (`:548,562/584,570`)
- `power.thermalChargeInhibit` (`:605`): `armHighC`, `armLowC`, `releaseHighC`, `releaseLowC` (`:627-630`)

`docs/config/default-settings-42131.json:1-10` has exactly these paths and the exact work-order values, with nothing extra. `Cloud.cpp:635-637,680-682` only obtains and forwards the ledgers. `LedgerClient.cpp:9-14,178-181` checks section presence, not leaf values. No other current defaults-ledger key reader was found.

Findings

1. **Medium — plausible firmware reads can evade the test.**  
   `tests/default_settings_coverage_test.py:16,20-31`

   Path that breaks: adding a normal read whose key is held in a local constant:

   `getMergedIntValue(defaultModes, deviceModes, newKey, x)`

   The scratch mutation exited 0 and still reported 27/27. A wrapper around `getMerged*Value` has the same problem. Reads introduced in another `.cpp` are also invisible because only one configured source file is opened. Reusing the same section-variable name in separate scopes can additionally map calls to the last globally observed section.

   Smallest fix: scan the relevant `src/**/*.cpp` files, strip comments, resolve simple literal constants, and fail closed on every `getMerged*Value` invocation or defaults-ledger consumer that is not parsed into an explicit section/key pair. Bind section variables by lexical scope rather than one global name map.

2. **Low — invalid JSON failure does not identify the product file.**  
   `tests/default_settings_coverage_test.py:44-46`

   Path that breaks: malformed `default-settings-bad.json` raises an uncaught `JSONDecodeError`. Exit is correctly 1, but the traceback names the test script and never prints `default-settings-bad.json`.

   Smallest fix: catch `OSError`/`json.JSONDecodeError` around each file load and append `FAIL <basename>: invalid JSON: <error>`.

3. **Medium — README contradicts production firmware behavior.**  
   `docs/config/README.md:13`

   The README says 60,000 ms is “for testing only.” In production firmware, `Config::DEFAULT_OCCUPANCY_DEBOUNCE_MS` is 60,000 ms (`src/Config.h:35`), initializes persistent sensor setting 1 (`src/MyPersistentData.cpp:549-558`), and is returned as the runtime fallback when configuration is invalid or zero (`src/Config.cpp:145-151`). Thus production can run with 60,000 ms.

   The other statements are true: type 1 is the only accepted type (`ConfigApply.cpp:350-351`); setting 1 is the PIR occupancy debounce in milliseconds (`MyPersistentData.h:493-494`, `State_Common.h:316-325`, `State_Modes.cpp:75-78`); type-1 PIR returns through the interrupt path before setting 2’s future polling use (`SensorManager.cpp:322-336`); settings 3–4 have no operational type-1 consumers. Production 300,000 is the ledger-file value, not a firmware constant.

   Smallest fix: amend the binding wording and change the README to “production ledger value 300000; firmware/runtime fallback 60000.”

Documented limits — constructed evasions

- A copied or commented-out `getMerged*Value` call is still parsed because comments are not removed. My comment-decoy mutation exited 0.
- A deliberately introduced macro alias or regex-defeating spelling can hide an added 28th read while the existing 27 keep the parser above its floor.
- These are constructed evasions, separate from the accidental constant/helper/other-file gap above.

Mutation results

| Mutation | Class | Exit/result |
|---|---|---|
| Delete `modes.enableHibernateSleep` | Accidental | 1; named missing path |
| Add `messaging.disconnectedMode` | Accidental | 1; named dead path |
| Add literal `modes.newKey` read to copied `ConfigApply.cpp` | Accidental | 1; named missing path |
| Break merged-call regex completely | Constructed | 1; parser floor reported 0 |
| No matching defaults file | Accidental | 1; directory named |
| Second file missing key | Accidental | 1; offending file and key named |
| Second file with dead key | Accidental | 1; offending file and key named |
| Second valid file | Control | 0; both files printed PASS |
| Invalid JSON file | Accidental | 1; offending filename not printed |
| Replace scalar with unread nested leaf | Accidental | 1; missing and dead paths named |
| Add key through C++ constant | Accidental | **0; survived** |
| Comment out a read while leaving matching text | Constructed decoy | **0; survived** |

Nonmatching `docs/config/README.md` is ignored by the `default-settings-*.json` glob. Bare execution succeeded both from the repository and `/private/tmp`; imports are standard-library only.

Historical old-name hits retained: `docs/releases/v3.23-implementation-status.md`, its generated `docs/html` page, prior Stage 6 transcripts/reports and Stage 7 verdicts under `docs/work-orders/`, the pre-Step-6 known-issues dispatch, and this WO’s migration spec/dispatch/report. No live pasted ledger content contradicts the new file.

Model/reasoning actually run: `gpt-5.6-sol`, medium.  
Interpreter: Python 3.14.6; zsh 5.9 (arm64 Apple Darwin).  
Round 1 of at most 2; failure goes to the architect.