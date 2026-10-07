**Overall: NOT VERIFIED.** B has one blocking defect: a post-boot report can invalidate the promised one-debounce over-credit bound. All other checks passed.

**Blocking finding — B:** [State_Common.h:341](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Common.h:341) reads `lastReport` at close time. Scheduled reporting is allowed before clock trust returns, and [State_Report.cpp:82](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Report.cpp:82) advances that timestamp.

The production-close host reproducer used a five-minute debounce:

- Session starts at relative epoch 1,000; last report and power-off at 5,000.
- Device boots after ten hours, at 41,000.
- Without a post-boot report: **4,300 seconds credited**, over-credit **300 seconds**.
- With a report at 41,050 before the trusted close: **40,000 seconds credited**, over-credit **36,000 seconds**.

This is ordinary reporting within the approved fault model. The existing restart test misses this sequence.

| Check | Result | Evidence |
|---|---|---|
| **1. A1** | **PASS — VERIFIED** | Exactly one signal-sampling call remains, inside connected/post-connect processing. `Connect: ok` reuses it; acquisition and timeout paths contain none. The other cellular RSSI call, `SensorManager::getSignalStrength()`, has no `src/` caller and no linked ELF symbol. All five restored-read mutations failed the structural test, including the removed WiFi duplicate. |
| **2. A2** | **PASS — VERIFIED** | The three decisions require `Cellular.isOff()` on non-standby cellular paths, blocking `isOn=false, isOff=false`. Hibernate, ULP and STOP fallbacks follow the gate. Existing budgets and alert-15/error exits remain; standby is unchanged. No new timer, state or breadcrumb. The host test compiles extracted production expressions; timeout wiring is checked structurally and by source review. Reverting each site failed the test. |
| **3. B** | **FAIL — NOT VERIFIED** | The moving-anchor defect defeats the long-outage bound. Other checks passed: boot clears persisted `millis()`; all three caller probes preserve untrusted sessions, credit zero, re-arm debounce, and produce no close-triggered reporting transition or `OccAnom`. `SessionState` is ordinary RAM, confirmed by source and ELF `.bss`; persistence is unchanged. Daily close preserves the explicit boundary ceiling and trusted-only daily gate, but shares the anchor defect. |
| **4. C** | **PASS — VERIFIED** | Compiled production `classifyWake()` plus event formatting passed 28 label/timing cases. DPD at 0, +30 and +60 seconds succeeds with real timing; early, +61, WATCHDOG and UNKNOWN fail. ALARM behavior remains unchanged. Four mutations tightening or widening either bound failed. |
| **5. D** | **PASS — VERIFIED** | Both formats emit numeric `vc`; invalid/unavailable samples leave `0.00`. The cache’s plausibility check excludes NaN and infinities. `key1` remains the only string-valued field. Conservative payload sizes fit, below. |
| **6. Existing test intent** | **PASS — VERIFIED** | Hibernate tests retain the previous gate/event assertions and add the approved DPD exception with source-fidelity pins. Payload tests retain existing field, numeric-type, buffer and provenance assertions while adding `vc`. |
| **7. Suite/build** | **PASS — VERIFIED** | **65/65:** 35 shell tests through **zsh**, 30 Python tests through **python3**. WITH_ACK, sleep ownership and ledger tests are unchanged against v36 and green. Clean local ARM build succeeded. |
| **8. Budget** | **PASS — VERIFIED** | A −69, B +14, C +3, D +4; no compressed statements. Proposed B fix reaches +15. |

**Smallest proposed fix, not applied:** use the existing RAM-only marker to snapshot the approved anchor at boot. The cap and first-trusted-close timing remain unchanged.

```diff
--- a/src/state/StateMachine.h
+++ b/src/state/StateMachine.h
@@
-  bool occupancySessionCrossedBoot = false; // An occupancy session was already open when this boot started
+  time_t occupancySessionBootAnchor = 0; // Boot snapshot; zero means no cross-boot session

--- a/src/Generalized-Core-Counter.cpp
+++ b/src/Generalized-Core-Counter.cpp
@@
-  session.occupancySessionCrossedBoot = CurrentReadings::get_occupied();
+  session.occupancySessionBootAnchor = CurrentReadings::get_occupied()
+      ? std::max(CurrentReadings::get_occupancyStartTime(), SystemConfig::get_lastReport()) : 0;
   CurrentReadings::set_lastOccupancyEvent(0);

--- a/src/state/State_Common.h
+++ b/src/state/State_Common.h
@@
-	if (session.occupancySessionCrossedBoot) {
-		session.occupancySessionCrossedBoot = false;
+	if (session.occupancySessionBootAnchor != 0) {
 		const time_t bootEpoch = now - (time_t)(millis() / 1000UL);
-		const time_t anchor = std::max(start, SystemConfig::get_lastReport());
+		const time_t anchor = session.occupancySessionBootAnchor;
+		session.occupancySessionBootAnchor = 0;
 		closeAt = std::min(closeAt, std::min(bootEpoch, anchor + (time_t)(occupancyDebounceMs() / 1000UL)));
```

Scratch verification of this proposal passed the existing occupancy behavioral cases and reduced the reproduced over-credit to **300 seconds**. The restart test’s marker pins/fake should follow the renamed anchor and add the post-boot-report regression case.

**Stage 6 mid-boot trust-lapse note — VERIFIED WITH NOTES:** for a session started during the current boot, the restart cap does not apply. When trust expires 24 hours after sync and debounce expires during the lapse, excess credit equals the delay from intended close to the eventual trusted close. Regular debounce retries can add approximately another debounce after trust returns.

A separate production-close probe showed **36,300 seconds credited instead of 300** after a ten-hour delay: **36,000 seconds excess**. The v36 probe instead cleared the session and credited zero. The existing 86,400-second session/total guard remains; this is not a one-debounce bound. I consider this a disclosed limitation acceptable within this narrow WO: approved B2 preserves untrusted sessions, while the one-debounce guarantee specifically addresses restarts. It is separate from the blocking anchor defect.

**Build evidence:**

| Build | text | data | bss |
|---|---:|---:|---:|
| v36 reference | 150740 | 1090 | 2180 |
| Reviewed v37 | **150788** | **1090** | **2188** |
| Difference | +48 | 0 | +8 |

The clean serial release build used Boron Device OS 6.4.1 and fresh scratch outputs. An initial parallel make encountered a dependency-ordering failure; the fresh serial build succeeded.

`strings` found `v37-PreStep6Fixes`. Product metadata contains `25 00`, little-endian **37**. `nm` confirms `setup`, the changed state handlers, `occupancyDebounceMs`, `closeOccupancySessionSafely`, `HibernateCycle::classifyWake`, and `publishData` in the ELF. The supplied cloud-build result—flash 152014 / RAM 3282—was not rerun.

**Payload recomputation:** all `%lu` fields use ten digits; timestamp adds its literal `000`; occupancy uses one digit; hourly/daily/connecttime five; alerts four; resets three; battery `100.00`; temperature `-100.00`; context `Disconnected`; voltage four characters, including rounded `5.00`.

| Format | Before, excluding NUL | After, excluding NUL | After, including NUL |
|---|---:|---:|---:|
| Occupancy | 237 | **247** | **248 / 256** |
| Counting | 224 | **234** | **235 / 256** |

**Budget counting:** net physical source lines after excluding blanks and comments; declarations, includes, preprocessor directives and braces count.

| Item | Actual | Limit |
|---|---:|---:|
| A | −69; A2 itself 0 | ≤20 |
| B | +14; proposed fix +15 | ≤15 |
| C | +3 | ≤3 |
| D | +4 | ≤4 |
| Total reviewed diff | **−48** | |

**Preservation confirmed:** SHA-256 comparison of **124,126 files** found **zero added, removed or changed files outside scratch**; Git status was byte-identical. Only `build-tmp/wo20261004-001-stage7/` was removed. `build-tmp/` and `build-tmp/connectivity-archive/` remain untouched. No lasting edits, commits, network/AWS access, flashing or settings changes occurred.

**Model actually used:** `gpt-6-astra`, **high** reasoning.