#!/usr/bin/env python3
"""
WO-2026-10-02-003 (v34-SleepConfigLeak) - structural test for sleep
configuration ownership.

Device OS's `SystemSleepConfiguration` move assignment
(`system/inc/system_sleep_configuration.h:205-210` in 6.4.1) `memcpy`s the
incoming configuration over the existing one *without* freeing the old
`wakeup_sources` list. Only the destructor frees that list. A long-lived
(global or static) configuration that is reset with
`config = SystemSleepConfiguration();` before each sleep is therefore never
destroyed, and orphans its whole wake-source list on every single wake
cycle. The fix is to build a *fresh local* configuration at each sleep site,
so the destructor runs and frees the list when the scope ends.

This test is a source-invariant check on the REAL checked-in files.
Comments and string/char literals are blanked out before any check, so prose
(including this file's own history, if pasted into source) cannot produce a
false positive or a false negative.

Checks (WO Acceptance 1 and 3):

  1. No reused long-lived sleep configuration remains anywhere in `src/`:
     no file-scope or `static` `SystemSleepConfiguration` object, no
     `extern SystemSleepConfiguration` declaration, and no assignment to an
     existing `SystemSleepConfiguration` (`... = SystemSleepConfiguration(`,
     or any `name = ...` to a declared configuration variable, whatever
     follows the `=`, e.g. `ulpConfig = {};`).

  2. Every `System.sleep(x)` call in `src/` passes a configuration that is
     declared as a block-scope local `SystemSleepConfiguration`, and no two
     sleep sites share one object.

  3. The four `State_Sleep.cpp` sleep sites still configure exactly the same
     wake sources - the same pins and edges, the same RTC duration, and
     network standby under the same `useNetworkStandby` / cellular guards -
     as at the WO's baseline commit 43b8a69. The expectations below are
     transcribed from that commit; when the baseline commit is reachable via
     git, the same extractor is additionally run against it and the two are
     required to agree.

Set SLEEP_CONFIG_SRC_ROOT to point the checks at a copy of `src/` (mutation
testing never touches the production tree); the self-mutation section at the
end does exactly that.
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASELINE_COMMIT = "43b8a69"

# The wake sources at each site, transcribed from 43b8a69. Keyed by the sleep
# mode each site uses plus a disambiguating tag, because the variable names
# are exactly what this WO changes and must not be pinned.
EXPECTED_SITES = {
    "hibernate": {
        "mode": "SystemSleepMode::HIBERNATE",
        "gpio": [("WAKEUP_PIN", "FALLING"), ("BUTTON_PIN", "FALLING")],
        "duration": None,
        "network": None,
    },
    "ulp": {
        "mode": "SystemSleepMode::ULTRA_LOW_POWER",
        "gpio": [("BUTTON_PIN", "FALLING"), ("intPin", "RISING")],
        "duration": "(uint32_t)wakeInSeconds*1000UL",
        "network": ("NETWORK_INTERFACE_CELLULAR",
                    "SystemSleepNetworkFlag::INACTIVE_STANDBY"),
    },
    "stop-gpio": {
        "mode": "SystemSleepMode::STOP",
        "gpio": [("BUTTON_PIN", "FALLING"), ("intPin", "RISING")],
        "duration": "(uint32_t)wakeInSeconds*1000UL",
        "network": None,
    },
    "stop-timer-only": {
        "mode": "SystemSleepMode::STOP",
        "gpio": [],
        "duration": "(uint32_t)wakeInSeconds*1000UL",
        "network": None,
    },
}

failures = []


def fail(msg):
    failures.append(msg)


def scrub(text):
    """Blank out comments and string/char literals, preserving offsets."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            while i < n and text[i] != '\n':
                out[i] = ' '
                i += 1
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            while i < n and not (text[i] == '*' and i + 1 < n and text[i + 1] == '/'):
                if text[i] != '\n':
                    out[i] = ' '
                i += 1
            for j in range(i, min(i + 2, n)):
                out[j] = ' '
            i += 2
        elif c in '"\'':
            quote = c
            out[i] = ' '
            i += 1
            while i < n and text[i] != quote:
                if text[i] == '\\':
                    out[i] = ' '
                    i += 1
                if i < n and text[i] != '\n':
                    out[i] = ' '
                i += 1
            if i < n:
                out[i] = ' '
                i += 1
        else:
            i += 1
    return "".join(out)


def sources(src_root):
    found = []
    for dirpath, _dirnames, filenames in os.walk(src_root):
        for name in sorted(filenames):
            if name.endswith((".cpp", ".h", ".hpp", ".ino")):
                found.append(os.path.join(dirpath, name))
    return sorted(found)


def line_of(text, index):
    return text.count("\n", 0, index) + 1


def statement_at(scrubbed, idx):
    """Return (start, end) of the statement containing offset idx.

    The statement starts after the previous `;`, `{` or `}` and ends at the
    next `;` that is not nested inside parentheses or braces.
    """
    start = max(scrubbed.rfind(ch, 0, idx) for ch in ";{}") + 1
    depth = 0
    for i in range(idx, len(scrubbed)):
        c = scrubbed[i]
        if c in "({[":
            depth += 1
        elif c in ")}]":
            depth -= 1
        elif c == ';' and depth <= 0:
            return start, i
    return start, len(scrubbed)


def split_args(arglist):
    """Split a call's argument list on top-level commas."""
    args, depth, cur = [], 0, []
    for c in arglist:
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        if c == ',' and depth == 0:
            args.append("".join(cur))
            cur = []
        else:
            cur.append(c)
    if "".join(cur).strip():
        args.append("".join(cur))
    return [re.sub(r"\s+", "", a) for a in args]


def calls_on(scrubbed, text, name):
    """Every `name.method(args)` call, including chained ones, in order.

    Returns a list of (method, [normalized args], offset).
    """
    out = []
    for m in re.finditer(r"\b%s\s*\." % re.escape(name), scrubbed):
        stmt_start, stmt_end = statement_at(scrubbed, m.start())
        stmt = scrubbed[stmt_start:stmt_end]
        raw = text[stmt_start:stmt_end]
        for call in re.finditer(r"\.\s*([A-Za-z_]\w*)\s*\(", stmt):
            open_paren = call.end() - 1
            depth, close = 0, None
            for i in range(open_paren, len(stmt)):
                if stmt[i] == '(':
                    depth += 1
                elif stmt[i] == ')':
                    depth -= 1
                    if depth == 0:
                        close = i
                        break
            if close is None:
                fail("unbalanced call arguments near %s.%s" % (name, call.group(1)))
                continue
            out.append((call.group(1),
                        split_args(raw[open_paren + 1:close]),
                        stmt_start + call.start()))
        # Dedupe: a statement is re-scanned once per `name.` occurrence in it.
        out = [c for i, c in enumerate(out) if c not in out[:i]]
    out.sort(key=lambda c: c[2])
    return out


def enclosing_text(text, scrubbed, idx, back=1400):
    """The raw source shortly before idx, for guard checks."""
    return text[max(0, idx - back):idx]


def extract_sites(sleep_src_text):
    """Describe every System.sleep() site in State_Sleep.cpp.

    Returns {tag: {mode, gpio, duration, network, var, decl_local,
                   network_guarded}}.
    """
    text = sleep_src_text
    scrubbed = scrub(text)
    sites = {}
    seen_vars = []
    for m in re.finditer(r"System\s*\.\s*sleep\s*\(\s*([A-Za-z_]\w*)\s*\)", scrubbed):
        var = m.group(1)
        seen_vars.append((var, line_of(text, m.start())))
        # A site's configuration starts at the nearest preceding point where
        # the object is created or reset: its declaration, or (in the old
        # shared-object pattern this WO removes) an assignment reset. Only
        # calls after that boundary belong to this site.
        boundaries = [b.start() for b in re.finditer(
            r"(?:SystemSleepConfiguration\s+%s\s*;)|(?:\b%s\s*=\s*SystemSleepConfiguration\s*\()"
            % (re.escape(var), re.escape(var)), scrubbed) if b.start() < m.start()]
        start_at = max(boundaries) if boundaries else -1
        calls = [c for c in calls_on(scrubbed, text, var)
                 if start_at < c[2] < m.start()]
        mode = None
        gpio = []
        duration = None
        network = None
        network_guarded = False
        for method, args, off in calls:
            if method == "mode" and args:
                mode = args[0]
            elif method == "gpio" and len(args) >= 2:
                gpio.append((args[0], args[1]))
            elif method == "duration" and args:
                duration = args[0]
            elif method == "network" and len(args) >= 2:
                network = (args[0], args[1])
                before = enclosing_text(text, scrubbed, off)
                network_guarded = ("if (useNetworkStandby)" in before
                                   and "#if HAL_PLATFORM_CELLULAR" in before)
        decl = re.search(r"(^|[;{}\n])([ \t]*)(static\s+)?SystemSleepConfiguration\s+%s\s*;"
                         % re.escape(var), scrubbed, re.M)
        decl_local = bool(decl) and decl.group(3) is None and decl.group(2) != ""
        if mode == "SystemSleepMode::STOP":
            tag = "stop-gpio" if gpio else "stop-timer-only"
        elif mode == "SystemSleepMode::HIBERNATE":
            tag = "hibernate"
        elif mode == "SystemSleepMode::ULTRA_LOW_POWER":
            tag = "ulp"
        else:
            tag = "unknown:%s" % mode
        sites[tag] = {
            "mode": mode, "gpio": gpio, "duration": duration,
            "network": network, "var": var, "decl_local": decl_local,
            "network_guarded": network_guarded,
            "line": line_of(text, m.start()),
        }
    sites["__vars__"] = seen_vars
    return sites


def check_no_long_lived(src_root):
    """WO Acceptance 1."""
    decl_re = re.compile(
        r"(?:^|[;{}])\s*(?:static\s+|extern\s+)?SystemSleepConfiguration\s+[A-Za-z_]\w*\s*[;=]")
    assign_re = re.compile(r"=\s*SystemSleepConfiguration\s*\(")
    extern_re = re.compile(r"\bextern\s+SystemSleepConfiguration\b")
    static_re = re.compile(r"\bstatic\s+SystemSleepConfiguration\b")

    for path in sources(src_root):
        rel = os.path.relpath(path, src_root)
        with open(path, "r", errors="replace") as fh:
            text = fh.read()
        if "SystemSleepConfiguration" not in text:
            continue
        scrubbed = scrub(text)
        for m in extern_re.finditer(scrubbed):
            fail("%s:%d: extern SystemSleepConfiguration - the shared sleep "
                 "configuration must not come back" % (rel, line_of(text, m.start())))
        for m in static_re.finditer(scrubbed):
            fail("%s:%d: static SystemSleepConfiguration - a long-lived "
                 "configuration is never destroyed and leaks its wake-source "
                 "list on every reset" % (rel, line_of(text, m.start())))
        for m in assign_re.finditer(scrubbed):
            fail("%s:%d: assignment to an existing SystemSleepConfiguration - "
                 "Device OS's move assignment memcpys over the old "
                 "wakeup_sources list without freeing it"
                 % (rel, line_of(text, m.start())))
        # Any assignment to a declared configuration variable, whatever the
        # right-hand side (`= {}`, `= std::move(...)`, ...), reaches the same
        # leaking move assignment (WO-2026-10-02-003 Stage 7 round 1, P2).
        names = set(re.findall(r"\bSystemSleepConfiguration\s+([A-Za-z_]\w*)", scrubbed))
        for name in names:
            for m in re.finditer(r"(?<![\w.>:])%s\s*=(?!=)" % re.escape(name), scrubbed):
                if re.search(r"SystemSleepConfiguration\s+$", scrubbed[:m.start()]):
                    continue  # a declaration with an initializer, not an assignment
                fail("%s:%d: assignment to SystemSleepConfiguration variable '%s' - "
                     "Device OS's move assignment memcpys over the old "
                     "wakeup_sources list without freeing it"
                     % (rel, line_of(text, m.start()), name))
        for m in decl_re.finditer(scrubbed):
            decl = m.group(0)
            # A file-scope definition has no leading indentation on its line.
            line_start = scrubbed.rfind("\n", 0, m.end()) + 1
            indent = scrubbed[line_start:m.end()]
            indent = indent[:len(indent) - len(indent.lstrip())]
            at_file_scope = scrubbed.count("{", 0, m.start()) == scrubbed.count("}", 0, m.start())
            if at_file_scope and "extern" not in decl:
                fail("%s:%d: file-scope SystemSleepConfiguration object - it is "
                     "never destroyed, so every reset orphans a wake-source list"
                     % (rel, line_of(text, m.start())))


def check_sites(sites):
    """WO Acceptance 3, plus 'each site owns a fresh local'."""
    vars_seen = sites.get("__vars__", [])
    if len(vars_seen) != 4:
        fail("expected exactly 4 System.sleep() sites in State_Sleep.cpp, found %d"
             % len(vars_seen))
    names = [v for v, _ln in vars_seen]
    if len(set(names)) != len(names):
        fail("the sleep sites share a configuration object (%s) - each site must "
             "build and own its own local" % ", ".join(sorted(set(
                 n for n in names if names.count(n) > 1))))

    for tag, expected in EXPECTED_SITES.items():
        actual = sites.get(tag)
        if actual is None:
            fail("sleep site '%s' not found - the %s site must still exist"
                 % (tag, tag))
            continue
        if not actual["decl_local"]:
            fail("sleep site '%s' (line %d): '%s' is not a block-scope local "
                 "SystemSleepConfiguration declaration"
                 % (tag, actual["line"], actual["var"]))
        if actual["mode"] != expected["mode"]:
            fail("sleep site '%s': mode is %s, expected %s"
                 % (tag, actual["mode"], expected["mode"]))
        if sorted(actual["gpio"]) != sorted(expected["gpio"]):
            fail("sleep site '%s': wake pins/edges changed - found %s, expected %s"
                 % (tag, sorted(actual["gpio"]), sorted(expected["gpio"])))
        if actual["duration"] != expected["duration"]:
            fail("sleep site '%s': RTC duration changed - found %r, expected %r"
                 % (tag, actual["duration"], expected["duration"]))
        if actual["network"] != expected["network"]:
            fail("sleep site '%s': network standby changed - found %r, expected %r"
                 % (tag, actual["network"], expected["network"]))
        if expected["network"] is not None and not actual["network_guarded"]:
            fail("sleep site '%s': network standby must stay under "
                 "'if (useNetworkStandby)' and '#if HAL_PLATFORM_CELLULAR'" % tag)


def baseline_sleep_source():
    """State_Sleep.cpp at the WO's baseline commit, or None."""
    try:
        out = subprocess.run(
            ["git", "-C", REPO_ROOT, "show",
             "%s:src/state/State_Sleep.cpp" % BASELINE_COMMIT],
            capture_output=True, check=True)
        return out.stdout.decode("utf-8", "replace")
    except Exception:
        return None


def run_checks(src_root):
    """Run every check against src_root; returns the failure list."""
    global failures
    failures = []
    sleep_src = os.path.join(src_root, "state", "State_Sleep.cpp")
    if not os.path.isfile(sleep_src):
        fail("missing %s" % sleep_src)
        return failures
    check_no_long_lived(src_root)
    with open(sleep_src, "r", errors="replace") as fh:
        sites = extract_sites(fh.read())
    check_sites(sites)
    return failures


def mutate(path, old, new):
    with open(path, "r") as fh:
        text = fh.read()
    if old not in text:
        return False
    with open(path, "w") as fh:
        fh.write(text.replace(old, new, 1))
    return True


def run_mutations():
    """Each mutation is applied to a throwaway copy of src/, never to src/."""
    mutations = [
        ("the removed global is restored",
         "Generalized-Core-Counter.cpp",
         "void outOfMemoryHandler(system_event_t event, int param);",
         "SystemSleepConfiguration config; // Sleep 2.0 configuration\n"
         "void outOfMemoryHandler(system_event_t event, int param);"),
        ("a sleep site is reset by assignment instead of built fresh",
         "state/State_Sleep.cpp",
         "    SystemSleepConfiguration stopConfig;\n    stopConfig.mode(",
         "    SystemSleepConfiguration stopConfig;\n"
         "    stopConfig = SystemSleepConfiguration();\n    stopConfig.mode("),
        ("a configuration is reset by brace assignment after its wake sources are added",
         "state/State_Sleep.cpp",
         "  ulpConfig.duration((uint32_t)wakeInSeconds * 1000UL);",
         "  ulpConfig = {};\n"
         "  ulpConfig.duration((uint32_t)wakeInSeconds * 1000UL);"),
        ("a sleep site is made static (long-lived)",
         "state/State_Sleep.cpp",
         "  SystemSleepConfiguration ulpConfig;",
         "  static SystemSleepConfiguration ulpConfig;"),
        ("a wake edge is changed",
         "state/State_Sleep.cpp",
         ".gpio(intPin, RISING);         // PIR sensor wake",
         ".gpio(intPin, FALLING);        // PIR sensor wake"),
    ]
    ok = True
    for label, rel, old, new in mutations:
        tmp = tempfile.mkdtemp(prefix="sleepcfg-mut-")
        try:
            copy_root = os.path.join(tmp, "src")
            shutil.copytree(os.path.join(REPO_ROOT, "src"), copy_root)
            target = os.path.join(copy_root, rel)
            if not mutate(target, old, new):
                print("  MUTATION SETUP FAILED (%s): anchor not found in %s"
                      % (label, rel))
                ok = False
                continue
            found = run_checks(copy_root)
            if found:
                print("  OK: detected - %s" % label)
            else:
                print("  NOT DETECTED: %s" % label)
                ok = False
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
    return ok


def main():
    src_root = os.environ.get("SLEEP_CONFIG_SRC_ROOT",
                              os.path.join(REPO_ROOT, "src"))
    found = run_checks(src_root)

    sleep_src = os.path.join(src_root, "state", "State_Sleep.cpp")
    if os.path.isfile(sleep_src):
        with open(sleep_src, "r", errors="replace") as fh:
            sites = extract_sites(fh.read())
        print("Sleep sites as found in %s:" % os.path.relpath(sleep_src, REPO_ROOT))
        for tag in ("hibernate", "ulp", "stop-gpio", "stop-timer-only"):
            s = sites.get(tag)
            if s is None:
                print("  %-16s MISSING" % tag)
                continue
            print("  %-16s line %-5d local=%-5s obj=%-18s gpio=%s duration=%s network=%s"
                  % (tag, s["line"], s["decl_local"], s["var"],
                     s["gpio"], s["duration"], s["network"]))
        print("")

    if found:
        for msg in found:
            print("FAILED: %s" % msg, file=sys.stderr)
        sys.exit(1)

    print("OK: no global, static or extern SystemSleepConfiguration in src/, "
          "and nothing assigns to an existing one")
    print("OK: all four sleep sites build and own a fresh local configuration")
    print("OK: wake pins, edges, RTC durations and network standby match "
          "baseline %s" % BASELINE_COMMIT)

    # Cross-check the transcribed expectations against the baseline commit.
    if os.environ.get("SLEEP_CONFIG_SRC_ROOT") is None:
        baseline = baseline_sleep_source()
        if baseline is None:
            print("NOTE: baseline commit %s unreachable - wake sources were "
                  "checked against the transcribed expectations only"
                  % BASELINE_COMMIT)
        else:
            base_sites = extract_sites(baseline)
            drift = []
            for tag, expected in EXPECTED_SITES.items():
                b = base_sites.get(tag)
                if b is None:
                    drift.append("%s: not present at baseline" % tag)
                    continue
                for key in ("mode", "duration", "network"):
                    if b[key] != expected[key]:
                        drift.append("%s.%s: baseline %r vs expected %r"
                                     % (tag, key, b[key], expected[key]))
                if sorted(b["gpio"]) != sorted(expected["gpio"]):
                    drift.append("%s.gpio: baseline %s vs expected %s"
                                 % (tag, sorted(b["gpio"]), sorted(expected["gpio"])))
            if drift:
                for d in drift:
                    print("FAILED: expectations do not match baseline %s - %s"
                          % (BASELINE_COMMIT, d), file=sys.stderr)
                sys.exit(1)
            print("OK: the expectations above were re-derived from %s by the "
                  "same extractor and agree" % BASELINE_COMMIT)

        print("")
        print("--- Mutations (each on a throwaway copy of src/) ---")
        if not run_mutations():
            print("FAILED: a mutation was not detected", file=sys.stderr)
            sys.exit(1)

    print("")
    print("sleep_config_ownership_structural_test passed")


if __name__ == "__main__":
    main()
