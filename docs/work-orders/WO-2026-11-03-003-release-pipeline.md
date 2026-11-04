# WO-2026-11-03-003 — Release pipeline: tagged builds, naming, version scheme, repo protections

**Status:** APPROVED by Chip and the architect (3 Nov). Renumbered from -002 (taken by the item-26 Step 0) to -003 on 3 Nov.
**Governing sections:** `AI_DEVELOPMENT_WORKFLOW.md` §2 (Chip: final authority, releases), §3 Stage 9 (release and feedback), §4, §5, §7 (access and security controls). Builds on WO-2026-11-02-003 (CI) and its Amendments 1–4.

## Goal (plain)
A release-candidate tag produces one named, traceable `.bin` that can go to the bench and, unchanged, to the fleet. Only Chip can put firmware into the product or onto devices. Nobody outside can trigger anything that matters.

## Decisions (Chip, 3 Nov)
1. **Triggers.** Push/PR: build + test + artifact (exists). A tag `v<major>.<iteration>` (e.g. `v49.2`): build, name, publish a GitHub Release with the `.bin` and `.sha256`. Promotion to the fleet stays a console action by Chip. **CI never flashes devices and never releases firmware.**
2. **Who.** Claude Code creates release-candidate tags when a WO set is ready. Uploads to product 42131 (part B) require Chip's approval via a GitHub Environment. Pinning and setting the default: Chip only.
3. **Version scheme.** `PRODUCT_VERSION` = major × 100 + iteration (v49.1 → 4901, v49.2 → 4902). **The iteration that passes its soak is the release; no rebuild** ("the binary that ships is the binary that was soaked"). The `FirmwareVersion.h` string reads `v49.<n>-<name>`.
4. **One build ships:** CI, once the Dev-09 24 h run on a CI-built binary is clean. The local build stays a mandatory Stage 7 check and is never shipped.
5. **Naming:** `gcc-v<major>.<iteration>-<platform>-dos<deviceOS>-<shortsha>.bin`, plus `.sha256`. Example: `gcc-v49.2-boron-dos6.6.0-1b3edd4.bin`.
6. **Bench testing on sleeping devices:** upload the test build to 42131 and pin only bench devices; the cloud delivers it at the next connect. Serial-Pi flashing: not now.

## Part A — implement now
- Release job on `push: tags: ['v*']`: build (existing job), rename per 5, `particle binary inspect` and fail if the embedded version ≠ major × 100 + iteration from the tag, write the success block to the job summary (run link, commit SHA, `.bin` name, size, SHA-256, embedded version, test count), and create the GitHub Release. Job permissions: `contents: write` for that job only; the workflow default stays `contents: read`.
- **Checks before the scheme is used (read-only, report back):** every use and type of `FIRMWARE_PRODUCT_VERSION` / `fw_version` (any field narrower than 16 bits, any version comparisons in code or ledgers); whether Ubidots dashboards key on `fw_version`; and whether a Development-marked device accepts a product pin, or only direct flashes (Particle docs).
- Update `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 9 with decisions 1–6 and the success-report block.

## Part B — after Chip creates the API user (WO -003 Amendment 2)
Upload job: needs the release job; GitHub Environment `particle-42131` with Chip as required reviewer and a deployment rule allowing only `v*` tags; environment secret only; `particle-iot/firmware-upload-action` (SHA-pinned), `firmware:create` scope. It uploads only.

## Repo protections (public repo)
Settings that Chip applies (admin), checked by Claude Code and recorded:
1. **Fork PR workflows:** Actions → "Require approval for all external contributors".
2. **Allowed actions:** allowlist only GitHub's own plus `particle-iot/compile-action` and `particle-iot/firmware-upload-action`.
3. **Default `GITHUB_TOKEN` permissions:** read-only.
4. **Rulesets:** `main`: PR required, the CI checks required, no force-push or deletion. Tags `v*`: creation restricted to Chip's account (which Claude Code's `gh` uses), no deletion or update.
5. **Environment `particle-42131`:** required reviewer Chip; deployment tags `v*` only.
6. **CODEOWNERS:** `.github/workflows/` → Chip (workflow changes need his review).
In the workflow: never `pull_request_target`; secrets only in the Environment-gated job.

## Budget
`src/`: **0 now.** The version constant changes at the v49 release itself. Workflow: **≤ +60 lines** for the release job (part A) and ≤ +25 for part B. Docs: Stage 9.

## Agents
Copilot, standard model, medium. Codex Stage 7: `gpt-5.6-sol`, **high** (security-relevant workflow: tokens, permissions), new SHA-named round files per §4, at most two rounds. Verification also needs two live dry runs on a throwaway branch: (a) tag `v48.90` with the constant set to 4890 → a correctly named Release is created; (b) tag `v48.91` with the constant left at 4890 → the version guard fails the job and no Release is created. Delete both tags, the Release and the branch afterwards.


## Amendment 1 (Chip, 3 Nov): release notes come from the annotated tag
- Release tags are **annotated** (`git tag -a v<major>.<iteration>`), and the message is written from the WO closing records (what shipped, WO ids, soak result, build record: CI run id, `.bin` SHA-256, buildpack image digest).
- **Part B:** the upload's title and description are that tag message (title = tag name plus the first line, description = the rest), so the Particle firmware entry matches the GitHub Release text.
- **Part A consequence:** the GitHub Release created by the release job uses the same annotated tag message as its notes (today's job writes only "Tag, commit"). A lightweight tag is rejected. This is a round-2 change to Part A (read the message with `git for-each-ref --format='%(contents)' refs/tags/$TAG` after checking `objecttype` is `tag`; the checkout must fetch tags). Its line cost counts against the Part A budget, which Stage 7 round 1 found already used up (+60 of +60): the architect's budget ruling is needed with the token-scoping decision.
- **Start condition for Part B:** Chip confirms the `firmware:create` API user's token is stored in the `particle-42131` Environment. Until then Part B does not start.

## Amendment 2 (Chip, 3 Nov): the Part B API user exists
- Product-42131 API user with **only `firmware:create`** (name relayed as `github-firmware-upload`), registered in `docs/notes/particle-api-users-register.md` (no token). Its token is the `PARTICLE_ACCESS_TOKEN` secret of the `particle-42131` Environment (required reviewer Chip; `v*` tags only).
- **Part B starts after the Stage 7 round-2 verdict on Part A** (Part A round 1 was NOT VERIFIED; round 2 waits for the token-scoping, line-budget and #177-merge rulings).
- **If an upload fails on scope, report the exact scope the error names. No broader scopes** are granted to make it work.

## Amendment 3 (architect/Chip rulings, 3 Nov, after Stage 7 r1 NOT VERIFIED on `78b4ba11`)
1. **Token scoping = option b with a hash link:** `firmware` records the `.bin` SHA-256 as a job output; a read-only `inspect` job (pinned `particle-cli`, `--ignore-scripts`, no token) outputs only the embedded version; a `publish` job (`contents: write`, token only on the `gh release create` step, preinstalled `gh` only) re-checks the SHA-256 against the recorded hash, compares the version with major × 100 + iteration, and runs `gh release create --verify-tag --notes-from-tag`.
2. **Overflow bypass:** tag validated with `^v[0-9]{1,2}\.[0-9]{1,2}$` before any arithmetic.
3. **Budget:** Part A ≤ **+85** workflow lines (raised for the split); `src/` 0 except the `FirmwareVersion.h` comment; Part B ≤ +25 separately.
4. **Merge order:** PR #177 first, then rebase, then Stage 7 round 2 (final) under the §4 SHA rule.
5. **No real `v*` tag** (including the dry-run tags `v48.90` / `v48.91`) until Chip has applied the six repo settings and Claude Code has verified them via the API.
6. `bump_version.sh` (it derived 49 from `v49.1-name`) and the `FirmwareVersion.h` comment: Copilot, in this WO. Ubidots `fw_version` check: the architect, at v49.
7. **Development-device pin question (docs check, 3 Nov, https://docs.particle.io/getting-started/console/development-devices/):** a device marked as a development device "will ignore both released product firmware as well as any firmware version it has been locked to run". The page does not say whether marking or un-marking changes a stored lock, and warns that un-marking "may trigger an immediate over-the-air update" to the targeted product firmware. **Consequence for decision 6:** pinning a Development-marked bench device does nothing. For bench testing of a CI-built `.bin` use one of: (i) flash it directly (USB or Console flash) while the device stays marked Development; (ii) un-mark it and pin the test version first, accepting that un-marking may OTA it immediately to its currently targeted version, so pin before un-marking. Stage 9 is edited to say so.

## Amendment 4 (architect ruling after the final Stage 7 round, 4 Nov 2026): simplify; round 3 authorized
Basis: `WO-2026-11-03-003-stage7-r2-verdict.md` (NOT VERIFIED on `a9fe68e7`, six blockers). A third Stage 6/7 round is authorized (`gpt-5.6-sol`, high; §4 SHA rule); then both dry-run tags (`v48.90` expected to publish, `v48.91` with a mismatched embedded version expected to fail with no Release). Expected size: under +85 workflow lines.
- **(a)** Delete the `inspect` job (no npm). Verify, **before building**, that the source `FIRMWARE_PRODUCT_VERSION` equals tag major × 100 + iteration (the embedded value equals the source value: yesterday's parity result). `particle binary inspect` stays as a **manual release-checklist step** (Stage 9).
- **(b)** The `publish` job has **no checkout**: `gh release create --repo … --verify-tag --notes-from-tag` plus `download-artifact`. GitHub-owned actions seeing the token is an accepted risk.
- **(c)** Docker digest: assign to a variable and fail if empty.
- **(d)** `bump_version.sh` accepts `49.1` and `v49.1` → 4901; remove the `BUMP_VERSION_SOURCE_ONLY` hook (the test sources a temp copy of the function); retire `release.sh`'s tag step (tag with `git tag -a`).
- **(e)** Fix the Stage 9 Development-device text.
- **Controller finding on (b), before Stage 6 (4 Nov):** `gh` 2.86.0 refuses `--notes-from-tag` together with `--repo` ("using `--notes-from-tag` with `--repo` is not supported"; run in an empty directory), and the flag reads the tag from a local git repository, which a no-checkout job does not have. Working default pending the architect's confirmation, same intent (no checkout, notes from the annotated tag): read the tag through the API in the Release step (`gh api repos/$GITHUB_REPOSITORY/git/ref/tags/$TAG`: object type must be `tag`; `git/tags/<sha>`: its `object.sha` must equal `$GITHUB_SHA`; its `message` is written to a file) and pass `--notes-file` with `--verify-tag`. The guards that need the token (these API reads) move into the one Release step, so `GH_TOKEN` is still on that step only. Alternative if the architect prefers: keep a checkout in `publish` with `token: ''` (anonymous clone of the public repo) and `--notes-from-tag` without `--repo`.

## Amendment 5 (architect ruling after Stage 7 round 3, 4 Nov 2026): preprocessor guard, no round 4
Stage 7 round 3 (`WO-2026-11-03-003-stage7-r3-verdict.md`, NOT VERIFIED on `971f91d9`) had one blocker: the firmware job's `sed` read of `FIRMWARE_PRODUCT_VERSION` could be fooled by a commented decoy plus an indented active definition. **Ruling:** replace the `sed` line with the compiler's own view, `echo FIRMWARE_PRODUCT_VERSION | cc -E -P -x c++ -include src/FirmwareVersion.h - | tail -n 1` (`build.yml:76`). **No Codex round 4** (a one-line, tested change). Acceptance: Codex's decoy as a fixture test (`tests/version_guard_preprocessor_test.sh`, which runs the workflow's own extracted line against the decoy, a line-commented decoy, an indented define, a trailing comment and the repository file; restoring the `sed` form fails it) plus the two live dry runs (`v48.90` publishes, `v48.91` fails with no Release); both tags and any Release are deleted afterwards. Host suite 109/109 (sh via zsh, py via python3); YAML parses; `bash -n` passes on all 7 `run:` blocks; workflow 154 lines (+71 over 83).
**Accepted residual (architect):** a moved annotated tag that still points at `GITHUB_SHA` passes the API check (the event's original tag-object SHA is not bound), and a tag moved after the checks is a race. The repository's tag ruleset (immutable tags) covers it; the dry runs confirm the ruleset's behavior.

## Record: live dry runs (4 Nov 2026), acceptance per Amendment 5
Run on a throwaway branch (`dryrun/release-pipeline-v48.90`, from `dfc4142` with `FIRMWARE_PRODUCT_VERSION` set to 4890 in `src/FirmwareVersion.h`; branch deleted afterwards); the tag ruleset allowed the creation, and the deletion, only through the owner's bypass.
- **`v48.90` (annotated tag at `f4e79cb`), run 41752937348: success** (host-tests, firmware, publish). A GitHub Release `v48.90` was created with `gcc-v48.90-boron-dos6.6.0-f4e79cb.bin` (152,898 bytes) and its `.sha256`; `shasum -c` OK; `particle binary inspect`: "firmware for product id 13 at version 4890"; the notes were the tag annotation (via the API read and `--notes-file`). The write token was used in the one "Guards and Release" step; `publish` ran no checkout.
- **`v48.91` (annotated, same commit), run 41753377855: failed before building**, as required: `source FIRMWARE_PRODUCT_VERSION '4890' != 4891 from tag v48.91; not building` (firmware job, step "Tag matches source FIRMWARE_PRODUCT_VERSION"); `publish` skipped; no Release `v48.91`.
- **Cleanup:** Release `v48.90`, tags `v48.90` and `v48.91`, and the dry-run branch deleted; `gh release list` is empty and `git ls-remote` shows no `v48.9*` or `dryrun/*` refs.
- Not exercised: the moved-annotated-tag case (accepted residual, covered by the tag ruleset).

## Amendment 6 (architect, 4 Nov 2026): Part B dispatched
Part A is approved (PR #180, `--notes-file` deviation accepted; Chip merges when CI is green). **Part B now:** workflow **≤ +25 lines** over Part A (`build.yml` at `f86f679` is 154 lines); Copilot standard/medium; Codex `gpt-5.6-sol` **high**, §4 SHA rule, at most two rounds. Start condition met: Environment `particle-42131` exists (required reviewer chipmc, deployment branches/tags custom policy) and holds the secret `PARTICLE_ACCESS_TOKEN` (the `github-firmware-upload` API user, `firmware:create` only; register: `docs/notes/particle-api-users-register.md`).
- **No dry-run upload:** a test version would stay in 42131's firmware list and the token cannot remove it. Verification is local (YAML, `bash -n`, stub runs of the shell steps, a structural test of the job). **Acceptance is the first real tag `v49.1`, behind Chip's Environment approval, followed by a Console check** that the product firmware list shows version 4901 with the right title and notes.
- **Job:** `upload`, `if: github.ref_type == 'tag'`, `needs: [firmware, publish]`, `environment: particle-42131`, `permissions: contents: read`. Steps: `download-artifact` (pinned); one shell step (read-only `GH_TOKEN`) that checks the `.bin` SHA-256 against the firmware job's, derives the version from the tag (`major*100+iteration`), reads the annotated tag message through the API and writes outputs `version`, `title` (tag name, then the first line of the message) and `description` (the rest) with a random heredoc delimiter; then `particle-iot/firmware-upload-action` (SHA-pinned) with `product-id: 42131`. `PARTICLE_ACCESS_TOKEN` appears in the `with:` of that action step only.
- **Pin:** the action's tags are `v1.0.5` (`9b0cda4c…`, committed 3 Nov 2026 17:39Z, one day old), `v1.0.4` (`9e393d6b362208488db91339ab72ee1d7e2e6ada`, 14 May 2026) and older; both run on `node24`. **The dispatch pins `v1.0.4`** (six months in the field, same inputs), not the one-day-old release; the architect may re-pin after reading v1.0.5's change.
- The action uploads only; it triggers no OTA. Inputs: `particle-access-token`, `firmware-path`, `firmware-version`, `product-id`, `title` (required), `description`.
- Residual risk accepted by design: the third-party action receives the Particle token (scope `firmware:create`, product 42131, Environment-gated and approved by Chip per run).

## Amendment 7 (Chip, 4 Nov 2026): Particle title and description format
Replaces Amendment 1's "title = tag name plus the first line, description = the rest" for the Particle upload (the GitHub Release keeps the whole annotation as its notes).
- **Title:** always `Development Release - v<major>.<iteration>`, generated from the tag (`Development Release - v49.1`), never from the annotation.
- **Description (Markdown):** the "What changed" bullets, "Not in this release", the rollback line, and a link to the GitHub Release (`$GITHUB_SERVER_URL/$GITHUB_REPOSITORY/releases/tag/$TAG`, appended by the workflow). **The part of the tag message above the first line that is exactly `---` goes to Particle**; everything below it (verification, build record, rollout) stays on the GitHub Release only. A tag message with no `---` line fails the upload job before the action runs (fail closed: nothing may leak into Particle by omission).
- Budget: the `upload` job keeps its +25 (179 lines); the change is in place (no net lines). Stage 6 Copilot standard/medium; Stage 7 Codex `gpt-5.6-sol` high, §4 SHA rule. Docs: Stage 9 item 7 and the release checklist; `docs/releases/v49.1-notes.md` is drafted to this format (the first-line title is dropped; the draft marks the Particle part).
