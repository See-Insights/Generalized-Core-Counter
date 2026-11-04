# Stage 6 round 2 report — WO-2026-11-02-004 Amendment 2 (default timezone)

- **Model / reasoning:** claude-sonnet-5.5; reasoning effort not visible to the agent (dispatch asked for medium).
- **Interpreters:** zsh 5.9 (`.sh`), Python 3.14.6 (`.py`).
- **Changes (uncommitted):** `src/Config.h` (+2/−1 = **+1 net**; `git diff --stat -- src/`: 2 insertions, 1 deletion), new `tests/default_timezone_test.sh` + `tests/default_timezone_test.cpp`.
  `DEFAULT_TIMEZONE = "EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"` with a one-line comment above it.

## Test: `tests/default_timezone_test.sh`
Links the real `src/MyPersistentData.cpp` against the persistence host stub and asserts: exact string; length 38; `strlen < sizeof(SysData::timeZoneStr)` (39); the real `SystemConfig::set_timeZoneStr` accepts it (`setValueString` rule: `strlen(value) < size`, 38 < 39) and the value round-trips.
Mutations (on copies of `Config.h`, never `src/`): `UTC0` → detected; truncated `EST5EDT,M3.2.0/2:00:00,M11.1.0` → detected.

## `DEFAULT_TIMEZONE` use table
| Site | Use | Effect of the change |
|---|---|---|
| `MyPersistentData.cpp:133` | fresh-device default in `sysStatusData::initialize()` | New devices store the Eastern string; fits (38 < 39). |
| `Generalized-Core-Counter.cpp:929` | empty-stored fallback in `setup()` | Falls back to Eastern, stored via `set_timeZoneStr`. |
| `DeviceStatusPublisher.cpp:645` | `hasNonDefaultConfig()` (`strcmp != 0`) | A device that already stored `UTC0` now counts as **non-default config** (true). Devices with the Eastern string count as default. No code change. |
| `Config.h` itself | definition | — |

No validation fallback or reset-to-defaults path other than the above references the name (searched `src/` and `tests/`). Existing stored values are not rewritten (no reset code), so already-provisioned devices keep `UTC0` until a ledger sets the zone.

## Existing tests assuming `UTC0`
- `tests/exerciser_baseline_test.py:45`: `tz="UTC0"` is its own explicit parameter, not the firmware default; passes unchanged.
- `tests/stubs/power_source_override_overrides/MyPersistentData.h:51`: independent stub value; passes unchanged.
No test needed editing.

## Verification
- **Host suite: 107/108 (sh via zsh, py via python3)** (includes the new test). The one failure, `thermal_coupling_structural_test.py`, scans the whole repo tree including `build-tmp/` copies of `SensorManager.*` (from the earlier stage6/stage7 builds and my scratch build) and fails on `readTmp112TemperatureC()` references there. It is unrelated to `Config.h`; I did not verify it against a clean baseline (stashing was not permitted), and did not delete the earlier `build-tmp/` dirs (not mine).
- **Local build:** Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE=build-tmp/wo-2026-11-02-004-stage6-r2/bpb`; succeeded, `app.elf` 151644 text / 1114 data / 2200 bss.
- **Linked-ELF proof:** `arm-none-eabi-objdump -s -j .text app.elf` shows `EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00` at `0xd1370` (the ELF has no `.rodata` section; literals are in `.text`). `strings` on the ELF finds the string; no standalone `UTC0` string remains. Caveat: the `constexpr const char*` has no symbol in `nm`, so this proves the literal is linked, not which call site uses it.

## Open questions
- Whether the `thermal_coupling_structural_test.py` scan should exclude `build-tmp/` (pre-existing, outside my scope).
- Devices already holding `UTC0` need a ledger timezone value to change; fleet-wide migration is outside this WO.
