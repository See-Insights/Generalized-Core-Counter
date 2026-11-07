# WO-2026-11-06-002 (S1b) Stage 6 round 2 report: implemented per Amendment 2

**Model:** claude-sonnet-5.5. The reasoning level actually run could not be observed from inside the session (medium was dispatched; I cannot confirm it). **Interpreter:** python3 (3.14, Homebrew) for `.py`, zsh for `.sh`, `c++ -std=c++17` for host C++. Uncommitted; nothing committed, pushed or flashed.

## Step 0 (file:line; Device OS 6.6.0 at `~/.particle/toolchains/deviceOS/6.6.0`, library at `lib/AB1805_RK/src`)
1. **Wire / I2C window.** Before `ensureBound` `setup()` runs only `validateRetainedBoot` and `waitForDebugSerialIfConnected`; nothing else touches the bus. `Wire.begin()` is not called by the app before that, so the header guards with `isEnabled()` (`wiring/src/spark_wiring_i2c.cpp:203`) then `begin()` (`:64-67`). The Wire lock is recursive (`hal/src/nRF52840/i2c_hal.cpp:331-333`), so the library's own `wire.lock()` (`AB1805_RK.cpp:715`) nests safely under the header's lock. No conflict with Device OS: `enableInterfaceIfNeeded()` (`hal/shared/am18x5.cpp:1323-1329`) and `Am18x5::begin()` (`:490-494`) only call `hal_i2c_begin` when the interface is not enabled.
2. **Amendment 2 library calls work without `ab1805.setup()`:** `AB1805::AB1805` only stores `wire`/`i2cAddr` (`AB1805_RK.cpp:14-16`; global `ab1805` at `Generalized-Core-Counter.cpp:126`); `getRtcAsTime`/`getRtcAsTm` (`:297-330`), `readRegister(s)` (`:707-747`) and `writeRegister(s)` (`:757-790`) use only `wire`, `i2cAddr` and the lock; none check setup state. **One gate:** `getRtcAsTm` reads the time only if WRTC is clear (`:318`, `isRTCSet()` at `AB1805_RK.h:97`). A chip already at WRTC=1 (cold, or after S1b has run once) returns false and is not seeded; the CONTROL1 write still happens. This is the accepted v49-chip case (WRTC=0 = valid time); it means a second unbound boot (bind failed once) does not re-seed. Not a break.
3. **Seeding.** `Time.setTime()` (`spark_wiring_time.cpp:284-296`) with no chip bound falls to the internal clock (`exrtc_hal.cpp:412-417`, `rtc_hal.cpp:116-130`); the bind reads the internal clock after `bind()` and writes it to the chip (`exrtc_hal.cpp:236-249`), so a pre-`enable()` seed is written back. Only a time-changed notification, as for any manual `setTime`.
4. **CONTROL1 RMW.** `CONTROL1_WRTC_MASK` 0x01, `CONTROL1_1224_MASK` 0x40 (`am18x5_defines.h:67`); `(v|0x01)&~0x40` is idempotent and keeps every other bit; the bind later only changes ARST (`am18x5.cpp:616`).
5. **Rollback to v49.** `AB1805_RK.cpp:33`-`:61`: with WRTC=1 v49 skips the seed, waits for the cloud, then `setRtcFromTime` rewrites the time and clears WRTC. No reset path, no bad seed.
6. **Count:** +14 net (below).
Accepted per Amendment 2 item 4: `hal_i2c_reset()` (`am18x5.cpp:497`, only when Wire is not enabled) is skipped on the first unbound boot because we enable Wire first. Bench should note it.

## Change
- `src/time/PreBindRtc.h` (new, 12 lines): `PreBindRtc::run(ab1805, Wire)`: `std::lock_guard<W>` (the `WITH_LOCK` macro's `decltype` breaks in a template), `begin()` if not enabled, `getRtcAsTime`, seed `Time` iff `Clock::isPlausibleEpoch` and `!Time.isValid()`, then CONTROL1 `(v|0x01)&~0x40` via `readRegister`/`writeRegister`; returns false on I2C failure.
- `Generalized-Core-Counter.cpp`: include + one gated call before `ensureBound` (`if (!Am18x5.status().bound() && !PreBindRtc::run(ab1805, Wire)) Log.warn(...)`): failure logs, boot continues, no reset.
- Comment corrections (line-neutral): `MyPersistentData.h:631`, `persist/CurrentReadings.h:47`, `Generalized-Core-Counter.cpp:655`.

**`git diff --numstat -- src`:** Generalized-Core-Counter.cpp +3/-1 (net +2), MyPersistentData.h +1/-1, CurrentReadings.h +1/-1, PreBindRtc.h +12 (untracked). **Net +14 ≤ +15.** No layout, `retained` or fixture change.

## Tests (`tests/pre_bind_rtc_test.cpp` + `.py`; real header and the real `isPlausibleEpoch` body extracted from `Clock.cpp`)
Cases: CONTROL1 property over all 256 values (`0x12→0x13`, `0x53→0x13`, register 0x10 for read and write); seed (plausible + `Time` invalid seeds; valid not seeded; year 2000 and 0 not seeded; chip time unreadable still sets WRTC); I2C failure returns false with no exit; event order `lock, begin, time, setTime, read, write, unlock`; no `begin` when Wire enabled. Structural: call gated on `!Am18x5.status().bound()`, exactly once, before `ensureBound`; no reset in the failure path; lock before every library call. Mutations (applied to in-memory copies, so no source file was edited or needs restoring); all 17 caught: WRTC write clears other bits, seed when `Time` valid, skip plausibility, wrong register (read and write), reset on I2C failure, 12/24 ignored, 12/24 not cleared, lock not held, lock released before the RMW, no `Wire.begin()`, `begin()` always, no WRTC write, run on bound boot, run after `enable()`, reset on failure at call site, never run. (First run showed two gaps, wrong-read-register and reset-at-call-site; the test was fixed and re-run clean.) "Wrong I2C address" is covered as wrong register: the address is the library's constructor default and the header names none.
Decoder/12-24 cases dropped per Amendment 2.
Existing allowlists extended for the new header (as S1 did for `ExternalRtc.h`): `clock_owner_structural_test.py` (single `isPlausibleEpoch` caller now also exempts `PreBindRtc.h`), and the src-file-list checks in `sleep_{cloud_gate,execution,plan,preflight,wake}_structural_test.py`.

## Verify
- Host suite **117/117** (zsh/python3, including the new test). `thermal_coupling_structural_test.py` also passes on a `git archive HEAD` export.
- Build: Device OS 6.6.0, Boron, gcc-arm 10.2.1, buildscripts 1.17.2, fresh `BUILD_PATH_BASE=build-tmp/wo-2026-11-06-002-stage6-r2/bpb` as a make argument: compiled; text/data/bss **153052 / 1114 / 2240** (S1 head baseline not rebuilt here).
- `nm -C`: `PreBindRtc::run<AB1805, TwoWire>` (b5d88), `AB1805::getRtcAsTime`, `readRegister`, `writeRegister` present. `objdump setup()`: `ExRtcBase::status()` (b5ee8) → `bl PreBindRtc::run` (b5efa) → `status()` → `ExRtcT::enable` (b5f56): in that order.
- Build output went to the git-ignored `target/6.6.0/boron/wt-s1b.*` (left in place; I created it). Scratch `build-tmp/wo-2026-11-06-002-stage6-r2/` holds `bpb`, `build.log` and `export`.

## Open questions
- A chip at WRTC=1 is never seeded (library gate); after S1b once sets WRTC=1 on a boot whose bind then failed, the next boot has no seed. Worth a line in the bench; no code change proposed.
- L must replace the two library calls with raw I2C (comment in the header).
- Bench (Dev-14, `settime ... hal`) is Chip's step; nothing claimed.
