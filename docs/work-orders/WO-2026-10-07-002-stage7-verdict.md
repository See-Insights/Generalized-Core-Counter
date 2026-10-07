**NOT VERIFIED — gpt-6-astra, high reasoning**  
**68/68 (sh via zsh, py via python3)**

Two lifecycle failures require round 2: an operator override can carry the flag across sleep, and startup clears the legacy migration flag before ledger reconciliation.

**Observed scope:** the branch matched, but HEAD was already `0e94f81`, not `73f752e`. There was no tracked uncommitted diff. I reviewed `73f752e` → current worktree, including the controller edit.

“Observed” below means source, executed host harness, or binary evidence. Device-path consequences are identified as control-flow inferences; no device or network access occurred.

| # | Result | Evidence |
|---|---|---|
| 1. Linkage | PASS | 26 external effective-mode call sites; ELF symbol at `0xBEF68`; reporting-state calls confirmed in disassembly. |
| 2. Local build | PASS | Fresh Boron/Device OS 6.4.1 build: **150940 / 1090 / 2196** text/data/bss. |
| 3. Tests | PASS | **68/68 (sh via zsh, py via python3)**: 37 shell and 31 Python tests. |
| 4. Binary retirement | PASS | Commit writes battery fields at offsets 20 and 138, not connection-mode offset 129; no connection-mode setter call. |
| 5. Retirement requirements | CONCERN | Writers/API retired correctly; ordinary downgrade/recovery pass, but lifecycle requirements fail in checks 9 and 14. |
| 6. Alerts | PASS | No alert definition, ranking, raise or clear rule changed; the Error-state CONNECTED/non-CONNECTED distinction is preserved. |
| 7. Reader classification | PASS | All callers listed below match the prescribed split; no missed raw-mode reader found. |
| 8. Branch equivalence | FAIL | Exhaustive execution confirms the table below; legacy `(1,1)` has an unintended pre-apply flag/recovery difference. |
| 9. Flag-clearing bound | FAIL | Connected config apply can reach `System.sleep()` before any commit; CONNECTED operation also permits an indefinite gap. |
| 10. Persistence | PASS | Flag remains in `sysStatus.dat`; layout unchanged; storage loads before effective-mode readers execute. |
| 11. Old-write side effects | PASS | Normal downgrade triggers/logs preserved; no setter-driven disconnect, publish or mode-change listener found. |
| 12. Configuration hash | PASS | Executed real hash code: downgrade/reapply/recovery unchanged; ledger mode change changes the hash. |
| 13. Flag readers | CONCERN | All ten consumers accounted for; failsafe suppression and stale status extend through the failed check-9 gap. |
| 14. Migration | FAIL | First apply writes ledger 3, but setup’s preceding commit has already cleared the legacy flag. |
| 15. Tests/mutations | CONCERN | All six mutations caught by their intended checks; existing tests omit the failing lifecycle sequences. |
| 16. Budget | PASS | **+3 net code lines**, 0 moved; 29 reader replacements plus 4 accessor rename lines. |

**Check 8 — executed case table**

I executed all **64 combinations against each commit implementation**, testing CRITICAL and SURVIVAL separately. The old implementation came from `73f752e`; only its getter spelling was adapted for linking.

Here, `m` is the input **old stored mode / new configured mode**. That distinction matters: an ordinary existing downgrade is represented by old stored mode 1 versus new configured mode 3.

Results are `(flag after commit, mode in use after commit, logs)`.

- **down:** one “Battery conservation: Disabling KEEP_ALIVE…” line.
- **clear:** one “Battery recovery: clearing lowBatteryMode…” line.
- **recover:** clearing plus “restoring INTERMITTENT_KEEP_ALIVE”.
- **—:** no downgrade/recovery log.
- The common tier-transition log remains unchanged when the persisted tier changes.

| Sensor | m | Initial flag | Tier | Old result | New result |
|---|---:|---:|---|---|---|
| COUNTING | 0–3 | 0 | All | `(0,m,—)` | `(0,m,—)` |
| COUNTING | 0–3 | 1 | All | `(0,m,clear)` | `(0,m,clear)` |
| OCCUPANCY | 0,2 | 0 | All | `(0,m,—)` | `(0,m,—)` |
| OCCUPANCY | 0,2 | 1 | All | `(0,m,clear)` | `(0,m,clear)` |
| OCCUPANCY | 1 | 0 | All | `(0,1,—)` | `(0,1,—)` |
| OCCUPANCY | 1 | 1 | HEALTHY | `(0,3,recover)` | `(0,1,clear)` |
| OCCUPANCY | 1 | 1 | CONSERVING or worse | `(1,1,—)` | `(0,1,clear)` |
| OCCUPANCY | 3 | 0 | HEALTHY | `(0,3,—)` | `(0,3,—)` |
| OCCUPANCY | 3 | 0 | CONSERVING or worse | `(1,1,down)` | `(1,1,down)` |
| OCCUPANCY | 3 | 1 | HEALTHY | `(0,3,clear)` | `(0,3,recover)` |
| OCCUPANCY | 3 | 1 | CONSERVING or worse | `(1,1,down)` | `(1,1,—)` |

**Observed:** the new configured value never changes during these commits. The old code writes 1 when downgrading and 3 when recovering.

The ordinary representation change and suppression of repeated downgrade logs are intended. Comparing an already-downgraded old `(stored=1, flag=1)` with new `(configured=3, flag=1)` gives equivalent healthy recovery and continued conservation. The legacy migration state, however, enters the new implementation as `(configured=1, flag=1)` and exposes the failure in check 14.

**Chip’s operator-override case:** I executed the real old and new ConfigApply blocks, beginning with old `(stored=1, flag=1)` and new `(configured=3, flag=1)`.

| Operator ledger value L | Old immediately after apply | New immediately after apply | Both after next commit |
|---|---|---|---|
| 0, 1 or 2 | `(flag=0, in-use=L)` | `(flag=1, in-use=L)` | `(flag=0, in-use=L)` |

The end state is identical **if the next commit occurs**. Old apply clears silently and logs the configuration change. New apply logs the configuration change; the subsequent commit emits the generic clearing log. For L=1, the effective mode remains INTERMITTENT throughout.

**Check 9 — no guaranteed bound within one wake cycle**

The complete production commit inventory is:

| Call site | Gate |
|---|---|
| `Generalized-Core-Counter.cpp:1585` | Setup, only when the flag is set |
| `State_Report.cpp:179` | Configuration valid, no preceding early return, and `!Particle.connected()` |
| `State_Sleep.cpp:1620` | After waking, within open hours, flag set |
| `State_Sleep.cpp:1689` | After an OCCUPANCY PIR wake, flag set |

**Observed source path:** config apply changes the mode while connected → connect completion returns directly to SLEEPING at `State_Connect.cpp:625`, or IDLE selects sleep at `State_Idle.cpp:250` → sleep teardown → `State_Sleep.cpp:1409` calls `System.sleep()`. No commit occurs between the apply and sleep.

**Control-flow inference:** the flag crosses that sleep. This is a **FAIL under Chip’s explicit rule**, even when the post-wake commit subsequently clears it.

| Path | Timing/bound assessment |
|---|---|
| Normal daytime sleep | Clearing can wait until after the scheduled/debounce sleep. Timer alignment can request reporting interval + 1 second before the post-wake commit. |
| Night sleep / hibernate | Sleep can precede commit; night requests are capped at **32,760 seconds**. Hibernate relies on a later successful setup to clear. |
| CONNECTED mode | Connected reports skip commit. With continuous open hours and connection, no finite clearing bound is established. Closed-hours operation can instead enter night sleep with the flag set. |
| DISCONNECTED mode | The ordinary low-power sleep path still carries the flag. The failsafe separately exits early for DISCONNECTED mode. |
| Firmware-update state | Applies configuration at `State_Connect.cpp:718`, but has no commit before its sleep exits at `:734`/`:752`; ongoing progress can prolong the dwell. |
| Error / reset loops | Error resolution 0 returns through IDLE to sleep without commit. Successful setup eventually clears, but early reset loops do not guarantee reaching it. Boot-storm holdoff sleeps **600 seconds at Main:884**, before storage load and setup’s commit. |

During eligible battery-powered hard-stage evaluations, the stale flag continues suppressing the failsafe at `Main:2664` and `ConnectivityFailsafeTest.cpp:186`.

**Findings**

1. **HIGH — flag reconciliation is not bounded before sleep. Round 2: required.**  
   Locations: [ConfigApply.cpp:449](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/ConfigApply.cpp:449), [State_Report.cpp:163](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Report.cpp:163), [State_Sleep.cpp:1409](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1409).  
   **Observed:** apply leaves the flag set; available commits are conditional or post-wake. **Inference:** operator override can leave failsafe suppression and status indicating an active downgrade across sleep or indefinitely while connected. Round 2 needs lifecycle coverage for these paths.

2. **HIGH — startup discards the legacy migration flag before reconciliation. Round 2: required.**  
   Locations: [BatteryAuthorityCommand.cpp:90](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/power/BatteryAuthorityCommand.cpp:90), [Generalized-Core-Counter.cpp:1585](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1585).  
   **Observed in executed harnesses:**

   | Legacy boot state `(stored=1, flag=1)` | Old after setup-equivalent commit | New after setup-equivalent commit |
   |---|---|---|
   | CONSERVING | `(flag=1, in-use=1)` | `(flag=0, in-use=1)` |
   | HEALTHY | `(flag=0, in-use=3)` | `(flag=0, in-use=1)` |

   First config apply does write 3, but the new flag is already false, yielding effective mode 3 even at CONSERVING. **Inference:** before reconciliation, the device also loses the old flag-based failsafe protection at CONSERVING/CRITICAL. The existing migration test applies configuration without first executing setup’s commit, so it misses this sequence.

**Reader classification — independent source inventory**

All line numbers below are current worktree locations.

| File | Effective-mode callers and purpose |
|---|---|
| `Generalized-Core-Counter.cpp` | `2567`: DISCONNECTED failsafe gate |
| `diagnostics/ConnectivityFailsafeTest.cpp` | `139`: gate; `254`: mode log |
| `state/State_Connect.cpp` | `442`: standby diagnostic |
| `state/State_Error.cpp` | `127`: reset-loop policy |
| `state/State_Idle.cpp` | `59,186`: reporting; `112`: park-hours behavior; `210,218`: sleep policy; `289`: awake ceiling; `318`: log |
| `state/State_Modes.cpp` | `80,143`: occupancy reporting cadence |
| `state/State_Report.cpp` | `201`: sleep-after-report decision; `231`: KEEP_ALIVE connection |
| `state/State_Sleep.cpp` | `88,869`: labels/logs; `373`: standby; `406`: sleep veto; `1627`: reconnect; `1638,1719,1757,1776`: reporting cadence; `1793`: return to sleep |

| Configured-mode caller | Purpose |
|---|---|
| `cloud/ConfigApply.cpp:449` | Compare ledger configuration |
| `cloud/DeviceStatusPublisher.cpp:113` | Configuration hash |
| `power/BatteryAuthorityCommand.cpp:77` | Architect-authorized configured-mode input to flag transitions |
| `power/PowerManager.cpp:240` | Single effective-mode derivation |
| `MyPersistentData.cpp:1050` | Facade forwarding to the renamed storage getter |

The underlying storage getter is at `MyPersistentData.cpp:324`; its declarations are not additional callers. No old getter, raw-field reader elsewhere, macro alias, or cached raw-mode consumer was found.

**Binary and retirement evidence**

The §3 build used Boron, Device OS 6.4.1, buildtools 1.1.1 and GCC ARM 10.2.1. `BUILD_PATH_BASE=build-tmp/wo20261007-002-stage7/fresh-arm-build` was verified absent before make. Build exit status was 0.

Observed ELF chain:

```text
handleReportingState(): 0xC238A
  → PowerManager::effectiveConnectionMode(): 0xBEF68
    → SystemConfig::get_configuredConnectionMode(): 0xB7C34
    → SystemConfig::get_sensorMode()
    → PowerConfig::get_lowBatteryMode(): 0xB7D9C
```

`BatteryAuthority::commit` is at `0xBDB5C`. Its storage-writing calls target:

- `set_currentBatteryTier`: offset **138**.
- `set_lowBatteryMode`: offset **20**.

Connection mode uses offset **129**. Commit’s direct stores are stack stores; its battery setters do not target 129. ELF connection-mode setter callers are only the facade, defaults initialization, and ConfigApply.

Both `get_connectionMode` and `clearLowBatteryMode` have **zero ELF symbols**. The controller’s declaration/definition/include removal is consistent with source and binary retirement.

**Persistence, side effects, hash and migration application**

- **Observed persistence:** `lowBatteryMode` remains in unchanged `SysData`, backed by `/usr/sysStatus.dat`. Main loads it at `:981`, before effective-mode readers run. The host getter returns INTERMITTENT for restored configured 3 plus flag 1 before commit. Actual filesystem reboot behavior was inspected in source, not exercised on hardware; ordinary deferred-save limitations remain unchanged.
- **Observed old setter behavior:** it changed storage, its integrity checksum and save scheduling. It did not directly disconnect, transition state, publish, or notify a mode-change listener. Normal runtime decisions now consume the equivalent effective mode. Downgrade logging fires once, verified by repeated commits.
- **Observed status behavior:** status payloads still include `battery.lowBatteryMode`, and duplicate suppression compares the full payload. No configuration-hash-change listener was found. The artificial config-reapply change/log/status request disappears with the intended flip-flop removal; genuine ledger changes still request status publication.
- **Observed hash execution:** baseline, downgrade, unchanged reapply and recovery all produced `8A5B4CD8`; changing ledger mode to 1 produced `618864BE`.
- **Observed application wiring:** every normal CONNECTING_STATE success invokes configuration loading at `State_Connect.cpp:557`, reaching apply at `Cloud.cpp:637`. Ledger callbacks also schedule deferred apply. Firmware-update state loads once per entry at `:718`. This establishes the normal connection path; it does not establish unconditional reapplication on every background transport reconnect.

**Every `lowBatteryMode` reader**

| Consumer | Lines | Use |
|---|---|---|
| Main | `1574,2664` | Setup reconciliation gate; failsafe suppression |
| `BatteryAuthorityCommand.cpp` | `78,95` | Sticky transition decision; non-OCCUPANCY clearing |
| `PowerManager.cpp` | `244` | Effective-mode derivation |
| `State_Sleep.cpp` | `1614,1683` | Post-wake reconciliation gates |
| `ConnectivityFailsafeTest.cpp` | `186` | Failsafe eligibility diagnostic |
| `DeviceStatusPublisher.cpp` | `255` | Status-ledger flag |
| `SensorManager.cpp` | `816` | Battery diagnostic log |

`MyPersistentData.cpp:1176` forwards to the storage reader at `:196–197`. No additional dependency on ConfigApply clearing was found beyond the lifecycle consequences described above.

**Tests and mutations**

The behavior test links the real commit, PowerManager and persistence implementation. ConfigApply’s connection-mode block is extracted afresh from production source and compiled. It cannot silently retain an older copy of that matched block, but surrounding application control flow and startup ordering are outside its coverage. The restart test models restored fields; it does not reload a real filesystem.

| Mutation | Intended check observed failing |
|---|---|
| Restore downgrade’s mode write | “downgrade leaves the configured mode at 3” |
| Drop OCCUPANCY derivation term | “COUNTING: a stale flag does not change the mode in use” |
| Restore ConfigApply flag comparison | “re-apply of an unchanged ledger value reports no change” |
| Sleep `:1638` uses configured getter | Configured-reader rejection and “expected 10 mode-in-use reads, found 9” |
| Drop `!lowBatteryDowngradeActive` | “later commits at CONSERVING or worse do not re-fire” |
| Restore stale-clear `!= INTERMITTENT` | “the flag clears at the next commit” |

**No mutation survived.** All five behavioral mutants compiled and failed at runtime. The reader mutation failed its intended structural check. Restored behavior and structural checks passed.

Existing test edits did not remove or weaken assertions: two pinned getter strings changed, the occupancy test gained a PowerManager adapter, and shared stubs gained required accessors. The persisted-layout test remained unchanged and passed.

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| WO-2026-10-07-002, including controller edit | +35 cap | Not raised | **+3**: 48 added, 45 removed | **68/68**, six targeted mutations caught |

Moved lines: **0** using `git diff --color-moved=zebra`. Replaced lines: **29 reader sites + 4 accessor renames**.

**Restoration and cleanup confirmed**

All four mutated source files were rewritten in place and restored byte-identically. SHA-256 verification matched **all 1,387 tracked files** to the starting snapshot.

```text
git diff --stat
Start:  empty
Finish: empty

git diff 73f752e --stat
Start and finish: 41 files changed, 895 insertions(+), 72 deletions(-)

src-only comparison
Start and finish: 18 files changed, 74 insertions(+), 67 deletions(-)
```

The pre-existing untracked Stage 7 dispatch remained unchanged in status. I deleted only **`build-tmp/wo20261007-002-stage7/`** and its contents. `build-tmp/` and `build-tmp/connectivity-archive/` remain.