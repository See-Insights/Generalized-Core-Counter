# WO-2026-11-03-003 Stage 6 round 3: Copilot report

- Model / reasoning: claude-sonnet-5.5, medium (as dispatched; no override observed).
- Interpreter line: host tests run as `zsh` for `*.sh`, `python3` (3.10) for `*.py`; workflow blocks syntax-checked with `bash -n` (macOS bash 3.2) and exercised under bash 3.2 with stubs. Ruby `YAML` used for the YAML parse (no PyYAML on the host).
- Base: detached HEAD at `1c4ae75` (dispatch names `401c370`); all changes uncommitted. Scratch: `build-tmp/wo-2026-11-03-003-stage6-r3/` (stubs, extracted blocks, harness `run_stubs.sh`).

## `gh` 2.86.0 `--notes-from-tag` with `--repo` (empty directory, fake `GH_TOKEN`)

Command: `GH_TOKEN=fake gh release create v1.0 --notes-from-tag --repo a/b`

```
using `--notes-from-tag` with `--repo` is not supported

Usage:  gh release create [<tag>] [<filename>... | <pattern>...]
...
      --notes-from-tag               Fetch notes from the tag annotation or message of commit associated with tag
...
rc=1
```

Fails on the flag check; no network call. I found no way to keep `--notes-from-tag` without a checkout, so `publish` reads the tag via the API and passes `--notes-file` with `--verify-tag`.

## Changes
- `build.yml`: `inspect` job removed; firmware tag-check step (tag runs, before compile); `os` output; digest assigned, checked non-empty, then written; Job summary moved after the record step and uses the recorded digest; `publish` has no checkout, `needs: [host-tests, firmware]`, one step that holds `GH_TOKEN`.
- `bump_version.sh`: accepts `[v]<major>.<iteration>[-name]` and legacy `v<NN>[-name]`; hook removed; header/usage updated.
- `tests/bump_version_scheme_test.sh`: sources a temp truncated copy; new cases; no-argument run with `BUMP_VERSION_SOURCE_ONLY=1` exits 1.
- `release.sh` / `release`: tag step, `--tag`, prompt removed; prints the tag instruction after push.
- `AI_DEVELOPMENT_WORKFLOW.md` Stage 9 items 2, 3, 6, tag description, checklist line, summary field name; `PROJECT_STATUS.md` lines 234, 248.

## Verification
- YAML parses (ruby); jobs `host-tests`, `firmware`, `publish` (needs `[host-tests, firmware]`); firmware outputs `sha256, digest, os`. `bash -n` OK on all 7 extracted `run:` blocks, `bump_version.sh`, `release.sh`, `release`; `zsh -n` OK on the test.
- Host suite: **108/108 (sh via zsh, py via python3)**. `tests/bump_version_scheme_test.sh` PASS.
- Workflow size: **154 lines**; base (`origin/main`) 83; **net +71**, budget +85 (met).
- Stub run (`run_stubs.sh`; `gh`/`docker`/`stat` stubs; actual blocks extracted from the YAML):
  - Tag check: `v48.90`/4890 pass; `v48.91`/4890 fail ("not building", before any build step); overflow tag fail; `v48.100` fail; `v08.05`/805 pass.
  - Publish: pass → exactly one `gh release create … --verify-tag --title … --notes-file …`, no `--notes-from-tag`, summary lines written. Each of these failed with 0 create calls: lightweight tag (ref type `commit`), tag object.sha ≠ `GITHUB_SHA`, changed artifact hash, two `.bin` files, existing Release, overflow tag, `v48.100`.
  - Empty docker digest: record block exits 1, writes no outputs; non-empty digest writes `sha256` and `digest`.
- `release.sh --dry-run --yes 49.1-Name …` (git stub): prints the planned commands and `Tag the release with: git tag -a v49.1 -m "Some notes" && git push origin v49.1 (…)`; `--tag` now "Unknown option".
- Greps: 0 `actions/checkout` in `publish`; `GH_TOKEN` once (line 130, the Release step); 0 `npm`; 0 `notes-from-tag` in the workflow; `git tag` in `release.sh` only in the header comment and the printed instruction, none in `release`; `git diff --stat -- src/` empty.
- `BUMP_VERSION_SOURCE_ONLY` remains only in `tests/bump_version_scheme_test.sh` (the required no-effect check) and in historical `docs/work-orders/` files.

## Deviations
- The two-`.bin` and empty-`.bin` check uses `find` + `wc -l` instead of `mapfile` (bash 3.2 here lacks it; same behavior on the runner).
- The printed tag instruction strips any `v` prefix and `-name` suffix from the version, since the release workflow only accepts `v<major>.<iteration>` tags.
- Stage 9 success-report wording changed "embedded version" to "source version" to match the workflow summary.

## Open questions
- `bump_version.sh 49.1` now writes `FIRMWARE_VERSION = "49.1"` (no `v`), as the dispatch says to pass the version through unchanged; Stage 9 says the string reads `v49.<n>-<name>`. Should the script normalise to a leading `v`?
- The doc check that source and tag agree is done on the runner with `sed` on the `#define`; the `particle binary inspect` confirmation is manual (checklist).

## Controller edit after this report (Claude Code, 4 Nov; answers open question 1)
`bump_version.sh` now normalises a version without a leading `v` (`49.1`) to `v49.1` before writing `FIRMWARE_VERSION`, the Doxygen number and the README header (3 lines: one comment, one `case`), so the firmware string always reads `v<major>.<iteration>[-name]` as Stage 9 says. `bash -n` passes; the derivation function and its test are unchanged. The change is in the main body, which the temp-copy test truncates away, so Codex reads it directly.
