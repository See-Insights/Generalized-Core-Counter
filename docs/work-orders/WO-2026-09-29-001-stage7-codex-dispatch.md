AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff; run the host suite and local ARM builds in a scratch copy (or with outputs restored); write temporary host harnesses under a gitignored scratch path; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access.

# Stage 7 dispatch (narrow) — WO-2026-09-29-001 (v27, small known fixes)

**Goal, in plain language:** five small, already-understood fixes, each checked against its own goal. No new mechanisms.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-29-001-v27-small-fixes`, base `56772d5` (v26-NoPdiag), with the Stage 6 change as an uncommitted working-tree diff. **Binding spec:** `docs/work-orders/WO-2026-09-29-001-v27-small-fixes.md`. Stage 6 report: `WO-2026-09-29-001-stage6-copilot-report.md`.

This is a narrow review: check each item against **its own goal only**. Do not widen the fault model or propose new mechanisms.

## Checks (PASS/FAIL with evidence for each)

- **A (closing report at close):** a host check, a real harness or a faithful extraction of `handleReportingState()`'s connect decision, showing that (1) a report where the boundary is due goes to `CONNECTING_STATE` when outside open hours and not in keep-alive mode (cadence not due), and (2) an ordinary report outside open hours, with the same mode, does not. Report which kind of check you used.
- **B (only our reply clears the wait):** `Particle.subscribe("hook-response/", ...)` is gone; the device-ID `responseTopic` subscription remains; `UbidotsHandler()` still clears `session.awaitingWebhookResponse`.
- **C (overflow can't overwrite the stack):** the `dataSize() >= sizeof(bufferBase)` guard returns before the terminator write `bufferBase[writerBase.dataSize()] = '\0'`, and the ledger payload is unchanged for normal sizes (same fields and bytes; e.g. `tests/device_status_payload_budget_test.py` if present, or a render comparison against `56772d5`).
- **D (TimeDiag live local time):** `logTimeDiag()` no longer reads `LocalTimeCache`, and the `TimeDiag` format string is unchanged. The `time/LocalTimeCache.h` include was also removed from `src/Generalized-Core-Counter.cpp` (Claude Code, authorized narrow edit); confirm nothing in that file needs it.
- **E (cloud = local library code):** `project.properties` has no `dependencies.*` lines; the only `lib/` change is the removal of the AB1805 "petting watchdog" debug log. Claude Code ran two real Particle cloud builds (your sandbox has no network) and placed them under `build-tmp/wo-2026-09-29-001-cloud/`: the **branch** (vendored libraries) and **`56772d5`** (`main`, registry libraries), each with its SHA-256. Show, in the machine code, that the branch binary uses `REG_OSC_STATUS_OMODE = 0x10` (vendored, PR #41) and the `main` binary uses `0x01` (registry), for example by locating `AB1805::usingRCOscillator()` via a local ELF with symbols and matching its code in each `.bin`. Also confirm that a local build's library objects are byte-identical to `56772d5`'s, except `AB1805_RK.o` (the deleted log line).
- **Test changes allowed:** one assertion in `tests/daily_cleanup_boundary_test.py` (item A, Copilot), and a no-op `error()` added to the two `TestLog` stubs (`tests/stubs/clock_status_republish_overrides/Particle.h`, `tests/stubs/power_source_override_overrides/Particle.h`), which is the archived companion to item C's guard (Claude Code, authorized narrow edit). Any other test change is a finding.
- **Everywhere:** suite `N/N (sh via zsh, py via python3)`, expected 44/44; `tests/publish_with_ack_structural_test.py` unchanged and green; total `src/` diff at most about 20 lines, with per-item counts against A ≤ 5, B ≤ 5, C ≤ 8, D about 5; version `v27-SmallFixes`, product 27; `strings` on the release `.bin` finds neither `pdiag` nor `petting watchdog`.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per item and overall, with evidence; binary sizes; the model and reasoning level actually used. Confirm the working tree is byte-identical to how you found it.
