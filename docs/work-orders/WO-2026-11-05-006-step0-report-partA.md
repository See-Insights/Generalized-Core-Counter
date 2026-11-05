MODEL: gpt-5.6-sol · REASONING: high · BASE read: main 41d9911

## Answer 1 — Present trust rule and production consumers

**OBSERVATION:** Trust is currently boot-local: `Clock::isClockTrusted()` requires `Time.isValid()` and a synchronization observed during this boot (`src/time/Clock.cpp:477-480`). The 24-hour maximum age is defined at `src/time/ClockTrust.h:39-42`; wrap-safe trust and resync predicates are at `src/time/ClockTrust.h:277-299`. The persisted `lastTimeSync` is reported but does not establish trust (`src/time/Clock.cpp:522-524`).

**OBSERVATION:** Direct `isClockTrusted()` consumers are connection-age telemetry (`src/Generalized-Core-Counter.cpp:1312-1321`), reset/count timestamps (`src/MyPersistentData.cpp:642-648`, `src/state/State_Modes.cpp:29-49,191-200`), ledger/status diagnostics (`src/cloud/DeviceStatusPublisher.cpp:285-293`, `src/state/State_Idle.cpp:120-140`, `src/time/ClockDiagnostics.cpp:74-108`).

**OBSERVATION:** `Clock::isTrusted()` controls boot-storm timing (`src/Generalized-Core-Counter.cpp:377-394`), daily configuration input (`src/cloud/LedgerClient.cpp:125-134`), report scheduling (`src/reporting/RuntimeReportingPolicy.cpp:42-53`, `src/reporting/ReportingPolicy.cpp:66-89`), occupancy close/credit (`src/state/State_Common.h:338-405`), alert correction (`src/state/State_Error.cpp:57-73`), measurement sampling (`src/state/State_Idle.cpp:145-163`), sleep/report alignment (`src/state/State_Sleep.cpp:569-589`), and daily rollover (`src/time/DailyBoundary.cpp:26-45`).

**OBSERVATION:** `Clock::openness()` returns `Unknown` whenever trust or configuration is unavailable (`src/time/Clock.cpp:563-584`). Its production effects include connectivity failsafe behavior (`src/power/ConnectivityFailsafe.cpp:218-235`), occupancy suppression and close clamping (`src/state/State_Common.h:61-63,357-362`), idle/report decisions (`src/state/State_Idle.cpp:108-141,189-221,290-314`, `src/state/State_Report.cpp:99-124`), and sleep/hibernate planning (`src/state/State_Sleep.cpp:491-590,690-700,1080-1098,1163-1176`).

**INFERENCE:** Carrying trust across restart affects real behavioral gates, not merely timestamps: daily close and credit, occupancy acceptance, report cadence, failsafe decisions, awake duration, overnight sleep, and hibernate eligibility. A false-positive trust result is therefore materially riskier than a delayed timestamp.

## Answer 2 — Failure history and remaining problem

**OBSERVATION:** Item 15 records occupancy loss across reset, potentially two to three hours, and remains unresolved (`docs/RECOVERY_PLAN_2026-09-26.md:244-247`). Item 18’s Dev09 stage-2 reset occurred at 10:40:55Z; afterward there was no trust, failsafe, night sleep, close, or credit until recovery near 00:53Z. The delayed close produced 550 total minutes versus 345 before recovery—about 205 minutes credited for 204.65 wall-clock minutes but only about 97.21 real occupancy minutes (`docs/RECOVERY_PLAN_2026-09-26.md:254-260`).

**OBSERVATION:** Item 18/#151 makes `Unknown` eligible for connectivity-failsafe action but does not restore close, night sleep, or occupancy semantics (`docs/RECOVERY_PLAN_2026-09-26.md:263-264`). Item 10 suppresses occupancy only when openness is known `Closed`; `Unknown` remains admitted (`src/state/State_Common.h:61-63`). The recovery plan identifies item 21 as persisting justified trust and withdrawing it on adverse evidence (`docs/RECOVERY_PLAN_2026-09-26.md:288-293`).

**OBSERVATION:** Dev11 showed an RTC roughly 5h09 behind while `Time.isValid()` remained true; the old one-shot write then perpetuated the bad value (`docs/work-orders/WO-2026-08-29-002-time-sync-guarantee-and-clock-trust.md:24-44,80-93,105-131`). The current recurring writeback reads RTC before writing and records the correction (`src/time/Clock.cpp:392-433`).

**INFERENCE:** Item 21 would have preserved trust through the clean Dev09 restart, allowing timely close/night behavior. It does not solve item 15’s lost persisted occupancy record, and it must not equate “RTC returned an epoch” or `Time.isValid()` with trustworthy time.

## Answer 3 — C1/C2 interaction

**OBSERVATION:** C1 centralizes the remaining debounce calculation (`[C1] src/state/State_Common.h:328-344`), uses it for awake/wake close decisions (`[C1] src/state/State_Modes.cpp:128-170`), and plans sleep for the remaining interval (`[C1] src/state/State_Sleep.cpp:607-620`). It does not change the existing trusted/untrusted credit split (`[C1] src/state/State_Common.h:356-371`).

**OBSERVATION:** C2 adds RAM-only `occupancyStartMs` and `pieceUntrusted` (`[C2] src/state/StateMachine.h:43-72`). On boot, an occupied untrusted session becomes a post-boot untrusted straddler; otherwise it receives the trusted B3 anchor (`[C2] src/Generalized-Core-Counter.cpp:497-507`). New pieces snapshot current trust (`[C2] src/state/State_Modes.cpp:63-110,220-267`), while untrusted pieces use millis credit and clear their marker at close (`[C2] src/state/State_Common.h:356-374,395-430,453-456`).

**OBSERVATION:** The held C2 report explicitly says #151 improves failsafe reachability while item 10 leaves `Unknown` unchanged; C1/C2 repair merged-piece and huge-gap overcredit but cannot define genuine restart trust or eliminate reset-straddler loss (`[C2] docs/work-orders/WO-2026-11-03-001-step0-report.md:19-25,45-59,83-101`).

**INFERENCE:** With item 21, a clean RTC restart takes C2’s trusted/B3 path instead of the nominal post-boot-only straddler path. If trust is later withdrawn, pieces opened afterward become untrusted. A piece opened while trusted remains under C2’s B2 “keep open until trust returns” behavior (`[C2] src/state/State_Common.h:368-374`); it is not retroactively converted and needs an explicit regression test.

## Answer 4 — Device OS 6.6.0 evidence and limitations

**OBSERVATION:** Device OS 6.6.0 is not vendored in this repository. I am relying on the recorded S-spike inspection, which states that HAL RTC get/set and `TimeClass(EXTERNAL)` validity exist (`docs/work-orders/WO-2026-10-31-001-step0-report.md:1-3,38-40`), but public APIs expose neither sticky oscillator-failure state nor raw AB1805 registers/RAM (`docs/work-orders/WO-2026-10-31-001-step0-report.md:48-53`).

**OBSERVATION:** That record says Device OS clears oscillator-failure state when event monitoring is configured and reports subsequent failures as events (`docs/work-orders/WO-2026-10-31-001-step0-report.md:64-69`). The architectural ruling is therefore “fault seen since bind,” not “absence of fault” (`docs/work-orders/WO-2026-10-31-001-step0-report.md:96-103`). A separate transcript records HAL handler registration/removal, but not the exact public event enum or complete payload list (`docs/work-orders/WO-2026-11-04-002-stage6-copilot-transcript.md:503-519`).

**OBSERVATION:** The v50 draft assigns `ExternalTime.onEvent`, HAL/`TimeClass(EXTERNAL)`, and event telemetry to the preceding S-set, with item 21 dependent on that work and C1/C2 (`[v50-plan] docs/work-orders/WO-2026-11-05-002-v50-ab1805-switch-draft.md:11-16,20-33`).

**INFERENCE:** I cannot independently certify the precise Device OS event values or all carried fields from this repo. Item 21 should consume a tested S-set boolean such as `faultSeenSinceBind`; it should not duplicate or guess the Device OS enum. A fault before callback binding may be indistinguishable from a recovered crystal if source, validity, and plausibility all look normal. Strict boot-gap coverage requires an earlier persistent/event latch.

## Answer 5 — Candidate rules and withdrawal semantics

**OBSERVATION:** The code already defines plausible external time as 2024-01-01 through, but not including, 2035-01-01 (`src/time/Clock.cpp:551-555`). Successful RTC writeback currently stamps `lastTimeSync=Time.now()` and saves the measured pre-write correction; failures neither stamp nor claim success (`src/time/Clock.cpp:392-441`). Correction is positive when the RTC was behind and negative when it was ahead (`src/time/Clock.cpp:150-158`).

**INFERENCE:** Candidate A is persisted-sync epoch plus successful external read, `TimeClass(EXTERNAL).isValid()`, plausible current and persisted epochs, nonnegative age, and age below the existing policy limit. It handles a clean restart but trusts a recently wrong yet plausible RTC.

**INFERENCE:** Candidate B adds correction withdrawal: after cloud sync, only an acceptable read/write with `abs(correctionSec) <= T` remains carry-eligible. An invalid read/write or `abs(correctionSec) > T` stores zero as the revocation sentinel. Current-boot cloud trust may remain valid, but another restart cannot inherit trust. Part B must select `T`; this report does not.

**INFERENCE:** Candidate C adds immediate oscillator-fault withdrawal: the S-set fault latch clears persisted eligibility and makes current-boot trust false. The rare revoke/fault transition should be synchronously flushed because normal persistence is delayed. This is the strongest design supported by the planned prerequisites.

**INFERENCE:** RTC-derived age is not independent: a slow RTC can postpone expiry and a fast RTC can withdraw early. A later large correction detects the problem only after connectivity and cannot undo actions already taken under false trust.

## Answer 6 — Persistence choices and survival matrix

**OBSERVATION:** `lastTimeSync` already occupies the frozen `SysData` layout (`src/MyPersistentData.h:115-143`) and is persisted in `/usr/sysStatus.dat` with a normal 100 ms deferred save (`src/MyPersistentData.cpp:57-80,268-273`). StorageHelper also supports forced flushing (`lib/StorageHelperRK/src/StorageHelperRK.h:593-605,914-993`; `lib/StorageHelperRK/src/StorageHelperRK.cpp:25-41,102-110`).

**OBSERVATION:** Retained RAM survives reset/watchdog/hibernate while powered but not complete power loss; the existing retained structure also requires a validity token rather than trusting a fresh initializer (`src/time/RtcSkewTest.h:107-120`, `src/Generalized-Core-Counter.cpp:215-230`).

**OBSERVATION:** AB1805 provides 256 bytes of battery-backed RAM and read/write methods in the library (`lib/AB1805_RK/src/AB1805_RK.h:635-707`, `lib/AB1805_RK/src/AB1805_RK.cpp:880-963`), but it is lost with RTC-battery removal and the planned native Device OS API does not expose raw RAM/registers.

**INFERENCE:** Reusing `lastTimeSync` is the smallest and safest persistence choice: reset, watchdog, hibernate, system power-down, and battery removal retain the flash record, assuming the write is flushed. It adds no steady-state write beyond the existing successful-sync write; revocations are rare. Retained RAM fails the cold-power requirement, and AB1805 RAM adds a new format/integrity/API ruling while still failing RTC-battery removal.

**INFERENCE:** Zero naturally means “no carry eligibility.” Existing upgraded devices with a recent valid value can carry immediately. If policy instead requires every first I21 boot to start untrusted, a migration/version marker is necessary and the change is no longer the smallest no-layout design.

## Answer 7 — Verification and bench observables

**OBSERVATION:** Existing structural tests exhaustively constrain trust-call sites and converted consumers (`tests/clock_trust_standard_structural_test.py:25-35,75-93,147-158`). Resync, retry, current-boot trust, telemetry, and timestamp behavior are pinned at `tests/clock_resync_wiring_test.py:424-449,532-630,644-664`; successful/failed recurring writes are covered at `tests/clock_rtc_writeback_test.cpp:223-248,267-315`. Openness, reporting, and daily gates are covered at `tests/clock_openness_test.cpp:15-77`, `tests/reporting_policy_test.cpp:94-153`, and `tests/daily_cleanup_boundary_test.py:127-149`. Per authorization, I ran no tests or build.

**INFERENCE:** Host tests should cover valid/invalid/unavailable external time; missing/zero/future/stale persisted epoch; age `N-1` and `N`; millis wrap; read/write failure; fault-before/after trust; correction `T-1`, `T`, and `T+1`; reboot following revocation; and C2 pieces opened before versus after withdrawal.

**INFERENCE:** Clean-reset bench: reset mid-session without cloud, then require immediate trust, correct openness, timely close, expected reporting boundary, and normal overnight sleep/hibernate. Under C2 the reboot should use the trusted B3 anchor, not `pieceUntrusted`; this improves the straddler but does not prove item 15’s persisted-session loss is solved.

**INFERENCE:** Wrong-RTC bench: inject a known signed skew, reset, then compare measured correction with the injected value within tolerance. Over-threshold correction must clear carry eligibility; at/below threshold plus successful write may restore it. The existing hook can inject −28,800 seconds (`src/time/RtcSkewTest.h:348-371`), but Part B must supply the actual acceptance threshold.

**INFERENCE:** Fault bench: host injection is mandatory. Physical oscillator-fault induction should be optional because the earlier spike classified it as unsafe/skipped (`docs/work-orders/WO-2026-10-31-001-step0-report.md:73-75`). A clean-day run must show unchanged close totals, report boundaries, and sleep behavior relative to baseline.

## Answer 8 — Safety envelope and size

**OBSERVATION:** False trusted/`Closed` state can discard occupancy (`src/state/State_Common.h:61-63`); false trust can alter daily credit, report alignment, alert/failsafe decisions, and awake duration. Overnight sleep is capped but may reach 546 minutes (`src/state/State_Sleep.cpp:525-531`). False untrusted state is safer operationally but recreates the recovery delays described above.

**INFERENCE:** The implementation should be a pure, host-testable carry predicate plus explicit withdrawal inputs. Boot carry requires: external read success, external validity, plausible epochs, `0 <= age < N`, nonzero persisted eligibility, and no latched oscillator fault. Normal cloud sync remains the authority for current-boot trust. Carry eligibility is renewed only by a successful acceptable RTC correction/write and is revoked by fault, invalid write/read, or an over-threshold correction.

**INFERENCE:** Do not change the 24-hour policy or select correction threshold `T` in Part A. Make the boundary semantics explicit: age uses `< N`; correction withdrawal uses `> T`.

## Candidate ranking

1. **C — persisted epoch + validity/plausibility/age + correction withdrawal + oscillator-fault withdrawal.** Best fit to “survives restart and is withdrawn on evidence.”
2. **B — persisted epoch + validity/plausibility/age + correction withdrawal.** Handles large observed error but misses crystal-failure evidence before correction.
3. **A — persisted epoch + validity/plausibility/age only.** Smallest, but repeats the Dev11 lesson that plausible/valid is not necessarily accurate.

## Stop decision

**PROCEED**, conditional on the S-set landing a tested `ExternalTime`/external-validity surface and a durable-in-process `faultSeenSinceBind` signal with verified Device OS enum/payload handling. C1/C2 should land or be the rebase target before final integration. No new persisted-layout ruling is required when zeroing the existing `lastTimeSync` field is accepted as revocation.

## Smallest I21 WO shape

- Add a pure carried-trust predicate using existing persisted epoch, external validity/plausibility, age, and the S-set fault latch.
- Combine carried trust with current-boot synchronization in `Clock::isClockTrusted()`.
- Renew eligibility after acceptable successful RTC correction/write; synchronously clear it on failure evidence or oscillator fault.
- Add host boundary/withdrawal tests and C2 transition tests; add the four bench scenarios above.
- Estimated production change: **17–20 `src/` lines** if the S-set already supplies the event latch and callback lifecycle. If I21 must implement that lifecycle itself, split it out rather than expanding I21; the combined production change would be roughly **24–30 `src/` lines**.