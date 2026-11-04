# WO-2026-11-02-003 Stage 7 verdict — round 2

**PASS** — blocker B1 is resolved. No blocking findings remain.

**Reviewer:** Codex · **Model:** `gpt-5.6-sol` · **Reasoning:** high

**Interpreter line:** 61 `tests/*.sh` files via `/bin/zsh` and 46 `tests/*.py` files via `/opt/homebrew/bin/python3`: **107/107 passed**.

## Verification

1. **B1 resolved.** The second checkout uses `particle-iot/device-os` at full SHA `533fe47b1d420c8c766c488471ee34256727b49e`. Its `path: device-os-6.4.1` agrees with `DEVICE_OS_ROOT: ${{ github.workspace }}/device-os-6.4.1`.

2. A scratch Device OS tree containing only the seven sparse-checkout directories was built from the installed 6.4.1 tree. `tests/sleep_config_leak_test.sh` passed against it. Compiler tracing found 45 Device OS header dependencies, all supplied by `system/inc`, `hal/inc`, `hal/shared`, `hal/src/gcc`, `wiring/inc`, or `services/inc`; `dynalib/inc` was present but unused. `platform/shared/inc` does not exist in Device OS 6.4.1, and its nonexistent `-I` path is harmless—the compile and test passed without it.

3. The workflow remains within scope:

   - 75 lines, within the 80-line limit.
   - Exactly `host-tests` and `firmware`.
   - Five action invocations, all pinned to full 40-hex SHAs with tag comments.
   - Checkout v7.0.1, upload-artifact v7.0.2, and compile-action v1.0.7 pins match the required values.
   - `permissions: contents: read`; no `pull_request_target`, secret references, `particle-access-token`, or `auto-version`.
   - Firmware remains Boron with Device OS read from `project.properties` (`6.6.0`).
   - Artifact paths still include the firmware output and `target-path/*.elf`, with `if-no-files-found: error`.
   - Zero staged, tracked, or untracked changes under `src/`.

4. Ruby parsed the YAML successfully, found four embedded `run:` blocks, and all four passed `bash -n`. Whitespace checks passed. `actionlint` is not installed, and no network installation was attempted.

## Findings

### Blocking

None.

### Notes

- **N1 — persisted checkout credentials:** `persist-credentials` is omitted, so all three `actions/checkout` steps use its default of `true`. No later authenticated Git operation needs the credentials. The added checkout is SHA-pinned, introduces no secret reference, and does not broaden the token beyond `contents: read`; therefore this is not a blocking supply-chain regression. Explicit `persist-credentials: false` would nevertheless be preferable hardening.

- **N2 — clean-runner reduced coverage:** two tests contain a hard-coded macOS ARM compiler path:

  - `tests/facade_header_isolation_test.py` — passes with a “host-only run” note when `/Users/chipmc/.particle/.../arm-none-eabi-g++` is absent; ARM coverage is omitted.
  - `tests/persisted_layout_preservation_test.py` — prints `SKIP` and returns success before performing layout validation.

- **N3 — shallow-checkout reduced coverage:** checkout’s default shallow history omits the historical baseline commits used by these tests. They still return success but skip or note their baseline comparisons:

  - `sleep_cloud_gate_equivalence_test.sh`
  - `sleep_execution_equivalence_test.sh`
  - `sleep_plan_equivalence_test.sh`
  - `sleep_preflight_equivalence_test.sh`
  - `sleep_teardown_equivalence_test.sh`
  - `sleep_wake_equivalence_test.sh`
  - `sleep_cloud_gate_structural_test.py`
  - `sleep_execution_structural_test.py`
  - `sleep_plan_structural_test.py`
  - `sleep_preflight_structural_test.py`
  - `sleep_wake_structural_test.py`
  - `sleep_config_ownership_structural_test.py`

  No other test references Brew, `/opt/homebrew`, macOS-only commands, `/Users/chipmc`, or an unprovisioned `~/.particle` path. The suite also relies on the standard `ubuntu-latest` C++ compiler commands (`clang++`, `g++`, and `c++`).

- **N4 — runtime acceptance:** the GitHub-hosted action execution, firmware outputs, artifact contents, and the binding specification’s CI/local size, `nm`, and firmware-version parity were not executable under this offline, host-test-only authorization. The combined upload selection also does not independently guarantee that both the firmware and ELF patterns matched. These remain CI acceptance checks, not blockers to this round-2 source/workflow verdict.