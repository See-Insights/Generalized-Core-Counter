AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit only `.github/workflows/build.yml` and write the round-2 report; scratch `build-tmp/wo-2026-11-02-003-stage6-r2/` / NOT authorized: any change under `src/` or `tests/`, committing, pushing, merging, `gh` write operations, deleting anything you didn't create (including `build-tmp/`).

# Stage 6 round 2 — WO-2026-11-02-003
Binding spec: `docs/work-orders/WO-2026-11-02-003-github-actions-build.md`. Round 1 verdict: `docs/work-orders/WO-2026-11-02-003-stage7-verdict.md` (NOT VERIFIED, blocker B1). Leave changes uncommitted; keep the workflow ≤ 80 lines (61 now). Keep the v7 pins already in the file.

## Fix B1
`tests/sleep_config_leak_test.sh` needs Device OS **6.4.1** headers at `$DEVICE_OS_ROOT` (default `~/.particle/toolchains/deviceOS/6.4.1`; includes `system/inc`, `hal/inc`, `hal/shared`, `hal/src/gcc`, `wiring/inc`, `services/inc`, `dynalib/inc`, `platform/shared/inc`). The controller verified it FAILS (5 checks) with 6.6.0 headers, so it must be 6.4.1. In the `host-tests` job, before the test step, add a second `actions/checkout` (same pinned SHA as the first, `3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1`) with `repository: particle-iot/device-os`, `ref: 533fe47b1d420c8c766c488471ee34256727b49e` (tag v6.4.1; a full commit SHA, no submodules), `path: device-os-6.4.1`, `sparse-checkout` of exactly those directories (plus anything else the test's `-I` list or included headers need: check `tests/sleep_config_leak_test.sh` and, if available, by running the test with the sparse tree), and set `DEVICE_OS_ROOT: ${{ github.workspace }}/device-os-6.4.1` in the test step's `env`. Do not edit tests. The first checkout stays at the workspace root.

## Verify
- Reproduce locally: `git clone`/copy is not available without network; instead create a scratch tree containing only the sparse directories copied from `~/.particle/toolchains/deviceOS/6.4.1` and run `DEVICE_OS_ROOT=<scratch> zsh tests/sleep_config_leak_test.sh`; it must pass using only those directories (this proves the sparse list is sufficient).
- YAML parse, `wc -l` ≤ 80, full-suite run as the job does (expect 107/107, sh via zsh, py via python3).
- Do NOT touch the two python tests with hard-coded macOS ARM paths; list them in the report as a coverage note.

## Report
Append a `## Round 2` section to `docs/work-orders/WO-2026-11-02-003-stage6-copilot-report.md`: model and reasoning actually used, the final pins (state that checkout/upload-artifact are v7, correcting round 1), the new lines, the sparse list and how it was proven sufficient, the interpreter line.
