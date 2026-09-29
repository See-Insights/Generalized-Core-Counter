AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff; run the host suite and local ARM builds (release and bench) in a scratch copy or with outputs restored; temporarily mutate files for the checks below, restoring each byte-identically / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, or AWS access.

# Stage 7 dispatch (narrow) — WO-2026-09-28-001 (diagnostics must never cost a report, v26)

**Goal, in plain language:** release builds don't publish `pdiag`, so diagnostics can't fill the publish queue and cause reports to be deleted.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-28-001-no-pdiag-release`, base `9d1a285`, with the Stage 6 change as an uncommitted working-tree diff. **Binding spec:** `docs/work-orders/WO-2026-09-28-001-no-pdiag-in-release.md`.

This is a narrow review. It uses an existing switch (`ENABLE_DIAGNOSTICS_PUBLISH_MODE`) and changes only its default. Verify against the WO's acceptance criteria and nothing wider. Do not propose new mechanisms.

## Checks (report PASS/FAIL with evidence for each)

1. **Scope:** the `src/` diff is only the flag default (`src/BuildProfile.h`), the version string and release notes (`src/Version.cpp`), and `FIRMWARE_PRODUCT_VERSION` 26 (`src/FirmwareVersion.h`); about 3 lines. The only test changes are the pinned default values in `tests/build_flags_witness_test.sh` (Copilot, Stage 6) and in `tests/power_source_override_test.cpp` (`0x6008` → `0x4008` at line 294 plus its comment at lines 263–270, applied by Claude Code as a narrow edit Chip authorized after Stage 6 stopped at 43/44). Report `git diff --numstat`; anything beyond these 4 source lines and 2 test files is a finding.
2. **Release build has no `pdiag` publish:** a local ARM build (boron, Device OS 6.4.1, README command, no `EXTRA_CFLAGS`), **after `make clean-user`**. Per `AI_DEVELOPMENT_WORKFLOW.md` §2 "Verifying compile-time flags", prove absence with `nm` on the linked ELF **and** on the `PowerDiagnostics` object: `PowerDiagnostics::flushDiagBatch` (and the batch storage) must be absent. Also show the string `pdiag` is absent from the `.bin` and `v26-NoPdiag` present. Record text/data/bss.
3. **Bench build can still enable it:** `make clean-user`, then the same build with `EXTRA_CFLAGS="-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1"`. Show with `nm` (ELF and object) that `PowerDiagnostics::flushDiagBatch` is present, and that the string `pdiag` is in the `.bin`. Record text/data/bss; a size equal to the release build's is the signature of a stale link. Then `make clean-user` and rebuild the release, so `target/` holds the release build when you finish.
4. **Serial logging unchanged:** the `PowerDiag:` and `ChargeDiag:` log format strings are present in both binaries.
5. **Suite:** every `tests/*.sh` with **zsh** (never bash) plus every bare `tests/*.py` with python3. Report `N/N (sh via zsh, py via python3)`; expected 44/44. `tests/publish_with_ack_structural_test.py` must pass unchanged.
6. **Witness test:** the new expected values in `tests/build_flags_witness_test.sh` (default `0x0008`, flipped `0x0103`) match what the harness computes. Mutation: revert the `BuildProfile.h` default to `1` and confirm the witness test fails; restore byte-identically.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, with the evidence for each check; binary sizes; the model and reasoning level actually used. Confirm the working tree is byte-identical to how you found it.
