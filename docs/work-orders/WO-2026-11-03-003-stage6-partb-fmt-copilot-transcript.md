# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `50f50356-3172-4927-9506-32e35c29a840`  
> - **Started:** 11/4/2026, 2:38:39 PM  
> - **Duration:** 14m 49s  
> - **Exported:** 11/4/2026, 2:53:29 PM  

---

<sub>6s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-03-003-stage6-partb-fmt-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>8s</sub>

### `bash`

**Read dispatch**

$ cat docs/work-orders/WO-2026-11-03-003-stage6-partb-fmt-copilot-dispatch.md; git status --short | head

<details>
<summary>16 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit only `.github/workflows/build.yml` (the `upload` job's `meta` step), `tests/release_upload_job_test.py`, `AI_DEVELOPMENT_WORKFLOW.md` Stage 9 item 7 and the release checklist lines, and write the report; run host tests (zsh for .sh, python3 for .py) and local shell/YAML checks; scratch `build-tmp/wo-2026-11-03-003-stage6-partb-fmt/` (create it; use only that) / NOT authorized: git commit/push/merge/rebase/reset/checkout/switch/stash, gh pr, creating tags or Releases, ANY use of a Particle token or call to api.particle.io, any network write, deleting anything you did not create (including build-tmp/ itself).

# Stage 6 dispatch: WO-2026-11-03-003 Amendment 7 (Particle title and description format)
Binding spec: `docs/work-orders/WO-2026-11-03-003-release-pipeline.md` **Amendment 7** (read it first; it replaces Amendment 1's title/description rule for Particle). Base: `main` at `824f78a` (Part B merged). Leave changes uncommitted. **Workflow budget: no net lines** (`build.yml` stays at 179 lines; the change is in place).
## Do
1. **`build.yml`, `upload` job, `meta` step** (currently lines ~175-176): 
   - `title` output becomes `Development Release - $TAG` (from the validated tag; never from the message).
   - `description` output becomes: the part of the tag message **above the first line that is exactly `---`** (use `sed '/^---$/,$d'` on the message), then a blank line and `[GitHub Release]($GITHUB_SERVER_URL/$GITHUB_REPOSITORY/releases/tag/$TAG)`, written through the existing random heredoc delimiter (keep it injection-safe).
   - **Fail closed:** if the message has no line exactly `---` (`grep -qx -- '---'`), `::error::` and exit 1 before any output is written and before the upload action runs (so the full notes can never reach Particle by omission). Keep the annotated-tag, SHA-256, regex and single-`.bin` checks unchanged. Keep both new behaviors within the existing two lines if possible (the first line gains the `grep`, the second line the new title and description pieces); if not, stop and report the exact line count.
2. **`tests/release_upload_job_test.py`**: update the expectations of the stub runs of the real `meta` step: normal message with a `---` line (title `Development Release - v49.1`; description = the text above `---` + blank + the Release link; text below `---` absent from every output); `---` as the first line (description is only the link); a message with **no `---` line → failure, no outputs**; a hostile message (`$(id)`, backticks, `EOF`, `description<<abc`, `::set-output`, `version=9999`, a `---` inside a fenced block is still the first `---` line: document that behaviour in a test comment) above and below the separator: outputs carry the above-separator text verbatim and only the four keys; a title can never come from the message (a message whose first line says `Evil title` leaves the title `Development Release - v49.1`); lightweight tag, bad tag names, changed hash, two `.bin` still fail. Mutations (each must fail): title from the first message line; description includes text below `---`; no fail-closed check; link missing; `sed` removed.
3. **`AI_DEVELOPMENT_WORKFLOW.md`**: Stage 9 item 7: the Particle title is generated (`Development Release - v<major>.<iteration>`); the Particle description is the tag-message part above the first `---` line plus a link to the GitHub Release; everything below `---` (verification, build record, rollout) stays on the GitHub Release only; a tag message without a `---` line fails the upload job. Update the post-tag Console check and any release-checklist line that mentions the old `<tag>: <first line>` title. State the tag-message convention: first lines are the Markdown description sections (What changed, Not in this release, Rollback), then `---`, then the GitHub-only sections.
## Verify
YAML parse; `bash -n` on every `run:` block; host suite `N/N (sh via zsh, py via python3)` (count with `p=$((p+1))`; `thermal_coupling_structural_test.py` also on a clean export if scratch copies exist); `wc -l build.yml` = 179 or less; `git diff --stat` shows only the allowed files; grep proof: `PARTICLE_ACCESS_TOKEN` still once in the action step, no `${{ }}` inside the `run:` block, the pin unchanged, `src/` untouched.
## Report
`docs/work-orders/WO-2026-11-03-003-stage6-partb-fmt-copilot-report.md`: model and reasoning actually used, interpreter line, line count, the exact new lines, each verification result, deviations, open questions.
<shellId: 0 completed with exit code 0>
```

</details>


---

<sub>10s</sub>

### `bash`

**Read amendment, workflow**

$ grep -n "Amendment 7" -A40 docs/work-orders/WO-2026-11-03-003-release-pipeline.md | head -90; sed -n 140,179p .github/workflows/build.yml; wc -l .github/workflows/build.yml tests/release_upload_job_test.py

<details>
<summary>48 lines</summary>

```
90:## Amendment 7 (Chip, 4 Nov 2026): Particle title and description format
91-Replaces Amendment 1's "title = tag name plus the first line, description = the rest" for the Particle upload (the GitHub Release keeps the whole annotation as its notes).
92-- **Title:** always `Development Release - v<major>.<iteration>`, generated from the tag (`Development Release - v49.1`), never from the annotation.
93-- **Description (Markdown):** the "What changed" bullets, "Not in this release", the rollback line, and a link to the GitHub Release (`$GITHUB_SERVER_URL/$GITHUB_REPOSITORY/releases/tag/$TAG`, appended by the workflow). **The part of the tag message above the first line that is exactly `---` goes to Particle**; everything below it (verification, build record, rollout) stays on the GitHub Release only. A tag message with no `---` line fails the upload job before the action runs (fail closed: nothing may leak into Particle by omission).
94-- Budget: the `upload` job keeps its +25 (179 lines); the change is in place (no net lines). Stage 6 Copilot standard/medium; Stage 7 Codex `gpt-5.6-sol` high, §4 SHA rule. Docs: Stage 9 item 7 and the release checklist; `docs/releases/v49.1-notes.md` is drafted to this format (the first-line title is dropped; the draft marks the Particle part).
          bin=$(find fw -name '*.bin')
          [ -n "$bin" ] && [ "$(wc -l <<<"$bin")" -eq 1 ] || { echo "::error::expected exactly one .bin"; exit 1; }
          [ "$(sha256sum "$bin" | cut -d' ' -f1)" = "$WANT_SHA" ] || { echo "::error::artifact SHA-256 differs from the firmware job's"; exit 1; }
          name="gcc-$TAG-boron-dos$OS-${GITHUB_SHA::7}.bin"
          mv "$bin" "$name"
          sha256sum "$name" > "$name.sha256"
          if gh release view "$TAG" --repo "$GITHUB_REPOSITORY" >/dev/null 2>&1; then echo "::error::a Release for $TAG already exists; not modifying it"; exit 1; fi
          [ "$(gh api "repos/$GITHUB_REPOSITORY/git/ref/tags/$TAG" --jq .object.type)" = tag ] || { echo "::error::$TAG is not an annotated tag"; exit 1; }
          tagsha=$(gh api "repos/$GITHUB_REPOSITORY/git/ref/tags/$TAG" --jq .object.sha)
          [ "$(gh api "repos/$GITHUB_REPOSITORY/git/tags/$tagsha" --jq .object.sha)" = "$GITHUB_SHA" ] || { echo "::error::$TAG no longer points at $GITHUB_SHA"; exit 1; }
          gh api "repos/$GITHUB_REPOSITORY/git/tags/$tagsha" --jq .message > "$RUNNER_TEMP/notes.txt"
          gh release create "$TAG" "$name" "$name.sha256" --repo "$GITHUB_REPOSITORY" --verify-tag --title "$TAG" --notes-file "$RUNNER_TEMP/notes.txt"
          printf '%s\n' "### Release $TAG" "- run: $GITHUB_SERVER_URL/$GITHUB_REPOSITORY/actions/runs/$GITHUB_RUN_ID" \
            "- commit: $GITHUB_SHA" "- bin: $name ($(stat -c%s "$name") bytes)" "- sha256: $WANT_SHA" \
            "- source version: $want" "- host tests: $TESTS" "- buildpack image digest: $DIGEST" >> "$GITHUB_STEP_SUMMARY"

  upload:
    if: github.ref_type == 'tag'
    needs: [firmware, publish]
    runs-on: ubuntu-latest
    environment: particle-42131
    permissions: {contents: read}
    steps:
      - uses: actions/download-artifact@9000827ccba6bdab643e8b6fd33ac0654aef8333 # v8.0.2
        with: {name: firmware-boron, path: fw}
      - id: meta
        env: {GH_TOKEN: "${{ github.token }}", TAG: "${{ github.ref_name }}", WANT_SHA: "${{ needs.firmware.outputs.sha256 }}"}
        run: |
          set -euo pipefail
          [[ "$TAG" =~ ^v([0-9]{1,2})\.([0-9]{1,2})$ ]] || { echo "::error::tag '$TAG' is not v<major>.<iteration>"; exit 1; }
          ver=$((10#${BASH_REMATCH[1]} * 100 + 10#${BASH_REMATCH[2]})); bin=$(find fw -name '*.bin')
          [ -n "$bin" ] && [ "$(wc -l <<<"$bin")" -eq 1 ] || { echo "::error::expected exactly one .bin"; exit 1; }
          [ "$(sha256sum "$bin" | cut -d' ' -f1)" = "$WANT_SHA" ] || { echo "::error::artifact SHA-256 differs from the firmware job's"; exit 1; }
          api=repos/$GITHUB_REPOSITORY/git; ref=$(gh api "$api/ref/tags/$TAG" --jq '.object.type + " " + .object.sha')
          [ "${ref%% *}" = tag ] || { echo "::error::$TAG is not an annotated tag"; exit 1; }
          msg=$(gh api "$api/tags/${ref##* }" --jq .message); d=$(openssl rand -hex 8)
          { echo "bin=$bin"; echo "version=$ver"; echo "title=$TAG: ${msg%%$'\n'*}"; echo "description<<$d"; tail -n +2 <<<"$msg"; echo "$d"; } >> "$GITHUB_OUTPUT"
      - uses: particle-iot/firmware-upload-action@9e393d6b362208488db91339ab72ee1d7e2e6ada # v1.0.4
        with: {particle-access-token: "${{ secrets.PARTICLE_ACCESS_TOKEN }}", firmware-path: "${{ steps.meta.outputs.bin }}", firmware-version: "${{ steps.meta.outputs.version }}",
          product-id: 42131, title: "${{ steps.meta.outputs.title }}", description: "${{ steps.meta.outputs.description }}"}
     179 .github/workflows/build.yml
     230 tests/release_upload_job_test.py
     409 total
<shellId: 1 completed with exit code 0>
```

</details>


---

<sub>12s</sub>

### `bash`

**Read test and doc refs**

$ cat tests/release_upload_job_test.py; grep -n "first line\|Stage 9\|<tag>:\|Console\|title" AI_DEVELOPMENT_WORKFLOW.md

<details>
<summary>210 lines</summary>

```
#!/usr/bin/env python3
"""Structural test and stub run of the `upload` job in .github/workflows/build.yml
(WO-2026-11-03-003 Part B). No network, no Particle token."""
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WORKFLOW = os.path.join(ROOT, ".github", "workflows", "build.yml")
PIN = "9e393d6b362208488db91339ab72ee1d7e2e6ada"
ACTION = "particle-iot/firmware-upload-action"
TOKEN = "PARTICLE_ACCESS_TOKEN"


def load_yaml(text):
    try:
        import yaml
        return yaml.safe_load(text)
    except ImportError:
        out = subprocess.run(
            ["ruby", "-ryaml", "-rjson", "-e", "puts JSON.generate(YAML.safe_load(STDIN.read))"],
            input=text, capture_output=True, text=True, check=True).stdout
        return json.loads(out)


def action_step(job):
    for s in job.get("steps", []):
        if s.get("uses", "").startswith(ACTION + "@"):
            return s
    return None


def structure_failures(text):
    f = []
    try:
        job = load_yaml(text)["jobs"]["upload"]
    except Exception as e:
        return ["upload job missing or YAML unparseable: %r" % e]
    if job.get("environment") != "particle-42131":
        f.append("environment is not particle-42131")
    needs = job.get("needs")
    needs = [needs] if isinstance(needs, str) else (needs or [])
    if "publish" not in needs:
        f.append("needs lacks publish")
    if job.get("if") != "github.ref_type == 'tag'":
        f.append("if is not the tag guard")
    if job.get("permissions") != {"contents": "read"}:
        f.append("permissions are not exactly contents: read")
    act = action_step(job)
    if act is None:
        f.append("no firmware-upload-action step")
    else:
        if act["uses"] != "%s@%s" % (ACTION, PIN):
            f.append("action not pinned to %s" % PIN)
        w = act.get("with", {})
        if TOKEN not in str(w.get("particle-access-token", "")):
            f.append("token not in the action step's with:")
        if str(w.get("product-id")) != "42131":
            f.append("product-id is not 42131")
        if act.get("env"):
            f.append("action step has env:")
    if len(re.findall(TOKEN, text)) != 1:
        f.append("%s does not appear exactly once" % TOKEN)
    if "pull_request_target" in text:
        f.append("pull_request_target present")
    if any("checkout" in s.get("uses", "") for s in job.get("steps", [])):
        f.append("checkout in upload job")
    sec = [l for l in text.splitlines() if "secrets." in l]
    if len(sec) != 1 or "secrets." + TOKEN not in sec[0] or "particle-access-token" not in sec[0]:
        f.append("secrets. referenced outside the one action line")
    for s in job.get("steps", []):
        if "${{" in s.get("run", ""):
            f.append("${{ }} inside a run: block")
    return f


def meta_script(text):
    job = load_yaml(text)["jobs"]["upload"]
    return next(s for s in job["steps"] if s.get("id") == "meta")["run"]


GH_STUB = r"""#!/bin/sh
case "$*" in
  *"/git/ref/tags/"*) echo "$STUB_TYPE 0123456789abcdef0123456789abcdef01234567" ;;
  *"/git/tags/0123456789abcdef"*) cat "$STUB_MSG" ;;
  *) echo "unexpected gh call: $*" >&2; exit 9 ;;
esac
"""
SHA_STUB = '#!/bin/sh\nexec shasum -a 256 "$@"\n'


def parse_outputs(path):
    out, lines, i = {}, open(path).read().split("\n"), 0
    while i < len(lines):
        l = lines[i]
        if not l:
            i += 1
            continue
        m = re.match(r"^([A-Za-z_]+)<<(.+)$", l)
        if m:
            end = lines.index(m.group(2), i + 1)
            out[m.group(1)] = "\n".join(lines[i + 1:end])
            i = end + 1
        else:
            k, v = l.split("=", 1)
            out[k] = v
            i += 1
    return out


def stub_run(script, tag, message, objtype="tag", bins=("a.bin",), good_hash=True):
    d = tempfile.mkdtemp()
    try:
        bd = os.path.join(d, "bin")
        os.mkdir(bd)
        for name, body in (("gh", GH_STUB), ("sha256sum", SHA_STUB)):
            if name == "sha256sum" and shutil.which("sha256sum"):
                continue
            p = os.path.join(bd, name)
            open(p, "w").write(body)
            os.chmod(p, os.stat(p).st_mode | stat.S_IXUSR)
        os.mkdir(os.path.join(d, "fw"))
        for b in bins:
            open(os.path.join(d, "fw", b), "wb").write(b"firmware " + b.encode())
        want = subprocess.run("sha256sum fw/a.bin 2>/dev/null || shasum -a 256 fw/a.bin",
                              shell=True, cwd=d, capture_output=True, text=True,
                              env=dict(os.environ, PATH=bd + os.pathsep + os.environ["PATH"])).stdout.split()[0]
        if not good_hash:
            want = "0" * 64
        open(os.path.join(d, "msg"), "w").write(message + "\n")
        outp = os.path.join(d, "out")
        open(outp, "w").close()
        env = dict(os.environ, PATH=bd + os.pathsep + os.environ["PATH"], GH_TOKEN="x", TAG=tag,
                   WANT_SHA=want, GITHUB_REPOSITORY="o/r", GITHUB_OUTPUT=outp,
                   STUB_TYPE=objtype, STUB_MSG=os.path.join(d, "msg"))
        r = subprocess.run(["bash", "-c", script], cwd=d, env=env, capture_output=True, text=True)
        return r.returncode, (parse_outputs(outp) if r.returncode == 0 else None), open(outp).read()
    finally:
        shutil.rmtree(d)


def main():
    text = open(WORKFLOW).read()
    fails = []

    def check(cond, msg):
        if not cond:
            fails.append(msg)

    for x in structure_failures(text):
        check(False, "structure: " + x)

    mutations = {
        "action @v1": (PIN, "v1"),
        "action v1.0.5 sha": (PIN, "9b0cda4c" + "0" * 32),
        "token moved to env": (None, None),
        "needs without publish": ("needs: [firmware, publish]", "needs: [firmware]"),
        "environment removed": ("    environment: particle-42131\n", ""),
        "token echoed in run": ("          set -euo pipefail\n          [[ \"$TAG\" =~ ^v([0-9]{1,2})\\.([0-9]{1,2})$ ]] || { echo \"::error::tag '$TAG' is not v<major>",
                                "          set -euo pipefail\n          echo ${{ secrets.%s }}\n          [[ \"$TAG\" =~ ^v([0-9]{1,2})\\.([0-9]{1,2})$ ]] || { echo \"::error::tag '$TAG' is not v<major>" % TOKEN),
        "pull_request_target": ("on:\n", "on:\n  pull_request_target:\n"),
    }
    for name, (a, b) in mutations.items():
        if name == "token moved to env":
            t = text.replace('particle-access-token: "${{ secrets.%s }}"' % TOKEN, 'particle-access-token: "x"')
            t = t.replace('GH_TOKEN: "${{ github.token }}"', 'GH_TOKEN: "${{ secrets.%s }}"' % TOKEN)
        else:
            t = text.replace(a, b) if name != "action @v1" else text.replace("@" + PIN, "@v1")
        check(t != text, "mutation %r did not change the file" % name)
        check(structure_failures(t), "mutation %r not detected" % name)

    script = meta_script(text)
    r = subprocess.run(["bash", "-n"], input=script, text=True, capture_output=True)
    check(r.returncode == 0, "bash -n of meta step: " + r.stderr)

    # normal, one-line, hostile messages: outputs carry the text verbatim and nothing else
    cases = [
        ("normal", "Title line\n\nBody one\nBody two"),
        ("one-line", "Only a title"),
        ("hostile", "Title $(id) `id`\n\nEOF\ndescription<<abc\nabc\n::set-output name=x::y\n"
                    "version=9999\n$(id) `id` ${HOME}\n::add-mask::x\nEOF"),
    ]
    for name, msg in cases:
        code, out, raw = stub_run(script, "v49.1", msg)
        check(code == 0, "%s: exit %s" % (name, code))
        if code:
            continue
        first, _, rest = msg.partition("\n")
        check(set(out) == {"bin", "version", "title", "description"}, "%s: output keys %s" % (name, sorted(out)))
        check(out.get("version") == "4901", "%s: version %r" % (name, out.get("version")))
        check(out.get("title") == "v49.1: " + first, "%s: title %r" % (name, out.get("title")))
        check(out.get("description") == rest, "%s: description %r != %r" % (name, out.get("description"), rest))
        check(out.get("bin") == "fw/a.bin", "%s: bin %r" % (name, out.get("bin")))
    code, out, _ = stub_run(script, "v7.12", "T")
    check(code == 0 and out["version"] == "712", "v7.12 should give 712")
    code, out, _ = stub_run(script, "v10.01", "T")
    check(code == 0 and out["version"] == "1001", "v10.01 should give 1001 (no octal error)")

    for name, kw in [
        ("lightweight tag", dict(tag="v49.1", message="T", objtype="commit")),
        ("bad tag name", dict(tag="v49.1.2", message="T")),
        ("bad tag name 3 digits", dict(tag="v100.1", message="T")),
        ("changed hash", dict(tag="v49.1", message="T", good_hash=False)),
        ("two bins", dict(tag="v49.1", message="T", bins=("a.bin", "b.bin"))),
        ("no bin", dict(tag="v49.1", message="T")),
    ]:
        kw = dict(kw)
        if name == "no bin":
            d_script = script.replace("find fw -name '*.bin'", "find fw -name '*.none'")
            code, _, _ = stub_run(d_script, kw["tag"], kw["message"])
        else:
            code, _, _ = stub_run(script, kw.pop("tag"), kw.pop("message"), **kw)
        check(code != 0, "%s should fail" % name)

    if fails:
        print("FAIL")
        for x in fails:
            print(" -", x)
        return 1
    print("release_upload_job_test: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
511:### Stage 9 — Release and feedback
521:6. **Bench testing on sleeping devices.** A device marked as a Development device ignores released firmware and version locks. Flash it directly over USB (or Console flash), or pin first and then un-mark it (un-marking can trigger an immediate OTA to the pinned version). Never rely on the pin alone for a Development-marked device. Serial-Pi flashing: not now.
523:7. **Upload to product 42131 (Part B, `upload` job).** Runs on tags only, after `publish`, behind the GitHub Environment `particle-42131`: Chip approves each run. It re-checks the tag name, the single `.bin` and its SHA-256 against the firmware job, and that the tag is annotated, then runs the SHA-pinned `particle-iot/firmware-upload-action` (v1.0.4) with the `github-firmware-upload` product API user (scope `firmware:create` only; secret `PARTICLE_ACCESS_TOKEN`, used in that one step). Version = major × 100 + iteration; title = `<tag>: <first line of the tag message>`; description = the rest of the tag message. The job uploads only: no OTA, no release, no pin, no default. There is **no dry-run upload**: a test version would stay in the product's firmware list and the token cannot remove it, so verification is local (YAML, `bash -n`, `tests/release_upload_job_test.py`) and acceptance is the first real tag.
524:   - **Post-tag Console check.** Console → product 42131 → Firmware shows the new version (major × 100 + iteration), the title (`<tag>: <first line>`) and the description (the rest of the tag message). If it does not, report to Chip: the token cannot remove a version; Chip deletes it in the Console.
596:A Codex verdict's first line quotes the `UNDER REVIEW` SHA. A missing or mismatched SHA invalidates the verdict: not counted as a round, archived, rerun.
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>20s</sub>

### `bash`

**Edit meta step**

$ python3 - <<'E'
p='.github/workflows/build.yml'
s=open(p).read()
a='''msg=$(gh api "$api/tags/${ref##* }" --jq .message); d=$(openssl rand -hex 8)
'''
b='''msg=$(gh api "$api/tags/${ref##* }" --jq .message); d=$(openssl rand -hex 8); grep -qx -- '---' <<<"$msg" || { echo "::error::tag message has no --- line"; exit 1; }
'''
c='''echo "title=$TAG: ${msg%%$'\\n'*}"; echo "description<<$d"; tail -n +2 <<<"$msg"; echo "$d"; }'''
e='''echo "title=Development Release - $TAG"; echo "description<<$d"; sed '/^---$/,$d' <<<"$msg"; echo; echo "[GitHub Release]($GITHUB_SERVER_URL/$GITHUB_REPOSITORY/releases/tag/$TAG)"; echo "$d"; }'''
assert a in s and c in s
open(p,'w').write(s.replace(a,b).replace(c,e))
E
wc -l .github/workflows/build.yml; git diff --stat

```
179 .github/workflows/build.yml
 .github/workflows/build.yml | 4 ++--
 1 file changed, 2 insertions(+), 2 deletions(-)
<shellId: 3 completed with exit code 0>
```


---

<sub>42s</sub>

### `bash`

**Update tests and run**

$ python3 - <<'E'
p='tests/release_upload_job_test.py'
s=open(p).read()
i=s.index("    # normal, one-line")
j=s.index("    code, out, _ = stub_run(script, \"v7.12\"")
new='''    link = "[GitHub Release](https://example.test/o/r/releases/tag/v49.1)"
    title = "Development Release - v49.1"
    hostile = ("Title $(id) `id`\\n\\nEOF\\ndescription<<abc\\nabc\\n::set-output name=x::y\\n"
               "version=9999\\n$(id) `id` ${HOME}\\n::add-mask::x\\nEOF")
    below = "SECRET-BELOW\\nverification $(id) `id`\\nEOF\\nversion=9999"
    # A `---` inside a fenced block is still the first `---` line: the split is purely line-based.
    cases = [
        ("normal", "What changed\\n- a\\n\\nRollback: x\\n---\\nVerification\\n" + below, "What changed\\n- a\\n\\nRollback: x"),
        ("separator first", "---\\n" + below, ""),
        ("hostile", hostile + "\\n---\\n" + below, hostile),
        ("fenced separator", "Intro\\n```\\n---\\n```\\n" + below, "Intro\\n```"),
        ("evil title", "Evil title\\nbody\\n---\\n" + below, "Evil title\\nbody"),
    ]
    for name, msg, above in cases:
        code, out, raw = stub_run(script, "v49.1", msg)
        check(code == 0, "%s: exit %s" % (name, code))
        if code:
            continue
        want = (above + "\\n" if above else "") + "\\n" + link
        check(set(out) == {"bin", "version", "title", "description"}, "%s: output keys %s" % (name, sorted(out)))
        check(out.get("version") == "4901", "%s: version %r" % (name, out.get("version")))
        check(out.get("title") == title, "%s: title %r" % (name, out.get("title")))
        check(out.get("description") == want, "%s: description %r != %r" % (name, out.get("description"), want))
        check("SECRET-BELOW" not in raw and "verification" not in raw, "%s: text below --- leaked" % name)
        check(out.get("bin") == "fw/a.bin", "%s: bin %r" % (name, out.get("bin")))
    code, out, _ = stub_run(script, "v49.1", "---\\nbelow")
    check(code == 0 and out["description"].strip() == link, "separator-first description is only the link")
    code, out, raw = stub_run(script, "v49.1", "No separator here\\nbody")
    check(code != 0 and raw == "", "no --- line must fail with no outputs (code %s, raw %r)" % (code, raw))
    code, out, raw = stub_run(script, "v49.1", "x\\n --- \\n----\\n---x")
    check(code != 0 and raw == "", "near-miss separators must not count")

    muts = {
        "title from first line": ('echo "title=Development Release - $TAG"', 'echo "title=$TAG: ${msg%%$\\'\\\\n\\'*}"'),
        "description includes below": ("sed '/^---$/,$d' <<<\\"$msg\\"", 'echo "$msg"'),
        "no fail-closed check": ("grep -qx -- '---' <<<\\"$msg\\" ||", "false ||"),
        "link missing": ('echo "[GitHub Release]($GITHUB_SERVER_URL/$GITHUB_REPOSITORY/releases/tag/$TAG)"; ', ""),
        "sed removed": ("sed '/^---$/,$d' <<<\\"$msg\\"", "cat <<<\\"$msg\\""),
    }
    for name, (a, b) in muts.items():
        ms = script.replace(a, b)
        check(ms != script, "script mutation %r did not change the script" % name)
        ok = True
        for _, msg, above in cases[:3]:
            code, out, raw = stub_run(ms, "v49.1", msg)
            want = (above + "\\n" if above else "") + "\\n" + link
            if code != 0 or out.get("title") != title or out.get("description") != want or "SECRET-BELOW" in raw:
                ok = False
        code, _, _ = stub_run(ms, "v49.1", "No separator\\nbody")
        if code == 0:
            ok = False
        check(not ok, "script mutation %r not detected" % name)
'''
s=s[:i]+new+s[j:]
s=s.replace('GITHUB_REPOSITORY="o/r",','GITHUB_REPOSITORY="o/r", GITHUB_SERVER_URL="https://example.test",')
s=s.replace('stub_run(script, "v7.12", "T")','stub_run(script, "v7.12", "T\\n---")').replace('stub_run(script, "v10.01", "T")','stub_run(script, "v10.01", "T\\n---")')
s=s.replace('message="T"','message="T\\n---"')
open(p,'w').write(s)
E
python3 tests/release_upload_job_test.py

```
release_upload_job_test: PASS
<shellId: 4 completed with exit code 0>
```


---

<sub>49s</sub>

### `bash`

**Edit docs, verify**

$ python3 - <<'E'
p='AI_DEVELOPMENT_WORKFLOW.md'
s=open(p).read()
a="title = `<tag>: <first line of the tag message>`; description = the rest of the tag message."
b="title = generated, `Development Release - v<major>.<iteration>` (never from the tag message); description = the part of the tag message above the first line that is exactly `---`, plus a link to the GitHub Release. Everything below `---` (verification, build record, rollout) stays on the GitHub Release only, and a tag message with no `---` line fails the upload job before the action runs. Tag-message convention: the first lines are the Markdown description sections (What changed, Not in this release, Rollback), then `---`, then the GitHub-only sections."
c="the title (`<tag>: <first line>`) and the description (the rest of the tag message)."
d="the title (`Development Release - v<major>.<iteration>`) and the description (the tag message above `---` plus the GitHub Release link)."
assert a in s and c in s
open(p,'w').write(s.replace(a,b).replace(c,d))
E
grep -n "first line\|release checklist\|Release checklist" AI_DEVELOPMENT_WORKFLOW.md | head
python3 - <<'E'
import yaml,subprocess
d=yaml.safe_load(open('.github/workflows/build.yml'))
for n,j in d['jobs'].items():
  for s in j['steps']:
    if 'run' in s: print(n,subprocess.run(['bash','-n'],input=s['run'],text=True).returncode, '${{' in s['run'])
E
grep -c PARTICLE_ACCESS_TOKEN .github/workflows/build.yml; git diff --stat; ls tests build-tmp 2>&1 | head -30

<details>
<summary>42 lines</summary>

```
518:   Release checklist: before pinning, run `particle binary inspect <the .bin>` and confirm `firmware for product id 13 at version <major×100+iteration>`.
523:7. **Upload to product 42131 (Part B, `upload` job).** Runs on tags only, after `publish`, behind the GitHub Environment `particle-42131`: Chip approves each run. It re-checks the tag name, the single `.bin` and its SHA-256 against the firmware job, and that the tag is annotated, then runs the SHA-pinned `particle-iot/firmware-upload-action` (v1.0.4) with the `github-firmware-upload` product API user (scope `firmware:create` only; secret `PARTICLE_ACCESS_TOKEN`, used in that one step). Version = major × 100 + iteration; title = generated, `Development Release - v<major>.<iteration>` (never from the tag message); description = the part of the tag message above the first line that is exactly `---`, plus a link to the GitHub Release. Everything below `---` (verification, build record, rollout) stays on the GitHub Release only, and a tag message with no `---` line fails the upload job before the action runs. Tag-message convention: the first lines are the Markdown description sections (What changed, Not in this release, Rollback), then `---`, then the GitHub-only sections. The job uploads only: no OTA, no release, no pin, no default. There is **no dry-run upload**: a test version would stay in the product's firmware list and the token cannot remove it, so verification is local (YAML, `bash -n`, `tests/release_upload_job_test.py`) and acceptance is the first real tag.
596:A Codex verdict's first line quotes the `UNDER REVIEW` SHA. A missing or mismatched SHA invalidates the verdict: not counted as a round, archived, rerun.
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
ModuleNotFoundError: No module named 'yaml'
1
 .github/workflows/build.yml      |  4 +--
 AI_DEVELOPMENT_WORKFLOW.md       |  4 +--
 tests/release_upload_job_test.py | 70 +++++++++++++++++++++++++++++-----------
 3 files changed, 56 insertions(+), 22 deletions(-)
ls: build-tmp: No such file or directory
tests:
README.md
SensorManagerBatteryAuthorityTests.cpp
alert19_clears_after_report_test.sh
awake_cycle_counters_test.sh
battery_authority_commit_sites_structural_test.py
battery_authority_seam_structural_test.py
battery_authority_structural_test.py
battery_health_test.cpp
battery_health_test.sh
battery_tier_guard_test.cpp
battery_tier_guard_test.sh
boron_soc_commit_test.py
build_flags_witness_test.sh
bump_version_scheme_test.sh
charge_inhibit_thermal_test.cpp
charge_inhibit_thermal_test.sh
clock_hibernate_helpers_move_structural_test.py
clock_openness_test.cpp
clock_openness_test.sh
clock_owner_structural_test.py
clock_resync_wiring_test.py
clock_rtc_writeback_test.cpp
clock_status_republish_test.cpp
clock_status_republish_test.sh
clock_trust_standard_structural_test.py
clock_trust_test.cpp
close_before_night_sleep_test.py
config_window_hold_test.cpp
<shellId: 5 completed with exit code 0>
```

</details>


---

<sub>53s</sub>

### Copilot

Use ruby for YAML (no pyyaml); run the whole suite.

---

<sub>12m 41s</sub>

### `bash`

**YAML, bash -n, full host suite**

$ ruby -ryaml -rjson -e 'd=YAML.safe_load(File.read(".github/workflows/build.yml"));d["jobs"].each{|n,j|j["steps"].each{|s|next unless s["run"];IO.popen("bash -n","w"){|f|f.write s["run"]};puts "#{n} bash-n=#{$?.exitstatus} expr=#{s["run"].include?("${{")}"}}'
mkdir -p build-tmp/wo-2026-11-03-003-stage6-partb-fmt
p=0;f=0;for t in tests/*_test.sh; do if zsh $t >/dev/null 2>&1; then p=$((p+1)); else f=$((f+1)); echo FAIL $t; fi; done
for t in tests/*_test.py; do if python3 $t >/dev/null 2>&1; then p=$((p+1)); else f=$((f+1)); echo FAIL $t; fi; done; echo "$p/$((p+f))"
git status --short; python3 --version; zsh --version

<details>
<summary>9 lines</summary>

```
host-tests bash-n=0 expr=false
host-tests bash-n=0 expr=false
firmware bash-n=0 expr=false
firmware bash-n=0 expr=false
firmware bash-n=0 expr=false
firmware bash-n=0 expr=true
publish bash-n=0 expr=false
upload bash-n=0 expr=false
<command with shellId: 6 is still running after 300 seconds. The command is still running. You will be automatically notified when it completes; if you need the command to complete end your response with no tool calls to wait for the notification. Use stop_bash to stop it.>
```

</details>


---

<sub>14m 36s</sub>

### `read_bash`

**Waiting up to 300 seconds for command output**

<details>
<summary>Arguments</summary>

```json
{
  "shellId": "6",
  "delay": 300
}
```

</details>

<details>
<summary>15 lines</summary>

```
host-tests bash-n=0 expr=false
host-tests bash-n=0 expr=false
firmware bash-n=0 expr=false
firmware bash-n=0 expr=false
firmware bash-n=0 expr=false
firmware bash-n=0 expr=true
publish bash-n=0 expr=false
upload bash-n=0 expr=false
112/112
 M .github/workflows/build.yml
 M AI_DEVELOPMENT_WORKFLOW.md
 M tests/release_upload_job_test.py
Python 3.14.6
zsh 5.9 (arm64-apple-darwin26.0)
<shellId: 6 completed with exit code 0>
```

</details>


---

<sub>14m 45s</sub>

### `bash`

**Write report**

$ cat > docs/work-orders/WO-2026-11-03-003-stage6-partb-fmt-copilot-report.md <<'E'
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
E

```

<shellId: 7 completed with exit code 0>
```


---

<sub>14m 49s</sub>

### Copilot

Amendment 7 is implemented and left uncommitted. `build.yml` is still 179 lines, the host suite passes 112/112, and the upload-job test and all five mutations pass. YAML parses and `bash -n` passes on every `run:` block.

- **`meta` step:** the title is now `Development Release - $TAG`. The description is the text above the first `---` line, a blank line, then the GitHub Release link. A tag message with no `---` line fails with `::error::` before any output is written. This took exactly the two existing lines.
- **Tests and docs:** `tests/release_upload_job_test.py` has the new cases and mutations. `AI_DEVELOPMENT_WORKFLOW.md` Stage 9 item 7 and the post-tag Console check describe the new format. No other checklist line mentioned the old title.
- **Other checks:** `PARTICLE_ACCESS_TOKEN` still appears once and the action pin is unchanged. The `upload` run block has no `${{ }}`, and `src/` is untouched. I did not run `thermal_coupling_structural_test.py` on a clean export because no scratch copies existed.
- **Deviation:** when `---` is the first line, the description is a blank line plus the link, not the bare link. The test compares the stripped value. Removing the blank line would need extra lines or a conditional.
- **Unused scratch dir:** I created `build-tmp/wo-2026-11-03-003-stage6-partb-fmt/` but did not use it.

The report is at `docs/work-orders/WO-2026-11-03-003-stage6-partb-fmt-copilot-report.md`.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>
