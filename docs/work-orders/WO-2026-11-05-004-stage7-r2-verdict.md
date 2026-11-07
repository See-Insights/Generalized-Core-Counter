UNDER REVIEW: wo/2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake @ edcebbcd75513f928467ce4aa8009918fe8d4ed1

NOT VERIFIED for S2 as scoped — Round 2 of 2.

`git rev-parse HEAD` matched the full required SHA. The tracked worktree remained untouched.

| # | Verdict | Evidence and conclusion |
|---|---|---|
| 1 | PASS | `src/time/HibernateWakeDiagnostics.h:167-173` defines `kPowerOffSentinel = 0xA5` and compares exactly against it. Tests cover 0, 1, legacy 2, adjacent and arbitrary values at `tests/watchdog_inferred_classification_test.cpp:34-39`; nonzero and legacy-2 mutations were caught. |
| 2 | PASS | Boot consumption is immediately forced at `Generalized-Core-Counter.cpp:761-762`. Returning Error and failsafe calls clear and flush at `State_Error.cpp:179-180` and `ConnectivityFailsafe.cpp:316-317`. Both `deepPowerDown()` calls themselves are unchanged. Stage 2 removes stage-3 evidence while preserving the next action as a stage-3 retry after cooldown. There is no intervening app work between clear and flush; interruption before the synchronous flush completes can inherently retain the prior durable value, but no additional software window remains. |
| 3 | FAIL | Production correctly defines reason 30 as a successful HIBERNATE wake at `HibernateCycle.cpp:63-65`; no timing arm remains, the 1,800-second case is restated, and GPIO wake coverage records the accepted residual at `hibernate_wake_diagnostics_test.cpp:84-102`. However, four reason-30 tests still use the retired `"ALARM"` label at `:119`, `:136`, `:244`, and `:374`, and no fidelity check pins `"POWER_MGMT"`. A private `POWER_MGMT`→`ALARM` production mutation survived. |
| 4 | PASS | `HibernateWakeDiagnostics.h:169-173` requires `timeValid` before stage-3 recency is accepted. Both invalid-time variants and the mutation accepting invalid time are covered. |
| 5 | PASS | `Generalized-Core-Counter.cpp:1293-1295` restores `"ab1805WakeReason":"N/A"` while retaining `watchdogSource` values `"inferred"`/`"NONE"`. The fragment is at most 53 bytes in an 80-byte buffer. Extreme-width modeling of the PMIC-enabled status payload produced 874 bytes versus 1,023 usable, so the truncation path is not reached. Persisted watchdog-source values 0/1 remain unchanged; inferred remains 2. |
| 6 | PASS | Both normalizers modify only the base side, require exact match counts, and never rewrite current code/transcripts: `s2_hibernate_normalise.py:8-48`, `s2_transcript_normalise.py:14-63`. Direct normalization against `3b5cfb8` matched byte-for-byte. The fixture is regenerated from base and checked by hash. The 137→121 reduction is exactly 12 cellular plus 4 Wi-Fi cases from the two retired stale-clear/alarm-program-failure families. GPIO, legacy 2, forced consumption, and returning-power-down families are present. |
| 7 | PASS | Alarm configuration, watchdog coverage and fallbacks remain correct at `State_Sleep.cpp:689-915`. ELF inspection found the two linked power-down callers and no retired alarm/wake/watchdog symbols; only the library-internal `AB1805::setWDT` remains through `deepPowerDown()`. S1/S1b helpers are unchanged. The persisted-layout test passed, `MyPersistentData.h` is unchanged, and retained declarations match the base. |
| 8 | FAIL | Functional verification passed: net `src/` is **139 additions / 272 deletions = −133**; host suite **118/118**; clean thermal export passed; build and mutations passed as detailed below. However, `tests/__pycache__/s2_hibernate_normalise.cpython-314.pyc` is tracked. Its timestamp header is stale on checkout, and an ordinary import rewrites it, dirtying a clean test tree. |
| 9 | FAIL | No additional runtime defect was found, but boot/watchdog comments still describe the retired chip-confirmed attribution at `Generalized-Core-Counter.cpp:532-535`, `:1374-1377`, and `MyPersistentData.cpp:792`. They now contradict the inferred classification used by the code. Physical behavior remains a bench item. |

### Findings

1. **Medium — reason-30 telemetry label is not regression-protected**  
   File: `tests/hibernate_wake_diagnostics_test.cpp:119,136,244,374`

   What breaks: a regression from `POWER_MGMT` back to the misleading `ALARM` label passes the relevant host test, contrary to Amendment 4’s “successful HIBERNATE wake” ruling.

   Smallest fix: replace those four reason-30 labels with `POWER_MGMT`, assert the emitted wake-reason field, and add a fidelity/mutation check for the production assignment. Test-only; no `src/` budget impact.

2. **Low — compiled Python cache is tracked**  
   File: `tests/__pycache__/s2_hibernate_normalise.cpython-314.pyc` (binary)

   What breaks: a normal Python import rewrites the tracked file, leaving an otherwise clean checkout dirty and introducing avoidable stale-bytecode risk.

   Smallest fix: remove the tracked `.pyc` and ignore `__pycache__/`/`*.pyc`. No `src/` budget impact.

3. **Low — obsolete watchdog-attribution comments remain**  
   Files: `src/Generalized-Core-Counter.cpp:532-535,1374-1377`; `src/MyPersistentData.cpp:792`

   What breaks: maintainers and forensic readers are told the chip wake register confirmed a reset, although S2 now infers it from missing persisted power-off evidence.

   Smallest fix: update the comments to say “inferred PIN_RESET”; neutral or negligible line impact and comfortably within `+20`.

### Verification record

- Model: **gpt-5.6-sol**
- Reasoning: **xhigh**
- Interpreters: **zsh 5.9** (`arm64-apple-darwin26.0`); **Python 3.14.6**; Apple clang **14.0.3**
- Target: Device OS **6.6.0**, Boron, GNU Arm **10.2.1**
- Build: **151508 text / 1114 data / 2240 bss**  
  Copilot r3: 151508 / 1114 / 2240; round 1: 151468 / 1114 / 2240.
- Build warnings: five, all outside S2-touched lines.
- Host suite: **118/118** — 66 zsh tests and 52 Python tests.
- Copilot mutations caught included sentinel-nonzero, legacy-2 sentinel, invalid-time acceptance, removed recency, ignored evidence byte, and broadened stages.
- Two new mutations caught: HIBERNATE `WAKEUP_PIN` edge `FALLING`→`RISING`; watchdog refresh moved after `checkClockResync()`.
- One diagnostic mutation survived: `POWER_MGMT`→`ALARM`, producing Finding 1.
- Scratch use was confined to `build-tmp/wo-2026-11-05-004-stage7-r2/`.

No hardware was run. The two real 8-hour nights, the physical watchdog proof, and the post-`POWER_OFF` reset reason remain Chip’s bench items.