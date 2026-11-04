# Report: WO-2026-11-03-003 Amendment 7 (Stage 6, Copilot)

Model/reasoning: claude-sonnet-5.5 (dispatch said medium; the runtime did not expose the effort setting). Interpreters: Python 3.14.6, zsh 5.9, ruby for YAML (no PyYAML on the host).

## Change
`build.yml` stays at **179 lines** (changed in place, 2 lines). Exact new lines (`meta` step):

```
msg=$(gh api "$api/tags/${ref##* }" --jq .message); d=$(openssl rand -hex 8); grep -qx -- '---' <<<"$msg" || { echo "::error::tag message has no --- line"; exit 1; }
{ echo "bin=$bin"; echo "version=$ver"; echo "title=Development Release - $TAG"; echo "description<<$d"; sed '/^---$/,$d' <<<"$msg"; echo; echo "[GitHub Release]($GITHUB_SERVER_URL/$GITHUB_REPOSITORY/releases/tag/$TAG)"; echo "$d"; } >> "$GITHUB_OUTPUT"
```
Also edited `tests/release_upload_job_test.py` (new cases and 5 script mutations) and `AI_DEVELOPMENT_WORKFLOW.md` (Stage 9 item 7, post-tag Console check; no other checklist line mentioned the old title).

## Verification
- YAML parse (ruby): OK. `bash -n` on every `run:` block: all 0.
- Host suite: **112/112** (sh via zsh, py via python3). No scratch export copies existed, so no clean-export run of `thermal_coupling_structural_test.py`.
- `release_upload_job_test.py`: PASS, including the mutations (title from first line, below-`---` text included, no fail-closed check, link missing, `sed` removed), each detected.
- `PARTICLE_ACCESS_TOKEN` appears once; no `${{ }}` in the `upload` run block; pin unchanged; `src/` untouched; `git diff --stat`: only the 3 allowed files plus this report.

## Deviations / notes
- When `---` is the first line, the description is `\n<link>` (a leading blank line from the `echo`), not the bare link; the test compares the stripped value. Fixing that needs extra lines or a conditional.
- If the text above `---` ends with a blank line, two blank lines precede the link (harmless in Markdown).
- The pre-existing `${{ }}` in a `firmware` job run block is unchanged and outside this scope.
- `build-tmp/wo-2026-11-03-003-stage6-partb-fmt/` was created but unused.

## Open questions
None.
