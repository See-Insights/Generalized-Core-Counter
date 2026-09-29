# Implementation Report — WO-2026-09-29-001 Stage 6

**Model / reasoning actually used:** claude-opus-5, medium.

## Per item

| Item | Files / lines | `src/` lines | Budget |
|---|---|---|---|
| A | `src/state/State_Report.cpp` (+3): `} else if (due) { transitionTo(CONNECTING_STATE, "daily close"); }` after the occupancy-change branch, before webhook-health/keep-alive/cadence | 3 | ≤5 ✓ |
| B | `src/Generalized-Core-Counter.cpp` 959–964 (−6): the 4-line comment, the `Particle.subscribe("hook-response/", ...)` call, and the blank separator. Device-ID `responseTopic` subscribe and `UbidotsHandler()` untouched | 5 named + 1 blank | ≤5 ✓ (see deviation 1) |
| C | `src/cloud/DeviceStatusPublisher.cpp` (+8): the 7-line guard verbatim from the WO, plus one comment line, immediately before `bufferBase[writerBase.dataSize()] = '\0';`. No payload-field change | 8 | ≤8 ✓ |
| D | `src/Generalized-Core-Counter.cpp` `logTimeDiag()` (+5/−4): live `LocalTimeConvert` → `getLocalTimeYMD()` / `getLocalTimeHMS().toSeconds()` / `.hour`. `localDate`/`localSecondsOfDay`/`localHour` names, minute/second derivation, and the `Log.info` call unchanged | 5 | ~5 ✓ |
| E | `project.properties` (−4 `dependencies.*`); `lib/AB1805_RK/src/AB1805_RK.cpp` (−3: blank, comment, `_log.info("petting watchdog...")`) | n/a | ✓ |
| Version | `src/Version.cpp` → `"v27-SmallFixes"` + new notes (no `pdiag`); `src/FirmwareVersion.h` 26→27 | 4 | — |

**Total `src/`:** 21 changed lines (~20 ✓).
**Shared files:** `src/Generalized-Core-Counter.cpp` is shared by **B and D** (separate hunks, ~915 lines apart — `git add -p` separable). All others unique.

## A test assertion (`tests/daily_cleanup_boundary_test.py`, +13)
In `verify_source_shape()`, on comment-stripped `handleReportingState()`: regex `\}\s*else\s+if\s*\(\s*due\s*\)\s*\{\s*transitionTo\(CONNECTING_STATE,\s*"daily close"\);` must match, and its offset must precede both `SystemConfig::INTERMITTENT_KEEP_ALIVE` and `reportingPolicy.cadenceDue`; otherwise `fail(...)`. No other test changed.

## Verification
1. **Host suite: 42/44 (sh via zsh, py via python3)** — 2 failures, both caused by item C. See blocker.
2. **Local ARM build (boron, release)**, `PLATFORM=boron`, Device OS 6.4.1, fresh `BUILD_PATH_BASE` scratch (clean user objects; `make clean-user` has no rule in this tree): **text 150220 / data 1090 / bss 2196** vs v26 149708/1090/2196 (+512 text). `strings` on the `.bin`: `v27-SmallFixes` = 1 hit; `pdiag` = 0; `petting watchdog` = 0. Also present: `daily close`, `LedgerPayloadStatus: overflow ...`.
3. `git diff --numstat` as tabled above. (`docs/work-orders/WO-2026-09-25-002-*.md` was already modified before this session — not mine.)

## 🔴 Blocker — stop-and-report condition
`tests/clock_status_republish_test.sh` and `tests/power_source_override_test.sh` fail to compile:
```
src/cloud/DeviceStatusPublisher.cpp:356:13: error: no member named 'error' in 'TestLog'
```
Their host stubs (`tests/stubs/clock_status_republish_overrides/Particle.h`, `tests/stubs/power_source_override_overrides/Particle.h`) define `TestLog` with `info()` and `warn()` only. The WO requires the guard's `Log.error(...)` verbatim, so the two are incompatible. Minimal remedy (**not applied — outside the authorization scope**): add a no-op `template <typename... Args> void error(const char *, Args...) {}` to both stubs. `tests/publish_with_ack_structural_test.py` passes unchanged.

## Deviations
1. **B removed 6 lines, not 5** — the 5 lines named plus the now-orphaned blank separator line.
2. **`make clean-user` not used** — no such target in `deviceOS/6.4.1/main`; equivalent achieved with a deleted, fresh `BUILD_PATH_BASE`/`TARGET_DIR`. No `EXTRA_CFLAGS` change was involved.
3. **`#include "time/LocalTimeCache.h"` left in place** in `Generalized-Core-Counter.cpp` (now unused there) — removing it was not authorized.
4. Two transient log files (`/tmp/wo29-host.log`, `/tmp/t.log`) could not be deleted by this environment. All in-repo artifacts (`build-tmp/`) are removed.

Nothing committed, pushed, merged, or flashed; branch unchanged (`wo/2026-09-29-001-v27-small-fixes`).

