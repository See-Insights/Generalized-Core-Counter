UNDER REVIEW: wo/2026-11-05-003-v50-s1-boot-config-time @ ed6ca1bd791f274717617c9c11fa40b91db59cbf

# PASS for S1 as scoped

This is not standalone shipment approval: S1 must be followed by S1b and the integrated set must pass the five bench items before merging to `main`.

| Area | Result | Evidence |
|---|---|---|
| A. WO items 1–5, Amendments 1–4 | **PASS** | Binding occurs before Time-dependent work with Wire/WKP/D5, fallback capability, and explicit external clock source at [Generalized-Core-Counter.cpp:295](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1-r2/src/Generalized-Core-Counter.cpp:295). Clock source is compared at [ExternalRtc.h:34](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1-r2/src/time/ExternalRtc.h:34); a stored `INTERNAL` configuration rebinds and emits the warning. Cause 8 is retired. Schema 3 and null raw oscillator fields are present. Amendments 2/3 correctly defer the oscillator latch to S4. Independent budget: **+102/−83 = +19 net `src/`**, within +21. |
| B. Build and linkage | **PASS** | Fresh Device OS 6.6.0 Boron build passed. `nm` found `ExRtcT<Am18x5Configuration>::enable`, `hal_exrtc_bind`, `hal_rtc_get_time`, `hal_rtc_set_time`, and `hal_rtc_time_is_valid`. `objdump` showed two `enable()` calls in `setup()`, `enable()` calling `hal_exrtc_bind`, and `checkClockResync()` calling `hal_rtc_set_time` followed by `ExternalRtc::get`/`hal_rtc_get_time` before success bookkeeping. |
| C. Host suite | **PASS** | **116/116**: 66/66 `.sh` via `zsh`; 50/50 `.py` via `python3`. `thermal_coupling_structural_test.py` also passed from a clean tracked-file export. |
| D. Mutations | **PASS** | All listed follow-up mutants were caught, including no read-back, read-back before set, removed threshold, ignored read-back error, wrong sign, ignored/wrong clock-source comparison, and missing fleet clock source. Three new mutants also failed: `< 2 s` instead of `≤ 2 s`, mismatch warning downgraded to info, and fleet source changed to `INTERNAL`. No survivor. |
| E. Other behavior | **PASS** | No additional boot, ledger/status, or time-handling defect found. Payload key order is preserved. The four comment-only files and corrected build baselines are named at [r1fix report:27](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1-r2/docs/work-orders/WO-2026-11-05-003-r1fix-stage6-copilot-report.md:27); schema documentation is corrected at [FIELD_MEANINGS_REFERENCE.md:202](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1-r2/docs/FIELD_MEANINGS_REFERENCE.md:202). |

## RTC read-back assessment

[ExternalRtc.h:16](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1-r2/src/time/ExternalRtc.h:16) is correct:

- The zero-initialized `timeval` writes whole seconds with `tv_usec=0`.
- The read-back uses `HAL_RTC_SOURCE_EXTERNAL`.
- Any set or read-back HAL error returns false.
- The conditional subtraction is symmetric and accepts exactly ±2 seconds.
- Device OS writes hundredths as zero and the helper intentionally compares only returned whole seconds.
- Clock advances `lastRtcWriteSyncedLastMs`, `lastTimeSync`, and correction telemetry only when the verified write returns true at [Clock.cpp:400](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s1-r2/src/time/Clock.cpp:400).
- A failed write cannot spin: Clock applies the existing 60-second retry floor. The skew hook remains one-shot. The extra read is harmless at both call sites.

## Findings

No S1-scoped findings. Consequently no S1 fix or +21-budget ruling is required.

The two prior High defects remain real but are explicitly carried to S1b by Amendment 4. S1 does not worsen them or make them harder to fix.

## S1b observations

S1 leaves the required insertion point immediately before `ensureBound()`; neither preceding function reads Time or owns the RTC. Its failed read-back cannot create an infinite retry.

The proposed seed/WRTC flow closes both carried defects:

1. Decode a plausible chip time and call `Time.setTime()` while unbound.
2. `Time.setTime()` updates the internal HAL clock even though the external source is not yet present.
3. Device OS binding then reads that seeded internal clock and writes it back to the chip.
4. WRTC remains enabled, so subsequent HAL writes can take effect and S1’s read-back confirms them.

One **High, S1b-only design gap requires an architect ruling**:

- **Evidence:** S1b draft `6d92c5d:...pre-bind-rtc-access.md:24,43` requires honoring 12/24-hour mode while setting only WRTC and preserving every other CONTROL1 bit. Device OS [am18x5.cpp:782](/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/am18x5.cpp:782) always writes `tm_hour` as 24-hour BCD, while its bind configuration only modifies ARST and preserves the 12/24 bit at [am18x5.cpp:616](/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/hal/shared/am18x5.cpp:616).
- **What breaks:** A chip already in 12-hour mode can be decoded and seed Time correctly, but bind then writes a 24-hour value while the chip remains in 12-hour mode, potentially corrupting the first-boot time.
- **Smallest fix:** After decoding using the original mode, S1b’s CONTROL1 RMW should set WRTC **and clear `CONTROL1_1224_MASK`**, preserving all other bits. Update the all-256-values property test accordingly.
- **Budget:** No S1 `src/` cost and effectively no extra S1b lines; it fits S1b’s +15 budget.
- **Required ruling:** Supersede “set bit 0 only/preserve every other bit” with “set WRTC, normalize to 24-hour mode, preserve every remaining bit.”

Wire is an implementation requirement for S1b Stage 7: hold `WITH_LOCK(Wire)` across the raw read and CONTROL1 RMW, and call `Wire.begin()` if it is not enabled. On Boron the PMIC/fuel gauge use I2C interface 2 while this RTC uses `Wire`, but locking still makes the pre-bind ownership window explicit and safe.

## Environment

AGENT: Codex · MODEL: `gpt-5.6-sol` · REASONING: `xhigh`

Interpreters: `zsh 5.9 (arm64-apple-darwin26.0)` for `.sh`; `Python 3.14.6` for `.py`.

Build: Device OS 6.6.0, Boron, gcc-arm 10.2.1, buildscripts 1.17.2.

**Build numbers: text 152708 / data 1114 / bss 2240.**

I could not and did not run flashing, hardware/OTA tests, the spike `settime … hal` bench, live ledger/cloud publication, or the integrated five-item bench. Historical baseline builds were not rerun; their exact SHAs and prior measured numbers are correctly recorded in the follow-up report. The checkout remains clean at the reviewed SHA; all generated files are confined to the authorized scratch directory.