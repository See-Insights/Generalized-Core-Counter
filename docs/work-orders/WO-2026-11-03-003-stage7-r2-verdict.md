UNDER REVIEW: wo/2026-11-03-003-release-pipeline @ a9fe68e78e9d99acb70e6ac6426664c499a86399

# Verdict: NOT VERIFIED

Final round. The overflow and ordinary artifact/tag guards are fixed, and the host suite passes 108/108, but several blocking defects remain. They require an architect ruling under the two-round rule.

## Blocking findings

1. **[BLOCKING — B2 not resolved] The publish job exposes its write-scoped token before the Release step.**

   `.github/workflows/build.yml:130-145` grants `contents: write`, then runs:

   - `actions/checkout` at line 139, whose default `token` input is `github.token`.
   - `actions/download-artifact` at line 142, whose token input defaults to `github.token`.

   `persist-credentials: false` at line 141 prevents checkout from storing credentials in Git configuration; it does not prevent the action itself receiving the write token.

   `GH_TOKEN` is explicitly placed only on the Release step at lines 160-165, and only preinstalled `gh` is used, but Amendment 3’s requirement that the token be available only to that step is still unmet.

   The inspect job’s download action similarly receives a token at lines 102-114, but that job explicitly has only `contents: read`. Nothing in inspect can write repository contents or create a Release.

2. **[BLOCKING — B3 not reliably resolved] Docker digest lookup failure is masked.**

   Both digest commands are embedded in `echo "$(docker image inspect …)"`:

   - PR #177 summary line: `.github/workflows/build.yml:84`
   - `rec` output: `.github/workflows/build.yml:100`

   Under `bash -euo pipefail`, failure inside this substitution does not fail the outer `echo`. Reproduction:

   ```text
   digest=
   MASKED_DOCKER_FAILURE_SURVIVED
   ```

   Therefore, if the image is absent or inspection fails, the firmware job can succeed with an empty digest, and `publish` can report an empty value.

   The inspect command itself is read-only and safely quoted in `rec`. No intervening committed step removes the image after compilation, so the expected happy path is plausible, but image presence cannot be established offline. Failure must be explicit rather than silently degrading because the digest is a required release record.

   PR #177’s summary line and `rec` inspect the same image tag. The summary does not feed the job output; `publish` relies only on `rec`.

3. **[BLOCKING] `release.sh` is broken by the new version syntax.**

   `bump_version.sh:25-34` now correctly requires either `v<major>.<iteration>` or a legacy dotless `vNN` form. But:

   - `release.sh:161` passes its `VERSION` directly to `bump_version.sh`.
   - `release.sh:137` constructs the tag as `v${VERSION}`.
   - The interactive wrapper still prompts for an unprefixed version at `release:51` and describes tag `v${VERSION}` at `release:65`.

   Consequently:

   - `release.sh … 49.1 …` → bump rejects `49.1`.
   - `release.sh … v49.1 …` → bump derives `4901`, but the tag becomes `vv49.1`, which the publish guard rejects.

   There is no invocation that both updates the new version correctly and creates a valid `v49.1` tag.

4. **[BLOCKING] The test-only environment hook changes normal script behavior.**

   `bump_version.sh:37-40` silently returns whenever `BUMP_VERSION_SOURCE_ONLY=1`, even when the script is executed normally rather than sourced.

   Exact check:

   ```text
   bump_version.sh with no arguments: exit 1
   BUMP_VERSION_SOURCE_ONLY=1 bump_version.sh with no arguments: exit 0
   ```

   Thus an inherited environment variable can turn a normal release operation into a successful no-op. The hook does change production behavior.

   The new test does exercise the real `derive_product_version()` function by sourcing the actual script at `tests/bump_version_scheme_test.sh:12-15`; it is not a copied formula.

5. **[BLOCKING — Amendment 3 item 7 missing] Stage 9 still gives the invalid Development-device procedure.**

   `AI_DEVELOPMENT_WORKFLOW.md:520` still says to upload and pin the bench devices. It does not say that a Development-marked device ignores its product pin, nor give the required direct-flash/unmark-and-pin alternatives.

   This directly contradicts Amendment 3 item 7 at `docs/work-orders/WO-2026-11-03-003-release-pipeline.md:60`.

   Two additional documentation inaccuracies remain:

   - `AI_DEVELOPMENT_WORKFLOW.md:517` calls inspect “token-less”, although its download action receives a read-only token.
   - `PROJECT_STATUS.md:234,248` says `FIRMWARE_VERSION` and `FIRMWARE_RELEASE_NOTES` live in `src/Version.cpp`; that file does not exist. Both version values now live in `src/FirmwareVersion.h`, and release notes live in `CHANGELOG.md`.

6. **[BLOCKING / architect ruling — inspector trust] A false inspect line is accepted as authoritative.**

   The parser at `.github/workflows/build.yml:123-124` accepts any single line matching the numeric product/version pattern. A stubbed result:

   ```text
   firmware for product id 999 at version 4890
   ```

   produced `version=4890`, allowing the subsequent version comparison to pass. The product ID is not checked.

   Normal malformed, empty, or multiple matching outputs fail closed, and `GOT_VER` is passed through a quoted environment variable rather than reparsed as shell source. However, a compromised `particle-cli` runtime or unlocked transitive dependency can simply report the tag-derived value. `--ignore-scripts` suppresses lifecycle scripts but does not stop installed dependency code from executing when the CLI runs.

   This is not an artifact-name or shell-injection bypass; it is a remaining trust dependency on the unlocked npm runtime.

## B1–B5 disposition

| Prior blocker | Result |
|---|---|
| B1 overflow | **Resolved.** Regex at `build.yml:149` runs before arithmetic at line 150. Overflow and `v48.100` failed before any `git` call. |
| B2 token exposure | **Not resolved.** npm no longer has a write token, but publish checkout/download receive the job’s write token. Inspector output also remains a trusted supply-chain input. |
| B3 digest absent | **Not resolved reliably.** Digest wiring exists, but Docker failure is masked and an empty digest is accepted. |
| B4 tag authorization | **Partially resolved in code; external gate not verified.** Annotated-tag and tag-at-`GITHUB_SHA` checks exist at lines 151-152. They do not establish tag creator or repository ruleset state. |
| B5 pre-use checks | **Partially resolved.** Derivation is fixed, but `release.sh`, the test hook, and Development-device documentation are not. Ubidots dependence and authoritative 16-bit confirmation remain pre-scheme-use items. |

## Guard, artifact, and privilege results

The exact committed guard block was exercised with local `git`/`gh` stubs:

- Overflow tag `v4611686018427387904.48`: rejected before arithmetic or Git access.
- `v48.100`: rejected before arithmetic.
- `v48.91` with embedded `48`: rejected; no Release step.
- `v48.90` with embedded `4890`: passed.
- `v08.05` with embedded `805`: passed.
- Lightweight tag: rejected.
- Tag not pointing to `GITHUB_SHA`: rejected.
- Changed artifact hash: rejected.
- Second `.bin`: failed closed.
- Attacker-selected input filename: accepted only when content/hash matched, then canonicalized to `gcc-v48.90-boron-dos6.6.0-aaaaaaa.bin`.
- Stubbed Release command used `--verify-tag --title v48.90 --notes-from-tag`.

The cross-job SHA-256 link prevents an artifact-service substitution unless SHA-256 itself is defeated or the firmware job/output producer is compromised. Inspect cannot modify the artifact later consumed by publish because publish downloads a fresh copy and rechecks the firmware job’s hash.

The publish checkout is functionally needed for `project.properties`, annotated-tag checks, and local tag material. `persist-credentials: false` is correct, but it does not satisfy the write-token isolation ruling.

## `--notes-from-tag`

Offline evidence establishes:

- On a tag event, checkout is asked to check out that tag ref.
- Before `gh` runs, `git cat-file -t refs/tags/$TAG` must return `tag`, and `git rev-parse "$TAG^{commit}"` must equal `GITHUB_SHA`.
- Therefore, any run reaching `gh` has a local annotated-tag object associated with the reviewed commit.
- Local `gh` 2.86.0 help documents `--notes-from-tag` as taking notes from the annotated tag, with the commit message as fallback for lightweight tags. The workflow rejects the fallback case first.

What remains live-only:

- The runner’s actual preinstalled `gh` behavior with the pinned checkout action.
- Successful use of the annotation on `v48.90`.
- Failure of `v48.91` without creating a Release.
- Repository tag ruleset enforcement and remote tag immutability.

Those two required live dry runs were not performed because tags, Releases, network access, and writes were outside this review’s authorization.

## Trigger behavior

- A tag push does not also match the `branches` filter, so one tag ref does not create both a branch and tag run.
- A push to a branch with an open same-repository PR can still produce both a `push` run and a `pull_request` run. Thus “no duplicate runs” is not universally true; this behavior already existed on `origin/main`.
- Tag runs start `host-tests` and `firmware` in parallel.
- `inspect` waits for firmware but may run before host tests finish.
- `publish` waits for `host-tests`, `firmware`, and `inspect`.
- A nonconforming tag such as `v48.100` still matches `tags: ['v*']`, runs the prerequisite work, then fails the publish regex. A tag not beginning with `v` does not trigger this workflow.

## `bump_version.sh`

Verified derivations:

```text
v49.1-Foo            -> 4901
v49.2                -> 4902
v48.90               -> 4890
v08.05               -> 805
v28-CloseBeforeSleep -> 28
v49                   -> 49
```

Verified rejections include `v100.1`, `v49.100`, `49.1`, `v49.x`, the overflow tag, `v49.`, and the empty string.

## Scope and budget

- `src/` changed only in `src/FirmwareVersion.h:6-11`, all comment text.
- `FIRMWARE_PRODUCT_VERSION` remains `48` at `src/FirmwareVersion.h:20`.
- CODEOWNERS syntax at `.github/CODEOWNERS:1` is valid; enforcement remains dependent on live ruleset settings.
- Workflow base: 83 lines.
- Reviewed workflow: 168 lines.
- Net: **+85**, exactly meeting the amended limit.
- The Stage 6 report’s 166/167 and +83 figures are inaccurate for the final merged commit.

## Local checks

- YAML parse: **PASS**, Ruby 2.6.10/Psych.
- `bash -n`: **8/8 workflow `run:` blocks PASS** using GNU Bash 3.2.57.
- Script syntax: `bump_version.sh` and `release.sh` via Bash; new test via zsh: **PASS**.
- Host suite at the exact reviewed SHA: **108/108 (sh via zsh, py via python3)**.
- Exact-block guard and publish dry run: completed with local stubs; no GitHub write or network access.
- Scratch: `build-tmp/wo-2026-11-03-003-stage7-r2/`.
- Source worktree remained clean.

**Model / reasoning:** `gpt-5.6-sol`, high.  
**Interpreter line:** YAML via Ruby 2.6.10/Psych; shell syntax and logic via GNU Bash 3.2.57; host tests via zsh 5.9 and Python 3.14.6; local `gh` documentation from gh 2.86.0.