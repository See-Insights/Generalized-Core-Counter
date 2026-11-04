# WO-2026-11-03-003 Part B: Copilot report

Model: claude-sonnet-5.5 (dispatch asked for medium reasoning; the actual setting is not visible to me). Nothing committed, pushed, tagged or flashed; no Particle token or api.particle.io use; no network writes.

Interpreter line: `N/N (sh via zsh, py via python3)` = **111/111 (sh via zsh, py via python3)**.

## Changes
- `.github/workflows/build.yml`: job `upload` added after `publish`.
- `tests/release_upload_job_test.py`: new (structure checks, 7 mutations, stub runs of the `meta` step).
- `AI_DEVELOPMENT_WORKFLOW.md` Stage 9: item 7 (Part B description, post-tag Console check, scope failures).

## Verification
- YAML parse: OK (Ruby YAML; the test uses PyYAML if installed, else Ruby).
- `bash -n` on all 8 `run:` blocks (host-tests 2, firmware 4, publish 1, upload 1): OK.
- Host suite: 111/111 (110 existing + the new test; the dispatch expected 110 total, 109+1, but the tree already had 110 before my file).
- `wc -l build.yml`: 179; net **+25** over 154 (budget +25). Single hunk `@@ -154,0 +155,25 @@`, so `publish` and the other jobs are unchanged.
- grep: `PARTICLE_ACCESS_TOKEN` once (line 178, the action step's `with:`); action pinned `@9e393d6b362208488db91339ab72ee1d7e2e6ada # v1.0.4`; no `checkout` in `upload`; no `pull_request_target`; `git diff --stat -- src/` empty.
- Test mutations all detected: `@v1`, v1.0.5 SHA, token moved to `env:`, `needs` without `publish`, `environment` removed, token echoed in `run:`, plus `pull_request_target`.
- Stub runs: normal, one-line (empty description), hostile message (`$(id)`, backticks, `EOF`, `description<<abc`, `::set-output`, `version=9999`): outputs carry the text verbatim, only the four keys. Lightweight tag, bad tag names (`v49.1.2`, `v100.1`), changed hash, two `.bin`, no `.bin`: all fail. `v10.01` gives 1001 (no octal error).

## Deviations
- To fit +25 with every check kept, `permissions`, `env` of the `meta` step and the action's `with:` use flow-style YAML (the last is two lines, with long lines).
- `meta` does not re-check that the tag still points at `GITHUB_SHA` (`publish` does, and `upload` needs `publish`); not in the dispatch list.
- The scratch dir `build-tmp/wo-2026-11-03-003-stage6-partb/` was used and removed (only that subdirectory). An attempted `pip install pyyaml` did not land in the `python3` I use; no repo effect.
- The new test needs `shasum` or `sha256sum`, `openssl`, `bash`, and PyYAML or Ruby (all present on ubuntu-latest).

## Open questions
- Particle's title length limit is unknown to me; a long first tag line could be rejected.
- Whether the action fails on an already-existing version is unverified (cannot test without uploading).
- A CRLF in the tag message first line would reach `title` as is.
