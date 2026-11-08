# v50.1 pre-ship bench: step-by-step checklist for 14 Nov 2026

For Chip. Parent: `WO-2026-11-05-002-v50-ab1805-switch.md` ("Pre-ship bench", five items), on `integration/v50-ab1805` (not on `main`). The integrated head is final: PR #208 merged, Stage 7 round 2 PASS on `cf09b80` (no findings). Nothing below has been run on hardware; every result is yours to record. Times in UTC.

## 0. The kit

| Role | File | Source | SHA-256 |
|---|---|---|---|
| Integration binary (product version 5001, `v50.1-NativeRTC`) | `bench-kit/integration-5001/firmware-boron-6.6.0.bin` | `integration/v50-ab1805` @ `1d1d5af7a22dc47094a4c23b515b348bd8ae153d`, tree `0c6cbcccc27cc0e0c6c37d320136eb8d54a13526`, built by CI run 42490341544 (artifact `firmware-boron`, FIELD build: `DEV_BUILD=0`, `CONNECTIVITY_FAILSAFE_TEST_MODE=0`) | `1aafa89417e65d6a719a9e8ae3af478d904821718cffe552f2084d96cafca7b5` |
| **No-S1b binary: BENCH ONLY, NEVER RELEASED** (product version 5099, `v50.1-NativeRTC-NOS1B-BENCH-ONLY`) | `bench-kit/no-s1b-5099/firmware-boron-6.6.0.bin` | branch `bench/2026-11-08-no-s1b-NEVER-RELEASE` @ `729ff14b0e0e05f9ce3f123fbe9638930ee8e6ce`, tree `c546fd767f430a3f8f8c05eb52481f24692a12bd`, CI run 42490763093 | `8659156e7fb920baded75a01d6cb54179697712d2ddf2f663579f742a78d8f01` |
| **Test-mode image: BENCH ONLY, NEVER RELEASED** (product version 5098, `v50.1-NativeRTC-TESTMODE-BENCH-ONLY`; `CONNECTIVITY_FAILSAFE_TEST_MODE=1`) | `bench-kit/testmode-5098/firmware-boron-6.6.0.bin` | branch `bench/2026-11-08-testmode-5098-NEVER-RELEASE` @ `2c38e4a298dbd61a23a29363725a26714e75c380`, tree `4245d99f901f8c1a5a91933c831b3a0cd700aa17`, CI run 42498221344 | `e08e6541963144f734bae1dd5c7d023fbdb10ac92ecba4251db03d27a17fd5f8` |
| **No-refresh image: BENCH ONLY, NEVER RELEASED** (product version 5097, `v50.1-NativeRTC-NOREFRESH-BENCH-ONLY`) | `bench-kit/norefresh-5097/firmware-boron-6.6.0.bin` | branch `bench/2026-11-08-norefresh-5097-NEVER-RELEASE` @ `daa10ed5a9a16e198c408d337e4e938b3a34c9d6`, tree `6cdbacf16f3d71e9bc55c2dd526a51aaace31925`, CI run 42498226470 | `a3790858f16e3549635a8f09dac7e81d3be1d88fe3394a0e545a598244c166fc` |
| Spike image | `bench-kit/spike/ab1805-spike.bin` (copy of `~/Downloads/spike-2026-11-03/ab1805-spike.bin`) | the 3 Nov spike | `029e8b6b23eff443e6ac45163b8643d11979e8cd390d93527ad3d84a12dba92d` |
| Rollback image v49.1 (product version 4901) | `bench-kit/rollback-v49.1/gcc-v49.1-boron-dos6.6.0-9a96854.bin` | GitHub release `v49.1` (Device OS 6.6.0) | `a00c5ceaa939aa274346d52e0cea9477c01a27c37aa1e68829bbc4bb8c145ec6` (matches the release's `.sha256`) |

The kit directory is `~/Downloads/v50-bench-kit-2026-11-14/` (with `SHA256SUMS.txt`). Verify each SHA-256 (`shasum -a 256`) before flashing.

**The no-S1b binary** is the integrated head with exactly one behavioural change: the `PreBindRtc::run(ab1805, Wire)` call in `setup()` is removed (S1b's pre-bind seed and WRTC set). Two further lines differ only so the image cannot be mistaken for a release: the product version (5001 -> 5099) and the version string. `git diff 1d1d5af 729ff14 --numstat`: `src/FirmwareVersion.h` 2/2, `src/Generalized-Core-Counter.cpp` 0/1. It is built on a branch that is never merged or tagged; CI's `host-tests` fail on that branch by design (S1b's structural tests require the call), and its `firmware` job passes. Do not OTA it, do not set it as a product default, do not upload it to the console.

Both CI binaries were downloaded from the run artifacts (`firmware-boron`, which also holds `target/workspace.elf`) and hashed locally; the CI log prints only the artifact zip digest, so compare the `.bin` hash with the run's job summary if you want a second source. Binary check (version strings via `strings`): the test-mode image carries `v50.1-NativeRTC-TESTMODE-BENCH-ONLY` and the `test=1` failsafe log literals (absent from the no-refresh image); the no-refresh image carries `v50.1-NativeRTC-NOREFRESH-BENCH-ONLY`. The integration image contains the string `v50.1-NativeRTC` and the `PreBindRtc: chip access failed` log text; the no-S1b image contains `v50.1-NativeRTC-NOS1B-BENCH-ONLY` and does not contain that log text.

**The test-mode image** is the integrated head with one line changed: the default in `BuildProfile.h` is `CONNECTIVITY_FAILSAFE_TEST_MODE 1` (stale 5 min, cooldown 15 min, jitter 0, closed-hours sleep cap 60 s), plus the version int/string (`git diff 1d1d5af 2c38e4a --numstat`: `src/BuildProfile.h` 1/1, `src/FirmwareVersion.h` 2/2). **The no-refresh image** is the integrated head with the single `ExternalWatchdog.refresh()` call in the main loop removed, plus the version int/string (`git diff 1d1d5af daa10ed --numstat`: `src/Generalized-Core-Counter.cpp` 0/1, `src/FirmwareVersion.h` 2/2). Both are built on branches that are never merged or tagged; their `host-tests` may fail by design, and only their `firmware` job matters. Do not OTA any BENCH ONLY image, do not set one as a product default, do not upload it to the console.

## 1. Ground rules
- Dev-14 first, then Dev-09. Serial attached for every step; keep the logs (UTC).
- Record, never infer: write the observed value, or "not observed".
- Spike commands come from its own help line: `help status time | p7 [sec] | bind unbind | settime <epoch> <ext|hal> | skew <sec> | p3 | wdt <sec> wdtstop | p5 p5abort res | reboot`. I did not run the spike.
- A result that differs from "expect" is a finding for the architect, not a retry.

## 2. Item 1: binding present, v49 over a bound device
1. Flash the **spike**. Run `bind`, `reboot`, `status`: expect `bound=1`.
2. Flash **v49.1**.
3. Expect: the binding-deletion log line **once**; reset reason data 8; the vendored setup works in the same session; the cloud connects; no reset loop.
4. Reboot v49.1: expect **no** deletion line on the second boot.

## 3. Item 2: rollback v50 -> v49
Two variants; run both on Dev-14, record each.
**A. Normal case (integration binary).**
1. Flash the **integration binary** and run until the chip is bound and the external watchdog is armed (it is initialised right after the bind, and refreshed every loop pass).
2. Flash **v49.1** over it.
3. Watch the first 130 s of v49's boot: **expect no reset before v49's first watchdog service** (`Generalized-Core-Counter.cpp:~811` of v49; the chip's dog is armed at 124 s). Record the time from flash to the first loop and any reset reason/data.
4. Expect v49 to delete the binding (reset data 8) and a single owner afterwards (vendored setup works; the spike's `status` shows unbound).
**B. Armed-watchdog source (no-refresh image, worst case).** Nothing refreshes the dog on this image, so it is armed and counting from the moment of its init, never topped up.
1. Flash the **no-refresh image (5097)**; note the boot time where the external watchdog is initialised.
2. Flash **v49.1** at once and record the elapsed seconds from that arm point to v49's first watchdog service. Same pass criterion: no reset before v49's first service. If the dog fires first, record the elapsed time: with this image a reset at about 124 s after arming is expected by construction, so it says the flash plus the v49 boot outlasted the window, not by itself a defect; a reset well before 124 s is a finding.
Note: the source does not bound this interval (Codex, integrated Stage 7 round 1); these variants test the normal case only. A rollback that lands during a boot storm is a **recorded residual for the readiness review** (`2026-11-08-v50-readiness-review-notes.md`), not a bench case.

## 4. Item 3: v50 on a bound and an unbound device (includes S1b)

### 3a. The WRTC discrimination, on Dev-14 (old-library history, WRTC = 0)
**ORDER RULE: run the NO-S1B binary FIRST (expect `settime … hal` to FAIL on the v49-touched chip), THEN the INTEGRATED binary (expect it to SUCCEED and the time to be valid). The reverse order gives a FALSE PASS, because WRTC = 1 persists on the chip after S1b has run. A v49 reset between runs is only the fallback, not the plan.**

0. Before each run, check the WRTC state: flash the **spike** and run `settime <epoch> hal` (then `time` to see the system, EXTERNAL and hal readings). WRTC = 0 shows as a **failure** (the read-back catches the no-op); WRTC = 1 shows as success. The probe does not change WRTC. Before the first run it must **fail**; if it succeeds, the chip is not in the v49-touched state: use the fallback (flash v49.1, let it sync once, confirm WRTC reads 0 afterwards and a second v49 boot reads the chip as set, per S1b's rollback item) and start again.
1. Flash the **no-S1b binary**. Let it boot. Record: is `Time` valid at the first log lines; does the log show the bind; what the chip holds before any cloud sync.
2. Flash the **spike**, run the WRTC probe (step 0): expect **FAIL** (WRTC still 0).
3. Flash the **integration binary**. Let it boot.
4. Flash the **spike**, run the probe: expect **SUCCESS**, read-back within 2 s.
Record both outcomes. Expect the no-S1b run may leave the chip at the year-2000 internal clock (the bind copies it over the chip when nothing seeded it); that is the very defect S1b fixes, not a failure of this step.

### 3b. First v50 boot keeps valid time
Needs a chip with **WRTC = 0 and a good time** (the no-S1b run in 3a can leave a bad time, and the integration run sets WRTC): restore it with the fallback (flash v49.1, let it cloud-sync, confirm WRTC 0 with the probe and the time with `time`). Then flash the **integration binary** and read the chip time **before** the cloud sync: expect a valid time (not year 2000) and `Time` valid at the first log lines.

### 3c. Bound and unbound
- **Unbound** (after v49.1, which deletes any binding; the spike's `unbind` also unbinds): the first integration boot **binds once**; later boots **do not re-enable**.
- **Bound** (spike `bind`, then the integration binary): no re-bind.
- The stored configuration is the fleet configuration (interrupt on WKP, watchdog pin `WATCHDOG_DONE_PIN`, internal-RC fallback allowed, EXTERNAL clock source). Record how you read it (log or spike `status`).
- Record the WRTC retry behaviour if the chip access ever fails (the log line `PreBindRtc: chip access failed`) and that `hal_i2c_reset()` is skipped on the first unbound boot (accepted in S1b; note any difference you see).

## 5. Item 4: failsafe stage 2 -> 3 and the error-state power-off (S3)
Use the **test-mode image (5098)**. Confirm it is the test build first: the boot log has `FailsafeTest=1` and `Failsafe: test=1 stale=300s cooldown=900s jitter=0s`, and the startup status shows `"failsafeTest":1` (`docs/bench-validation.md`). The ladder is spaced by the 15 min cooldown after a 5 min stale threshold, so allow well over an hour per run and record the actual times. Break connectivity the way you did for #151.
1. Stage 2 -> 3 (#151): watch the rail, the modem and the board during the power-off. **Measure the off time** (expect about 30 s; pass band +-5 s of the intended time). On USB power the rail may not collapse; record what you see.
2. The **reset reason after the wake** (expected 20, `PIN_RESET`; this is the open ruling-5 observation: if it is not 20, S2's classification table changes before release). Record `resetReasonData` and the log line (`PIN_RESET: power-off evidence - power-up wake` expected).
3. The RTC state after the wake (valid or lost) and the first-sync time.
4. The next status is a normal boot; the failsafe state clears as it does today.
5. Repeat for the ERROR_STATE hard recovery (the error-state power-off path).
6. S3 watchdog-window residual (accepted): watch for any reset between the external watchdog stop and the power cut; record if you see one. It is not a pass/fail item.
Test mode also changes sleep behaviour (`SleepWarn: HIB returned fallback=ULP test=1` can appear); do not use this image for item 5.

### S2 watchdog proof (required pre-ship item, S2 WO) and the boot-storm bracketing
Use the **no-refresh image (5097)**.
1. Flash it and let it boot. With nothing refreshing the external watchdog, expect the chip to reset the board about **124 s** after the arm point; record the time from the arm log to the reset.
2. Expect the reset to be attributed as the `inferred` watchdog: the boot log `PIN_RESET: no power-off evidence - inferred watchdog` and the watchdog forensics fields in the status. Record them.
3. Left running, the image resets about every 124 s. After **six early resets** (if the firmware counts them as early under its own rule; record whether it does) expect the `BOOT STORM: 6 early resets detected` log and the 600 s holdoff sleep. **That exercises the boot-storm bracketing:** the external watchdog is stopped around the storm sleep, so the chip must **not** reset the board during the ~600 s; record the time between the storm log and the next boot (about 600 s expected) and that the watchdog is restarted afterwards (the next reset again about 124 s after the restart).
4. Reflash the **integration binary** to end the test.

## 6. Item 5: two real 8 h nights, serial attached (integration binary)
- **No human contact during the night**: nobody presses the button, touches the wake pin or the board, or powers it. (Reason 30 covers RTC and GPIO wakes alike, so timing plus "no one near it" is the evidence; the raw AM18x5 STATUS at boot is **not** asked for, because the production image does not publish it.)
- Per wake, record from the hibernate event and serial: **requested versus actual** sleep (`req`, `actual`, `err`, by GCC's own method), the reset reason (expect 30), the teardown time before the sleep call, and the boot clock versus the first cloud sync.
- **Pass:** each wake within 5 s of the request, and the second night not early.
- An early wake with a button/GPIO cause would show as a short `actual` with reason 30 and a person nearby; with no one present it is a finding.

## 7. Rulings applied (architect, 8 Nov)
Both missing images are in the kit as bench-only builds (test-mode 5098 for item 4; no-refresh 5097 for the S2 watchdog proof and the armed-watchdog source for item 2). Item 3's split into a WRTC pass and a valid-time pass is accepted. The spike commands are those of its own help line.

## 8. Open observations to record (not pass/fail unless stated)
1. The reason after `POWER_OFF` (assumed 20): **must be confirmed before release**.
2. The rail and the 30 s off time: measured, not assumed; any change comes from this evidence.
3. Early button/GPIO wake: only if it occurs; compare requested versus actual.
4. The WRTC retry and the skipped `hal_i2c_reset()`.
5. The S3 watchdog-window residual.
