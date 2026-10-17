# NOT VERIFIED — Codex `gpt-5.6-sol` (standard tier) · reasoning: high

Test interpreters: **83/83 — 51 `.sh` via `zsh`, 32 `.py` via `python3`** (HEAD inventory: 81 = 50 shell + 31 Python).

| # | Result | Evidence |
|---:|:---:|---|
| 1 | PASS | `src/persist/RetainedBoot.h:6-11` contains exactly the nine main-file variables with their original types. Boron DWARF gives a 32-byte struct: `magic` u32 @0, `version` u8 @4, `bootStormCount` u8 @5, `bootStormLastResetReason` u8 @6, `bootStormTripCount` u8 @7, `bootStormWindowStart` `time_t` @8, `bootStormLastResetReasonData` u32 @16, `appBreadcrumbMs` u32 @20, `appBreadcrumb` u8 @24, `bootInProgress` bool @25, `bootStormAlertPending` bool @26; only bytes 27-31 are tail padding. Other retained definitions: `retainedLoopForensics` is magic/version validated (`Generalized-Core-Counter.cpp:612-625`); RTC-skew state is build-token/build-flag guarded (`:505-523`, `:1202-1227`); hibernate fields are pending/classification guarded and its count is report-only (`time/HibernateCycle.cpp:22-25,89-131`); `lastTrend*` is sentinel validated (`power/PmicFaultMonitor.cpp:46-48,431-445`); `ledOffTime` is zero/time/deadline guarded (`device_pinout.cpp:95,118-138`); ThrashGuard and PMIC-anomaly fields are unvalidated diagnostics/reporting state (`ThrashGuard.cpp:28-30`; `sensors/SensorManager.cpp:34-40`). No unvalidated decision-bearing retained definition falls outside the WO's exclusions. |
| 2 | PASS | `validateRetainedBoot(retainedBoot);` is the first statement of `setup()` at `Generalized-Core-Counter.cpp:786`, before loop-forensics initialization at `:789`. Bare and qualified searches find no other source user of the struct or the nine names; the only `STARTUP()` is `cloud/Particle_Functions.cpp:11` and touches none. Static reference binding at `Generalized-Core-Counter.cpp:495-499` takes addresses but does not read retained values; no constructor or ISR reads them before `setup()`. |
| 3 | PASS | Value-initializing `RetainedBoot` sets every member to zero, matching all nine former initializers (`git show HEAD:src/Generalized-Core-Counter.cpp`, old lines 491-499): numeric fields `0`, bools `false`, and breadcrumb `BREADCRUMB_NONE`. The enum defines `BREADCRUMB_NONE = 0` at `Generalized-Core-Counter.cpp:196-197`, and `:494` statically asserts it. |
| 4 | PASS | Manual trace: after validation, `bootInProgress == false`; therefore `previousBootFailedEarly` is false at `Generalized-Core-Counter.cpp:819`, so neither `:849-853` nor the trip at `:874-885` can execute, regardless of reset reason or clock trust. The trusted-clock block can only set the window from zero; it cannot raise the count. `retained_boot_test.cpp:66-75` checks all 256 reasons under both trust states, and `:78-83` proves repeated qualifying boots still trip. |
| 5 | **FAIL** | Fresh 6.4.1 Boron builds produce architect outcome **B**: `.backup` stays 0x98 bytes (`0x2003f400-0x2003f497`) and `__backup_sram_signature` stays at `0x2003f494`, while all three unvalidated ThrashGuard symbols move. Exact effects: candidate `thrashTripCount` at `0x2003f440` reads HEAD's `thrashResetCount`; candidate `lastThrashReason` at `0x2003f442` reads old padding byte `0x442`; candidate `thrashResetCount` at `0x2003f444` reads old padding bytes `0x444-445` (all within the unused `0x442-447` gap before `ledOffTime`). The padding has no defined live numeric value; it would be zero after a full retained clear, but this OTA preserves the signature and therefore cannot rely on that history. Source-wide bare-name search finds only increments/writes at `ThrashGuard.cpp:112-113,151` and cloud exposure of the two counters at `cloud/Particle_Functions.cpp:52-53`; tiering uses non-retained `recentTripCount_`/timers at `ThrashGuard.cpp:115-130`, and alert/reset actions are already selected before `thrashResetCount++` at `:139-152`. Consequence is report-only, but the architect defined any such unvalidated move with a stable signature as STOP. A 32-byte, 4-aligned explicitly packed layout could start at the old `0x2003f41c` and end at `0x2003f43c` without adding the trio to scope, but it costs target-specific packing, explicit reserved/padding bytes and compile-time offset assertions; natural `time_t` alignment/portability would need deliberate handling. |
| 6 | PASS | Fresh sizes are HEAD **151396/1094/2212** and candidate **151468/1094/2212** (`text/data/bss`), exactly +72 text. `nm -C -S`: `setup` 0xbd0→0xc0c (+0x3c), `setAppBreadcrumb` 0x1c→0x20 (+4), `refreshRetainedLoopForensics` 0x98→0x9c (+4); the remaining 4 bytes are image alignment. Other differences are same-sized local `CSWTCH` renumberings. The nine old data symbols disappear and `retainedBoot` is the sole new symbol; validation is inlined. Source diff contains no logic change beyond the specified layout/validation design. |
| 7 | PASS | In isolated copies, `retained_boot_test.sh` failed when the magic check was removed, when the version check was removed, when `bootStormTripCount` reset to 1, and when garbage `bootInProgress` was preserved. `retained_boot_structural_test.py` failed when validation moved below `ensureRetainedLoopForensicsInitialized()`. The `serial_settle_test.py` diff adds exactly one optional first statement, requiring its exact spelling; the existing serial-helper placement and all later assertions remain. |
| 8 | CONCERN | Golden payload test passed and its fixture/script have no diff. Clean full suite: **83/83**; baseline inventory **81**. `git diff --check` passed. `src/` is +28/-9 = **+19 net**, within the hard +20. Tests are +227/-1 = **+226 net**: 216 lines in the three new files plus +11/-1 in `serial_settle_test.py`. The 227 additions are 2.27× the +100 guideline; per dispatch this is flagged but is not itself a failure. |

**Outcome from check 5: B-report-only.**

## Whole `.backup` symbol comparison

| Symbol(s) | HEAD | Candidate |
|---|---|---|
| `retainedLoopForensics` | `0x2003f400`, 0x1c | unchanged |
| Nine former globals | breadcrumb `0x41c`; breadcrumb ms `0x420`; count `0x424`; window `0x428`; last reason `0x430`; reason data `0x434`; in-progress `0x438`; trip `0x439`; alert-pending `0x43a` | replaced by `retainedBoot` at `0x2003f420`, 0x20 |
| `thrashTripCount` / `lastThrashReason` / `thrashResetCount` | `0x43c` / `0x43e` / `0x440` | **`0x440` / `0x442` / `0x444`** |
| `ledOffTime` | `0x448`, 0x8 | unchanged |
| `lastTrendSoc` / `lastTrendVcell` / `lastTrendEpoch` | `0x450` / `0x454` / `0x458` | unchanged |
| PMIC anomaly count / epoch / uptime / SoC / charge / source / VBUS | `0x460` / `0x468` / `0x470` / `0x474` / `0x478` / `0x479` / `0x47a` | unchanged |
| Hibernate RTC / request / count / pending | `0x480` / `0x488` / `0x48c` / `0x490` | unchanged |
| `__backup_sram_signature` | `0x494`, 0x4 | unchanged |

## Findings

- **HIGH — Observation:** `src/ThrashGuard.cpp:28-30` declares three retained values without magic, version, sentinel or validity guard; fresh `nm` evidence shows all three move while `__backup_sram_signature` does not. **Inference:** the first post-OTA values are one old counter transplanted into another plus two stale padding-derived values. This cannot select a tier, alert, sleep, or reset, but it corrupts field diagnostics and meets the architect's explicit STOP definition.
- **LOW — Observation:** tests add 227 lines against the +100 guideline. **Inference:** this is maintenance overhead rather than a product-correctness defect; the required mutations are all caught.
- **INFO — Observation:** the new retained block itself is correctly validated before every value read and its reset state cannot trigger boot-storm handling. **Inference:** after the layout STOP is repaired, the requested behavior is otherwise supported by source, mutation, suite and binary evidence.

## Budget versus actual

| Budget | Actual | Result |
|---|---:|:---:|
| `src/` net ≤ +20 hard; tests +100 guideline | `src/` **+28/-9 = +19 net**; tests **+227/-1 = +226 net** | **PASS** source; **CONCERN** tests |

Before writing this verdict, status plus binary diff plus untracked-file hashes exactly matched the starting snapshot (`25106a49…42ba0`). I removed only `build-tmp/wo20261017-001-stage7/` and `$TMPDIR` artifacts that I created; `build-tmp/` is empty. The implementation tree is unchanged; this verdict file is the sole authorized addition.

*Controller's note (Claude Code, 2026-10-17): Codex's `-o` summary overwrote this file at the end of its run; the text above is its full verdict, recovered from the run log. Claude Code's own reproduction (6.4.1 Boron, fresh builds, suite 83/83, `src/` +19) matched Copilot's and Codex's figures before Stage 7 was dispatched.*
