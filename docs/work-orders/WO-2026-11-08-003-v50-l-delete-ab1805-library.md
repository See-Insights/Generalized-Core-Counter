# WO-2026-11-08-003: L, delete the vendored `lib/AB1805_RK` and replace S1b's two library calls with raw I2C (v50.2)

**Author:** Claude Code, 8 Nov 2026, **DRAFT for the architect's review; not dispatched** · **Status:** DRAFT
**Branch (when dispatched):** `wo/…-l-delete-ab1805-library`, from `main` after v50.1 has merged (the S-set and the integrated fixes are then on `main`); ships in **v50.2** with I21. Never merge.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §10 (CI discovery); §12; §13 (c) · **Agents:** Stage 6 Copilot medium (the report states the level actually run); Stage 7 Codex `gpt-5.6-sol` **xhigh**, 90-minute box, at most two rounds; a failure goes to the architect.
**Inputs:** parent `WO-2026-11-05-002-v50-ab1805-switch.md` (v50.2 = L + I21); S1b `WO-2026-11-06-002-v50-s1b-pre-bind-rtc-access.md` (Amendment 2 item 5 is this WO's mandate); the integrated Stage 7 records `WO-2026-11-08-001-*`; `src/time/PreBindRtc.h` as merged.

## Plain goal
After the S-set, Device OS's `Am18x5` driver owns the AB1805. The vendored library survives only because S1b's pre-bind step calls three of its functions (`readRegister`, `writeRegister`, `getRtcAsTime`) and because the old `AB1805 ab1805(Wire)` object still exists. L removes the library (about 2,070 lines under `lib/AB1805_RK`) and the object, and replaces those three calls with a minimal raw I2C version that behaves **identically** at the chip, so the one remaining use of the vendored code is gone and there is a single owner of the chip for good. Nothing else changes.

## What must be preserved (S1b, Amendments 1-3; Codex-reviewed; not reopened)
1. **The lock:** the whole sequence runs under one `std::lock_guard<Wire>` (a single critical section), after `Wire.begin()` if `Wire` is not enabled.
2. **The transaction order**, exactly as the library produces it today: read CONTROL1 (0x10); if WRTC (bit 0) is set, write `c1 & ~0x01` so the time can be read; the time read (the library first re-reads CONTROL1 to gate on WRTC, then bursts 8 registers from 0x00); decode; `Time.setTime(chip)` only if `Clock::isPlausibleEpoch(chip)` and `!Time.isValid()`; then the final CONTROL1 write.
3. **The saved-CONTROL1 normalisation:** the final value is `(c1 | 0x01) & ~0x40`, computed from the **first** read (the saved `c1`, not a re-read), WRTC = 1 and 24-hour mode, every other bit unchanged; it is written even when the time read failed.
4. **The error aggregation:** the result is false if **any** step failed, built with non-short-circuit `&` (so the final write is attempted regardless); the caller logs `PreBindRtc: chip access failed` once.
5. **The skipped `hal_i2c_reset()`** (accepted in S1b) and the WRTC=1 persistence (the rollback expectation recorded in S1b) are unchanged.

## The change
1. **Delete `lib/AB1805_RK/`** (library, examples, README, LICENSE). Record the removal and the library's origin and licence in one line of the changelog/notice file that exists for third-party code (Step 0 names it).
2. **Remove the object and includes:** `AB1805 ab1805(Wire)` (`Generalized-Core-Counter.cpp:127`), `#include "AB1805_RK.h"` (`:47` and `src/state/StateMachine.h:4`), `extern AB1805 ab1805` (`StateMachine.h:31`), and the `app.ab1805` log category (`BuildProfile.h:271`, the library's logger).
3. **`src/time/PreBindRtc.h`:** `run(Wire)` (the `ab1805` parameter goes; the call at `Generalized-Core-Counter.cpp:294` changes to match) written with raw `beginTransmission`/`write`/`endTransmission(false)`/`requestFrom`/`read` at address 0x69, byte-for-byte the library's transactions: `readRegisters` = write the register, `endTransmission(false)`, `requestFrom(0x69, n, true)`, require `count == n`; `writeRegisters` = write the register and the value, `endTransmission(true)`, require 0. The time decode copies the library's `registersToTm` (BCD, `tm_year = bcd + 100`, `tm_mon = bcd - 1`) and `mktime` (the library's note that the C library is at +0000, so local equals UTC; Step 0 confirms on Device OS 6.6.0). No 12/24 decode (S1b: every chip is 24-hour).
4. **Comments to correct:** `Clock.cpp:83, 172, 341, 353` and `State_Common.h:225` cite `lib/AB1805_RK`; `docs/ARCHITECTURE_OVERVIEW.md:70,127` and `STYLE.md:29` name it. Reword to history ("the former vendored library"), no behaviour.

## Step 0 (read-only, inside Stage 6; stop and report if any answer breaks the plan)
1. **Re-cite** every line above on the merged `main`.
2. **Every other user of the library** (bare-name and header search, §13 (c)): any remaining `AB1805` call, type or transitive include (`AB1805_RK.h` pulls in `Particle.h`, `<time.h>`, others: list what a file loses when the include goes and fix with the real include, not by keeping the library). The linked footprint: `nm` the integrated image for `AB1805::` symbols; record text/data/bss before and expect it to fall.
3. **Tests that read or stub the library:** at least `tests/clock_resync_wiring_test.py` (lines 122-184 assert the library's `timeSet` latch is untouched), `tests/clock_hibernate_helpers_move_structural_test.py`, `tests/s2_alarm_watchdog_wake_structural_test.py`, `tests/rtc_skew_test.*`, `tests/clock_trust_test.cpp`, `tests/clock_rtc_writeback_test.cpp`, `tests/hibernate_wake_diagnostics_test.*`, `tests/s2_hibernate_normalise.py` and `tests/README.md`. For each: what it asserts about the library, and the replacement assertion ("the library and the object are absent", "no retired symbol") that keeps the intent. None is weakened without a ruling.
4. **Golden transaction traces (the equivalence proof).** Before deleting anything, with the library present, drive `PreBindRtc::run` (S1b's version) over a fake `Wire`/chip model for this case table and record, per case, the Wire call sequence, the return value, the final CONTROL1 value, and whether `Time.setTime` was called: WRTC set initially; WRTC clear initially; the 12/24 bit set initially; a NACK or short read at **each** transaction position (the first CONTROL1 read, the WRTC-clear write, the library's CONTROL1 re-read, the burst read, the final write); an implausible chip time; `Time` already valid; `Wire` not enabled at entry. Commit the traces as fixtures. The raw version must reproduce every trace; any difference is listed and goes to the architect. **Open point 1 below** (the CONTROL1 re-read) decides one structural difference.
5. **`mktime` and time zone** on Device OS 6.6.0: confirm the C library's local time is UTC at this point in `setup()` (the library's own comment says so), so the raw decode and the library give the same epoch for the same registers.
6. **Count** the net lines: `src/` and, separately, `lib/`.

## Tests (host; the real functions)
- The golden-trace table of Step 0 item 4, run against the raw `PreBindRtc::run` (the equivalence test), plus the BCD decode over 0..99 per field and the epoch for boundary dates (28/29 Feb, 31 Dec, year rollover 2099 -> 2100 is out of range for the chip: record, do not extend).
- The structural tests: one lock over the whole sequence; `Wire.begin()` only when not enabled; the final write uses the **saved** CONTROL1; the aggregation uses `&` not `&&` (so a failed read cannot skip the final write); the library, the object, the include and the log category are gone from `src/` and `lib/`.
- The rewritten tests of Step 0 item 3.
- **Mutations (each must fail; back up the file, mutate, restore from the backup with cp; never `git checkout`/`git restore` while your work is uncommitted):** lock removed; lock released before the final write; final CONTROL1 value computed from a re-read; `|0x40` instead of `& ~0x40` (24-hour bit not cleared); final write skipped when the time read fails; `&&` instead of `&`; WRTC-clear write skipped; time read before the WRTC clear; plausibility check removed; `Time.isValid()` guard removed; BCD year offset wrong; month `-1` dropped; `endTransmission(true)` for the register-select write (it must be `false` for a read).

## Bench (Chip; after L is on a bench unit)
1. **Repeat bench item 3 of the v50.1 checklist** (3a, 3b, 3c) with the L binary on Dev-14: the WRTC discrimination (`settime <epoch> hal` succeeds after the L boot on a WRTC = 0 chip), the first boot keeps valid time, binds once. The L binary must give the same observations as the v50.1 integration binary; any difference is a finding.
2. **One 8 h night** with serial attached as a smoke test (L touches neither sleep nor the watchdog, so a full two-night soak is not proposed; the architect may rule otherwise).
3. Bind/unbind and rollback to v49.1 as in items 1 and 2 (the library is gone, v49.1 brings its own).

## Budget
`src/`: the raw version replaces about 12 lines with roughly 35-40, and 4-5 lines of declarations go, so **`src/` is expected at about +20 net, not negative** (stated here so the number is not a surprise). **`lib/`: about -2,070 lines.** The repository net is about -2,050 and the linked image is expected to shrink. **Stop and report if `src/` exceeds +30 net**, rather than trim style. The "strongly negative" claim in the parent plan holds for the repository, not for `src/` alone.

## Protected / not in scope
S1 (the bind), S1b's semantics (this WO only re-expresses them), S2, S3, S4, I21 (which also ships in v50.2 and must not conflict: it reads the S4 latch and `Clock`, not the library), the clock-trust rule, the failsafe ladder, the persisted and `retained` layouts, the Device OS `Am18x5` driver.

## Rollback
Revert the L commit: the library, the object and the three-call `PreBindRtc` return (v50.1 behaviour). v50.1 remains the floor.

## Open points for the architect
1. **The CONTROL1 re-read inside the library's time read.** `getRtcAsTm` re-reads CONTROL1 to gate on WRTC (one extra transaction, `AB1805_RK.cpp:313-325`). Recommended: **keep it** in the raw version, so the trace is identical and the outcome when the WRTC-clear write failed (the gate sees WRTC still set, the time read fails) is unchanged. Dropping it saves one transaction and about 3 lines but changes a failure-path outcome; it would need a ruling and a changed trace.
2. **Delete the whole directory,** including `examples/` and `LICENSE`? Recommended: yes, with a one-line origin/licence note.
3. **L's bench:** one smoke night (proposed) or none or the full two? 
4. **Stage 7 focus** (proposed): the trace equivalence and the four preserved properties; chip ownership (unchanged, but re-walk the boot sequence once); no retired symbol; the changed tests are not weakened.
