# VERIFIED

**Model:** gpt-5.6-sol · **Reasoning:** high  
**Test interpreter:** **68/68 (sh via zsh, py via python3)** — 37 shell, 31 Python

Round 2 closes the flag gap. For every enumerated path, a stale raw flag no longer changes the effective mode, published downgrade state, or failsafe eligibility. No new edge case or surviving mutation was found.

## Checks 1–16

| # | Result | Evidence |
|---|---|---|
| 1. Linkage | PASS | ELF contains `downgradeActive()` at `0xBEF74` and `effectiveConnectionMode()` at `0xBEF92`. Reporting, failsafe, and status call chains reach the configured mode and raw flag through the derived predicate. |
| 2. Local build | PASS | Fresh `BUILD_PATH_BASE`, Boron, Device OS 6.4.1: **150964 / 1090 / 2196** text/data/bss, versus round 1’s 150940/1090/2196: **+24 text**, data/bss unchanged. |
| 3. Tests | PASS | **68/68 (sh via zsh, py via python3)**. The suite was run before creating any repository-local source copy. |
| 4. Binary retirement | PASS | ELF `BatteryAuthority::commit` calls the tier and low-battery setters, never `set_connectionMode`; offsets are tier 138, raw flag 20, connection mode 129. |
| 5. Retirement | PASS | The only configured-mode writes are ConfigApply and factory defaults; no old getter remains. Downgrade, recovery, and operator override behavior pass. |
| 6. Alerts | PASS | No alert definition, severity, raise, or clear logic changed. Stale-flag failsafe suppression is deliberately removed without changing alert rules. |
| 7. Reader classification | PASS | All 26 effective-mode call sites and all configured-mode/storage readers were re-enumerated; none is misclassified or missed. |
| 8. Branch equivalence | PASS | Executed all **64** sensor/configured/flag/tier combinations: 64/64 matched the expected current behavior. The accepted legacy migration difference is confined to check 14. |
| 9. Flag gap | PASS | For overrides 0, 1, 2 and COUNTING, the predicate immediately becomes false. Every enumerated lifecycle path uses configured mode, publishes false, and is not failsafe-blocked by the stale flag. |
| 10. Persistence | PASS | Raw flag remains in `/usr/sysStatus.dat`; storage loads at setup before effective-mode readers. Restart behavior was rechecked through the real persistence facade and modeled restored fields. |
| 11. Old-write side effects | PASS | Downgrade mode effect, status, and one-time log remain; no setter listener was found. Config-hash/reapply churn is intentionally retired. |
| 12. Configuration hash | PASS | Hash input remains `get_configuredConnectionMode()` at [DeviceStatusPublisher.cpp:113](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/DeviceStatusPublisher.cpp:113); flag changes cannot affect it, while ledger mode changes do. |
| 13. Flag readers | PASS | Every raw and derived consumer is classified below. No unapproved behavioral raw reader remains. |
| 14. Migration | **ACCEPTED** | Per architect: legacy downgraded devices temporarily run KEEP_ALIVE after reconciliation until a disconnected battery check re-downgrades them. No migration code required. |
| 15. Tests/mutations | PASS | `testStaleFlagBeforeCommit` and structural additions are sound; all **10 unique targeted mutations** were caught by their intended checks. None survived. |
| 16. Budget | PASS | Round 2 **+1** net nonblank/non-comment `src/` line against +10; total WO **+4** against +35. Zero moved lines. |

## Check 9 path audit

Observed invariant: after configured mode becomes 0, 1, or 2—or sensor mode becomes COUNTING—[PowerManager.cpp:239](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/power/PowerManager.cpp:239) returns `downgradeActive() == false`, even while the persisted flag remains true.

Tier `SURVIVAL` may independently suppress hard failsafe actions; that is not an effect of the stale flag.

| Path before clearing `commit()` | Failsafe blocked by stale flag? | Status `lowBatteryMode` | Mode in use | Remaining raw-flag effect |
|---|---:|---:|---|---|
| Normal daytime sleep | No | false | Configured mode | Post-wake raw gate invokes `commit()`, which clears it |
| Night sleep | No | false | Configured mode | May persist through sleep, but is inert |
| Hibernate | No | false | Configured mode | Setup’s raw gate clears it after persistent storage loads |
| CONNECTED mode | No | false | CONNECTED/configured | May persist indefinitely without a commit, but is inert |
| DISCONNECTED mode | No | false | DISCONNECTED | Failsafe exits on effective DISCONNECTED; a later commit clears it |
| Firmware-update state | No | false | Configured mode | Update state already defers failsafe; stale flag adds nothing |
| Error loop | No | false | Configured mode | Error resolution reads effective mode, not the raw flag |
| Reset loop | No | false | Configured mode | Setup clears after load; pre-load boot-storm sleep cannot read persisted flag |

The three intended raw gates are:

- [Generalized-Core-Counter.cpp:1574](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1574)
- [State_Sleep.cpp:1614](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1614)
- [State_Sleep.cpp:1683](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1683)

Each only gates an evaluate/`commit()` sequence. For the specified stale states, that commit clears the raw flag. [SensorManager.cpp:816](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/sensors/SensorManager.cpp:816) can still print the raw value in a PMIC anomaly log, but it changes no control or persisted state.

## Reader inventories

Effective-mode consumers:

- Main: `2567`
- Connectivity diagnostic: `139, 254`
- State Connect: `442`
- State Error: `127`
- State Idle: `59, 112, 186, 210, 218, 289, 318`
- State Modes: `80, 143`
- State Report: `201, 231`
- State Sleep: `88, 373, 406, 869, 1627, 1638, 1719, 1757, 1776, 1793`

Configured-mode readers:

- Config comparison: [ConfigApply.cpp:449](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/ConfigApply.cpp:449)
- Configuration hash: [DeviceStatusPublisher.cpp:113](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/DeviceStatusPublisher.cpp:113)
- Battery transition owner: [BatteryAuthorityCommand.cpp:77](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/power/BatteryAuthorityCommand.cpp:77)
- Effective-mode owner: [PowerManager.cpp:241](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/power/PowerManager.cpp:241) and `:246`
- Storage implementation/forwarder: `MyPersistentData.cpp:324, 1050`

`lowBatteryMode` readers:

| Classification | Readers |
|---|---|
| Derived | `PowerManager.cpp:242`; downstream effective mode, failsafe at Main `:2664`, diagnostic `:186`, and status `:257` |
| Intended raw owner/gates | `BatteryAuthorityCommand.cpp:78,95`; Main `:1574`; State Sleep `:1614,1683` |
| Log only | `SensorManager.cpp:816` |
| Storage plumbing | `MyPersistentData.cpp:196–197,1176` |
| Unexpected | **None** |

## Check 15 mutations

All ten unique mutations were detected:

| Mutation | Targeted failure |
|---|---|
| Restore downgrade write to configured mode | Configured mode must remain 3 |
| Drop OCCUPANCY term | COUNTING stale flag must not alter effective mode |
| Restore ConfigApply raw-flag comparison | Unchanged ledger reapply must report no change |
| Sleep reader uses configured getter | Structural effective-reader inventory |
| Drop inactive-flag guard | Repeated conserving commits must not re-fire |
| Restore stale clear as `!= INTERMITTENT` | Override to 1 must clear at next commit |
| Drop configured-mode term | Modes 0/1/2 with stale flag must not be downgraded |
| Failsafe F reads raw flag | F structural assertion |
| Diagnostic D reads raw flag | D structural assertion |
| Status S reads raw flag | S structural assertion |

The behavior test links the real commit, persistence facade, and PowerManager. It extracts the ConfigApply block afresh, preventing a stale copied block from passing. The restart test models loaded persisted fields rather than performing a filesystem reboot; persistence layout and boot order were checked separately.

## Migration — accepted

Observation: a legacy state `(configured=1, raw flag=1)` reaches setup’s [BatteryAuthority commit](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/power/BatteryAuthorityCommand.cpp:90), which clears the flag before cloud configuration restores ledger value 3.

Inference: it then runs KEEP_ALIVE until a battery check while disconnected sets the downgrade again. Round 2 does not alter that timing. It does ensure that status and failsafe represent the derived state rather than treating the legacy raw flag alone as an active downgrade. Chip’s pre-rollout fleet check is the accepted mitigation.

## Findings

No new defect or edge-case findings.

The previously reported migration behavior remains **ACCEPTED** by the architect and is not a round-2 failure.

## Budget versus actual

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---:|---|---:|---|
| Round 1, including controller edit | +35 cap | — (not raised) | **+3**: 48 added, 45 removed | 68/68; six targeted mutations |
| Round 2 | +10 cap | — (not raised) | **+1**: 10 added, 9 removed | 68/68; 64-case matrix; ten unique mutations |
| WO total, `73f752e` → worktree | +35 cap | — (not raised) | **+4**: 52 added, 48 removed | 68/68; all ten mutations caught |

Moved lines: **0** in each scope using `git diff --color-moved=zebra`.

Replaced lines:

- Round 1: 29 reader sites plus four accessor-renaming lines.
- Round 2: three behavioral reader sites.
- WO total: 32 reader replacements plus four accessor-renaming lines.

## Restoration and cleanup

No worktree file was mutated for mutation testing; all mutants lived in temporary or scratch copies. Before cleanup, the complete worktree `src/` tree matched the pristine pre-mutation build copy byte-for-byte. The unchanged behavior and structural test copies also matched.

Only `build-tmp/wo20261007-002-stage7r2/` was deleted. `build-tmp/` and all pre-existing contents were preserved.

Final `git status --short`, identical to the start:

```text
 M docs/work-orders/WO-2026-10-07-002-config-downgrade.md
 M src/Generalized-Core-Counter.cpp
 M src/cloud/DeviceStatusPublisher.cpp
 M src/diagnostics/ConnectivityFailsafeTest.cpp
 M src/power/PowerManager.cpp
 M src/power/PowerManager.h
 M tests/connection_mode_downgrade_structural_test.py
 M tests/connection_mode_downgrade_test.cpp
 M tests/connection_mode_downgrade_test.sh
?? docs/work-orders/WO-2026-10-07-002-stage6-round2-copilot-dispatch.md
?? docs/work-orders/WO-2026-10-07-002-stage7-codex-dispatch.md
?? docs/work-orders/WO-2026-10-07-002-stage7-round2-codex-dispatch.md
?? docs/work-orders/WO-2026-10-07-002-stage7-verdict.md
```

Final `git diff --stat`, identical to the start:

```text
 .../WO-2026-10-07-002-config-downgrade.md          | 14 +++++++++
 src/Generalized-Core-Counter.cpp                   |  2 +-
 src/cloud/DeviceStatusPublisher.cpp                |  8 +++--
 src/diagnostics/ConnectivityFailsafeTest.cpp       |  2 +-
 src/power/PowerManager.cpp                         | 13 +++++----
 src/power/PowerManager.h                           | 14 +++++++++
 tests/connection_mode_downgrade_structural_test.py | 28 +++++++++++++++++-
 tests/connection_mode_downgrade_test.cpp           | 28 ++++++++++++++++++
 tests/connection_mode_downgrade_test.sh            | 34 ++++++++++++++++++++--
 9 files changed, 129 insertions(+), 14 deletions(-)
```

The staged diff remained empty. No device, network, AWS, commit, stash, reset, checkout, or push operation was performed.