# WO-2026-11-06-002: v50 piece S1b — pre-bind access to the AB1805: seed the clock, set WRTC

**Author:** Claude Code, 6 Nov 2026, DRAFT for Chip's review; not dispatched · **Status:** DRAFT (rulings of 6 Nov, Chip, after Codex's Stage 7 round 1 on S1)
**Parent:** `WO-2026-11-05-002-v50-ab1805-switch.md` · **Origin:** `WO-2026-11-05-003-stage7-r1-verdict.md` findings 1 and 2; S1 Amendment 4 · **Position:** on `integration/v50-ab1805` **between S1 and S2** (S1 → S1b → S2 → S3, S4)
**Branch (when dispatched):** `wo/…-v50-s1b-…`, from `integration/v50-ab1805` after S1 has passed Stage 7 round 2 and its PR has been merged to the integration branch; S1b's PR goes to the integration branch, **never to `main` alone**; the set merges to `main` only under the 6 Nov merge gate.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule; the 90-minute Stage 7 time-box once #194 merges); §5 (boot-time hardware and time: Codex xhigh); §12; §13 (c), (j) · **Agents:** Stage 6 Copilot standard/medium (re-run at the dispatched level if a run comes back lower on anything larger than a few lines); Stage 7 Codex `gpt-5.6-sol` **xhigh**, at most two rounds · **Budget:** `src/` at most **+15 net** (removals count); stop and report rather than trim style.
**Dispatch gate:** S1 reviewed and merged to the integration branch (it needs S1's bind call to sit just after this code).

## Plain goal
Two defects found in Device OS 6.6.0's built-in AM18x5 driver by S1's Stage 6 and Stage 7 round 1 make the switch unsafe on the real fleet:
1. **Writes can silently no-op (WRTC).** The chip only accepts writes to its time registers while the control-register write-enable bit **WRTC** is 1. Device OS's `Am18x5::setTime()` never sets it and `writeContinuousRegisters()` ignores every I2C result (`am18x5.cpp:770-788`, `:1350-1359`), so a chip whose WRTC is 0 returns success and keeps its old time. **Every chip the vendored library has touched has WRTC = 0:** the library sets WRTC, writes, then clears it (`lib/AB1805_RK/src/AB1805_RK.cpp:256-295`; `isRTCSet()` is literally "WRTC is clear", `AB1805_RK.h:97`). S1's read-back (Amendment 3) detects the failure; it cannot make the write work.
2. **The first unbound boot throws the chip time away.** On the first v50 boot after v49 (no stored binding), Device OS's bind copies the internal clock (year 2000, nothing has seeded it) into the chip and never reads the valid chip time into the internal clock (`exrtc_hal.cpp:236-249`). Once WRTC works that overwrite takes effect.

S1b fixes both from the application side, in the one window where the application is the only owner of the chip: **before `Am18x5.enable()`, on a boot where the chip is not bound, one raw I2C access to address 0x69.** (A Device OS issue is drafted for Chip to file; if Device OS fixes it later this piece becomes unnecessary but is harmless.)

## Rulings that bind this piece (Chip, 6 Nov; not reopened)
1. **Where and when:** on a boot where the chip is **not bound**, strictly **before** `Am18x5.enable()` (S1's `ExternalRtc::ensureBound`, `Generalized-Core-Counter.cpp:~296`); never on a bound boot (Device OS owns the chip then). Single owner: nothing else touches the chip at that moment (`ab1805` is not started: S1 removed `.setup()`).
2. **(a) Seed:** read the chip's time; **if it is plausible and system `Time` is not valid, seed `Time` from it.**
3. **(b) WRTC:** **set WRTC = 1** (the chip's power-on default; Device OS never clears it).
4. **Then S1's `enable()`.** S1's read-back stays as the detector.
5. **First unbound boot:** with (a), the internal clock is valid before the bind, so the bind's copy writes the same time back (within rounding) and the first v50 boot keeps valid time. (This reverses S1 Amendment 3 ruling 2.)

## The change (lines at the integration head after S1; **re-cite at Step 0**)
1. A small pure, host-testable decoder (BCD registers 0x00-0x07 → `time_t`, honouring the chip's 12/24-hour bit; year 2000-2099) and a read-modify-write of CONTROL1 (set bit 0 only, keep the other bits). A new header is allowed if the budget holds (S1 added `time/ExternalRtc.h` the same way).
2. In `setup()`, immediately before S1's `ensureBound`: `if (!Am18x5.status().bound()) { PreBindRtc::run(...); }` with failures **logged and the boot continued** (never a reset; the bind and the internal-clock fallback still happen).
3. Plausibility is `Clock::isPlausibleEpoch` (`Clock.cpp:551-555`) or the equivalent already in the tree; no new rule.
4. Nothing else: no change to the write-back, the payload, the failsafe, `retained` or persisted layouts.

Register facts (Device OS 6.6.0 headers; Step 0 re-verifies): `HUNDREDTHS` at 0x00 (`am18x5.h:39`), `CONTROL1` at 0x10 (`am18x5.h:57`), `CONTROL1_WRTC_MASK` 0x01 (`am18x5_defines.h:67`; the only use of WRTC in the driver), `CONTROL1_1224_MASK` 0x40; the vendored library's `REG_CTRL_1_DEFAULT` is 0x13 (WRTC set), `AB1805_RK.h:795`.

## Step 0 (read-only, inside Stage 6, before editing; stop and report if any answer breaks the plan)
1. **The I2C window.** Who owns `Wire` at that point in `setup()` (the fuel gauge and PMIC share the bus); whether `Wire.begin()` has run and `WITH_LOCK(Wire)` is needed; whether Device OS's own `enableInterfaceIfNeeded()` (`am18x5.cpp:~1352`) conflicts with an application `Wire` session that ends before `enable()`.
2. **Decode details:** the time-register layout and the BCD/hour-mode rules (`am18x5.cpp:770-788` is the write side; read the `getTime` side `:791-814`); behaviour on an invalid BCD digit or a chip reading year 00 (not plausible: do not seed).
3. **Seeding:** `Time.setTime()` versus the HAL clock Device OS seeds; whether setting `Time` before `enable()` is seen by `bindIfNeededAndResync` (it must be, so the bind writes the right time back); no cloud-sync or event side effect of an early `Time.setTime()`.
4. **WRTC set when already 1** is a no-op; confirm the read-modify-write does not disturb the other CONTROL1 bits (STOP, 12/24, OUT, RSO, PWR2).
5. **Rollback (added by Claude Code, not ruled):** after v50 has set WRTC = 1, the chip reads "not set" to the vendored library's `isRTCSet()` (WRTC clear = set). State what v49's `ab1805.setup()` and the clock code do on the first v49 boot after a rollback (it should treat the chip as unset and take time from the cloud; confirm there is no reset loop or bad seed), and put the answer into the rollback bench (parent pre-ship item 2).
6. **Count** the net `src/` lines per file; if over +15, stop and report the cut.

## Tests (host; the real functions)
- Decoder table: valid 24 h and 12 h (AM/PM) times, year rollover, leap day, an invalid BCD digit, year 00, all-zero and all-FF registers → not plausible.
- Seed: plausible chip time and `Time` invalid → seeded; `Time` already valid → not seeded; implausible chip time → not seeded.
- WRTC: CONTROL1 `0x12` → writes `0x13`; `0x13` → no change or the same value; other bits preserved (property over all 256 values).
- Control flow: not run when the chip is bound; runs once, **before** `enable()` (structural, in order); an I2C failure logs and the boot continues to `enable()` with no reset.
- Mutations (each must fail): WRTC write clears other bits; seed even when `Time` is valid; run on a bound boot; run after `enable()`; skip the plausibility check; wrong I2C address; reset on I2C failure; the 12/24 bit ignored.

## Bench (Chip; part of the integrated head's bench, not alone)
On **Dev-14 (old-library history: WRTC = 0)** with the spike binary's `settime <epoch> hal`:
1. **Without S1b** (S1 only): `settime … hal` fails (the read-back catches the no-op) or the chip keeps its old time; record it.
2. **With S1b:** the same `settime … hal` succeeds and reads back within 2 s.
3. **First v50 boot after v49** (no stored binding): the chip keeps valid time through the bind (read it before the cloud sync: not year 2000), and `Time` is valid at the first log lines.
4. **Rollback v50 → v49** observation from Step 0 item 5 (what the vendored `isRTCSet()` sees, and the first v49 boot).

## Protected / not in scope
Device OS source; S1 (the read-back, the bind), S2 (alarm, watchdog, wake classification, `ab1805.loop()`), S3, S4, L, I21; the clock-trust rule; the persisted and `retained` layouts.

## Rollback
Revert the S1b commit on the integration branch before the set merges. After the set ships, rollback is to v49.1 (the guard deletes any stored binding); Step 0 item 5 and bench item 4 cover the WRTC state it leaves.
