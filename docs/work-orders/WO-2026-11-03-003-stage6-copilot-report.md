# WO-2026-11-03-003 Stage 6 — Copilot report (Part A)

- **Model / reasoning:** claude-sonnet-5.5; the reasoning effort the session ran at was not shown to me (dispatch asked for medium).
- **Interpreter line:** `.github/workflows/build.yml` run blocks use bash (`ubuntu-latest`); the dry run used macOS bash 3.2 with stubs for `gh`, `sha256sum`, `stat`; real `particle-cli` 3.52.4.
- **Branch:** `wo/2026-11-03-003-release-pipeline`; nothing committed or pushed.
- **Line budget:** `build.yml` 141 lines (limit 143; +58 over the original 83). `src/` and `tests/`: 0 changes (`git diff --stat` empty for both).

## Changes
- `build.yml`: triggers `push: branches ['**'], tags ['v*']` plus `pull_request`; host-tests step now exposes `result=N/N` as a job output; new `release` job (`if: github.ref_type == 'tag'`, `needs: [host-tests, firmware]`, `contents: write` on that job only, only `GITHUB_TOKEN`). It rejects tags not matching `^v[0-9]+\.[0-9]+$`, downloads `firmware-boron`, renames the `.bin`, writes `.sha256`, runs the version guard, refuses to touch an existing Release, runs `gh release create --verify-tag`, then writes the summary block. Tag and SHA values come in via `env`, not inline `${{ }}` in scripts.
- `.github/CODEOWNERS`: `/.github/workflows/ @chipmc`.
- `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 9: decisions 1-6 and the success-report block. No other section touched.

## Pins as used
- `actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1` # v7.0.1 (existing)
- `actions/download-artifact@9000827ccba6bdab643e8b6fd33ac0654aef8333` # v8.0.2 — re-verified with `gh api repos/actions/download-artifact/git/ref/tags/v8.0.2` (returned that SHA).
- `particle-iot/compile-action` and `actions/upload-artifact`: existing pins unchanged.
- `particle-cli@3.52.4` via `npm install -g` (version matches local `particle --version` 3.52.4). No `pull_request_target`, no `workflow_dispatch`.

## Verification done
- YAML parsed (Ruby); triggers, default `contents: read`, release job `contents: write`.
- `bash -n` on all 7 `run:` blocks: OK. `actionlint` is not installed: not run.
- Dry run of the release step with `dev09-ci-built-v48-0c8b5a2.bin` (real `particle binary inspect`: "firmware for product id 13 at version 48"):
  - Tag `v48.91`: name `gcc-v48.91-boron-dos6.6.0-0c8b5a2.bin`; guard FAILS (`embedded version '48' != 4891`), no `gh release create`.
  - Simulated 4890 with tag `v48.90`: guard PASSES; sha256 line `83218db3b47b4d5f9030284105f66bd04b86e0da6eb45c7e65e4cda78c3667b3  gcc-v48.90-boron-dos6.6.0-0c8b5a2.bin`; summary block printed with size 152890 bytes.
  - Tags `v48.9.1` and `v49` are rejected by the tag step.
- Scratch is in `build-tmp/wo-2026-11-03-003-stage6/` (left in place).

## Unverified / judgement calls
- The workflow was not run; the controller's live dry-run tags are needed.
- Buildpack image digest: the firmware job in this workflow does not print one (only device-os-version and firmware-version), so I did not pass it; the summary line says "see the firmware job summary". If the compile-action emits a digest it still needs adding to the firmware job.
- Existing Release for the tag: the job fails with an error rather than skipping silently, so a re-run of a successful tag fails. Change to skip if you prefer.
- Iteration > 99 is rejected (it would collide with the next major's numbers); not in the WO text.
- The inspect parse depends on the line "firmware for product id N at version V" in 3.52.4 output; the pinned version keeps it stable. `stat -c%s` and `sha256sum` are GNU forms (fine on ubuntu-latest; stubbed locally).
- Tag runs also run host-tests and firmware before the tag check, so a bad `v*` tag fails only after those complete.
- CODEOWNERS only takes effect once a ruleset requires code-owner review (Chip's setting).
