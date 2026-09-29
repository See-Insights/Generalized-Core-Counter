AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit the `src/` lines named in items A–D and the version files; remove the four `dependencies.*` lines from `project.properties`; delete the debug-log lines named in item E from `lib/AB1805_RK/src/AB1805_RK.cpp`; add one test assertion for item A; run the host suite and a local ARM build / Not authorized: commits, pushes, merges, branch changes, stash, reset, flashing, device settings, AWS access, any other `lib/` edit, edits under `docs/`, and any change beyond this WO.
**SIZE BUDGET: A ≤ 5, B ≤ 5, C ≤ 8, D about 5 lines of `src/`; total about 20. Going over any item's budget means STOP and report, not continue.**

# Stage 6 dispatch — WO-2026-09-29-001 (v27, small known fixes)

**Goal, in plain language:** five small, already-understood fixes, each checked against its own goal. No new mechanisms.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-29-001-v27-small-fixes` (from `main` at `56772d5`, v26-NoPdiag). Do not switch branches. Files under `docs/work-orders/` are records: do not touch them.
**Binding spec:** `docs/work-orders/WO-2026-09-29-001-v27-small-fixes.md`, including its "Pre-dispatch findings".

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec, even an obviously right one, is reported as a deviation. Leave an uncommitted working-tree diff. **Keep each item's change in files no other item touches where possible, and report any file two items share**, because Chip will commit A–E separately. Temporary artifacts: visible, descriptive names under a gitignored path, removed when you finish.

## What to implement (nothing else)

**A. The day's closing report goes out at close** (`src/state/State_Report.cpp`, `handleReportingState()`, the `!Particle.connected()` connect decision). Add a branch that connects immediately when `due` is true, placed with the existing immediate triggers (after the occupancy-change branch, before the long-term-webhook and keep-alive branches), e.g. `} else if (due) { transitionTo(CONNECTING_STATE, "daily close"); }`. Reuse the existing `due` variable; add no new state. ≤ 5 lines.

**B. Only our webhook reply clears the wait** (`src/Generalized-Core-Counter.cpp:959–964`). Delete `Particle.subscribe("hook-response/", UbidotsHandler);` and the 4-line comment above it. Keep the device-ID `responseTopic` subscription and `UbidotsHandler()` unchanged. ≤ 5 lines.

**C. Status-payload overflow guard** (`src/cloud/DeviceStatusPublisher.cpp`, `Cloud::writeDeviceStatusToCloud()`). Immediately before `bufferBase[writerBase.dataSize()] = '\0';`, insert the guard exactly as quoted in the WO (the `if (writerBase.dataSize() >= sizeof(bufferBase))` block with its `Log.error(... "LedgerPayloadStatus: overflow ...")` and `return false;`), with at most a one-line comment. Do not change the payload fields. ≤ 8 lines.

**D. `TimeDiag` live local time** (`src/Generalized-Core-Counter.cpp`, `logTimeDiag()`). Replace the `LocalTimeCache::getLocalTimeSnapshot()` source of `localDate`, `localSecondsOfDay` and `localHour` with a live `LocalTimeConvert` (`withConfig(LocalTime::instance().getConfig()).withCurrentTime().convert()`, then `getLocalTimeYMD()` and `getLocalTimeHMS()`; `LocalTimeHMS::toSeconds()` exists). Keep the variable names and the minute/second derivation lines, so the `Log.info` call is unchanged. About 5 lines.

**E. Cloud builds use the vendored libraries.** Remove the four `dependencies.*` lines from `project.properties`. In `lib/AB1805_RK/src/AB1805_RK.cpp`, delete the "Temporary debug logging" comment and its `_log.info("petting watchdog at %lu ms", ...)` line (and the blank line before them), and nothing else in `lib/`.

**Version.** `src/Version.cpp`: `FIRMWARE_VERSION` → `"v27-SmallFixes"`, with a matching one-line `FIRMWARE_RELEASE_NOTES` that does **not** contain the string `pdiag`. `src/FirmwareVersion.h`: `FIRMWARE_PRODUCT_VERSION` 26 → 27.

## Tests

- **A:** add one assertion to `tests/daily_cleanup_boundary_test.py` (a source-fidelity test of this function): in the connect decision, a `due` branch transitions to `CONNECTING_STATE`, and it comes before the keep-alive and cadence branches. Report the assertion.
- Change no other test. `tests/publish_with_ack_structural_test.py` must pass unchanged. If any other test fails, stop and report.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh** (never bash) plus every bare `tests/*.py` with python3. Report `N/N (sh via zsh, py via python3)`. Current: 44/44.
2. Local ARM build (boron), release, after `make clean-user` (see `AI_DEVELOPMENT_WORKFLOW.md` §2, "Verifying compile-time flags"): text/data/bss against v26's 149708 / 1090 / 2196; `strings` on the `.bin` finds `v27-SmallFixes` and does not find `pdiag` or `petting watchdog`.
3. `git diff --numstat` per file, and the `src/` line count **per item** against its budget.

## Implementation Report (required)

Per item: files and lines changed, and its line count against its budget; the A test assertion; files shared between items (or "none"); commands and results with interpreters and sizes; deviations (or "none"); the model and reasoning level actually used.
