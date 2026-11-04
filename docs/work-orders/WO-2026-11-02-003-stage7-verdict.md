# WO-2026-11-02-003 Stage 7 verdict — round 1

**NOT VERIFIED** — one blocking finding prevents the `host-tests` job from passing on a clean GitHub-hosted Ubuntu runner.

**Reviewer:** Codex · **Model:** `gpt-5.6-sol` · **Reasoning:** high

**Interpreter line:** local verification ran 61 `tests/*.sh` files via `/bin/zsh` and 46 `tests/*.py` files via `/opt/homebrew/bin/python3`: **107/107 passed**. The workflow prints `N/N (sh via zsh, py via python3)` to both the log and `$GITHUB_STEP_SUMMARY`.

## Blocking finding

### B1 — `host-tests` lacks the Device OS header required by one test

`tests/sleep_config_leak_test.sh:26` defaults `DEVICE_OS_ROOT` to `$HOME/.particle/toolchains/deviceOS/6.4.1`; lines 39-40 require `system/inc/system_sleep_configuration.h` there and exit 1 if it is absent. The host job only checks out the repository and installs zsh. It does not provision Device OS 6.4.1 or set `DEVICE_OS_ROOT`. The compile action is in a separate job, whose filesystem is not shared with `host-tests`.

A controlled local run with `DEVICE_OS_ROOT` pointed at an absent directory reproduced the expected exit 1 and `FAILED: missing Device OS header .../system/inc/system_sleep_configuration.h`. A clean `ubuntu-latest` runner therefore cannot be expected to reach 107/107; the local macOS run passes because this host already has that Particle toolchain. The host job must provision the required header/toolchain (and point `DEVICE_OS_ROOT` to it), or the test must be made self-contained, before this workflow is verified.

## Checks that pass

1. `.github/workflows/build.yml` is 61 lines (limit 80) and defines exactly `host-tests` and `firmware`. It triggers on `push` and `pull_request`, not `pull_request_target`.
2. The host loop runs every top-level `tests/*.sh` with `zsh` and every `tests/*.py` with `python3`, counts all 107 files, sets `TMPDIR: ${{ runner.temp }}`, prints the interpreter line, accumulates failures, and exits nonzero after reporting them.
3. The firmware job reads `deviceOS=6.6.0` from `project.properties`, passes it as `device-os-version`, and sets `particle-platform-name: boron`. There is no `particle-access-token` key, `auto-version` key, secret reference, or other secret material.
4. All four third-party action invocations use full 40-hex commit SHAs with tag comments. The pins exactly match the architect's amendment: checkout v7.0.1 `3d3c42e5aac5ba805825da76410c181273ba90b1`, upload-artifact v7.0.2 `cf430e030ddbb5b0abf93d22962f4752f3646cd9`, and compile-action v1.0.7 `3b6e15e99da59d074120f854a71659d4c81f092c`.
5. Top-level permissions are `contents: read`. The upload lists `${{ steps.compile.outputs.firmware-path }}` and the required `${{ steps.compile.outputs.target-path }}/*.elf`, with `if-no-files-found: error`.
6. There are zero tracked, staged, or untracked changes under `src/`.
7. Ruby parsed the YAML and found the two expected jobs; every embedded `run:` block passed `bash -n`; the workflow passed a whitespace check. `actionlint` is not installed, and no network installation was attempted.

## v1.0.7 / v7 compatibility assessment

`particle-iot/compile-action@v1.0.7` does **not** inherently require checkout or upload-artifact v4. GitHub actions are isolated steps: checkout populates the workspace, compile-action consumes the workspace and emits paths, and upload-artifact consumes those paths. Their bundled Node runtimes do not link to one another, so compile-action running on Node 24 creates no v4 coupling. Checkout v7 and upload-artifact v7 are compositionally compatible with it on a runner that supports their Node 24 runtime.

Offline, I could not independently inspect the remote action manifests, prove the tag-to-SHA mappings, confirm the hosted runner's exact action-runner version, or execute the three actions together. Those pins were checked against the architect-supplied values, not against GitHub. I also could not confirm that Actions is enabled for the See-Insights organization.

## Notes

- The Stage 6 report is stale about action versions: its “SHAs as pinned” section and conclusion say checkout/upload-artifact v4, while the reviewed workflow now contains the architect-mandated v7 pins. This is a report defect, not a separate workflow defect.
- `if-no-files-found: error` is present, but upload-artifact applies it to the combined multi-line selection. If the `.bin` matches while the ELF glob does not, the step may still succeed; it does not independently prove that each listed pattern matched. CI artifact inspection remains necessary.
- Ubuntu differs materially from this macOS run. The scripts rely heavily on unversioned `clang++` without installing it; `ubuntu-latest` normally provides it, but image drift remains a risk. Linux uses different compiler/system headers and paths. Zsh is explicitly installed, and the `${0:A:...}` path expressions are correctly evaluated by zsh.
- `tests/facade_header_isolation_test.py` and `tests/persisted_layout_preservation_test.py` hard-code `/Users/chipmc/.particle/toolchains/gcc-arm/10.2.1/bin/arm-none-eabi-g++`; on Ubuntu they respectively reduce to host-only coverage or report a successful skip. Thus even after B1 is fixed, CI coverage will be weaker than this local macOS run unless those paths are made portable.
- The firmware compile, action outputs, `.bin`/ELF production and upload, firmware-version reporting, GitHub-hosted execution, and the binding spec's size/`nm`/version parity acceptance could not be exercised offline and remain unverified until a CI run and the authorized local firmware comparison are performed.
