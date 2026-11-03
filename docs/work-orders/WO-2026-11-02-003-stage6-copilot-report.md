# WO-2026-11-02-003 Stage 6 report

**Model / reasoning:** claude-sonnet-5.5 (Copilot CLI 1.0.105); reasoning effort was set low by the runtime, not the medium the dispatch asked for.
**Interpreter line:** host tests run `.sh` via `zsh` (/bin/zsh locally), `.py` via `python3`. The CI summary prints `N/N (sh via zsh, py via python3)`.

## Files changed
- New: `.github/workflows/build.yml` (61 lines, limit 80). Nothing under `src/` or `tests/`; nothing committed.
- The dispatch file `docs/work-orders/WO-2026-11-02-003-stage6-copilot-dispatch.md` was already untracked. This report is new.

## Design notes
- Triggers `push` and `pull_request`; `permissions: contents: read`; no secrets; no `particle-access-token`; no `auto-version`.
- `host-tests` sets `TMPDIR=${{ runner.temp }}`. The tests use `$TMPDIR` under `set -u`, and it isn't set on GitHub runners. The tests use `clang++`, which is on ubuntu-latest.
- `firmware` reads `deviceOS=` from `project.properties` (6.6.0 today) and passes it as `device-os-version`. It echoes the action's `device-os-version` and `firmware-version` outputs to the job summary. It uploads `firmware-path` (the .bin) and `target-path/*.elf` as artifact `firmware-boron`.

## SHAs as pinned (re-verified with `gh api .../git/ref/tags/<tag>`; all commit objects)
- `actions/checkout` v4.2.2 → `11bd71901bbe5b1630ceea73d27597364c9af683`
- `actions/upload-artifact` v4.6.2 → `ea165f8d65b6e75b540449e92b4886f43607fa02`
- `particle-iot/compile-action` v1.0.7 → `3b6e15e99da59d074120f854a71659d4c81f092c`

The predispatch-checks note lists v7 SHAs for checkout and upload-artifact. I followed the dispatch (v4.x, which satisfies Amendment 1). Bumping later is trivial.

## Verification
- `actionlint` is not installed. A Ruby YAML parse succeeded, with jobs `host-tests` and `firmware` and the expected top-level keys. (`on` parses as `true` under YAML 1.1, which is normal.) No PyYAML is available.
- Local host tests, run as the job does (zsh for .sh, python3 for .py): **107/107 passed**.
- `git status`: only `.github/` and the docs files are untracked or new.

## Unverified
- The workflow has not been run (it can't be run locally). Unconfirmed: the compile action's behaviour with this repo and the `PRODUCT_VERSION` macro, the `target-path/*.elf` glob, and that the host tests all pass on Ubuntu (they passed on macOS only). The controller records parity (bin size, `nm`, Device OS and firmware version) after the first CI run.

## Round 2

**Model / reasoning:** claude-sonnet-5.5 (Copilot CLI 1.0.105); reasoning effort was set low by the runtime, not the medium the dispatch asked for.
**Interpreter line:** unchanged: `.sh` via `zsh`, `.py` via `python3`; summary prints `N/N (sh via zsh, py via python3)`.

### Fix B1 (workflow now 75 lines, limit 80)
- Final pins: `actions/checkout` v7.0.1 `3d3c42e5aac5ba805825da76410c181273ba90b1`, `actions/upload-artifact` v7.0.2 `cf430e030ddbb5b0abf93d22962f4752f3646cd9`, `particle-iot/compile-action` v1.0.7 `3b6e15e9...`. This corrects round 1, which said v4.x: checkout and upload-artifact are v7.
- New lines in `host-tests`: a second `actions/checkout` (same SHA) with `repository: particle-iot/device-os`, `ref: 533fe47b1d420c8c766c488471ee34256727b49e` (v6.4.1, verified a commit via `gh api`), `path: device-os-6.4.1`, and `sparse-checkout`. The test step gets `DEVICE_OS_ROOT: ${{ github.workspace }}/device-os-6.4.1`. The first checkout is unchanged at the workspace root.
- Sparse list: `system/inc`, `hal/inc`, `hal/shared`, `hal/src/gcc`, `wiring/inc`, `services/inc`, `dynalib/inc`. The dispatch also listed `platform/shared/inc`; it does not exist in v6.4.1 (absent from the local 6.4.1 toolchain, and `gh api` returns 404 for `platform/shared` at that SHA), so I omitted it. The test's `-I` for it is a harmless nonexistent path.
- Proof of sufficiency: copied only those 7 directories from `~/.particle/toolchains/deviceOS/6.4.1` into `build-tmp/wo-2026-11-02-003-stage6-r2/dos`; `DEVICE_OS_ROOT=<that> zsh tests/sleep_config_leak_test.sh` passed.

### Verification
- Ruby YAML parse OK (4 steps in `host-tests`); `wc -l` = 75.
- Full suite as the job does it, with `DEVICE_OS_ROOT` set to the sparse scratch tree: **107/107**.
- Not run in real CI. Nothing under `src/` or `tests/` changed; nothing committed.

### Coverage note
Two python tests contain hard-coded macOS paths and were not touched: `tests/facade_header_isolation_test.py`, `tests/persisted_layout_preservation_test.py`. They passed locally; their Ubuntu behaviour is unconfirmed until first CI run.
