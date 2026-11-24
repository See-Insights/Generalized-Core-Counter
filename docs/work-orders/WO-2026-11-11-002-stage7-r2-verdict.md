UNDER REVIEW: wo/2026-11-11-002-default-settings-coverage @ c79b3e39fcc28ba350953518fb11f3c8e5e2cd15

NOT VERIFIED

| # | Verdict | Evidence |
|---|---|---|
| 1 | FAIL | Real tree passes 27/27; required local-constant and second-`.cpp` cases fail correctly. However, a plausible dereferenced-map call is silently skipped as a definition by `tests/default_settings_coverage_test.py:33,45-46`. |
| 2 | PASS | Per-file handling at `tests/default_settings_coverage_test.py:64-70` names malformed, empty, array, scalar, and unreadable files without traceback; valid files continue to be checked. |
| 3 | PASS | Correct wording at `docs/config/README.md:13-15` and WO `:62,69`; citations verified at `src/Config.h:35` and `src/Config.cpp:145-151`. No other false README statement found. |
| 4 | PASS | All eleven required scan/ledger mutations failed as intended. The additional dereferenced-map mutation exposed Finding 1; a second-`.cpp` definition control passed. |
| 5 | PASS | `src/`: 0 changed lines; test: 108 lines; README: 17 lines. Clean CI discovery: **119/119**—68 `.sh` via zsh, 51 `.py` via python3. Bare test passed from repository root and `/tmp`. Imports are standard-library only. |

## Finding

**Medium — a plausible real call can be skipped as a definition**  
`tests/default_settings_coverage_test.py:33,45-46`

Path that breaks:

```cpp
Variant *p = &defaultModes;
getMergedIntValue(*p, deviceModes, "newKey", x);
```

The `*` in the first argument matches `NOT_A_CALL`, so the occurrence is discarded before parsing. The scratch test exited 0 with the original 27/27 even though `modes.newKey` was absent.

This falls within Finding 1’s explicitly requested definition-versus-call review; it is an accidental C++ calling form, not a macro or regex evasion.

Smallest fix: make the `&`/`*` declaration alternatives require a type-and-name shape, for example `\w+\s*[*&]\s*\w+`, rather than treating any `&` or `*` in the first argument as a declaration. The dereferenced call would then fail closed as unparsed.

The four real definitions at `src/cloud/ConfigApply.cpp:29,43,57,71` are correctly skipped. A conventional definition added to another `.cpp` was also skipped without error. Header definitions cannot be miscounted because headers are not scanned.

## README verification

- Production ledger value 300000: `docs/config/default-settings-42131.json:3`.
- Compiled default 60000: `src/Config.h:35`.
- Missing/zero runtime fallback 60000: `src/Config.cpp:145-151`.
- Only sensor type 1 accepted: `src/cloud/ConfigApply.cpp:350-351`.
- `setting1` is PIR occupancy debounce: `src/MyPersistentData.h:493-494`, `src/state/State_Common.h:316-325`.
- Settings 2–4 are zero/unused for today’s interrupt-driven PIR: `src/MyPersistentData.cpp:555-558`, `src/sensors/SensorManager.cpp:322-336`.

## Documented limits

- Section-variable mapping is not lexically scoped.
- Commented-out calls are still counted.
- Macro aliases and regex-defeating spellings are invisible.

## Known gaps

- Only `.cpp` files are scanned; calls in headers or `.ino` files are invisible.
- Defaults-ledger readers other than `getMerged*Value` are not detected.
- Non-identifier section arguments and non-`\w` literal keys fail closed.
- Live console ledger content is not checked.

## Mutation results

| Mutation | Class | Result |
|---|---|---|
| Delete `modes.enableHibernateSleep` | Required | Exit 1; missing path named |
| Add `messaging.disconnectedMode` | Required | Exit 1; dead path named |
| Add literal `"newKey"` in copied `ConfigApply.cpp` | Required | Exit 1; `modes.newKey` named |
| Make merged-call pattern match nothing | Required | Exit 1; parser floor 0 |
| No defaults file | Required | Exit 1; directory named |
| Second file missing a key | Required | Exit 1; file and key named |
| Second file with dead key | Required | Exit 1; file and key named |
| Key through local constant | Required | Exit 1; `ConfigApply.cpp:680` named unparsed |
| Malformed JSON | Required | Exit 1; bad filename/error named; good file checked |
| Literal read in second `.cpp` | Scan verification | Exit 1; `modes.otherKey` missing |
| Unparseable read in second `.cpp` | Scan verification | Exit 1; `Other.cpp:1` named |
| Empty JSON file | Edge | Exit 1; filename named, no traceback |
| Top-level array | Edge | Exit 1; filename named, coverage errors |
| Top-level scalar | Edge | Exit 1; filename named, coverage errors |
| Permission-denied file | Edge | Exit 1; filename named, no traceback |
| Definition in second `.cpp` | Control | Exit 0; correctly ignored |
| Dereferenced first argument | Accidental | **Exit 0; survived—Finding 1** |

No Device OS build was needed because `src/` is unchanged.

Model/reasoning actually run: `gpt-5.6-sol`, medium.  
Interpreter: Python 3.14.6; zsh 5.9 (arm64 Apple Darwin).  
Round 2 of 2, the last.