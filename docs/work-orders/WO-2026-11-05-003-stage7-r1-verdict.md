UNDER REVIEW: wo/2026-11-05-003-v50-s1-boot-config-time @ ce2e597de8448b869d9d44ed8c5175272e428b4b

# NOT VERIFIED overall

Two High defects must be resolved before S1 is accepted: Device OS can report a successful RTC write without changing the chip, and the first unbound boot does not preserve/seed the valid external time required by the WO’s bench contract.

| Item | Result | Evidence / conclusion |
|---|---|---|
| 1. WO items 1–5 and amendments | **FAIL** | Binding, cause retirement, snapshot, schema, and untouched S2/S3 sites are present, but item 4’s confirmed-write semantics and item 5’s first-boot behavior fail. Stored clock-source mismatches are also missed. |
| 2. Source budget | **PASS** | Independently counted **+99/−83 = +16 net**. No additional `src/` files. Four comment-only files were not named individually in the Stage 6 report; see Low finding. |
| 3. Three Stage 6 risks | **FAIL** | WRTC is a fix-now S1 defect; first-unbound-boot seeding is required; the `isRTCSet()` semantic change is safe at both call sites. |
| 4. Amendment 2 latch | **PASS** | No latch exists in `src/`; adding the reported nine lines would take S1 to about +25. Device OS exposes the required event and callback. Nothing in S1 depends on the latch. |
| 5. Mandatory checks | **PASS** | Current build, linkage, retirement replacements, host suite, `nm` and `objdump` passed. The historical v49.1 size comparison was not reproducible; see Low finding. |
| 6. Mutations | **PASS** | Six independently applied scratch mutants were caught; no survivor. |
| 7. Other behavior/contracts | **FAIL** | Stored internal-clock configuration is treated as matching; several first-boot comments are false; field-reference documentation still calls schema 2 current. Payload key order itself is unchanged. |

## WO item-by-item

- **Item 1 — FAIL.** Bind runs early with Wire/WKP/D5 and only the fallback capability, leaving AOS off: `src/Generalized-Core-Counter.cpp:295-297`, `src/device_pinout.cpp:44`. Bound/no-bound and mismatch branches are at `src/time/ExternalRtc.h:37-50`; failures return and boot continues without reset. However, clock-source mismatches are not compared.
- **Item 2 — PASS.** Cause 8 is reserved but no longer declared or used: `src/ResetCause.h:15-24`; the structural test requires seven distinct reset sites and rejects cause 8 at `tests/reset_cause_structural_test.py:38-45,107-139`.
- **Item 3 — PASS.** Live clock source and configured AOS/FOS are captured at `src/Generalized-Core-Counter.cpp:658-663`. `oscStatus`/`oscCtrl` remain in their original order and are null at `src/cloud/DeviceStatusPublisher.cpp:312-316`. Schema 3 is shared by status and data at lines 51, 208, and 511. That shared bump is acceptable—and preferable—because the status representation also changed.
- **Item 4 — FAIL.** External get/set/validity helpers use the intended source and preserve HAL error-to-bool mapping at `src/time/ExternalRtc.h:6-28`; Clock only records success after the helper returns true at `src/time/Clock.cpp:400-419`. Device OS nevertheless returns success for an unconfirmed/no-op write, invalidating that contract.
- **Item 5 — FAIL.** Bound boots seed before `setup()`, but new binding copies the invalid internal clock to the external chip and does not seed the internal clock from the valid chip.
- **Item 6 — PASS.** `classifyWake`, `setWDT`, and `loop` remain byte-equivalent apart from line movement at `src/Generalized-Core-Counter.cpp:756,776,1141`. `State_Sleep.cpp` is unchanged at lines 699, 709, 742, 785, 898, and 926; both `deepPowerDown()` callers remain unchanged at `src/state/State_Error.cpp:178` and `src/power/ConnectivityFailsafe.cpp:316`.

## Findings

1. **High — RTC writes can silently no-op yet be recorded as confirmed success.**

   `hal_rtc_set_time(EXTERNAL)` forwards the result through `rtc_hal.cpp:116-127` and `exrtc_hal.cpp:412-416`. `Am18x5::setTime()` writes the calendar directly without setting WRTC at `am18x5.cpp:770-788`. Its `writeContinuousRegisters()` ignores every I²C write/end-transmission result and returns the requested length at `am18x5.cpp:1350-1359`; `CHECK` accepts non-negative values at `services/inc/check.h:141-149`.

   The vendored implementation documents that RTC registers require WRTC=1, sets it, writes, then clears it at `lib/AB1805_RK/src/AB1805_RK.cpp:261-295`. Consequently, chips previously touched by that library have WRTC=0. `ExternalRtc::set()` nevertheless returns true solely on HAL result zero at `src/time/ExternalRtc.h:15-21`, allowing Clock to advance its confirmed-write bookkeeping although the RTC is unchanged.

   **Decision:** this is an S1 defect to fix now, not merely a bench item. Bench `settime … hal` should still confirm the repaired path, but cannot substitute for a valid implementation.

   **Smallest durable fix:** fix Device OS `Am18x5::setTime()` to set WRTC, write the calendar, clear WRTC, and propagate real I²C results. This is outside the application `src/` budget. An app-side read-back would detect failure but would not make WRTC=0 writes work and would push the current +16 close to/over budget.

2. **High — first unbound boot neither preserves chip time nor seeds system time.**

   With no stored binding, `hal_rtc_init()` cannot read the external RTC (`rtc_hal.cpp:77-85`), so the internal clock retains its 2000-01-01 default (`rtc_hal.cpp:35-39`). After binding, `bindIfNeededAndResync()` explicitly reads that internal value and writes it to the external RTC (`exrtc_hal.cpp:236-249`). It never reads the valid chip time into the internal clock.

   This contradicts bench step 1. The current WRTC defect may accidentally prevent the overwrite, but system time is still not seeded; once WRTC is fixed, the overwrite becomes effective. It also makes the claims at `src/Generalized-Core-Counter.cpp:655,665` and `src/time/ClockTrust.h:16` false for the first unbound boot, and the occupancy clamp remains a no-op until a later boot.

   **Decision:** seed-before-bind behavior is required in S1; the exposure is not acceptable merely because an OTA boot normally connects to cloud.

   **Smallest durable fix:** change Device OS binding logic to bind, read and validate the existing external time, seed internal time when valid, and only seed external from internal when the chip is invalid. This is outside the application `src/` budget.

3. **Medium — a stored internal-clock configuration is not treated as a mismatch.**

   `sameFleetConfig()` deliberately omits `clockSource()` at `src/time/ExternalRtc.h:30-34`, and the fleet constructor at `src/Generalized-Core-Counter.cpp:296-297` does not explicitly request `RtcClockSource::EXTERNAL`. Device OS returns the actual internal/external configuration at `am18x5.cpp:311-338`. A scratch probe with otherwise identical configs returned `sameFleetConfig=1` when only clock source differed.

   This permits a stored internal-RC configuration to survive despite the external-crystal fleet configuration and Amendment 1’s rebind-on-mismatch rule.

   **Smallest fix:** explicitly add `.clockSource(particle::RtcClockSource::EXTERNAL)` and compare `clockSource()` in `sameFleetConfig`; add a corresponding host case/mutation. About +2 `src/` lines, taking the budget to roughly +18, so it fits.

4. **Low — Stage 6’s per-file inventory is incomplete.**

   The report says only “stale comments fixed” and “others 1/1 or 1/0”; it does not name `src/BuildProfile.h`, `src/MyPersistentData.h`, `src/persist/CurrentReadings.h`, or `src/time/ClockTrust.h`. All four are comment-only and in scope, so behavior is unaffected.

   **Smallest fix:** name the four files in the report. No `src/` cost.

5. **Low — schema documentation contradicts itself.**

   `docs/FIELD_MEANINGS_REFERENCE.md:202` says schema 3 from v50, while lines 205-207 still call schema 2 current for both ledgers.

   **Smallest fix:** update those three documentation lines to schema 3. No `src/` cost.

6. **Low — the reported v49.1 binary baseline is not reproducible from the tag.**

   Stage 6 reports `151732/1114/2200` at `docs/work-orders/WO-2026-11-05-003-stage6-copilot-report.md:35`. Under the same Device OS/toolchain invocation, I obtained:

   - Under review: **152604 / 1114 / 2240**, reproduced twice.
   - Integration base `06aacd9`: **153100 / 1114 / 2240**.
   - Actual `v49.1` tag (`9a96854`): **152692 / 1114 / 2232**.

   Thus the current number is verified, but the claimed `+872 text/+40 bss` comparison is not. The older `151732` appears to describe an earlier pre-release baseline rather than the tagged v49.1 tree.

   **Smallest fix:** identify the exact baseline SHA and invocation in the report or replace the comparison with a build of the named tag. No `src/` cost.

## Risk (c): `isRTCSet()` semantics

**Safe.** The old predicate tested only that WRTC was clear (`lib/AB1805_RK/src/AB1805_RK.h:97`). The new helper performs a nonblocking external read and requires an epoch after 2018 (`rtc_hal.cpp:88-90`).

- In `setup()`, it only affects the diagnostic warning at `src/Generalized-Core-Counter.cpp:833-836`.
- In hibernate eligibility, false rejects earlier; true is followed by another external read and the stricter 2024–2035 plausibility gate at `src/time/HibernateEligibility.cpp:76-89` and `src/time/Clock.cpp:549-552`.

No hibernate-safety condition is weakened.

## Latch

The current diff contains no `onEvent`, `oscFaultSeen`, or `CLOCK_SOURCE_EXTERNAL_FAILURE` source implementation. Device OS confirms:

- enum and callback signature: `spark_wiring_exrtc.h:85-94`;
- registration: `spark_wiring_exrtc.h:510-534`;
- XT failure mapping: `am18x5.cpp:695-715`;
- OFIE only for the external crystal: `am18x5.cpp:595-600,635-638`;
- registration clears the oscillator-failure flag: `am18x5.cpp:645-648`.

The proposed latch was about nine net lines, which would make +25; S4 deferral is therefore correct. S1 has no dependency on it.

## Tests, mutations, and linkage

Host suite: **116/116** — 66/66 `.sh` via zsh and 50/50 `.py` via python3. `thermal_coupling_structural_test.py` passed on a clean tracked-file export. One initial runner path typo was corrected; it was not a test failure.

Independent scratch mutations, all caught:

- unconditional enable → host assertions at `external_rtc_test.cpp:61,66,70`;
- skip mismatch rebind → lines 66-79;
- wrong write source → line 40;
- missing fallback → structural configuration check;
- ignored `enable()` result → line 74;
- removed `Time.isValid()` precondition → line 49.

No mutation survived.

The retired guard behavioral test previously proved deletion/recheck/reset-once/data-8/boot-storm behavior; its order test proved the guard preceded Time/Wire and vendored setup. Those behaviors are intentionally retired. Replacement coverage now proves native bind/no-rebind/mismatch/failure behavior and order, while `reset_cause_structural_test.py` proves cause 8 is absent.

`nm`/`objdump` confirmed:

- two `ExRtcT<Am18x5Configuration>::enable()` calls from `setup()` and its call to `hal_exrtc_bind`;
- `hal_rtc_get_time` from Clock and hibernate eligibility;
- `hal_rtc_set_time` through `ExternalRtc::setFromSystem`;
- `hal_rtc_time_is_valid` from hibernate eligibility;
- no new vendored `AB1805` symbol versus `06aacd9`; seven were removed, including `AB1805::setup`, `setRtcFromSystem`, and `usingRCOscillator`.

No payload key-order change and no actual `Time` read occurs before binding: the two preceding calls are the pure retained-state validator and serial settle.

## Environment and unrun work

AGENT: Codex · MODEL: `gpt-5.6-sol` · REASONING: `xhigh`

Interpreters: `zsh 5.9 (arm64-apple-darwin26.0)` for `.sh`; `Python 3.14.6` for `.py`.

Build: Device OS 6.6.0, Boron, gcc-arm 10.2.1, buildtools 1.1.1; fresh scratch `BUILD_PATH_BASE`; **152604 text / 1114 data / 2240 bss**.

I could not run hardware/flash/OTA tests, the spike binary’s `settime <epoch> hal`, live cloud/ledger publication, or inspect fleet-ops consumers outside this repository. The checkout remains clean at the exact reviewed SHA.