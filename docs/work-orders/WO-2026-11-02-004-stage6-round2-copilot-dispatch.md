AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/Config.h` and add one test under `tests/`; write the round-2 report; scratch `build-tmp/wo-2026-11-02-004-stage6-r2/` / NOT authorized: any other `src/` change, committing, pushing, merging, flashing, deleting anything you didn't create (including `build-tmp/`).

# Stage 6 round 2 — WO-2026-11-02-004 Amendment 2 (default timezone)
Binding spec: `docs/work-orders/WO-2026-11-02-004-timezone-applies-to-running-clock.md`, **Amendment 2 (replaces the planned-reset design)**. The tree is already reverted to main `21804e3`; do not reintroduce any reset code. **Budget: `src/` ≤ +2 net.** Leave changes uncommitted.

## Do
1. `src/Config.h:30`: `DEFAULT_TIMEZONE` = `"EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"`, with a one-line comment that it mirrors the 42131 product default (comment on the same line or the line above, within +2 net).
2. One host test (new file, zsh `.sh`, plus `.cpp`/`.py` only if needed; model it on an existing small structural/behavior test) asserting the default string exactly, and that it fits `sysStatusData::SysData::timeZoneStr` (39 bytes incl. NUL, `MyPersistentData.h:127`; the string is 38 chars) and is accepted by `SystemConfig::set_timeZoneStr` (read its length check). Also assert a mutation fails it (`UTC0`, a truncated string).
3. Check every use of `DEFAULT_TIMEZONE`: `MyPersistentData.cpp:133` (fresh default), `Generalized-Core-Counter.cpp:929` (empty-stored fallback), `DeviceStatusPublisher.cpp:645` (`hasNonDefaultConfig`: what does a device that stored `UTC0` now look like to it?), plus any validation fallback or reset-to-defaults path (search `src/` and `tests/` for the bare name and for `"UTC0"`). Existing tests that assume `UTC0` as the default must be listed (do not silently change them: if one fails, report it and the minimal edit).

## Verify
Host suite (zsh for `.sh`, python3 for `.py`; `N/N (sh via zsh, py via python3)`); local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE`) and `nm`/`strings`-independent proof that the new string is in the linked ELF (e.g. `objdump -s` of `.rodata` or `nm` on the symbol plus the ELF string search on the **ELF**, noting the caveat). `git diff --stat -- src/` ≤ +2 net.

## Report
Write `docs/work-orders/WO-2026-11-02-004-stage6-round2-copilot-report.md`: model and reasoning actually used, interpreter line, net `src/` lines, test names and mutation results, the `DEFAULT_TIMEZONE` use table, build evidence, open questions.
