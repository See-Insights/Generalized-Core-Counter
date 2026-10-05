## Implementation Report — WO-2026-09-24-004 Stage 6 (v36-HourRules)

**Changes (net `src/` code lines −16; budget: net negative ✅)**

| # | File | Change | Lines |
|---|---|---|---|
| 1 | `Config.h` | `Config::hoursRuleFailure(int,int)` (returns the rule broken, or `nullptr`) + `hoursFollowRules()` — the only place the rules are written | 33→48 (+15) |
| 2 | `cloud/ConfigApply.cpp` | `applyTimingConfig()`: merged pair (absent value falls back to current), one shared check, both-or-neither apply; on reject logs values + rule, keeps last valid hours, `success = false`. `+#include "../Config.h"` | 556→555 (−1) |
| 3 | `MyPersistentData.cpp` | `sysStatusData::validate()` uses the shared check | 862→858 (−4) |
| 4 | `Config.cpp` | `validateConfigFields()` uses the shared check, same log/failure-reason style | 169→162 (−7) |
| 5 | `time/DailyBoundary.cpp` | `close = SystemConfig::get_closeTime();`, `openHour` removed | 37→35 (−2) |
| 6–7 | `time/Clock.cpp` | daytime window only; overnight and `open == close` branches removed from both functions; unused `closeSec`/param name dropped | 192→175 (−17) |
| 8 | `FirmwareVersion.h` | `./bump_version.sh v36-HourRules …` → product 36 (also CHANGELOG/README/Doxyfile) | ±0 |

**Tests** — new `tests/hour_rules_test.sh` (+`hour_rules_test.cpp`, `tests/stubs/hour_rules_overrides/Particle.h`):
- Part 1 compiles the **real** `src/Config.h` check and the **real** `Clock.cpp` window functions (extracted verbatim; they are file-static). Accepted 6/22, 6/23, 0/24, 12/13; rejected 6/6, 22/22, 20/6, 13/22, 0/25, 13/24, 7/7 and every `close ≤ open` (exhaustive −1..30 sweep). Valid-pair equivalence vs. v35 copies for `isWithinOpenHoursForHour`, `secondsUntilNextOpenForSeconds` and the daily close; 0/24 open at every hour, closes at midnight. **Pass.**
- `applyTimingConfig()` is **structural** (Device OS `LedgerData`/`Variant`; no harness stubs it): merge, exactly one shared check before any write, adjacent `set_openTime`/`set_closeTime`, `Log.warn` + rule + `success = false` on reject, no `validateRange(openHour/closeHour)`. Plus src-wide: no sentinel, no overnight branch.
- Mutations (shadowed copies of `Config.h`): rule 1 (`>24`→`>23`), rule 2 (`<=`→`<`), rule 3 (bound dropped) — **all three detected**.
- Existing test updated (1): `tests/daily_cleanup_boundary_test.py` pinned `close = (openHour == closeHour) ? 24 : closeHour` and the `get_openTime()` read — both removed by this WO; now pins `close = SystemConfig::get_closeTime()`. `publish_with_ack`, `sleep_config_ownership`, `ledger_no_retry` unchanged and passing.

**Verification** — suite before **61/61**, after **62/62** (`tests/*.sh` via zsh, bare `tests/*.py` via python3). Boron / Device OS 6.4.1 release build after `make clean-user`: **text 150740** (v35 150684, +56 — the new log strings), **data 1090** (=), **bss 2180** (=); `strings` finds `v36-HourRules`. `target/` was snapshotted and restored byte-identically (`diff -rq` clean).

**Scratch** — only `build-tmp/wo20260924-004-stage6/` was deleted; `build-tmp/`, `connectivity-archive/` and the prior transcript untouched. No commits, pushes, merges or branch changes; working tree left dirty.

**Deviations:** two, both minor. (1) `#include "Config.h"` in `cloud/ConfigApply.cpp` failed the ARM build (another `Config.h` shadows it); used `"../Config.h"`, as `time/Clock.cpp` does. (2) The shared check is `inline` in `Config.h` rather than defined in `Config.cpp`, so `MyPersistentData.cpp`'s existing host test links without a new stub; the rules are still written in exactly one place. The WO's `secondsUntilNextOpenForSeconds()` value difference at 0/24 is asserted (noon → next midnight) for Stage 7.

**Model:** Claude Opus 5 (`claude-opus-5`), reasoning medium.

