# WO-2026-11-02-004 Stage 6 report (Copilot)

Branch `wo/2026-11-02-004-timezone-planned-reset`, base `21804e3`. Nothing committed, pushed or flashed.

## Model and interpreter
- Model: Claude Sonnet 5.5 (`claude-sonnet-5.5`). The dispatch header says "medium"; the CLI session was started at reasoning effort low, so medium was not what ran.
- Interpreter line: host tests run as `zsh tests/*.sh` and `python3 tests/*.py`; compiler `clang++ -std=c++17`; ARM build with gcc-arm 10.2.1, Device OS 6.6.0, buildtools 1.1.1.

## Change (`git diff --stat -- src/`: 13 insertions, 3 deletions = **+10 net**)
| File | Net |
|---|---|
| `src/ResetCause.h` | +1 (`RESET_CAUSE_TIMEZONE_CHANGE = 8`) |
| `src/cloud/ConfigApply.cpp` | +9 (include, file-local flag, flag set at the store, flush + reset block; the existing 3-line "no forced flush" comment was reworded in place, since it would otherwise contradict the block) |

- The dispatch says `src/ConfigApply.cpp`; the file on this base is `src/cloud/ConfigApply.cpp`.
- Line numbers differ from the WO's `a7b69ce` figures: the store is now at about `:261-264` and the success branch at `:144-157`. The `strcmp` guard, `set_timeZoneStr`, the log and `changed = true` are as the WO describes.
- Block: after `validateStoredData()`, `if (timezoneStored) { SystemConfig::flushNow(); System.reset(RESET_CAUSE_TIMEZONE_CHANGE); }`. This matches the failsafe pairing (`SystemConfig::flushNow()` then `System.reset(<cause>)`, `ConnectivityFailsafe.cpp:86,306`). The flush API is the same, so no stop condition was hit.
- The flag is never cleared (the device resets). If the zone is stored in a call whose apply then fails, the flag stays set and the next successful apply resets once. The running clock would otherwise stay on the old zone, and the stored value is already persisted by then, so there is no loop. This is a decision for review.

## Tests
- New `tests/timezone_planned_reset_test.sh` and `.cpp`: extracts the real flag, the real timezone block and the real `Cloud::applyConfigurationFromLedger()` from `src/cloud/ConfigApply.cpp` and runs one process per case (the flag is file-local).
  - `changed_zone`: events are store, flush, reset(8), in that order.
  - `after_reboot`: same ledger value as stored, so no reset.
  - `unchanged_zone`: no reset and no flush.
  - `failed_apply`: another section fails, so no reset and no flush.
  - `failed_timing`: the timing section fails after storing the zone, so no reset.
  - `other_section_only`: no reset, and the status publish is still deferred.
  - `zone_absent`: no reset.
- `tests/reset_cause_structural_test.py` extended: site count 7 to 8, code 8 declared, and the site must be in `src/cloud/ConfigApply.cpp`. Distinct and non-zero were already checked. It also takes an optional `RESET_CAUSE_ROOT` environment variable so mutations can run on a copy.

## Mutations
Each is planted on a copy (the repo is untouched, so nothing to revert) and is run as Part 2 and Part 3 of `timezone_planned_reset_test.sh`. All are caught at run time, none by a compile error.

| Mutation | Caught by |
|---|---|
| reset without the flush | `changed_zone` (order) |
| reset on every apply | `after_reboot` |
| reset when the zone is unchanged | `after_reboot` |
| reset on a failed apply | `failed_apply` |
| the flag is never set | `changed_zone` |
| reset before the flush | `changed_zone` (order) |
| cause code 0 | structural test |
| reused code (7) | structural test |

## Verify
- Host suite: **108/108 (sh via zsh, py via python3)**: 62 `.sh` and 46 `.py`, including the new test.
- Local toolchain build: Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE` under `build-tmp/wo-2026-11-02-004-stage6/`. `make` returned rc=0 with only pre-existing LocalTimeRK warnings. Size: text 151636, data 1114, bss 2200 for `app.elf`.
- `nm -C app.elf` shows `Cloud::applyConfigurationFromLedger` (`000baf78`), `SystemConfig::flushNow()` (`000b6cac`), `(anonymous namespace)::timezoneStored` (`2003da84`) and `system_reset`. The disassembly of the apply function shows `bl SystemConfig::flushNow()`, then `movs r0, #8`, then `bl SystemClass::reset(unsigned long, Flags)`.
- `git diff --stat -- src/` is +10 net (above).
- `docs/reference/alert-codes.md` not touched: no alert is involved.

## Stage 7 check (a), setup-time reach
`applyConfigurationFromLedger` is reached only through `Cloud::loadConfigurationFromCloud()` (`State_Connect.cpp:536` in CONNECTING, and `:695` in FIRMWARE_UPDATE) and the deferred apply in `Cloud::loop()` (`Cloud.cpp:682`). Not from `setup()`: `Cloud::instance().setup()` (`Generalized-Core-Counter.cpp:907`) does not apply the ledger, and `withConfig` is at `:932`, before any connect. So there is no needless reset before `LocalTime` is configured; the other paths are post-connect.

## Open questions
1. In `Cloud::loadConfigurationFromCloud()` and the deferred path, `finalizeLedgerAppliedConfig()` (which marks `hasValidLedgerConfig` and the config source) runs after the apply returns. On a timezone change the reset fires first, so that happens one boot later, when the apply finds no change. Net effect: `hasValidLedgerConfig` is delayed by one connect. Not a loop. Please confirm this is acceptable.
2. The pending "ConfigApply" status publish and the Alert 41 handling on the changed-zone connect are skipped by the reset. The startup status reports after the reboot.
3. Stage 7 check (b), survival of totals, the open session and the publish queue, is not covered by host tests; it needs a bench check against the failsafe stage-2 path.
4. Scratch output is left in `build-tmp/wo-2026-11-02-004-stage6/` (gitignored); delete when done.
