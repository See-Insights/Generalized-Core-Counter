# VERIFIED WITH CONCERNS

**Model:** Codex `gpt-5.6-sol` (standard tier; user-confirmed one-line probe on 2026-10-19)

**Reasoning:** medium

**Test interpreters:** **87/87** — 51 shell tests via `zsh`; 36 bare Python tests via `python3` (sequential)

The helper move is byte-faithful, within the hard source budget, buildable, and retained-layout neutral. The concern is a confirmed pre-existing coverage gap: bypassing all AB1805 stale-alarm clearing passes both the 86-test pre-move suite and the 87-test moved-tree suite.

## Checks 1–7

| # | Result | Evidence |
|---|---|---|
| 1. Byte-identical moved bodies | **PASS** | Direct byte comparisons against main `768c3c0` matched: `ab1805WakeReasonNameLocal` plus its doc block (24 lines, SHA-256 `b1eae30d…a6fe1e`), `ab1805WakeReasonName` (3 lines, `17ba0208…578b`), `logTimeDiag` (75 lines, `f05a7b30…8c27b`), and the Boron guard plus all three hibernate helpers (89 lines, `ddbfb9ab…2544`). This includes `kRegStatus`, `kRegIntMask`, `kMaskAieTie`, `kMinHibernateSleepSec`, and `kMaxHibernateSleepSec`. No body/comment/constant change was found. |
| 2. Linkage, guards, declarations | **PASS** | `ab1805WakeReasonNameLocal` remains internal in the anonymous namespace at `src/time/ClockDiagnostics.cpp:4,14–29`; its wrapper remains external at `:31`. `logTimeDiag` remains external at `:35`, declared once at `src/state/State_Common.h:199`, and called by Idle and Sleep. The three Sleep helpers are external definitions inside the single Boron guard at `src/time/HibernateEligibility.cpp:5–93`, and declarations inside the same guard at `src/time/HibernateEligibility.h:3–7`. The two hibernate callers remain under the Boron guard at `src/state/State_Sleep.cpp:1033–1039`. Guard placement is correct by inspection for non-Boron builds. |
| 3. Retained layout | **PASS** | The complete 24-symbol `.backup` listing is identical before/after; `.backup` is `0x98` bytes in both. `retainedBoot` stays at `0x2003f420`; the hibernate fields stay at `0x2003f480`, `0x2003f488`, `0x2003f48c`, and `0x2003f490`; the signature stays at `0x2003f494`. `src/time/HibernateCycle.cpp` is byte-identical to `HEAD`. After comments and strings were stripped, none of the four new files contained `retained`. Four independent probes each failed with the applicable filename in the message. |
| 4. ARM text delta | **PASS** | Fresh Boron/Device OS 6.4.1 builds reproduced **151820/1094/2228 → 151764/1094/2228** (`text/data/bss`), exactly **−56/0/0**. The complete accounting is below. The static-initializer name/size set is identical; no new initializer stub and no new semantic function beyond the three newly out-of-line Sleep helpers exists. |
| 5. Tests re-pointed and falsified | **CONCERN** | The existing test predicates are unchanged except for the declared stronger `clock_owner_structural_test.py` checks. Unconditional TimeDiag logging failed `idle_timediag_on_change_test.sh`; a second moved definition failed the new structural test with `logTimeDiag is still defined in Generalized-Core-Counter.cpp`; each new-file `retained` probe failed with a named message. However, an early `return true;` at the start of `clearAb1805StaleAlarmInterrupts()` passed **86/86** at `HEAD` and **87/87** after the move. This is pre-existing, but real, missing coverage. |
| 6. Budget and hygiene | **PASS** | Independent rule: ignore blank lines; count every nonblank new scaffolding line not present in the byte-identical moved regions, including includes, pragmas, namespace braces, declarations, and header guard lines. Result: 19 in the four new files plus the one new `State_Sleep.cpp` include = **20**, exactly the hard limit. `git diff --check` is clean (there are no hits to waive). Main is **1596→1492** lines; Sleep is **1814→1725**. |
| 7. Golden and suite | **PASS** | `tests/report_payload_golden_test.sh` passed all 186 vectors and its built-in mutants. The fixture is unchanged at SHA-256 `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637`. Full suite: **87/87**, sequential, shell via `zsh` and Python via `python3`. |

**Retained outcome: A — nothing moved.**

## ARM symbol accounting

| Item | Before | After | Text delta |
|---|---:|---:|---:|
| `isDeviceOsAtLeast6400()` | inlined | 104 | +104 |
| `clearAb1805StaleAlarmInterrupts()` | inlined | 96 | +96 |
| `shouldUseBoronRtcAlarmHibernate(unsigned long, long long&)` | inlined | 184 | +184 |
| `handleSleepingState()` | 7340 | 7048 | −292 |
| `logTimeDiag(bool)` | 464 | 504 | +40 |
| `setup` | 3096 | 3136 | +40 |
| `LocalTimePosixTimezone::operator=(...) [clone .isra.0]` 220-byte clone | 220 | absent | −220 |
| Named-code subtotal |  |  | **−48** |
| Alignment / unnamed gaps in `.text` | 31,259 | 31,251 | **−8** |
| Total `.text` | 151,708 | 151,652 | **−56** |

The three out-of-line hibernate helpers add 384 bytes while their former inlining site shrinks by 292. The translation-unit split changes optimizer choices: `logTimeDiag` and `setup` each grow by 40, while a 220-byte `LocalTimePosixTimezone::operator=` clone disappears. Those named changes total −48; the remaining −8 is section gaps/alignment.

The full `CSWTCH.*` size multiset is identical before/after (27 tables, 620 bytes); only compiler-local suffixes change. The four `__tcf_1` sizes are also identical as a multiset: `{2, 2, 24, 40}`. Thus the supplied raw `__tcf_1` “24→2” pairing is a same-name/address-order artifact, not a size change. All 22 `_GLOBAL__sub_I*` names and sizes are identical, so no static-initialization stub was introduced.

## Complete `.backup` comparison

Every address and size below is identical in the before and after ELF.

| Symbol | Before | After |
|---|---:|---:|
| `link_global_retained_start` | `0x2003f400` | `0x2003f400` |
| `platform_system_flags_ram_end` | `0x2003f400` | `0x2003f400` |
| `platform_user_backup_ram_start` | `0x2003f400` | `0x2003f400` |
| `retainedLoopForensics` | `0x2003f400`, `0x1c` | `0x2003f400`, `0x1c` |
| `retainedBoot` | `0x2003f420`, `0x20` | `0x2003f420`, `0x20` |
| `thrashTripCount` | `0x2003f440`, `0x02` | `0x2003f440`, `0x02` |
| `lastThrashReason` | `0x2003f442`, `0x01` | `0x2003f442`, `0x01` |
| `thrashResetCount` | `0x2003f444`, `0x02` | `0x2003f444`, `0x02` |
| `ledOffTime` | `0x2003f448`, `0x08` | `0x2003f448`, `0x08` |
| `(anonymous namespace)::lastTrendSoc` | `0x2003f450`, `0x04` | `0x2003f450`, `0x04` |
| `(anonymous namespace)::lastTrendVcell` | `0x2003f454`, `0x04` | `0x2003f454`, `0x04` |
| `(anonymous namespace)::lastTrendEpoch` | `0x2003f458`, `0x08` | `0x2003f458`, `0x08` |
| `pmicAnomalyCount` | `0x2003f460`, `0x02` | `0x2003f460`, `0x02` |
| `lastPmicAnomalyEpoch` | `0x2003f468`, `0x08` | `0x2003f468`, `0x08` |
| `lastPmicAnomalyUptimeMs` | `0x2003f470`, `0x04` | `0x2003f470`, `0x04` |
| `lastPmicAnomalySoc` | `0x2003f474`, `0x04` | `0x2003f474`, `0x04` |
| `lastPmicAnomalyChargeStatus` | `0x2003f478`, `0x01` | `0x2003f478`, `0x01` |
| `lastPmicAnomalyPowerSource` | `0x2003f479`, `0x01` | `0x2003f479`, `0x01` |
| `lastPmicAnomalyVbusStatus` | `0x2003f47a`, `0x01` | `0x2003f47a`, `0x01` |
| `HibernateCycle::…::retainedHibernateRtcBefore` | `0x2003f480`, `0x08` | `0x2003f480`, `0x08` |
| `HibernateCycle::…::retainedHibernateRequestedSleep` | `0x2003f488`, `0x04` | `0x2003f488`, `0x04` |
| `HibernateCycle::…::retainedHibernateCount` | `0x2003f48c`, `0x04` | `0x2003f48c`, `0x04` |
| `HibernateCycle::…::retainedHibernatePending` | `0x2003f490`, `0x01` | `0x2003f490`, `0x01` |
| `__backup_sram_signature` | `0x2003f494`, `0x04` | `0x2003f494`, `0x04` |

## Test-diff review

| Test | Assessment |
|---|---|
| `clock_hibernate_helpers_move_structural_test.py` | New 100-line ownership/guard/declaration/no-retained test. Its moved-definition and all four no-retained checks were independently falsified. |
| `clock_owner_structural_test.py` | Reads the moved `Clock::isPlausibleEpoch` caller from `HibernateEligibility.cpp`, additionally forbids it in `State_Sleep.cpp`, and scans main plus `ClockDiagnostics.cpp` for the existing telemetry-only `Time.isValid()` allowlist. This is strictly stronger; no old predicate was dropped. |
| `clock_resync_wiring_test.py` | Extracts the same `logTimeDiag` body from `ClockDiagnostics.cpp`; assertions are unchanged. The extra filename argument only improves extraction diagnostics. |
| `rtc_skew_test.cpp` | Path comments only; executable assertions unchanged. |
| `watchdog_ab1805_classification_test.cpp` | Path comment only; executable assertions unchanged. |

The on-change condition is at `src/state/State_Idle.cpp:132`, outside `logTimeDiag`; changing it to unconditional caused six named failures in `idle_timediag_on_change_test.sh`. The move does not touch that condition.

## Findings

1. **MEDIUM — pre-existing coverage gap (`src/time/HibernateEligibility.cpp:24–51`; formerly `src/state/State_Sleep.cpp:239–267`).**
   - **Observation:** inserting `return true;` at the start of `clearAb1805StaleAlarmInterrupts()` bypasses the status clear, interrupt-mask clear, and timer-repeat disable. The complete `HEAD` suite still passes **86/86**, and the moved-tree suite passes **87/87**.
   - **Inference:** this move did not introduce the gap, and byte identity protects this patch, but a future regression that leaves stale AB1805 alarm state uncleared will not be detected by the host suite. It should remain on the recovery plan.

2. **LOW — work-order mutation wording (`src/state/State_Idle.cpp:126–135`).**
   - **Observation:** `logTimeDiag()` contains no on-change suppression. The flood prevention is its caller's `enteredIdle || timeDiagKey != lastTimeDiagKey` condition at line 132. Mutating that condition is caught by `idle_timediag_on_change_test.sh`.
   - **Inference:** coverage is present, but future instructions should name the caller condition rather than ask to make `logTimeDiag` itself log on every call.

No implementation, linkage, retained-layout, golden-payload, or build defect was found.

## Budget versus actual

| Budget | Actual | Result |
|---|---|---|
| `src/` hard limit: ≤ +20 nonblank non-move lines | **+20** by the rule stated in check 6. Physical diff: +213/−194, net +19; WO-defined moved function spans: 178 lines (the byte comparison additionally covered the 9-line doc block and Boron guard). Main 1596→1492; Sleep 1814→1725. Tests: +116/−8, net +108 versus the +100 guideline. | **PASS** (source hard limit met exactly; test guideline exceeded by 8 net lines) |

## Confirmations

- The reviewed implementation/test diff matched the start-of-review binary diff snapshot exactly (SHA-256 `e3e1c4f8dab7b7a6f875b08f2285e16e3a6155a34428bad24e5b116c16de8fb5`) before this verdict was added.
- Only `build-tmp/wo20261019-003-stage7/` and one `$TMPDIR` per-test log were created for review; the log was removed during its suite run. The Stage 7 scratch directory was removed after evidence collection.
- No commit, push, stash, reset, checkout, restore, device, network, or AWS operation was performed.
