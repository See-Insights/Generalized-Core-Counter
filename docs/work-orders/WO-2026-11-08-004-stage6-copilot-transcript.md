# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `8c38b263-f818-459b-a708-cff31562f3e2`  
> - **Started:** 11/8/2026, 9:44:41 PM  
> - **Duration:** 16m 22s  
> - **Exported:** 11/8/2026, 10:01:03 PM  

---

<sub>6s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-08-004-stage6-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>8s</sub>

### `bash`

**Read dispatch doc**

`$ cat docs/work-orders/WO-2026-11-08-004-stage6-copilot-dispatch.md; git status --short | head`

<details>
<summary>13 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `.github/workflows/build.yml` (the one new step only), add `tests/release_flags_guard_test.sh`, optionally one note line in `docs/bench-validation.md`, and write the report; run host tests (zsh for .sh, python3 for .py); scratch `build-tmp/wo-2026-11-08-004-stage6/` (create it; use only that) / NOT authorized: any change under `src/` or `lib/`, any other workflow step, committing, pushing, merging, creating PRs or tags, deleting anything you did not create (including `build-tmp/` itself).

# Stage 6 dispatch: WO-2026-11-08-004 (F5, release-flag guard)
Binding spec: `docs/work-orders/WO-2026-11-08-004-f5-release-flag-guard.md` and `docs/RECOVERY_PLAN_2026-09-26.md` item 28. Base: this branch HEAD (`main` at e467364 plus the records). Leave changes uncommitted. **Run at the dispatched reasoning level (medium); the report states the level actually run, or that it could not be observed.**
## Do
Implement the WO's "The change" items 1-3 (item 4 only if it fits): the tag-only step before Compile; the extracted-script test with the six cases; the six mutations (back up the file, mutate, restore from the backup with cp; never `git checkout`/`git restore` while your work is uncommitted). The new test must exit 0 when run bare. Do not touch `src/`.
## Verify
CI's discovery inside this git worktree: `tests/*.sh` via zsh and `tests/*.py` via python3 (count `p=$((p+1))`; report `N/N (sh via zsh, py via python3)`; state the measured baseline before editing); `thermal_coupling_structural_test.py` also on a clean export of tracked files (remove scratch copies from `build-tmp/` first); `git diff --stat` shows nothing under `src/`.
## Stop and report if
the step cannot be tested without a `src/` change; the macros cannot be read standalone; the workflow needs more than +12 lines.
## Report
`docs/work-orders/WO-2026-11-08-004-stage6-copilot-report.md`: model and reasoning level actually used, interpreter line, measured baseline, the step as written, tests and mutation results, deviations, open questions. Claim nothing about a real tag build.
<shellId: 0 completed with exit code 0>
```

</details>


---

<sub>9s</sub>

### `bash`

**Read WO, workflow, tests**

`$ cat docs/work-orders/WO-2026-11-08-004-f5-release-flag-guard.md; grep -n "item 28\|28\." docs/RECOVERY_PLAN_2026-09-26.md | head; cat .github/workflows/build.yml; ls tests build-tmp`

<details>
<summary>374 lines</summary>

```
# WO-2026-11-08-004: F5, a `v*` tag build fails unless `DEV_BUILD` and `CONNECTIVITY_FAILSAFE_TEST_MODE` are 0 (recovery item 28)

**Author:** Claude Code, 8 Nov 2026 · **Status:** DISPATCHED (architect, 8 Nov 2026) · **Branch:** `wo/2026-11-08-004-f5-release-flag-guard`, from `main`; PR to `main`, for Chip to merge (workflow files are under CODEOWNERS). Never merge.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §10 (CI discovery); §12 · **Agents:** Stage 6 Copilot medium (the report states the level actually run); Stage 7 Codex `gpt-5.6-sol` **high**, 90-minute box, at most two rounds.
**Inputs:** `docs/RECOVERY_PLAN_2026-09-26.md` item 28 (the spec); `docs/reviews/2026-11-grok-progress/triage.md` (F5 accepted now); `.github/workflows/build.yml`; `src/BuildProfile.h`.
**Budget:** `src/` **none** (no firmware change; the binary must be byte-identical); `.github/workflows/build.yml` at most +12 lines; one new test.

## Plain goal
Grok's F5: nothing stops a tagged release from being built with the developer profile or the short-timer failsafe test mode. The existing `FIRMWARE_PRODUCT_VERSION` check on tags already reads the macro "as the compiler sees it"; add the same kind of check for the two flags, so a release built with either set to anything but 0 fails before it is compiled, uploaded or published.

## The change
1. **One step in the `firmware` job of `.github/workflows/build.yml`,** beside "Tag matches source FIRMWARE_PRODUCT_VERSION (before building)", `if: github.ref_type == 'tag'`, before "Compile": preprocess `src/BuildProfile.h` the same way (`cc -E -P -x c++ -include src/BuildProfile.h -` on the two macro names) and **fail with a `::error::` message naming the macro and its value** unless both are exactly `0`. It must also fail (not pass silently) if the preprocess fails or prints anything other than two tokens.
2. **The step's script is extracted and tested:** a new host test `tests/release_flags_guard_test.sh` (zsh, exits 0 bare, no arguments) pulls the step's shell body out of `build.yml` and runs it against (a) the real tree: passes; (b) a temporary copy whose `BuildProfile.h` default is `DEV_BUILD 1`: fails and names `DEV_BUILD`; (c) a copy with `CONNECTIVITY_FAILSAFE_TEST_MODE 1`: fails and names it; (d) both set; (e) a copy whose `BuildProfile.h` does not preprocess: fails; (f) a `-D` override is not the case here (the compile action passes no flags), so the test also asserts that `build.yml` contains no `EXTRA_CFLAGS`, `-D`, or `cflags` input on the compile step (a flag added to the workflow later must trip the test and be ruled on, not slip past a header-only check).
3. **Mutation tests (each must fail the test):** the step's `if:` guard removed; the comparison loosened to `-ge 0`; only one of the two macros checked; the `::error::` exit removed; `DEV_BUILD` compared as a string with a different value; the step moved after Compile.
4. Optional, only if it fits: a one-line note in `docs/bench-validation.md` ("Production verification") naming the guard.

## Not in scope
Any `src/` change; the tag-version check; publish and upload jobs; making bench-only builds impossible to build on branches (they build on branches by design: the guard is for tags only).

## Verify (workflow §10)
CI's discovery in a git worktree (`tests/*.sh` via zsh, `tests/*.py` via python3; state the measured baseline before editing); the new test exits 0 bare; `thermal_coupling_structural_test.py` also on a clean export of tracked files; `git diff --stat` shows no file under `src/`. Confirm `gh pr checks` green before saying ready. A real failing-tag build cannot be tried (no tags are created), so the extracted-script test is the proof; say so.

## Rollback
Revert the commit; tag builds return to checking only the product version.
379:28. **Release gate: fail a `v*` tag build if `DEV_BUILD` or `CONNECTIVITY_FAILSAFE_TEST_MODE` is not 0** (Chip, 6 Nov; Grok review F5 accepted now). Priority: **before RC1.** A small WO: one CI step in `.github/workflows/build.yml` that reads the active macros as the compiler sees them (as the existing `FIRMWARE_PRODUCT_VERSION` check at `build.yml:70-77` does) and fails the build, with a test that a `DEV_BUILD=1` source is rejected. `src/` change: none expected. Stage 7: Codex `gpt-5.6-sol`, **high**. Workflow files are under CODEOWNERS.
484:29. **Reason-0 restarts on Court1 and PCKL3** (Chip, 7 Nov; found in the v49.1 fleet check, read-only note `scratchpad` rr0, 15 Oct-7 Nov data). Priority: **pre-RC1 Step 0 (read-only); no fix proposed until it answers.** *Numbering: Chip asked for "item 28"; item 28 is already the release-gate item, so this is item 29.*
name: build

on:
  push:
    branches: ['**']
    tags: ['v*']
  pull_request:

permissions:
  contents: read

jobs:
  host-tests:
    runs-on: ubuntu-latest
    outputs:
      tests: ${{ steps.tests.outputs.result }}
    steps:
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
        with:
          persist-credentials: false
          fetch-depth: 0  # equivalence tests compare against historical commits
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
        with:
          persist-credentials: false
          repository: particle-iot/device-os
          ref: 533fe47b1d420c8c766c488471ee34256727b49e # v6.4.1
          path: device-os-6.4.1
          sparse-checkout: |
            system/inc
            hal/inc
            hal/shared
            hal/src/gcc
            wiring/inc
            services/inc
            dynalib/inc
      - name: Install zsh
        run: sudo apt-get install -y zsh
      - name: Run host tests (sh via zsh, py via python3)
        id: tests
        env:
          TMPDIR: ${{ runner.temp }}
          DEVICE_OS_ROOT: ${{ github.workspace }}/device-os-6.4.1
        run: |
          pass=0; total=0; failed=()
          for t in tests/*.sh; do
            total=$((total+1))
            if zsh "$t"; then pass=$((pass+1)); else failed+=("$t"); fi
          done
          for t in tests/*.py; do
            total=$((total+1))
            if python3 "$t"; then pass=$((pass+1)); else failed+=("$t"); fi
          done
          echo "$pass/$total (sh via zsh, py via python3)" | tee -a "$GITHUB_STEP_SUMMARY"
          echo "result=$pass/$total" >> "$GITHUB_OUTPUT"
          if [ "${#failed[@]}" -gt 0 ]; then
            printf 'FAILED: %s\n' "${failed[@]}"
            exit 1
          fi

  firmware:
    runs-on: ubuntu-latest
    outputs:
      sha256: ${{ steps.rec.outputs.sha256 }}
      digest: ${{ steps.rec.outputs.digest }}
      os: ${{ steps.props.outputs.os }}
    steps:
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
        with:
          persist-credentials: false
      - name: Tag matches source FIRMWARE_PRODUCT_VERSION (before building)
        if: github.ref_type == 'tag'
        run: |
          set -euo pipefail
          [[ "$GITHUB_REF_NAME" =~ ^v([0-9]{1,2})\.([0-9]{1,2})$ ]] || { echo "::error::tag '$GITHUB_REF_NAME' is not v<major>.<iteration> (1-2 digits each)"; exit 1; }
          want=$((10#${BASH_REMATCH[1]} * 100 + 10#${BASH_REMATCH[2]}))
          got=$(echo FIRMWARE_PRODUCT_VERSION | cc -E -P -x c++ -include src/FirmwareVersion.h - | tail -n 1)  # the active macro, as the compiler sees it
          [ "$got" = "$want" ] || { echo "::error::source FIRMWARE_PRODUCT_VERSION '$got' != $want from tag $GITHUB_REF_NAME; not building"; exit 1; }
      - name: Read Device OS version from project.properties
        id: props
        run: echo "os=$(sed -n 's/^deviceOS=//p' project.properties)" >> "$GITHUB_OUTPUT"
      - name: Compile (on the runner; no Particle token, no auto-version)
        id: compile
        uses: particle-iot/compile-action@3b6e15e99da59d074120f854a71659d4c81f092c # v1.0.7
        with:
          particle-platform-name: boron
          device-os-version: ${{ steps.props.outputs.os }}
      - uses: actions/upload-artifact@cf430e030ddbb5b0abf93d22962f4752f3646cd9 # v7.0.2
        with:
          name: firmware-boron
          path: |
            ${{ steps.compile.outputs.firmware-path }}
            ${{ steps.compile.outputs.target-path }}/*.elf
          if-no-files-found: error
      - name: Record .bin SHA-256 and buildpack image digest
        id: rec
        env:
          BIN: ${{ steps.compile.outputs.firmware-path }}
          OS: ${{ steps.props.outputs.os }}
        run: |
          set -euo pipefail
          sha=$(sha256sum "$BIN" | cut -d' ' -f1)
          digest=$(docker image inspect "particle/buildpack-particle-firmware:$OS-boron" --format '{{join .RepoDigests ","}}')
          [ -n "$digest" ] || { echo "::error::buildpack image has no RepoDigest; cannot record it"; exit 1; }
          echo "sha256=$sha" >> "$GITHUB_OUTPUT"
          echo "digest=$digest" >> "$GITHUB_OUTPUT"
      - name: Job summary
        env:
          DIGEST: ${{ steps.rec.outputs.digest }}
        run: |
          {
            echo "device-os-version: ${{ steps.compile.outputs.device-os-version }}"
            # The action's firmware-version output is unreliable (reported 24 for product version 48).
            echo "firmware-product-version: $(sed -n 's/^#define FIRMWARE_PRODUCT_VERSION *//p' src/FirmwareVersion.h)"
            echo "buildpack-image: $DIGEST"
          } >> "$GITHUB_STEP_SUMMARY"

  publish:
    if: github.ref_type == 'tag'
    needs: [host-tests, firmware]
    runs-on: ubuntu-latest
    permissions:
      contents: write  # this job only; the token is in the env of the one Release step alone
    steps:
      - uses: actions/download-artifact@9000827ccba6bdab643e8b6fd33ac0654aef8333 # v8.0.2
        with:
          name: firmware-boron
          path: fw
      - name: Guards and Release (the only step with the write token)
        env:
          GH_TOKEN: ${{ github.token }}
          TAG: ${{ github.ref_name }}
          WANT_SHA: ${{ needs.firmware.outputs.sha256 }}
          OS: ${{ needs.firmware.outputs.os }}
          DIGEST: ${{ needs.firmware.outputs.digest }}
          TESTS: ${{ needs.host-tests.outputs.tests }}
        run: |
          set -euo pipefail
          [[ "$TAG" =~ ^v([0-9]{1,2})\.([0-9]{1,2})$ ]] || { echo "::error::tag '$TAG' is not v<major>.<iteration> (1-2 digits each)"; exit 1; }
          want=$((10#${BASH_REMATCH[1]} * 100 + 10#${BASH_REMATCH[2]}))
          bin=$(find fw -name '*.bin')
          [ -n "$bin" ] && [ "$(wc -l <<<"$bin")" -eq 1 ] || { echo "::error::expected exactly one .bin"; exit 1; }
          [ "$(sha256sum "$bin" | cut -d' ' -f1)" = "$WANT_SHA" ] || { echo "::error::artifact SHA-256 differs from the firmware job's"; exit 1; }
          name="gcc-$TAG-boron-dos$OS-${GITHUB_SHA::7}.bin"
          mv "$bin" "$name"
          sha256sum "$name" > "$name.sha256"
          if gh release view "$TAG" --repo "$GITHUB_REPOSITORY" >/dev/null 2>&1; then echo "::error::a Release for $TAG already exists; not modifying it"; exit 1; fi
          [ "$(gh api "repos/$GITHUB_REPOSITORY/git/ref/tags/$TAG" --jq .object.type)" = tag ] || { echo "::error::$TAG is not an annotated tag"; exit 1; }
          tagsha=$(gh api "repos/$GITHUB_REPOSITORY/git/ref/tags/$TAG" --jq .object.sha)
          [ "$(gh api "repos/$GITHUB_REPOSITORY/git/tags/$tagsha" --jq .object.sha)" = "$GITHUB_SHA" ] || { echo "::error::$TAG no longer points at $GITHUB_SHA"; exit 1; }
          gh api "repos/$GITHUB_REPOSITORY/git/tags/$tagsha" --jq .message > "$RUNNER_TEMP/notes.txt"
          gh release create "$TAG" "$name" "$name.sha256" --repo "$GITHUB_REPOSITORY" --verify-tag --title "$TAG" --notes-file "$RUNNER_TEMP/notes.txt"
          printf '%s\n' "### Release $TAG" "- run: $GITHUB_SERVER_URL/$GITHUB_REPOSITORY/actions/runs/$GITHUB_RUN_ID" \
            "- commit: $GITHUB_SHA" "- bin: $name ($(stat -c%s "$name") bytes)" "- sha256: $WANT_SHA" \
            "- source version: $want" "- host tests: $TESTS" "- buildpack image digest: $DIGEST" >> "$GITHUB_STEP_SUMMARY"

  upload:
    if: github.ref_type == 'tag'
    needs: [firmware, publish]
    runs-on: ubuntu-latest
    environment: particle-42131
    permissions: {contents: read}
    steps:
      - uses: actions/download-artifact@9000827ccba6bdab643e8b6fd33ac0654aef8333 # v8.0.2
        with: {name: firmware-boron, path: fw}
      - id: meta
        env: {GH_TOKEN: "${{ github.token }}", TAG: "${{ github.ref_name }}", WANT_SHA: "${{ needs.firmware.outputs.sha256 }}", OS: "${{ needs.firmware.outputs.os }}"}
        run: |
          set -euo pipefail
          [[ "$TAG" =~ ^v([0-9]{1,2})\.([0-9]{1,2})$ ]] || { echo "::error::tag '$TAG' is not v<major>.<iteration>"; exit 1; }
          ver=$((10#${BASH_REMATCH[1]} * 100 + 10#${BASH_REMATCH[2]})); bin=$(find fw -name '*.bin')
          [ -n "$bin" ] && [ "$(wc -l <<<"$bin")" -eq 1 ] || { echo "::error::expected exactly one .bin"; exit 1; }
          [ "$(sha256sum "$bin" | cut -d' ' -f1)" = "$WANT_SHA" ] || { echo "::error::artifact SHA-256 differs from the firmware job's"; exit 1; }; new="$(dirname "$bin")/gcc-$TAG-boron-dos$OS-${GITHUB_SHA::7}.bin"; mv "$bin" "$new"; bin=$new
          api=repos/$GITHUB_REPOSITORY/git; ref=$(gh api "$api/ref/tags/$TAG" --jq '.object.type + " " + .object.sha')
          [ "${ref%% *}" = tag ] || { echo "::error::$TAG is not an annotated tag"; exit 1; }
          msg=$(gh api "$api/tags/${ref##* }" --jq .message); d=$(openssl rand -hex 8); grep -qx -- '---' <<<"$msg" || { echo "::error::tag message has no --- line"; exit 1; }
          { echo "bin=$bin"; echo "version=$ver"; echo "title=Development Release - $TAG"; echo "description<<$d"; sed '/^---$/,$d' <<<"$msg"; echo; echo "[GitHub Release]($GITHUB_SERVER_URL/$GITHUB_REPOSITORY/releases/tag/$TAG)"; echo "$d"; } >> "$GITHUB_OUTPUT"
      - uses: particle-iot/firmware-upload-action@9e393d6b362208488db91339ab72ee1d7e2e6ada # v1.0.4
        with: {particle-access-token: "${{ secrets.PARTICLE_ACCESS_TOKEN }}", firmware-path: "${{ steps.meta.outputs.bin }}", firmware-version: "${{ steps.meta.outputs.version }}",
          product-id: 42131, title: "${{ steps.meta.outputs.title }}", description: "${{ steps.meta.outputs.description }}"}
ls: build-tmp: No such file or directory
tests:
README.md
SensorManagerBatteryAuthorityTests.cpp
alert19_clears_after_report_test.sh
awake_cycle_counters_test.sh
battery_authority_commit_sites_structural_test.py
battery_authority_seam_structural_test.py
battery_authority_structural_test.py
battery_health_test.cpp
battery_health_test.sh
battery_tier_guard_test.cpp
battery_tier_guard_test.sh
binding_delete_guard_order_test.py
binding_delete_guard_test.sh
boron_soc_commit_test.py
build_flags_witness_test.sh
bump_version_scheme_test.sh
charge_inhibit_thermal_test.cpp
charge_inhibit_thermal_test.sh
clock_hibernate_helpers_move_structural_test.py
clock_openness_test.cpp
clock_openness_test.sh
clock_owner_structural_test.py
clock_resync_wiring_test.py
clock_rtc_writeback_test.cpp
clock_status_republish_test.cpp
clock_status_republish_test.sh
clock_trust_standard_structural_test.py
clock_trust_test.cpp
close_before_night_sleep_test.py
config_window_hold_test.cpp
config_window_hold_test.sh
conn_phase_modem_off_test.sh
connect_attempt_counter_test.sh
connect_no_modem_wait_structural_test.py
connected_online_last_connection_test.cpp
connected_online_last_connection_test.sh
connection_mode_downgrade_structural_test.py
connection_mode_downgrade_test.cpp
connection_mode_downgrade_test.sh
connection_signal_validity_test.sh
connectivity_failsafe_move_structural_test.py
connectivity_failsafe_open_hours_test.cpp
connectivity_failsafe_open_hours_test.sh
connectivity_failsafe_stage1_retired_test.py
connectivity_failsafe_unknown_openness_test.cpp
connectivity_failsafe_unknown_openness_test.sh
daily_cleanup_boundary_test.py
debounce_expiry_c1_test.sh
default_timezone_test.cpp
default_timezone_test.sh
deliberate_reset_flush_test.py
exerciser_baseline_test.py
facade_header_isolation_test.py
failsafe_cadence_rule_test.cpp
failsafe_cadence_rule_test.sh
firmware_update_dwell_test.sh
firmware_update_wiring_structural_test.py
fixtures
hibernate_cycle_single_owner_structural_test.py
hibernate_wake_diagnostics_test.cpp
hibernate_wake_diagnostics_test.sh
hour_rules_test.cpp
hour_rules_test.sh
hourly_while_occupied_test.cpp
hourly_while_occupied_test.sh
idle_timediag_on_change_test.cpp
idle_timediag_on_change_test.sh
ledger_no_retry_test.cpp
ledger_no_retry_test.sh
loop_forensics_move_structural_test.py
loop_stage_sleep_prep_exclusion_test.py
modem_health_equivalence_test.cpp
modem_health_equivalence_test.sh
modem_health_owner_structural_test.py
modem_teardown_confirmation_logging_test.py
nightly_heap_guard_flush_test.sh
occupancy_ends_at_close_structural_test.py
occupancy_report_by_mode_test.cpp
occupancy_report_by_mode_test.sh
occupancy_session_restart_test.sh
occupancy_start_time_clamp_test.cpp
occupancy_start_time_clamp_test.sh
oom_immediate_reset_test.sh
persisted_layout_preservation_test.py
persistence_facade_behavior_test.cpp
persistence_facade_behavior_test.sh
persistence_facade_fanin_structural_test.py
pir_sensor_lifecycle_test.cpp
pir_sensor_lifecycle_test.sh
pir_sensor_reset_totals_test.cpp
pir_sensor_reset_totals_test.sh
pmic_fault_monitor_test.cpp
pmic_fault_monitor_test.sh
pmic_latch_call_site_structural_test.py
power_composition_test.cpp
power_composition_test.sh
power_constants_structural_test.py
power_constants_values_test.cpp
power_constants_values_test.sh
power_consumer_regression_test.sh
power_diagnostics_batch_test.cpp
power_diagnostics_batch_test.sh
power_diagnostics_log_guard_test.cpp
power_diagnostics_log_guard_test.sh
power_manager_accessors_test.cpp
power_source_override_test.cpp
power_source_override_test.sh
power_tier_test.cpp
power_tier_test.sh
publish_with_ack_structural_test.py
release_upload_job_test.py
report_payload_fields_test.py
report_payload_golden_test.cpp
report_payload_golden_test.sh
report_publisher_move_structural_test.py
reporting_interval_cap_test.cpp
reporting_interval_cap_test.sh
reporting_interval_store_seam_structural_test.py
reporting_policy_adapter_test.cpp
reporting_policy_adapter_test.sh
reporting_policy_clock_owner_structural_test.py
reporting_policy_test.cpp
reset_cause_structural_test.py
reset_wiring_test.cpp
reset_wiring_test.sh
retained_boot_structural_test.py
retained_boot_test.cpp
retained_boot_test.sh
rtc_skew_test.cpp
rtc_skew_test.sh
sensor_type_source_test.cpp
sensor_type_source_test.sh
sensor_type_validation_test.cpp
sensor_wake_source_test.cpp
sensor_wake_source_test.sh
serial_settle_test.py
sleep_breadcrumb_sequence_test.py
sleep_cloud_gate_equivalence_test.cpp
sleep_cloud_gate_equivalence_test.sh
sleep_cloud_gate_structural_test.py
sleep_config_leak_test.cpp
sleep_config_leak_test.sh
sleep_config_ownership_structural_test.py
sleep_duration_consumes_openness_test.py
sleep_execution_equivalence_test.cpp
sleep_execution_equivalence_test.sh
sleep_execution_structural_test.py
sleep_gate_modem_off_test.sh
sleep_plan_equivalence_test.cpp
sleep_plan_equivalence_test.sh
sleep_plan_structural_test.py
sleep_preflight_equivalence_test.cpp
sleep_preflight_equivalence_test.sh
sleep_preflight_structural_test.py
sleep_prep_span_timing_test.cpp
sleep_teardown_equivalence_test.cpp
sleep_teardown_equivalence_test.sh
sleep_teardown_structural_test.py
sleep_wake_equivalence_test.cpp
sleep_wake_equivalence_test.sh
sleep_wake_structural_test.py
standby_latch_daily_clear_test.cpp
standby_latch_daily_clear_test.sh
startup_snapshot_test.cpp
stubs
thermal_coupling_structural_test.py
thermal_threshold_migration_test.cpp
thermal_threshold_migration_test.sh
untrusted_piece_c2_test.sh
version_guard_preprocessor_test.sh
wake_gate_single_owner_structural_test.py
wake_summary_led_test.cpp
wake_summary_led_test.sh
watchdog_ab1805_classification_test.cpp
watchdog_ab1805_classification_test.sh
watchdog_alert_code_test.cpp
watchdog_alert_code_test.sh
webhook_timeout_range_test.py
<shellId: 1 completed with exit code 1>
```

</details>


---

<sub>12s</sub>

### Copilot

Look at an analogous existing test and BuildProfile.h.

---

<sub>12s</sub>

### `bash`

**Inspect analog test and header**

`$ cat tests/version_guard_preprocessor_test.sh; cat src/BuildProfile.h | head -60; grep -n "Production verification" docs/bench-validation.md; which cc; cc --version|head -1; zsh --version; python3 --version`

<details>
<summary>103 lines</summary>

```
#!/bin/zsh
set -euo pipefail

# WO-2026-11-03-003 Amendment 4/5: the release workflow's source-version guard asks the
# C++ preprocessor for the ACTIVE FIRMWARE_PRODUCT_VERSION, so a commented-out or
# indented decoy cannot fool it. Runs the workflow's own `got=` line (extracted, not copied)
# against fixtures; Stage 7 round 3 (Codex) supplied the decoy.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WF="$ROOT/.github/workflows/build.yml"
fail=0

line="$(grep -F 'got=$(echo FIRMWARE_PRODUCT_VERSION' "$WF" | sed -e 's/^ *//' -e 's/  # .*$//')"
[ -n "$line" ] || { echo "FAIL: guard line not found in build.yml (the sed form must be gone)"; exit 1; }
[ "$(grep -c 'got=$(sed' "$WF")" = 0 ] || { echo "FAIL: the old sed parse is still in build.yml"; fail=1; }

# got <fixture header contents> -> prints what the guard would read
got() {
  local d; d="$(mktemp -d)"; mkdir -p "$d/src"
  printf '%s\n' "$1" > "$d/src/FirmwareVersion.h"
  bash -c "set -euo pipefail; cd '$d'; $line; printf '%s' \"\$got\""
  rm -rf "$d"
}

expect() { # <label> <want> <header>
  local g; g="$(got "$3")"
  [ "$g" = "$2" ] || { echo "FAIL: $1: read '$g', want '$2'"; fail=1; }
}

expect "plain define" 4890 '#define FIRMWARE_PRODUCT_VERSION 4890'
expect "indented define" 4890 ' #define FIRMWARE_PRODUCT_VERSION 4890'
expect "trailing comment" 4890 '#define FIRMWARE_PRODUCT_VERSION 4890 // v48.90'
expect "Codex decoy: commented canonical, indented active" 1 '/*
#define FIRMWARE_PRODUCT_VERSION 4890
*/
 #define FIRMWARE_PRODUCT_VERSION 1'
expect "line-commented canonical, active below" 7 '// #define FIRMWARE_PRODUCT_VERSION 4890
#define FIRMWARE_PRODUCT_VERSION 7'
expect "the repository file" "$(sed -n 's/^#define FIRMWARE_PRODUCT_VERSION *//p' "$ROOT/src/FirmwareVersion.h")" "$(cat "$ROOT/src/FirmwareVersion.h")"

# The decoy must NOT equal the tag's value: that is what makes the guard fail closed.
[ "$(got '/*
#define FIRMWARE_PRODUCT_VERSION 4890
*/
 #define FIRMWARE_PRODUCT_VERSION 1')" != 4890 ] || { echo "FAIL: decoy read as the commented value"; fail=1; }

[ "$fail" = 0 ] && echo "PASS: version_guard_preprocessor_test" || exit 1
#pragma once

/**
 * @file BuildProfile.h
 * @brief Single owner of compile-time build profile, logging policy and the
 *        compiled build-flags witness.
 *
 * @details
 * - FIELD builds must never block on USB serial.
 * - DEV builds may opt-in to blocking waits by defining
 *   ALLOW_BLOCKING_SERIAL_WAITS=1 at build time.
 * - Logging remains enabled in all profiles; these flags only affect
 *   whether blocking waits are compiled into the firmware.
 *
 * Profile selector (the one switch that picks a whole build shape):
 * - Release (the default): no build flag needed.
 * - Bench: -DBUILD_PROFILE_BENCH=1 turns on the bench diagnostics
 *   (ENABLE_DIAGNOSTICS_PUBLISH_MODE). It deliberately does NOT turn on
 *   ENABLE_RTC_SKEW_TEST, which stays an explicit, separately-requested flag.
 * - Any individual flag below can still be overridden with its own -D.
 *
 * Usage examples (build flags):
 * - FIELD (non-blocking): -DDEV_BUILD=0
 * - DEV (non-blocking): -DDEV_BUILD=1
 * - DEV (blocking waits enabled): -DDEV_BUILD=1 -DALLOW_BLOCKING_SERIAL_WAITS=1
 * - Bench profile: -DBUILD_PROFILE_BENCH=1
 * - Serial log level (0=none,1=error,2=warn,3=info+filters,4=all): -DSERIAL_LOG_LEVEL=4
 * - Connectivity failsafe bench test: -DCONNECTIVITY_FAILSAFE_TEST_MODE=1
 * - PMIC forensics override: -DENABLE_PMIC_FORENSICS=0
 * - Ledger trace logging: -DENABLE_LEDGER_TRACE=1
 * - Connect trace logging: -DENABLE_CONNECT_TRACE=1
 * - Connect decision trace logging: -DENABLE_CONNECT_DECISION_TRACE=1
 * - Performance trace logging: -DENABLE_PERF_TRACE=1
 * - Sleep-gate trace logging: -DENABLE_GATE_TRACE=1
 * - Sleep routine trace logging: -DENABLE_SLEEP_TRACE=1
 * - Routine config trace logging: -DENABLE_CONFIG_TRACE=1
 * - Diagnostics publish mode: -DENABLE_DIAGNOSTICS_PUBLISH_MODE=1
 * - RTC skew bench hook (Boron only): -DENABLE_RTC_SKEW_TEST=1
 *
 * WO-2026-08-24-001: the Boron USB power-source override in
 * PowerManager::refreshInputProfile() previously had its own build flag
 * (ENABLE_BORON_USB_SOURCE_OVERRIDE) which was found compiled OFF in a
 * shipped product build despite the source default being 1 and the version
 * string being identical to a build where it was ON -- an undetectable
 * silent-drift failure mode. The flag has been removed; the override now
 * compiles unconditionally on Boron (still gated only by the real
 * PLATFORM_ID == PLATFORM_BORON platform check in PowerManager.cpp). Its own
 * `usbEnumerated` predicate already self-limits cost on non-USB devices.
 *
 * Trace flag notes:
 * - ENABLE_LEDGER_TRACE=1 enables detailed ledger request lifecycle logs.
 * - ENABLE_CONNECT_TRACE=1 enables connection acquire diagnostics.
 * - ENABLE_CONNECT_DECISION_TRACE=1 enables connection decision rationale logs.
 * - ENABLE_PERF_TRACE=1 enables successful report performance timing logs.
 * - ENABLE_GATE_TRACE=1 enables sleep gate polling/detail logs.
 * - ENABLE_SLEEP_TRACE=1 enables routine sleep/watchdog/pre-sleep battery logs.
 * - ENABLE_CONFIG_TRACE=1 enables routine config apply/validation diagnostics.
 * - Defaults remain 0 for field-safe release logging.
 *
 * Current repo defaults are set for field-safe behavior.
/usr/bin/cc
Apple clang version 14.0.3 (clang-1403.0.22.14.1)
zsh 5.9 (arm64-apple-darwin26.0)
Python 3.14.6
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>16s</sub>

### `bash`

**Probe macros standalone**

```
$ grep -n "DEV_BUILD\|CONNECTIVITY_FAILSAFE_TEST_MODE\|#include" src/BuildProfile.h | grep -v '^\S*\s*\*'; grep -n -i "production verification" docs/*.md | head -3
echo DEV_BUILD CONNECTIVITY_FAILSAFE_TEST_MODE | cc -E -P -x c++ -include src/BuildProfile.h - ; echo rc=$?
```

<details>
<summary>14 lines</summary>

```
79:#ifndef DEV_BUILD
83:#define DEV_BUILD 0
90:#define FIELD_BUILD (!DEV_BUILD)
103:#ifndef CONNECTIVITY_FAILSAFE_TEST_MODE
114:#define CONNECTIVITY_FAILSAFE_TEST_MODE 0
117:#if (CONNECTIVITY_FAILSAFE_TEST_MODE != 0) && (CONNECTIVITY_FAILSAFE_TEST_MODE != 1)
118:#error "CONNECTIVITY_FAILSAFE_TEST_MODE must be 0 or 1"
298:    ((DEV_BUILD ? 0x0001 : 0) |                                                \
300:     (CONNECTIVITY_FAILSAFE_TEST_MODE ? 0x0004 : 0) |                          \
docs/bench-validation.md:80:Recommended production verification:
docs/recovery-architecture.md:79:- Production verification should confirm `FailsafeTest=0` in the boot log and production timing values in the failsafe diagnostic line.
0 0
rc=0
<shellId: 3 completed with exit code 0>
```

</details>


---

<sub>6m 58s</sub>

### `bash`

**Measure baseline**

`$ mkdir -p build-tmp/wo-2026-11-08-004-stage6; p=0; t=0; for f in tests/*.sh; do t=$((t+1)); if zsh $f >/dev/null 2>&1; then p=$((p+1)); else echo FAIL $f; fi; done; for f in tests/*.py; do t=$((t+1)); if python3 $f >/dev/null 2>&1; then p=$((p+1)); else echo FAIL $f; fi; done; echo "$p/$t baseline"`

```
117/117 baseline
<shellId: 4 completed with exit code 0>
```


---

<sub>7m 9s</sub>

### `bash`

**View macro defaults**

`$ sed -n 76,116p src/BuildProfile.h`

<details>
<summary>37 lines</summary>

```
#error "BUILD_PROFILE_BENCH must be 0 or 1"
#endif

#ifndef DEV_BUILD
/**
 * @brief Set to 1 for developer builds.
 */
#define DEV_BUILD 0
#endif

#ifndef FIELD_BUILD
/**
 * @brief Derived flag for field builds (true when DEV_BUILD=0).
 */
#define FIELD_BUILD (!DEV_BUILD)
#endif

#ifndef ALLOW_BLOCKING_SERIAL_WAITS
/**
 * @brief Allow explicit blocking waits for USB serial attachment.
 *
 * Disabled in v22-Diag-Soak: Pi forwarder now captures serial continuously,
 * so the bench-only forced wait serves no purpose and wastes battery on reset.
 */
#define ALLOW_BLOCKING_SERIAL_WAITS 0
#endif

#ifndef CONNECTIVITY_FAILSAFE_TEST_MODE
/**
 * @brief Bench-only override for connectivity failsafe timing.
 *
 * Set to 1 only for explicit test builds that need short stale/cooldown
 * windows. Production builds must leave this at 0.
 *
 * This is compile-time only. It is not sourced from cloud/ledger config,
 * and should not be toggled through Config.h. Enable it only with an
 * explicit build flag or a deliberate local edit in this build-profile layer.
 */
#define CONNECTIVITY_FAILSAFE_TEST_MODE 0
#endif
<shellId: 5 completed with exit code 0>
```

</details>


---

<sub>7m 13s</sub>

### Copilot

Add the workflow step.

---

<sub>7m 13s</sub>

### `edit`

**edit the file at .github/workflows/build.yml.**

`.github/workflows/build.yml`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-f5/.github/workflows/build.yml updated with changes.
```


---

<sub>7m 30s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-f5/tests/release\_flags\_guard\_test.sh.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-f5/tests/release_flags_guard_test.sh`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-f5/tests/release_flags_guard_test.sh with 3805 characters
```


---

<sub>7m 34s</sub>

### `bash`

**Run new test bare**

`$ zsh tests/release_flags_guard_test.sh; echo rc=$?`

```
FAIL: (e) header does not preprocess: no ::error:: message
rc=1
<shellId: 6 completed with exit code 0>
```


---

<sub>7m 39s</sub>

### `bash`

**Fix preprocess-failure error and rerun**

```
$ python3 - <<'E'
p='.github/workflows/build.yml'
s=open(p).read()
a="include src/BuildProfile.h - | tr '\\n' ' ')  # the active macros, as the compiler sees them"
assert a in s
s=s.replace(a,"include src/BuildProfile.h - | tr '\\n' ' ') || { echo \"::error::preprocessing src/BuildProfile.h failed; not building\"; exit 1; }  # the active macros, as the compiler sees them")
open(p,'w').write(s)
E
zsh tests/release_flags_guard_test.sh; echo rc=$?; git diff --stat
```

```
PASS: release_flags_guard_test
rc=0
 .github/workflows/build.yml | 9 +++++++++
 1 file changed, 9 insertions(+)
<shellId: 7 completed with exit code 0>
```


---

<sub>7m 55s</sub>

### `bash`

**Run mutation tests**

```
$ S=build-tmp/wo-2026-11-08-004-stage6; F=.github/workflows/build.yml; cp $F $S/build.yml.orig
m() { # label, python expr transforming s
  python3 - "$F" "$2" <<'E'
import sys
p,expr=sys.argv[1],sys.argv[2]
s=open(p).read(); t=eval(expr)
assert t!=s, "no change"
open(p,'w').write(t)
E
  if zsh tests/release_flags_guard_test.sh >$S/m.out 2>&1; then echo "MUTANT SURVIVED: $1"; else echo "killed: $1 -> $(head -1 $S/m.out)"; fi
  cp $S/build.yml.orig $F
}
m "if guard removed" 's.replace("        if: github.ref_type == '"'"'tag'"'"'\n        run: |\n          set -euo pipefail\n          out=","        run: |\n          set -euo pipefail\n          out=")'
m "-ge 0" 's.replace("[ \"$dev\" = 0 ]","[ \"$dev\" -ge 0 ]")'
m "only DEV_BUILD checked" 's.replace("          [ \"$fs\" = 0 ] || { echo \"::error::CONNECTIVITY_FAILSAFE_TEST_MODE is","          true || { echo \"::error::CONNECTIVITY_FAILSAFE_TEST_MODE is")'
m "only FAILSAFE checked" 's.replace("[ \"$dev\" = 0 ] ||","true ||")'
m "error exit removed" 's.replace("must be 0 for a tag build; not building\"; exit 1; }\n          [ \"$fs\"","must be 0 for a tag build; not building\"; }\n          [ \"$fs\"")'
m "string compare other value" 's.replace("[ \"$dev\" = 0 ]","[ \"$dev\" = \"00\" ]")'
m "extra tokens allowed" 's.replace(" && [ -z \"$extra\" ]","")'
m "step after Compile" '(lambda a,b: s)(0,0) if False else __import__("re").sub(r"(      - name: Release flags.*?\n)(      - name: Read Device OS)", "\\2", s, flags=16) if False else None or s.replace(s[s.index("      - name: Release flags"):s.index("      - name: Read Device OS")],"").replace("      - uses: actions/upload-artifact", s[s.index("      - name: Release flags"):s.index("      - name: Read Device OS")]+"      - uses: actions/upload-artifact",1)'
m "EXTRA_CFLAGS added" 's.replace("          device-os-version: ${{ steps.props.outputs.os }}\n","          device-os-version: ${{ steps.props.outputs.os }}\n          particle-platform-name: boron\n          extra-cflags: -DDEV_BUILD=1\n",1)'
cmp $F $S/build.yml.orig && echo restored; git diff --stat
```

<details>
<summary>13 lines</summary>

```
killed: if guard removed -> FAIL: the step has no tag-only if: guard
killed: -ge 0 -> FAIL: (b) DEV_BUILD 1: guard passed, must fail
killed: only DEV_BUILD checked -> FAIL: (c) CONNECTIVITY_FAILSAFE_TEST_MODE 1: guard passed, must fail
killed: only FAILSAFE checked -> FAIL: (b) DEV_BUILD 1: guard passed, must fail
killed: error exit removed -> FAIL: (b) DEV_BUILD 1: guard passed, must fail
killed: string compare other value -> FAIL: (a) real tree: guard failed: ::error::DEV_BUILD is '0', must be 0 for a tag build; not building
MUTANT SURVIVED: extra tokens allowed
killed: step after Compile -> FAIL: the guard step is not before the Compile step
killed: EXTRA_CFLAGS added -> FAIL: the Compile step passes compiler flags (EXTRA_CFLAGS, -D or cflags); rule on it, then update this test
restored
 .github/workflows/build.yml | 9 +++++++++
 1 file changed, 9 insertions(+)
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>8m 4s</sub>

### `bash`

**Add extra-token case, re-mutate**

```
$ python3 - <<'E'
p='tests/release_flags_guard_test.sh'
s=open(p).read()
a="[ \"$fail\" = 0 ] && echo"
s=s.replace(a,'''# A third token (a macro expanding to two tokens) is not "two tokens": fail closed.
d="$(fixture extra 's/^#define DEV_BUILD 0$/#define DEV_BUILD 0 0/')"
expect_fail "extra preprocessor tokens" "$d" ''

'''+a)
open(p,'w').write(s)
E
S=build-tmp/wo-2026-11-08-004-stage6; F=.github/workflows/build.yml
zsh tests/release_flags_guard_test.sh
sed -i.bak 's/ && \[ -z "\$extra" \]//' $F; rm $F.bak; zsh tests/release_flags_guard_test.sh; echo rc=$?; cp $S/build.yml.orig $F; cmp $F $S/build.yml.orig && echo restored; zsh tests/release_flags_guard_test.sh
```

<details>
<summary>7 lines</summary>

```
PASS: release_flags_guard_test
FAIL: extra preprocessor tokens: guard passed, must fail
FAIL: extra preprocessor tokens: no ::error:: message
rc=1
restored
PASS: release_flags_guard_test
<shellId: 9 completed with exit code 0>
```

</details>


---

<sub>8m 6s</sub>

### Copilot

Now the doc note, full discovery, clean export, and report.

---

<sub>8m 6s</sub>

### `bash`

**View doc section**

`$ sed -n 78,90p docs/bench-validation.md`

<details>
<summary>10 lines</summary>

```
override it with `-DCONNECTIVITY_FAILSAFE_TEST_MODE=0`.

Recommended production verification:

- Build without `EXTRA_CFLAGS` overrides.
- Confirm boot logs show `FailsafeTest=0`.
- Confirm failsafe timing logs show the production values: `stale=43200s`,
  `cooldown=21600s`, `jitter=1800s`.

## Bench Setup

- Put the device on a **bench supply** or a known-good LiPo.
- Measure current with either:
<shellId: 10 completed with exit code 0>
```

</details>


---

<sub>8m 11s</sub>

### `bash` — Failed

**Doc note, full discovery, clean export**

```
$ python3 - <<'E'
p='docs/bench-validation.md'
s=open(p).read()
a="  `cooldown=21600s`, `jitter=1800s`.\n"
assert s.count(a)==1
s=s.replace(a,a+"- A `v*` tag build fails before compiling unless `DEV_BUILD` and `CONNECTIVITY_FAILSAFE_TEST_MODE` are 0 (`build.yml`, tested by `tests/release_flags_guard_test.sh`).\n")
open(p,'w').write(s)
E
p=0;t=0; for f in tests/*.sh; do t=$((t+1)); zsh $f >/dev/null 2>&1 && p=$((p+1)) || echo FAIL $f; done; for f in tests/*.py; do t=$((t+1)); python3 $f >/dev/null 2>&1 && p=$((p+1)) || echo FAIL $f; done; echo "$p/$t (sh via zsh, py via python3)"
git ls-files -z | grep -zv '^build-tmp/' >/dev/null; rm -rf build-tmp/wo-2026-11-08-004-stage6/export; mkdir build-tmp/wo-2026-11-08-004-stage6/export; git ls-files | grep -v '^build-tmp/' | tar -cf - -T - 2>/dev/null | tar -xf - -C build-tmp/wo-2026-11-08-004-stage6/export; (cd build-tmp/wo-2026-11-08-004-stage6/export && python3 tests/thermal_coupling_structural_test.py >/dev/null 2>&1; echo export rc=$?); git diff --stat; git status --short
```

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>8m 13s</sub>

### `bash`

**Check state**

`$ git diff --stat; git status --short`

```
.github/workflows/build.yml | 9 +++++++++
 1 file changed, 9 insertions(+)
 M .github/workflows/build.yml
?? tests/release_flags_guard_test.sh
<shellId: 12 completed with exit code 0>
```


---

<sub>8m 17s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-f5/docs/bench-validation.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-f5/docs/bench-validation.md`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-f5/docs/bench-validation.md updated with changes.
```


---

<sub>15m 31s</sub>

### `bash`

**Full test discovery**

`$ p=0;t=0; for f in tests/*.sh; do t=$((t+1)); if zsh $f >/dev/null 2>&1; then p=$((p+1)); else echo FAIL $f; fi; done; for f in tests/*.py; do t=$((t+1)); if python3 $f >/dev/null 2>&1; then p=$((p+1)); else echo FAIL $f; fi; done; echo "$p/$t (sh via zsh, py via python3)"`

```
118/118 (sh via zsh, py via python3)
<shellId: 13 completed with exit code 0>
```


---

<sub>15m 34s</sub>

### Copilot

Now the clean export check.

---

<sub>16m 1s</sub>

### `bash`

**Thermal test on clean export**

`$ E=build-tmp/wo-2026-11-08-004-stage6/export; mkdir -p $E; git ls-files | grep -v '^build-tmp/' | while read f; do mkdir -p "$E/$(dirname "$f")"; cp "$f" "$E/$f"; done; (cd $E && python3 tests/thermal_coupling_structural_test.py >/dev/null 2>&1; echo export rc=$?); rm -rf $E; git diff --stat; git status --short`

<details>
<summary>8 lines</summary>

```
export rc=0
 .github/workflows/build.yml | 9 +++++++++
 docs/bench-validation.md    | 1 +
 2 files changed, 10 insertions(+)
 M .github/workflows/build.yml
 M docs/bench-validation.md
?? tests/release_flags_guard_test.sh
<shellId: 14 completed with exit code 0>
```

</details>


---

<sub>16m 16s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-f5/docs/work-orders/WO-2026-11-08-004-stage6-copilot-report.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-f5/docs/work-orders/WO-2026-11-08-004-stage6-copilot-report.md`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-f5/docs/work-orders/WO-2026-11-08-004-stage6-copilot-report.md with 3903 characters
```


---

<sub>16m 22s</sub>

### Copilot

The tag-only release-flag guard is in place and the new test passes bare. Full discovery is 118/118 (sh via zsh, py via python3), against a measured baseline of 117/117 before editing. `thermal_coupling_structural_test.py` also passes on a clean export of tracked files. Nothing was committed, and `git diff --stat` shows nothing under `src/` or `lib/`. I did not run a real tag build, so the extracted-script test is the only proof.

- **Workflow step:** the new step in `.github/workflows/build.yml` is +9 lines (limit +12). It runs only on tags, before "Compile", and emits `::error::` naming the macro and value unless both are exactly `0`. It also fails on a preprocess failure or anything other than two tokens.
- **Test:** `tests/release_flags_guard_test.sh` covers the six WO cases (a–f). I added three more: an undefined macro, extra preprocessor tokens, and the step carrying its tag-only `if:` guard and sitting before Compile.
- **Mutations:** all six mutations fail the test. I also tried two extras. The `[ -z "$extra" ]` mutant survived at first, so I added the extra-token case, and it now fails. `build.yml` was restored from the backup and byte-compared.
- **Doc note:** I added the one line to `docs/bench-validation.md` (item 4).
- **Not observed:** I could not observe the reasoning level actually run (dispatched as medium); the report says so.
- **Not run:** `gh pr checks` (no PR exists), and the test ran on macOS clang rather than the runner's `cc`.

The report is `docs/work-orders/WO-2026-11-08-004-stage6-copilot-report.md`.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>
