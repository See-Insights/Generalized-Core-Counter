UNDER REVIEW: wo/2026-11-05-001-upload-file-name @ 8f3b63f382d55b54ce5f2e0710f1ec216cefd8d8

# Verdict: PASS

No blocking findings.

**Reviewer:** Codex · **Model:** `gpt-5.6-sol` · **Reasoning:** high  
**Interpreter line:** **115/115 (sh via zsh, py via python3)** — Python 3.14.6; zsh 5.9 arm64-apple-darwin26.0.

## Findings

1. **[PASS] Rename logic and byte preservation**

   At `.github/workflows/build.yml:169-172`, the tag regex, exactly-one-`.bin` check, and SHA-256 comparison all precede the rename. A failed hash or second `.bin` exits before `mv`. The destination is:

   `$(dirname "$bin")/gcc-$TAG-boron-dos$OS-${GITHUB_SHA::7}.bin`

   This matches `publish`’s asset format at `.github/workflows/build.yml:138-145`. Because source and destination remain in the same directory, `mv` renames the file without rewriting its contents; the test also confirms byte-for-byte content preservation.

   Edge cases:

   - `dirname` equal to `.` works, producing `./gcc-….bin`.
   - Nested artifacts remain in their original directory.
   - A separate existing canonical `.bin` makes the single-`.bin` guard fail before rename.
   - If the source is already canonically named, source and destination are identical. BSD `mv` treats that as a no-op; Ubuntu GNU `mv` normally rejects it. Neither behavior changes bytes. The current compile artifact is not canonically named, so this does not block.

2. **[PASS] Injection and output safety; validation recommended as a note**

   `TAG` is validated before use at line 169. The new `${{ needs.firmware.outputs.os }}` expression appears only in `env:` at line 166; the `run:` block contains no `${{ }}` expressions. `OS`, `bin`, and `new` are quoted wherever used.

   `OS` can syntactically contain spaces or shell metacharacters, but quoting keeps them literal; a direct hostile-value test caused no command execution. `/` is path syntax and makes `mv` fail closed when the implied directory does not exist.

   A newline cannot propagate as part of `OS` through the single-line `GITHUB_OUTPUT` form at lines 78-80: multiple `deviceOS=` matches would create separate/invalid output records rather than a multiline `os` value. The single-`.bin` check likewise rejects artifact paths containing embedded newlines before `echo "bin=$bin"`.

   **[NOTE]** Validate `deviceOS` once, preferably where `project.properties` is read, with the intended version regex. This would reject malformed spaces, slashes, or punctuation earlier and guarantee canonical naming. It is not blocking: the reviewed tree has exactly `deviceOS=6.6.0`, inputs are repository-controlled, unsafe shell evaluation was not found, and the same unvalidated value already feeds `publish` and the compile/buildpack paths.

3. **[PASS] Existing security guarantees are unchanged**

   `.github/workflows/build.yml:156-179` retains:

   - Tag-only execution and `needs: [firmware, publish]`.
   - Environment `particle-42131`.
   - Exactly `permissions: {contents: read}`.
   - No checkout in `upload`.
   - Annotated-tag, tag-regex, single-binary, and SHA-256 checks.
   - Amendment 7’s generated title, description-above-first-`---`, Release link, and fail-closed missing-separator behavior.
   - `PARTICLE_ACCESS_TOKEN` exactly once, in the action’s `with:` only.
   - Pin `9e393d6b362208488db91339ab72ee1d7e2e6ada # v1.0.4`.

   The `publish` job is byte-identical to `origin/main`.

4. **[PASS, with live-acceptance boundary] Action behavior**

   The pinned action’s basename behavior means the renamed input path will be presented to Particle as:

   `gcc-v<major>.<iteration>-boron-dos<deviceOS>-<sha7>.bin`

   The prior v49.1 result independently demonstrates that the action used the basename of the old path. The action’s bundled `dist` source was not present in this repository’s objects, and network access was prohibited, so it was not independently fetched.

   **[NOTE]** Only the next authorized real tag can confirm the final Console-visible filename and the complete GitHub Environment/action/Particle API path. No Particle token or API call was used during this review.

5. **[PASS] Tests, failure ordering, and mutations**

   `tests/release_upload_job_test.py:214-217` asserts the canonical output path, sole renamed file, original disappearance, and unchanged content. Lines 258-271 verify changed-hash and two-binary failures leave the original files unrenamed.

   Independent mutations were all caught:

   - Original path output: caught (`bin='fw/a.bin'`).
   - Missing `dos`: caught.
   - Missing sha7: caught.
   - Rename before hash check: caught; bad-hash execution failed after exposing the incorrect renamed-file state.

   The committed test also passes its title, description, separator, pin, permission, Environment, and token mutations.

6. **[PASS] Documentation, scope, and budget**

   `AI_DEVELOPMENT_WORKFLOW.md:499` correctly documents the canonical Console filename and equality with the Release asset.

   - Workflow length: **179 lines**.
   - Workflow net change: **0 lines** (`2 additions`, `2 deletions`).
   - `src/`: untouched.
   - YAML parse: PASS.
   - `bash -n`: PASS for all **8** `run:` blocks.
   - `tests/release_upload_job_test.py`: PASS.
   - Full host suite: **115/115 (sh via zsh, py via python3)**.