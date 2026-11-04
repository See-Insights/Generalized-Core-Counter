UNDER REVIEW: wo/2026-11-03-003-release-pipeline @ 78b4ba11d9eb57bd0ca2201c6c356c2339264e33

# Verdict: NOT VERIFIED

Round 1 of at most 2.

## Blocking findings

1. **[BLOCKING] The version guard is bypassable through Bash integer overflow.**

   `.github/workflows/build.yml:114-119` accepts unbounded decimal digits and multiplies the major using signed shell arithmetic. Using the exact committed run block under Bash 3.2:

   - Tag `v4611686018427387904.48`
   - Stubbed inspect result: product 13, version `48`
   - `major * 100` wrapped to zero
   - The guard accepted `48` and invoked the stubbed `gh release create`

   Validate a supported major range before conversion/multiplication and ensure the final value is within the supported product-version range. If the intended scheme ends at 9999, restricting major to `0..99` is sufficient.

2. **[BLOCKING] The write token is exposed to the npm installation and Particle CLI supply chain.**

   `contents: write` is correctly limited to the release job at `.github/workflows/build.yml:93-94`, but `GH_TOKEN` is job-wide at lines 95-98. It is therefore available to:

   - The unlocked npm dependency installation at line 110, including lifecycle scripts.
   - `particle binary inspect` and all its dependencies at line 124.
   - Every other step in the job.

   `particle-cli@3.52.4` pins the top-level package, but npm does not use a dependency package’s lockfile when installing it globally. Transitive ranges can therefore resolve differently on later runs.

   A lockfile plus `npm ci` can be workflow-line-neutral, but the stronger design is a read-only preparation/inspection job followed by a fresh, minimal publishing job holding `contents: write`. At minimum, the npm tree should be integrity-locked and the token removed from all CLI installation/inspection steps.

3. **[BLOCKING DEPENDENCY] The required buildpack digest is absent at this SHA.**

   The firmware summary at `.github/workflows/build.yml:75-80` has no digest, while line 140 directs readers to that nonexistent field. PR #177’s local branch adds the required `buildpack-image:` line. A local three-way merge simulation found that change applies cleanly.

   PR #177 must be merged into this branch, and the combined workflow rechecked, before the success report is accurate.

4. **[BLOCKING RELEASE CONTROL] `github.ref_type == 'tag'` is gating, not authorization.**

   `.github/workflows/build.yml:90` correctly prevents the release job on PRs and branch pushes, including a branch named `v48.90`. A fork PR cannot obtain the upstream write token; a tag in a fork can only release in that fork.

   However, anyone allowed to create an upstream `v*` tag can select the commit being tagged and cause its tag workflow to run with `contents: write`. They can publish any matching-version binary, and potentially tag a commit containing a modified workflow. `gh release create --verify-tag` at lines 127-131 verifies that the tag exists; it does not verify its creator, signature, ancestry, or continued association with `GITHUB_SHA`.

   The `v*` tag ruleset required by `WO-2026-11-03-003-release-pipeline.md:27-32`—creation restricted to Chip and no update/deletion—is therefore essential. Its live repository configuration was not independently verifiable offline.

5. **[BLOCKING BEFORE SCHEME USE] Required pre-use checks remain unresolved or outside the reviewed SHA.**

   A user-owned untracked `WO-2026-11-03-003-checks-report.md` appeared during review and was read without modification. It is not part of the reviewed commit. It reports:

   - `bump_version.sh:29` would derive `49`, not `4901`, from `v49.1-name`.
   - Development-marked devices ignore product firmware locks, conflicting with Stage 9’s pin-based bench procedure unless those devices are first removed from development mode.
   - Whether live Ubidots dashboards depend on `fw_version` remains unconfirmed.
   - The 16-bit product-version representation is only community-sourced, not confirmed by the cited official documentation.

   These must be recorded and resolved before activating the new scheme. The two WO-required live tag dry runs also remain NOT VERIFIED because this review was expressly offline and prohibited tag/release writes.

## Verified behavior and notes

- **[PASS] Privileges and triggers:** Workflow default is `contents: read` at lines 9-10; only the release job overrides it. No `pull_request_target`, `workflow_dispatch`, `secrets.*`, Particle token, or `curl | sh` exists.
- **[PASS] Normal guard cases:** Exact committed shell logic produced:
  - `v48.91` with embedded `48`: exit 1, no release invocation.
  - `v48.90` with embedded `4890`: exit 0 and release invocation.
  - `v08.05` with embedded `805`: exit 0, proving `10#` handles leading zeros.
  - `v48.100`: rejected before artifact processing.
- **[PASS] Inspect parsing:** Unrelated output before and after `firmware for product id N at version V` did not affect parsing. The parser requires a numeric product ID but does not confirm that it equals Boron product ID 13.
- **[PASS] Injection handling in the release job:** Tag, token, test count, SHA, repository, and server values enter through environment variables; release arguments and filenames are quoted. The glob in `find fw -name '*.bin'` is quoted, and exactly one lowercase `.bin` is required.
- **[NOTE] Existing direct expression interpolation:** `.github/workflows/build.yml:78-79` inserts action outputs directly into shell source. Those are in the read-only firmware job, not the writer job, but using `env` would be safer because shell syntax in an output would be reparsed.
- **[PASS] Failure ordering:** The version comparison precedes both `gh release view` and `gh release create`. A failed guard leaves a renamed binary and checksum only in the ephemeral workspace; it creates no Release or published release asset. The previously uploaded `firmware-boron` artifact still exists.
- **[NOTE] `set -euo pipefail`:** A failed Particle command exits through the assignment. A `sed` no-match returns success but produces an empty value, which the explicit comparison rejects. `gh release view` is an `if` condition and is intentionally exempt from `errexit`; any failure is treated as “not found,” after which `gh release create` is attempted.
- **[PASS] Artifact selection:** The firmware job uploads the exact `firmware-path` plus only `target-path/*.elf` at lines 81-87. Thus normal output contains one `.bin`; the recursive `find` also fails closed if another appears.
- **[PASS] Job wiring:** `release` needs both host tests and firmware. The test step exports `N/N` through lines 15-16 and 39-54. Tag runs execute both prerequisite jobs; branch pushes and PRs retain their runs without a second tag-triggered run.
- **[PASS except digest dependency] Success fields:** Lines 133-140 contain run link, full commit, binary name, byte size, SHA-256, embedded version, and host-test result. The actual buildpack digest depends on PR #177.
- **[PASS] Supply-chain action syntax:** Every `uses:` reference is a full 40-hex SHA. The download-artifact SHA is structurally pinned, and the Stage 6 report says it was checked as v8.0.2. I could not independently verify that tag mapping offline, so that mapping is **[NOTE: NOT VERIFIED]**.
- **[PASS] Stage 9 and CODEOWNERS:** `AI_DEVELOPMENT_WORKFLOW.md:513-523` adds decisions 1–6 and the success-report block only. `.github/CODEOWNERS:1` uses valid root-anchored directory syntax: `/.github/workflows/ @chipmc`. Its enforcement still depends on the live code-owner-review ruleset.
- **[NOTE] Line-count report:** The actual commit changes `build.yml` from 81 to 141 lines: `+60`, not `+58`. After merging PR #177, the expected combined file is 143 lines over its 83-line amended base, still `+60`. The budget is met exactly, but `WO-2026-11-03-003-stage6-copilot-report.md:6` mixes the future 83-line baseline with the current 141-line file.
- **[PASS] Scope:** Zero changes under `src/` and `tests/`.

## Local checks

- YAML parse: **PASS** using Ruby 2.6.10/Psych.
- `bash -n`: **7/7 run blocks PASS** using GNU Bash 3.2.57.
- Guard logic: normal fail/pass, leading-zero, iteration-limit, extra-output, and overflow cases executed from the exact committed run block with local stubs.
- Scratch: `build-tmp/wo-2026-11-03-003-stage7-r1/`.
- Host test suite: **not run; explicitly out of scope because no tests changed.**

**Model / reasoning:** `gpt-5.6-sol`, high.  
**Interpreter line:** YAML via Ruby 2.6.10/Psych; shell syntax and guard checks via GNU Bash 3.2.57; workflow host tests not run.