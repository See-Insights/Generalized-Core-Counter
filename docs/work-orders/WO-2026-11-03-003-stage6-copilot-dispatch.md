AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `.github/workflows/build.yml`, add `.github/CODEOWNERS`, edit `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 9 only, write the report; scratch `build-tmp/wo-2026-11-03-003-stage6/` / NOT authorized: any change under `src/` or `tests/`, committing, pushing, creating tags or releases, any `gh` write operation, adding secrets or tokens, deleting anything you didn't create (including `build-tmp/`).

# Stage 6 dispatch — WO-2026-11-03-003 Part A (release pipeline)
Binding spec: `docs/work-orders/WO-2026-11-03-003-release-pipeline.md` (decisions 1-6, Part A, Repo protections, Budget) and WO-2026-11-02-003 (the CI workflow it extends, Amendments 1-4). Branch `wo/2026-11-03-003-release-pipeline`, base = origin/main `0c8b5a2`+ (v48). Leave changes uncommitted. **Budget: 0 `src/`; workflow ≤ +60 lines over the current 83 (so ≤ 143); no `tests/` changes.**

## Do
1. `build.yml`: keep push/PR behavior (build + test + artifact) exactly. Add a tag trigger `v*` without duplicating runs for branch pushes (`push: branches: ['**']` plus `tags: ['v*']`). Workflow default stays `permissions: contents: read`.
2. New `release` job, runs only for tag refs matching `^v[0-9]+\.[0-9]+$` (reject other `v*` tags with a clear failure), `needs: [host-tests, firmware]`, `permissions: contents: write` **for this job only**, no Particle token, no secrets other than the default `GITHUB_TOKEN`:
   - download the `firmware-boron` artifact (`actions/download-artifact`, pinned by full SHA: v8.0.2 = `9000827ccba6bdab643e8b6fd33ac0654aef8333`; re-verify with `gh api repos/actions/download-artifact/git/ref/tags/v8.0.2`);
   - parse major and iteration from the tag; rename the `.bin` to `gcc-v<major>.<iteration>-boron-dos<deviceOS>-<shortsha>.bin` (deviceOS from `project.properties`, shortsha 7 chars of the tagged commit); write `<name>.sha256` (`sha256sum`, file name only);
   - **version guard:** `particle binary inspect` on the renamed `.bin` (install `particle-cli` pinned to an exact version: the controller's is 3.52.4; `npm install -g particle-cli@3.52.4`, no login, no token); fail the job unless the embedded firmware version equals major × 100 + iteration (v48.90 → 4890); no Release is created on failure;
   - create the GitHub Release for the tag with the `.bin` and `.sha256` (`gh release create` with `GITHUB_TOKEN`; title and notes minimal: tag, short SHA); do not create a Release if one exists;
   - **success block** in the job summary: run link, commit SHA, `.bin` name, size in bytes, SHA-256, embedded version (from the inspect output), test count (pass the host-tests `N/N` through a job output; add the minimal lines to the existing host-tests step/job outputs), and the buildpack-image digest line already printed by the firmware job (pass it through too if it fits, else say it is in the firmware job summary).
3. `.github/CODEOWNERS`: `/.github/workflows/ @chipmc` (the account is `chipmc`; verified with `gh api user`).
4. `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 9: add decisions 1-6 of the WO (triggers, who, version scheme with "the iteration that passes its soak is the release; no rebuild", one build ships and the bench soaks the CI-built `.bin`, naming, bench testing on sleeping devices) and the success-report block (fields above). Keep wording tight; do not edit other sections.
5. Never use `pull_request_target`. No `workflow_dispatch` that can publish. Every third-party action pinned by full SHA with the tag in a comment (the existing pins stay).

## Verify
YAML parse; `wc -l` ≤ 143; `bash -n` on every `run:` block; if `actionlint` is installed use it; a local dry run of the shell logic: given a tag name, a sample `.bin` (`~/Downloads/dev09-ci-soak-2026-11-03/dev09-ci-built-v48-0c8b5a2.bin`, embedded version 48) and the pinned `particle binary inspect`, show the name, the sha256 line, and that the guard FAILS for tag `v48.91` against embedded 48 and PASSES for a simulated 4890 (a plain shell function test, no GitHub). `git diff --stat` shows no `src/` or `tests/`. You cannot run the workflow; say so (the controller runs two live dry-run tags).

## Report
Write `docs/work-orders/WO-2026-11-03-003-stage6-copilot-report.md`: model and reasoning actually used, the interpreter line, the line budget, the pins as used, and anything unverified.
