# WO-2026-11-02-004 Stage 7 verdict — round 2 (final)

**Verdict: PASS**

**Blocking findings:** none.  
**Model actually used:** `gpt-5.6-sol`  
**Reasoning actually used:** `high`  
**Interpreter line:** shell tests with zsh 5.9; Python tests with Python 3.14.6; host C++ with Apple clang 14.0.3; ARM build with gcc-arm 10.2.1 and Device OS 6.6.0.

## Production change

Against specified base/origin-main `21804e3`, `src/` changes only `src/Config.h`:

- 2 insertions, 1 deletion: **+1 net**, within the ≤ +2 budget.
- The only changes are the product-default comment and:
  `EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00`
- All 106 `src/` files are otherwise unchanged.
- `ResetCause.h`, `cloud/ConfigApply.cpp`, and `tests/reset_cause_structural_test.py` are byte-identical to the base.
- No `timezoneStored`, timezone reset cause, or timezone-triggered `System.reset()` remains.
- `git diff --check` passes.

## String, persistence, and parser

The string is exactly the Amendment 2/product-42131 value and is 38 characters.

- `timeZoneStr` capacity is 39 bytes, so the value plus NUL occupies exactly 39 bytes.
- The production `StorageHelperRK::setValueString()` condition is `strlen(value) < size`; therefore 38 `<` 39 is accepted.
- `ConfigApply` also stages timezone values in a 39-byte buffer and accepts lengths below 39.
- LocalTimeRK splits the POSIX string into timezone, DST-start, and standard-start tokens. `LocalTimeChange::parse()` accepts `M3.2.0`/`M11.1.0`, and `LocalTimeHMS::parse()` parses `/2:00:00` with `%d:%d:%d`.
- The vendored LocalTimeRK tests use this exact string and assert its Eastern/DST fields. It is valid for `LocalTimePosixTimezone`.

## Consumers and migration behavior

Every production use was checked:

- `MyPersistentData.cpp:133`: a genuinely new store initializes with Eastern.
- `Generalized-Core-Counter.cpp:927-932`: an empty stored value is replaced with Eastern before `LocalTime::withConfig()`.
- `DeviceStatusPublisher.cpp:645`: a device still storing `UTC0` now satisfies the source predicate for “non-default.” However, `hasNonDefaultConfig()` has no callers and is absent from the linked ELF, so this currently has no runtime effect.
- Status configuration generation hashes the actual stored timezone, not `DEFAULT_TIMEZONE`.

Existing nonempty stored values are not rewritten. A device holding `UTC0` continues running in UTC. When its ledger subsequently stores Eastern, the running `LocalTime` converter remains UTC for that boot; the ledger zone takes effect at the next reboot/hibernate wake/OTA/failsafe reset when `setup()` configures the converter again.

The explicit `UTC0` instances in the exerciser and power-source stub are independent test inputs, not assumptions about the firmware default. Contracts define the timezone field but no compiled default. The historical `default-settings-v3.23.json` contains `SGT-8`, not `UTC0`, and is not a runtime firmware input. In-repository fleet-health material uses explicit UTC timestamps or “local” reporting windows and does not consume this constant.

## First-boot and boundary behavior

There is no UTC-to-Eastern transition partway through a fresh boot:

- Persistence setup initializes Eastern, or the empty-value fallback selects Eastern.
- `LocalTime::withConfig()` runs before sensor initialization and before open/close decisions.
- `DailyBoundary` reads that configured singleton, so its first eligible boundary calculation is Eastern.

The default 06:00–22:00 window therefore becomes:

- Standard time: 11:00Z open, 03:00Z next-day close.
- Daylight time: 10:00Z open, 02:00Z next-day close.

Previously, `UTC0` made those boundaries 06:00Z and 22:00Z. This first-boot boundary shift is the intended behavior.

On a genuinely fresh device, factory-default configuration remains untrusted until a valid ledger apply, as before: `Clock::openness()` is `Unknown`, and the raw open-hours helper fails open. Once ledger configuration is finalized, the 06–22 window is enforced in Eastern. `DailyBoundary::check()` itself is clock-trust-gated and computes its boundary with Eastern.

## Test verification

The new test compiles the real `Config.h` and `MyPersistentData.cpp` persistence facade using the established host persistence backend.

- Exact value, length, capacity, setter acceptance, and round-trip pass.
- `UTC0` mutation: caught at runtime.
- Truncated-string mutation: caught at runtime.
- Neither mutation relied on a compilation failure.

Host suite: **108/108**

- **62/62 shell tests via zsh**
- **46/46 Python tests via python3**

The suite was run against a clean scratch source copy to prevent pre-existing `build-tmp/` source copies from confusing repository-wide structural scans. Four Git-history-aware Python tests were invoked from the real repository with their supported scratch `SRC` argument; all passed.

## Device OS build and linked ELF

Fresh build succeeded with:

- Device OS 6.6.0
- Platform `boron`
- Fresh `BUILD_PATH_BASE=build-tmp/wo-2026-11-02-004-stage7-r2/bpb`

ELF size:

- text: 151644
- data: 1114
- bss: 2200
- total: 154958

Linkage evidence:

- `nm` contains `setup`, `sysStatusData::initialize()`, and `SystemConfig::set_timeZoneStr(char const*)`.
- `objdump -s -j .text` shows the exact Eastern string at linked address `0x0d1372`.
- The exact string occurs once in the ELF.
- No standalone `UTC0` occurs in the ELF.
- The build-copy and reviewed `Config.h` SHA-256 hashes match.

## Alerts

No alert is raised, ranked, or cleared by this change:

- No alert/reset path changed.
- No deliberate reset occurs, so the round-1 legacy-alert-14 clearing side effect is gone.
- The accepted timezone does not create a config-apply failure.
- `docs/reference/alert-codes.md` is byte-identical to the base.

## Non-blocking notes

1. Generated Doxygen pages under `docs/html/` still display the historical `UTC0` value. The dated 29 September configuration inventory also correctly records the value that existed at that time. These are not runtime consumers or normative ledger contracts, but generated documentation should be refreshed in its normal documentation workflow.

2. The external fleet-ops implementation and live Particle ledger were outside this repository-only, no-network authorization. The binding Amendment 2 supplies the authoritative product-42131 value; no in-repository fleet-ops or contract dependency on `UTC0` was found.