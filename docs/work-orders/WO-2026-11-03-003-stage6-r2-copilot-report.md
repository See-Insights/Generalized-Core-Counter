# WO-2026-11-03-003 Part A — Stage 6 round 2 report

**Model / reasoning actually used:** claude-sonnet-5.5; the reasoning level was not visible to me (the dispatch header says "medium").
**Interpreter line:** YAML parsed by Ruby 2.6.10/Psych (→ JSON, then Python 3); shell via `bash -n` and `bash -e` on macOS (system bash); the workflow itself runs on ubuntu bash. Host suite: zsh / python3. Node v26.3.0 for the scratch `particle-cli`. Nothing committed, pushed, tagged, or released; no `gh` write.

## What changed
- `.github/workflows/build.yml`: `release` job replaced by `inspect` + `publish`; `firmware` gained `outputs` (`sha256`, `digest`) and a new final step `rec` (placed after `upload-artifact`, so PR #177's edit inside the summary step applies without a conflict).
- `bump_version.sh`: `derive_product_version()` (new scheme, legacy dotless kept, else rejected); the function sits before the usage check and a `BUMP_VERSION_SOURCE_ONLY=1` hook returns before any side effect.
- `tests/bump_version_scheme_test.sh` (new): sources the real function.
- `src/FirmwareVersion.h`: comment only (the constant stays 48; the comment grew by 2 lines, still in the header block).
- `PROJECT_STATUS.md`: version-strategy sentences (and the `bump_version.sh 20` example, now rejected by the script).
- `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 9: "release job" → `publish`, digest source, hash-link sentence (it contradicted the split).

## Budget (`wc -l build.yml`)
- Current file: **166** lines.
- vs round-1 base 81 (no #177 line): **+85** (limit ≤ 85).
- vs the 83-line merged base: **+83**. #177's digest line is a one-line edit inside the summary step; after it merges the file will be 167.
- `src/`: comment-only (`git diff` shows only ` * ` lines).

## Pins
`actions/checkout` 3d3c42e5… (v7.0.1), `actions/download-artifact` 9000827c… (v8.0.2), `upload-artifact` and `compile-action` unchanged; `particle-cli@3.52.4` with `--ignore-scripts`, installed with `--prefix "$RUNNER_TEMP/pcli"` (no `-g`, no sudo). `gh` is the runner's preinstalled one.

## Verification (what ran)
- YAML parses; `bash -n` on every `run:` block passes. **`actionlint` is not installed; not run.**
- `particle-cli@3.52.4` installed in a scratch prefix with `--ignore-scripts` (629 packages): `particle --version` = 3.52.4, and `particle binary inspect` on the Dev-09 `.bin` works ("product id 13 at version 48"; the workflow's sed extracts `48`).
- Publish-step dry run (`build-tmp/wo-2026-11-03-003-stage6-r2/dry.py`; the exact `run:` blocks extracted from the YAML, stub `gh`, real Dev-09 `.bin`, scratch git repos):
  - `v48.91` vs 48: fails, no `gh`. `v48.90` vs 4890: passes, `gh release view` then `gh release create … --verify-tag --title v48.90 --notes-from-tag`. `v08.05` vs 805: passes.
  - `v4611686018427387904.48` and `v48.100`: rejected by the regex before any arithmetic.
  - Hash mismatch: fails. Lightweight tag (`git cat-file -t` is `commit`): fails. Tag not at `GITHUB_SHA`: fails. Existing Release: fails before create.
  - Token: only the `Release` step has `GH_TOKEN`; the stub `gh` saw it there. The `inspect` job has `contents: read` and no `env` at all; `publish` job-level env has no token; the workflow default stays `contents: read`.
- Host suite: see the result line at the end of this file.

## Could not verify
- **`gh release create --notes-from-tag` itself:** `gh release` is blocked for me, and it would be a write. The design relies on gh reading the annotated tag message from local git, which is why `publish` has a checkout. `actions/checkout` on a tag-triggered run fetches `refs/tags/<tag>`, which I expect to include the tag object, so the annotation is present; I could not run it. The `git cat-file -t` and `rev-parse "$TAG^{commit}"` guards would fail first if it were not. **The `v48.90` live dry run (after Chip applies the repo settings) is the real test.**
- **Digest:** `docker image inspect` is wired in `rec` using the same command as #177 but was not run (no docker here). The step fails the firmware job if the image tag is absent. Please confirm on the first CI run. If #177 changes how the digest is read, keep `rec` and #177's summary line consistent.
- **Token on `actions/*` steps:** `download-artifact` (inspect) and `checkout` (publish) have a `github-token`/`token` input that defaults to `github.token`; they are not `env` exposures and I left those defaults. In `inspect` that token is read-only. I did not test whether `github-token: ''` works, so I did not use it. The strict reading of "no GITHUB_TOKEN-using step in inspect" is therefore not met for the download action; say if you want it tried.
- `stat -c%s` in the success summary is GNU (Linux runner). It fails on macOS, so the dry run's summary line was not exercised there.
- Live Actions behaviour (job outputs wiring, `if: github.ref_type == 'tag'` skipping `inspect`/`publish` on branches) was not run.
- Open items from Stage 7 r1 that are outside this dispatch (ruleset settings, dev-device pins, Ubidots, the 16-bit product version) are unchanged.

## Blockers B1–B5
| # | Blocker | Resolution |
|---|---|---|
| B1 | Overflow bypass of the version guard | Tag validated with `^v([0-9]{1,2})\.([0-9]{1,2})$` before any arithmetic; `10#` kept. Overflow tag and `v48.100` rejected in the dry run. |
| B2 | Write token exposed to npm / CLI | Split: `inspect` (read-only, no env, `--ignore-scripts`, pinned CLI, outputs only the version) → `publish` (preinstalled `gh` only, no npm; `GH_TOKEN` on the single `gh release create` step). SHA-256 recorded by `firmware`, re-checked in both downstream jobs. Residual: transitive npm deps are still unlocked, but they now run with no token and no install scripts. Residual: default-token inputs of `download-artifact`/`checkout`, see above. |
| B3 | Digest absent | `firmware` outputs `digest` (step `rec`, after `upload-artifact` so #177 applies cleanly); `publish` prints it in the summary. Untested without docker; wired. #177's own summary line stays as is. |
| B4 | Tag creation is the authorization (ruleset) | Not a workflow fix; stays with the live `v*` ruleset. Added in the workflow: annotated-tag requirement and a check that the tag still points at `GITHUB_SHA`. |
| B5 | Pre-use checks / `bump_version.sh:29` | `bump_version.sh` now maps `v49.1-name` → 4901 and `v49` → 49 (legacy), and rejects the rest. Test `tests/bump_version_scheme_test.sh` covers `v49.1-Foo`, `v49.2`, `v48.90`, `v28-CloseBeforeSleep`, `v49`, `v08.05`, and rejects `v100.1`, `v49.100`, `49.1`, `v49.x`, the overflow tag, `v49.`, and the empty string. Dev-device pins, Ubidots and the 16-bit question are not Part A and remain open. |

**Host suite: 108/108 (sh via zsh, py via python3)** = previous 107 + the new test.
