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


LAST = {}
SHA7 = "abcdef1"
CANON = "gcc-v49.1-boron-dos6.6.0-" + SHA7 + ".bin"


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
                   WANT_SHA=want, GITHUB_REPOSITORY="o/r", GITHUB_SERVER_URL="https://example.test", GITHUB_OUTPUT=outp,
                   OS="6.6.0", GITHUB_SHA=SHA7 + "0123456789abcdef0123456789abcdef", STUB_TYPE=objtype, STUB_MSG=os.path.join(d, "msg"))
        r = subprocess.run(["bash", "-c", script], cwd=d, env=env, capture_output=True, text=True)
        LAST.clear()
        LAST["files"] = sorted(os.listdir(os.path.join(d, "fw")))
        for f in LAST["files"]:
            LAST[f] = open(os.path.join(d, "fw", f), "rb").read()
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

    link = "[GitHub Release](https://example.test/o/r/releases/tag/v49.1)"
    title = "Development Release - v49.1"
    hostile = ("Title $(id) `id`\n\nEOF\ndescription<<abc\nabc\n::set-output name=x::y\n"
               "version=9999\n$(id) `id` ${HOME}\n::add-mask::x\nEOF")
    below = "SECRET-BELOW\nverification $(id) `id`\nEOF\nversion=9999"
    # A `---` inside a fenced block is still the first `---` line: the split is purely line-based.
    cases = [
        ("normal", "What changed\n- a\n\nRollback: x\n---\nVerification\n" + below, "What changed\n- a\n\nRollback: x"),
        ("separator first", "---\n" + below, ""),
        ("hostile", hostile + "\n---\n" + below, hostile),
        ("fenced separator", "Intro\n```\n---\n```\n" + below, "Intro\n```"),
        ("evil title", "Evil title\nbody\n---\n" + below, "Evil title\nbody"),
    ]
    for name, msg, above in cases:
        code, out, raw = stub_run(script, "v49.1", msg)
        check(code == 0, "%s: exit %s" % (name, code))
        if code:
            continue
        want = (above + "\n" if above else "") + "\n" + link
        check(set(out) == {"bin", "version", "title", "description"}, "%s: output keys %s" % (name, sorted(out)))
        check(out.get("version") == "4901", "%s: version %r" % (name, out.get("version")))
        check(out.get("title") == title, "%s: title %r" % (name, out.get("title")))
        check(out.get("description") == want, "%s: description %r != %r" % (name, out.get("description"), want))
        check("SECRET-BELOW" not in raw and "verification" not in raw, "%s: text below --- leaked" % name)
        check(out.get("bin", "").endswith(CANON), "%s: bin %r does not end with %s" % (name, out.get("bin"), CANON))
        check(out.get("bin") == "fw/" + CANON, "%s: bin %r" % (name, out.get("bin")))
        check(LAST.get("files") == [CANON], "%s: fw/ holds %r, want only the renamed file" % (name, LAST.get("files")))
        check(LAST.get(CANON) == b"firmware a.bin", "%s: renamed content differs" % name)
    code, out, _ = stub_run(script, "v49.1", "---\nbelow")
    check(code == 0 and out["description"].strip() == link, "separator-first description is only the link")
    code, out, raw = stub_run(script, "v49.1", "No separator here\nbody")
    check(code != 0 and raw == "", "no --- line must fail with no outputs (code %s, raw %r)" % (code, raw))
    code, out, raw = stub_run(script, "v49.1", "x\n --- \n----\n---x")
    check(code != 0 and raw == "", "near-miss separators must not count")

    muts = {
        "title from first line": ('echo "title=Development Release - $TAG"', 'echo "title=$TAG: ${msg%%$\'\\n\'*}"'),
        "description includes below": ("sed '/^---$/,$d' <<<\"$msg\"", 'echo "$msg"'),
        "no fail-closed check": ("grep -qx -- '---' <<<\"$msg\" ||", "false ||"),
        "link missing": ('echo "[GitHub Release]($GITHUB_SERVER_URL/$GITHUB_REPOSITORY/releases/tag/$TAG)"; ', ""),
        "bin is original path": ('bin=$new\n', 'new=$new\n'),
        "name missing dos": ("-boron-dos$OS-", "-boron-$OS-"),
        "name missing sha7": ("-${GITHUB_SHA::7}.bin", ".bin"),
        "sed removed": ("sed '/^---$/,$d' <<<\"$msg\"", "cat <<<\"$msg\""),
    }
    for name, (a, b) in muts.items():
        ms = script.replace(a, b)
        check(ms != script, "script mutation %r did not change the script" % name)
        ok = True
        for _, msg, above in cases[:3]:
            code, out, raw = stub_run(ms, "v49.1", msg)
            want = (above + "\n" if above else "") + "\n" + link
            if code != 0 or out.get("title") != title or out.get("description") != want or "SECRET-BELOW" in raw \
                    or out.get("bin") != "fw/" + CANON or LAST.get("files") != [CANON]:
                ok = False
        code, _, _ = stub_run(ms, "v49.1", "No separator\nbody")
        if code == 0:
            ok = False
        check(not ok, "script mutation %r not detected" % name)
    code, out, _ = stub_run(script, "v7.12", "T\n---")
    check(code == 0 and out["version"] == "712", "v7.12 should give 712")
    code, out, _ = stub_run(script, "v10.01", "T\n---")
    check(code == 0 and out["version"] == "1001", "v10.01 should give 1001 (no octal error)")

    for name, kw in [
        ("lightweight tag", dict(tag="v49.1", message="T\n---", objtype="commit")),
        ("bad tag name", dict(tag="v49.1.2", message="T\n---")),
        ("bad tag name 3 digits", dict(tag="v100.1", message="T\n---")),
        ("changed hash", dict(tag="v49.1", message="T\n---", good_hash=False)),
        ("two bins", dict(tag="v49.1", message="T\n---", bins=("a.bin", "b.bin"))),
        ("no bin", dict(tag="v49.1", message="T\n---")),
    ]:
        kw = dict(kw)
        if name == "no bin":
            d_script = script.replace("find fw -name '*.bin'", "find fw -name '*.none'")
            code, _, _ = stub_run(d_script, kw["tag"], kw["message"])
        else:
            code, _, _ = stub_run(script, kw.pop("tag"), kw.pop("message"), **kw)
        check(code != 0, "%s should fail" % name)
        if name in ("changed hash", "two bins"):
            want_files = ["a.bin", "b.bin"] if name == "two bins" else ["a.bin"]
            check(LAST.get("files") == want_files, "%s: renamed before failing (%r)" % (name, LAST.get("files")))

    if fails:
        print("FAIL")
        for x in fails:
            print(" -", x)
        return 1
    print("release_upload_job_test: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
