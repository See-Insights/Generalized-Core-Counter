AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit the four `src/` lines named below and the expected values in `tests/build_flags_witness_test.sh`; run the host suite and a local ARM build / Not authorized: commits, pushes, merges, branch changes, stash, reset, flashing, device settings, AWS access, any edit to `lib/`, `project.properties`, or `docs/`, and any change beyond this WO.
**SIZE BUDGET: about 3 lines of `src/` (the four physical lines below). Going over the budget means STOP and report, not continue.**

# Stage 6 dispatch — WO-2026-09-28-001 (diagnostics must never cost a report, v26)

**Goal, in plain language:** release builds don't publish `pdiag`, so diagnostics can't fill the publish queue and cause reports to be deleted.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-28-001-no-pdiag-release`, `HEAD` `9d1a285` (v25-WithAck). Do not switch branches. Files under `docs/work-orders/` are records: do not touch them.
**Binding spec:** `docs/work-orders/WO-2026-09-28-001-no-pdiag-in-release.md`.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec, even an obviously right one, is reported as a deviation. Leave an uncommitted working-tree diff. Temporary artifacts: visible, descriptive names under a gitignored path, removed when you finish.

## What to implement (nothing else)

A switch already exists; **set it, don't add a mechanism.**

1. `src/BuildProfile.h:246`: `#define ENABLE_DIAGNOSTICS_PUBLISH_MODE 1` → `0`. You may put a short trailing comment on that same line saying bench builds enable it with `-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1` (WO-2026-09-28-001). Do not change the `#ifndef` guard or the `#error` check, and do not rewrite the block comment above it.
2. `src/Version.cpp`: `FIRMWARE_VERSION` → `"v26-NoPdiag"`, and a matching one-line `FIRMWARE_RELEASE_NOTES`.
3. `src/FirmwareVersion.h:28`: `FIRMWARE_PRODUCT_VERSION` 25 → 26.

Do not change any `#if ENABLE_DIAGNOSTICS_PUBLISH_MODE` block, `PowerDiagnostics.cpp`, `State_Sleep.cpp`, or `lib/`.

## Tests

- `tests/build_flags_witness_test.sh` pins the default build-flags word, which includes this flag's bit (`0x2000`). Update its expected values and the explanatory comment to the new default: default `0x0008` (8), flipped `0x0103` (259). Verify these numbers yourself from the harness; if they differ, report it and use what the harness proves.
- `tests/publish_with_ack_structural_test.py` must pass **unchanged** (it counts publish call sites in the source text, and the `pdiag` call stays in the source).
- Change no other test. If another test fails because of the new default, stop and report it.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh** (shebang or `zsh <script>`, never bash) plus every bare `tests/*.py` with python3. Report `N/N (sh via zsh, py via python3)`. Current: 44/44.
2. Local ARM build (boron), release (no `EXTRA_CFLAGS`): text/data/bss against v25's 150640 / 1090 / 2444, and confirm `strings` on the `.bin` finds **no** `pdiag` and does find `v26-NoPdiag`.
3. Local ARM build (boron), bench: `EXTRA_CFLAGS="-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1"`: confirm `strings` finds `pdiag`. Then restore the release build output (rebuild without the flag) so the tree's `target/` is the release build.
4. `git diff --numstat -- src/` against the ~3-line budget.

## Implementation Report (required)

Files and lines changed; the exact `src/` line count; the test change and the numbers you verified; commands and results with interpreters, sizes, and the `strings` results for both builds; deviations (or "none"); the model and reasoning level actually used.
