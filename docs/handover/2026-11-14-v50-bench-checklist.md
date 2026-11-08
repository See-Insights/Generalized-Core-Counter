# v50.1 pre-ship bench: step-by-step checklist for 14 Nov 2026

For Chip. Parent: `WO-2026-11-05-002-v50-ab1805-switch.md` ("Pre-ship bench", five items), on `integration/v50-ab1805` (not on `main`). The integrated head is final: PR #208 merged, Stage 7 round 2 PASS on `cf09b80` (no findings). Nothing below has been run on hardware; every result is yours to record. Times in UTC.

## 0. The kit

| Role | File | Source | SHA-256 |
|---|---|---|---|
| Integration binary (product version 5001, `v50.1-NativeRTC`) | `bench-kit/integration-5001/firmware-boron-6.6.0.bin` | `integration/v50-ab1805` @ `1d1d5af7a22dc47094a4c23b515b348bd8ae153d`, tree `0c6cbcccc27cc0e0c6c37d320136eb8d54a13526`, built by CI run 42490341544 (artifact `firmware-boron`, FIELD build: `DEV_BUILD=0`, `CONNECTIVITY_FAILSAFE_TEST_MODE=0`) | `1aafa89417e65d6a719a9e8ae3af478d904821718cffe552f2084d96cafca7b5` |
| **No-S1b binary: BENCH ONLY, NEVER RELEASED** (product version 5099, `v50.1-NativeRTC-NOS1B-BENCH-ONLY`) | `bench-kit/no-s1b-5099/firmware-boron-6.6.0.bin` | branch `bench/2026-11-08-no-s1b-NEVER-RELEASE` @ `729ff14b0e0e05f9ce3f123fbe9638930ee8e6ce`, tree `c546fd767f430a3f8f8c05eb52481f24692a12bd`, CI run 42490763093 | `8659156e7fb920baded75a01d6cb54179697712d2ddf2f663579f742a78d8f01` |
| Spike image | `bench-kit/spike/ab1805-spike.bin` (copy of `~/Downloads/spike-2026-11-03/ab1805-spike.bin`) | the 3 Nov spike | `029e8b6b23eff443e6ac45163b8643d11979e8cd390d93527ad3d84a12dba92d` |
| Rollback image v49.1 (product version 4901) | `bench-kit/rollback-v49.1/gcc-v49.1-boron-dos6.6.0-9a96854.bin` | GitHub release `v49.1` (Device OS 6.6.0) | `a00c5ceaa939aa274346d52e0cea9477c01a27c37aa1e68829bbc4bb8c145ec6` (matches the release's `.sha256`) |

The kit directory is `~/Downloads/v50-bench-kit-2026-11-14/` (with `SHA256SUMS.txt`). Verify each SHA-256 (`shasum -a 256`) before flashing.

**The no-S1b binary** is the integrated head with exactly one behavioural change: the `PreBindRtc::run(ab1805, Wire)` call in `setup()` is removed (S1b's pre-bind seed and WRTC set). Two further lines differ only so the image cannot be mistaken for a release: the product version (5001 -> 5099) and the version string. `git diff 1d1d5af 729ff14 --numstat`: `src/FirmwareVersion.h` 2/2, `src/Generalized-Core-Counter.cpp` 0/1. It is built on a branch that is never merged or tagged; CI's `host-tests` fail on that branch by design (S1b's structural tests require the call), and its `firmware` job passes. Do not OTA it, do not set it as a product default, do not upload it to the console.

Both CI binaries were downloaded from the run artifacts (`firmware-boron`, which also holds `target/workspace.elf`) and hashed locally; the CI log prints only the artifact zip digest, so compare the `.bin` hash with the run's job summary if you want a second source. Binary check: the integration image contains the string `v50.1-NativeRTC` and the `PreBindRtc: chip access failed` log text; the no-S1b image contains `v50.1-NativeRTC-NOS1B-BENCH-ONLY` and does not contain that log text.

Not in the kit (see section 7): any build with `CONNECTIVITY_FAILSAFE_TEST_MODE=1`, and any build for the S2 watchdog proof.

## 1. Ground rules
- Dev-14 first, then Dev-09. Serial attached for every step; keep the logs (UTC).
- Record, never infer: write the observed value, or "not observed".
- Spike commands below are the ones the work orders name (`bind`, `status`, `settime <epoch> hal`, `time`). I did not run the spike; if its command set differs, use what it prints and note it.
- A result that differs from "expect" is a finding for the architect, not a retry.

## 2. Item 1: binding present, v49 over a bound device
1. Flash the **spike**. Run `bind`, reboot, `status`: expect `bound=1`.
2. Flash **v49.1**.
3. Expect: the binding-deletion log line **once**; reset reason data 8; the vendored setup works in the same session; the cloud connects; no reset loop.
4. Reboot v49.1: expect **no** deletion line on the second boot.

## 3. Item 2: rollback v50 -> v49
1. Flash the **integration binary** and run until the chip is bound and the external watchdog is armed (the watchdog is initialised right after the bind; it is armed from boot).
2. Flash **v49.1** over it.
3. Watch the first 130 s of v49's boot: **expect no reset before v49's first watchdog service** (v49 services it at `Generalized-Core-Counter.cpp:~811` of its source; the chip's own dog is armed at 124 s). Record the time from flash to the first loop and any reset reason/data.
4. Expect v49 to delete the binding (reset data 8) and a single owner afterwards (vendored setup works; the spike's `status` shows unbound).
Note: the source does not bound this interval (Codex, integrated Stage 7 round 1); this item tests the normal case only. A rollback that lands during a boot storm is a **recorded residual for the readiness review** (see `2026-11-08-v50-readiness-review-notes.md`), not a bench case.

## 4. Item 3: v50 on a bound and an unbound device (includes S1b)

### 3a. The WRTC discrimination, on Dev-14 (old-library history, WRTC = 0)
**ORDER RULE: run the NO-S1B binary FIRST (expect `settime … hal` to FAIL on the v49-touched chip), THEN the INTEGRATED binary (expect it to SUCCEED and the time to be valid). The reverse order gives a FALSE PASS, because WRTC = 1 persists on the chip after S1b has run. A v49 reset between runs is only the fallback, not the plan.**

0. Before each run, check the WRTC state: flash the **spike** and run `settime <epoch> hal`. WRTC = 0 shows as a **failure** (the read-back catches the no-op); WRTC = 1 shows as success. The probe does not change WRTC. Before the first run it must **fail**; if it succeeds, the chip is not in the v49-touched state: use the fallback (flash v49.1, let it sync once, confirm WRTC reads 0 afterwards and a second v49 boot reads the chip as set, per S1b's rollback item) and start again.
1. Flash the **no-S1b binary**. Let it boot. Record: is `Time` valid at the first log lines; does the log show the bind; what the chip holds before any cloud sync.
2. Flash the **spike**, run the WRTC probe (step 0): expect **FAIL** (WRTC still 0).
3. Flash the **integration binary**. Let it boot.
4. Flash the **spike**, run the probe: expect **SUCCESS**, read-back within 2 s.
Record both outcomes. Expect the no-S1b run may leave the chip at the year-2000 internal clock (the bind copies it over the chip when nothing seeded it); that is the very defect S1b fixes, not a failure of this step.

### 3b. First v50 boot keeps valid time
Needs a chip with **WRTC = 0 and a good time** (the no-S1b run in 3a can leave a bad time, and the integration run sets WRTC): restore it with the fallback (flash v49.1, let it cloud-sync, confirm WRTC 0 with the probe and the time with `time`). Then flash the **integration binary** and read the chip time **before** the cloud sync: expect a valid time (not year 2000) and `Time` valid at the first log lines.

### 3c. Bound and unbound
- **Unbound** (after v49.1, which deletes any binding): the first integration boot **binds once**; later boots **do not re-enable**.
- **Bound** (spike `bind`, then the integration binary): no re-bind.
- The stored configuration is the fleet configuration (interrupt on WKP, watchdog pin `WATCHDOG_DONE_PIN`, internal-RC fallback allowed, EXTERNAL clock source). Record how you read it (log or spike `status`).
- Record the WRTC retry behaviour if the chip access ever fails (the log line `PreBindRtc: chip access failed`) and that `hal_i2c_reset()` is skipped on the first unbound boot (accepted in S1b; note any difference you see).

## 5. Item 4: failsafe stage 2 -> 3 and the error-state power-off (S3)
**Needs a `CONNECTIVITY_FAILSAFE_TEST_MODE=1` build, which is not in this kit (section 7).** What to record when you have it:
1. Stage 2 -> 3 (#151): watch the rail, the modem and the board during the power-off. **Measure the off time** (expect about 30 s; the pass band is +-5 s of the intended time). On USB power the rail may not collapse; record what you see.
2. The **reset reason after the wake** (expected 20, `PIN_RESET`; this is the open ruling-5 observation: if it is not 20, S2's classification table changes before release). Record `resetReasonData` and what the log prints (`power-off evidence - power-up wake` expected).
3. The RTC state after the wake (valid or lost) and the first-sync time.
4. The next status is a normal boot; the failsafe state clears as it does today.
5. Repeat for the ERROR_STATE hard recovery (the error-state power-off path).
6. S3 watchdog-window residual (accepted): watch for any reset between the external watchdog stop and the power cut; record if you see one. It is not a pass/fail item.

## 6. Item 5: two real 8 h nights, serial attached (integration binary)
- **No human contact during the night**: nobody presses the button, touches the wake pin or the board, or powers it. (Reason 30 covers RTC and GPIO wakes alike, so timing plus "no one near it" is the evidence; the raw AM18x5 STATUS at boot is **not** asked for, because the production image does not publish it.)
- Per wake, record from the hibernate event and serial: **requested versus actual** sleep (`req`, `actual`, `err`, by GCC's own method), the reset reason (expect 30), the teardown time before the sleep call, and the boot clock versus the first cloud sync.
- **Pass:** each wake within 5 s of the request, and the second night not early.
- An early wake with a button/GPIO cause would show as a short `actual` with reason 30 and a person nearby; with no one present it is a finding.

## 7. Gaps you need to decide (not in the four-binary kit)
- **Item 4 needs a `CONNECTIVITY_FAILSAFE_TEST_MODE=1` build** of the integrated head (the CI binary is FIELD). `docs/bench-validation.md` lines 40-56 give the flag. I can build one on a bench-only branch the same way as the no-S1b image, if you ask.
- **S2's watchdog proof** (a bench build that stops refreshing the external watchdog; the chip resets the board in about 124 s; the reset is attributed as `inferred`) needs a bench build and a small source change; not built.

## 8. Open observations to record (not pass/fail unless stated)
1. The reason after `POWER_OFF` (assumed 20): **must be confirmed before release**.
2. The rail and the 30 s off time: measured, not assumed; any change comes from this evidence.
3. Early button/GPIO wake: only if it occurs; compare requested versus actual.
4. The WRTC retry and the skipped `hal_i2c_reset()`.
5. The S3 watchdog-window residual.
