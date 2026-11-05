# WO-2026-11-05-001 report: Particle console file name for the upload job

**Model / reasoning:** claude-sonnet-5.5; the dispatch header says medium (the CLI was launched with `--reasoning-effort medium`; I cannot independently observe the effective level).
**Interpreter line:** Python 3.14.6 (py tests), zsh 5.9 arm64-apple-darwin26.0 (.sh tests), ruby for YAML parsing (PyYAML is not installed on the host).

## Changed lines
`.github/workflows/build.yml` (still 179 lines), `upload` job `meta` step only:
- `env:` map gained `OS: "${{ needs.firmware.outputs.os }}"`.
- The SHA-256 check line now ends with
  `; new="$(dirname "$bin")/gcc-$TAG-boron-dos$OS-${GITHUB_SHA::7}.bin"; mv "$bin" "$new"; bin=$new`
  so the rename happens only after the single-`.bin` and hash checks pass, and `bin=$bin` in the output block now emits the renamed path.

`tests/release_upload_job_test.py`: stub env gets `OS=6.6.0` and `GITHUB_SHA`; every success case asserts `bin` ends with `gcc-v49.1-boron-dos6.6.0-abcdef1.bin`, `fw/` holds only that file, and content is unchanged (original path gone); changed-hash and two-`.bin` cases assert no rename occurred. New script mutations (all detected): bin = original path, name missing `dos`, name missing sha7.

`AI_DEVELOPMENT_WORKFLOW.md` Stage 9 item 7: added the console-file-name sentence.

## Verification
- YAML parses (ruby); `bash -n` OK on all 8 `run:` blocks.
- `tests/release_upload_job_test.py`: PASS.
- Host suite: 115/115 (sh via zsh, py via python3).
- `wc -l build.yml` = 179.
- `git diff --stat`: only build.yml, AI_DEVELOPMENT_WORKFLOW.md, the test (plus this report, untracked).
- `PARTICLE_ACCESS_TOKEN` appears once (action step); pin `9e393d6b…` unchanged; `publish` untouched.

## Deviations
- In the doc I also added "renames the `.bin` in place," to the existing sentence describing the job's re-checks, alongside the one new sentence.
- YAML check done with ruby, not PyYAML (not installed).
- Scratch `build-tmp/wo-2026-11-05-001/` holds only my temp files (r*.sh, o.txt).

## Open questions
- None blocking. Acceptance remains the next real tag (no dry-run upload exists).
