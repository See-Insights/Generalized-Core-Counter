# WO-2026-11-06-002 (S1b) Stage 6 round 3 report: Amendment 3 (Stage 7 round 1 fixes)

**Model:** claude-sonnet-5.5. The reasoning level actually run could not be observed from inside the session (medium was dispatched; I cannot confirm it). **Interpreter:** python3 for `.py`, zsh for `.sh`, `c++ -std=c++17` for host C++. Uncommitted; nothing committed, pushed or flashed.

## Change
- `src/time/PreBindRtc.h` (15 lines), under the existing Wire lock: read CONTROL1 -> if WRTC is set, clear only WRTC -> `getRtcAsTime` -> seed `Time` iff plausible and `!Time.isValid()` (rule unchanged) -> always write `(c1 | 0x01) & ~0x40` -> return false if any transaction (CONTROL1 read, clear, time read, final write) failed. The final write is unconditional on the time read (finding 2) and joined with bitwise `&` so it is never short-circuited. If the CONTROL1 read itself fails there is no saved value, so no final write is attempted (writing a guessed value would clobber the register) and it returns false.
- Comment text only, line-neutral: `Generalized-Core-Counter.cpp:657` and `:667`, `MyPersistentData.h:631`, `persist/CurrentReadings.h:47` now say seeding happens only on a successful plausible pre-bind read.
- Vendored library behaviour relied on: the time read returns false while WRTC is set (`AB1805_RK.cpp:317-320`), so clearing WRTC first is what makes it read; the test fake models this.

**`git diff --numstat d6da67c -- src`:** Generalized-Core-Counter.cpp +4/-2, MyPersistentData.h +1/-1, CurrentReadings.h +1/-1, PreBindRtc.h +15 -> **net +17 (budget +17)**. No `lib/`, layout, `retained` or allowlist change.

## Tests (`tests/pre_bind_rtc_test.cpp` + `.py`)
Added: WRTC=1 with valid chip time (event order `lock, read, write, time, setTime, write, unlock`, seeded, CONTROL1 ends `0x13`); retry (second unbound boot after a run that left WRTC=1 still seeds); WRTC=1 with implausible time does not seed; time-read failure returns false after the CONTROL1 write; failed CONTROL1 read returns false (no write); failed clear returns false and still restores (time read forced to succeed so the clear is the only failure); failed final write returns false (with and without a clear). Existing order test updated to `lock, begin, read, time, setTime, write, unlock`. The fake now ignores writes to registers other than 0x10.
Mutations (applied to in-memory copies, so no source file was edited and no restore was needed); all 24 caught: previous 16 header/call-site mutations plus WRTC not cleared before the read, final write skipped on time-read failure, saved value not restored (constant 0x01), saved value not normalised (`c1`), wrong clear register, clear result ignored, final write result ignored, CONTROL1 read result ignored, time-read result ignored. (First run left two alive, wrong clear register and clear result ignored; the fake and that case were tightened, re-run clean.)

## Verify
- Host suite **117/117** (sh via zsh, py via python3). `thermal_coupling_structural_test.py` fails in the working tree only because it scans the earlier round's untracked `build-tmp/wo-2026-11-06-002-stage6-r2/export` copy (not mine, left in place); it passes on a clean export of tracked files (my export was removed afterwards).
- Build: Device OS 6.6.0, Boron, gcc-arm 10.2.1, buildscripts 1.17.2, fresh `BUILD_PATH_BASE=build-tmp/wo-2026-11-06-002-stage6-r3/bpb` as a make argument: compiled; text/data/bss **153100 / 1114 / 2240** (r2: 153052, +48 text).
- `nm -C`: `PreBindRtc::run<AB1805, TwoWire>` (b5d88), `AB1805::getRtcAsTime` (c5cf4), `readRegister` (c576c), `writeRegister` (c5810). `objdump setup()` (b5ed0): `ExRtcBase::status()` (b5f14) -> `bl PreBindRtc::run` (b5f26), warn on false -> `Am18x5Configuration` set-up -> `ExRtcT::enable` (b5f82): run is before enable.
- Build output is in the git-ignored `target/6.6.0/boron/`; scratch in `build-tmp/wo-2026-11-06-002-stage6-r3/` (`bpb`, `build.log`, `dis.txt`, `o.txt`).

## Deviations / notes
- No final CONTROL1 write when the CONTROL1 read fails (see Change); the WO text did not cover this case.
- Reads and writes in `run()` use `&` rather than `&&` solely to keep the final write unconditional within the line budget.
- Bench (Dev-14) is Chip's step; nothing claimed.
