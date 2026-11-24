# WO-2026-11-11-002 Stage 6 round 3 report (Copilot), Amendment 2

**Model/level:** claude-sonnet-5.5; dispatched at medium, actual level not observable.
**Interpreter:** Python 3 (python3), zsh, macOS, in the git worktree. Nothing committed.

## Baseline (before editing)
119/119 (sh via zsh, py via python3), CI discovery as in `.github/workflows/build.yml:45-52`.

## Failing before (head test, scratch trees in `build-tmp/wo-2026-11-11-002-stage6-r3/`, script `mutate.py old`)
- (a) local constant `getMergedIntValue(defaultModes, deviceModes, newKey, v)`: exit 0, "27 == 27" PASS.
- (b) `default-settings-bad.json` with `{"a": `: exit 1 by uncaught traceback (`JSONDecodeError`); the file is never named; the good file was not reported either.
- (c) read in a second `.cpp` (literal key `otherKey`, and an unparseable key): exit 0 both; the old test opens one file only.

## Test
`tests/default_settings_coverage_test.py`: **108 lines** (was 79; budget about 100, +8 over). Stdlib only, `src/` 0 lines.
- Scan: `DEFAULTS_COVERAGE_SRC` now names a **source root directory** (default `<repo>/src`); every `**/*.cpp` is read. Section-variable map (`getTopLevelMap`/`getNestedMap`) is built across all scanned files, unchanged otherwise. `DEFAULTS_COVERAGE_DIR` unchanged.
- Calls vs definitions: every `getMerged{Int,Bool,String,Float}Value(` occurrence is a candidate; the text up to the first comma is the first argument. If it has `const`, `&`, `*`, or two words (`Type name`) it is a definition/declaration and skipped (`ConfigApply.cpp:29,43,57,71` all skipped). Otherwise it is a call and must match `(sectionVar, anyIdent, "literalKey")` with `sectionVar` mapped, else the run fails listing `file:line`.
- Documented limits in the header: lexical scoping, commented-out calls, macro/regex evasions.
- Invalid JSON: `OSError`/`ValueError` (covers `JSONDecodeError`) caught per file; prints `FAIL <basename>: invalid JSON: <error>`; other files still checked; exit 1.

## README (`docs/config/README.md`, 17 lines)
`setting1` cell and the added sentence use the Amendment 2 wording. Citations verified at this commit:
- `src/Config.h:35`: `constexpr uint32_t DEFAULT_OCCUPANCY_DEBOUNCE_MS = 60000UL;` correct.
- `src/Config.cpp:145-151`: `occupancyDebounceMsForRuntime()`; fallback on invalid config at `:147`, on zero at `:151`. Correct, not moved.

## Mutations (`mutate.py new`)
| Mutation | Exit | Result |
|---|---|---|
| delete `modes.enableHibernateSleep` | 1 | missing key named |
| add `messaging.disconnectedMode` | 1 | dead key named |
| literal `"newKey"` in copied `ConfigApply.cpp` | 1 | missing `modes.newKey` |
| regex matches nothing (`getMerged` renamed) | 1 | parser floor 0 < 25 |
| scratch dir, no file | 1 | "no default-settings-*.json files" |
| second file, key removed | 1 | 99999 missing key |
| second file, dead key | 1 | 99999 dead key |
| control: second identical file | 0 | both PASS |
| NEW key through local constant | 1 | `cloud/ConfigApply.cpp:680` not parsed |
| NEW invalid JSON file | 1 | `FAIL default-settings-bad.json: invalid JSON: Expecting value: line 1 column 7 (char 6)`; good file still PASS |
| NEW literal read in second `.cpp` | 1 | missing `modes.otherKey` |
| NEW unparseable read in second `.cpp` | 1 | `Other.cpp:1` not parsed |

## Suite after
119/119 (sh via zsh, py via python3). Test exits 0 bare from the repo root and from `docs/`; I could not run it from `/tmp` (command denied), but paths derive from `__file__`. `git diff --numstat -- src` empty; `default-settings-42131.json` untouched.

## Known gaps (not fixed)
- The documented limits above (scoping, comments, macros/regex).
- Only `.cpp` files are scanned; a call in a `.h`/`.ino` is invisible.
- Calls that are not `getMerged*Value` (other defaults-ledger readers, wrappers) are not detected.
- A call with a literal key but a non-`\w` key or a non-identifier first argument fails closed (reported), by design.

## Deviations
- Test is 108 lines, about 8 over the stated 100. `DEFAULTS_COVERAGE_SRC` semantics changed from file to directory (as the dispatch allowed).
- Changes uncommitted: modified `tests/default_settings_coverage_test.py`, `docs/config/README.md`; new this report.
