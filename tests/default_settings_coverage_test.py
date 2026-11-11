#!/usr/bin/env python3
"""WO-2026-11-11-002: every product default ledger file lists every key the firmware reads.

Scans src/**/*.cpp for getMerged{Int,Bool,String,Float}Value( calls and compares
the (section path, key) pairs they read with the leaf key paths of each
docs/config/default-settings-*.json (at least one must exist). Presence only.
DEFAULTS_COVERAGE_DIR / DEFAULTS_COVERAGE_SRC (a source root dir) override paths.

A call has the map variable (a bare identifier) as first argument; a definition
or declaration has a type there (const, & or *, or two words). Every call must
parse into a mapped section variable and a string-literal key, or the run fails
listing file:line.

Documented limits (not fixed): the same section-variable name in separate scopes
maps to one section; commented-out calls are still counted; macro aliases and
spellings the patterns do not match are invisible.
"""
import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.environ.get("DEFAULTS_COVERAGE_SRC", os.path.join(ROOT, "src"))
CONFIG_DIR = os.environ.get("DEFAULTS_COVERAGE_DIR", os.path.join(ROOT, "docs", "config"))
MIN_KEYS = 25

TOP = re.compile(r'getTopLevelMap\(\s*defaults\s*,\s*"(\w+)"\s*,\s*(\w+)\s*\)')
NESTED = re.compile(r'getNestedMap\(\s*defaults\s*,\s*"(\w+)"\s*,\s*"(\w+)"\s*,\s*(\w+)\s*\)')
CALL = re.compile(r'getMerged(?:Int|Bool|String|Float)Value\s*\((?=([^,)]*))')
PARSED = re.compile(r'\(\s*(\w+)\s*,\s*\w+\s*,\s*"(\w+)"')
NOT_A_CALL = re.compile(r'\bconst\b|[&*]|\w\s+\w')


def firmware_keys(texts):
    """texts: {path: source}. Returns (keys, file:line of calls that did not parse)."""
    sections = {}
    for text in texts.values():
        sections.update({m.group(2): m.group(1) for m in TOP.finditer(text)})
        sections.update({m.group(3): m.group(1) + "." + m.group(2) for m in NESTED.finditer(text)})
    keys, unparsed = set(), []
    for path, text in texts.items():
        for m in CALL.finditer(text):
            if NOT_A_CALL.search(m.group(1)):
                continue
            p = PARSED.match(text, m.end() - 1)
            if p and p.group(1) in sections:
                keys.add(sections[p.group(1)] + "." + p.group(2))
            else:
                unparsed.append("%s:%d" % (os.path.relpath(path, SRC), text.count("\n", 0, m.start()) + 1))
    return keys, unparsed


def leaf_paths(node, prefix=""):
    if not isinstance(node, dict):
        return {prefix}
    out = set()
    for k, v in node.items():
        out |= leaf_paths(v, prefix + "." + k if prefix else k)
    return out


def check_file(path, fw):
    name = os.path.basename(path)
    try:
        with open(path, encoding="utf-8") as f:
            file_keys = leaf_paths(json.load(f))
    except (OSError, ValueError) as e:
        return ["FAIL %s: invalid JSON: %s" % (name, e)]
    problems = []
    if len(file_keys) < MIN_KEYS:
        problems.append("FAIL %s: only %d leaf keys (floor %d)" % (name, len(file_keys), MIN_KEYS))
    for label, keys in (("incomplete default, read by firmware but missing from the file", sorted(fw - file_keys)),
                        ("dead field, in the file but not read by firmware", sorted(file_keys - fw))):
        if keys:
            problems.append("FAIL %s: %s:\n%s" % (name, label, "\n".join("  " + k for k in keys)))
    if not problems:
        print("PASS %s: %d firmware keys == %d file keys" % (name, len(fw), len(file_keys)))
    return problems


def main():
    texts = {}
    for path in sorted(glob.glob(os.path.join(SRC, "**", "*.cpp"), recursive=True)):
        with open(path, encoding="utf-8", errors="replace") as f:
            texts[path] = f.read()
    fw, unparsed = firmware_keys(texts)
    files = sorted(glob.glob(os.path.join(CONFIG_DIR, "default-settings-*.json")))

    problems = []
    if unparsed:
        problems.append("FAIL: getMerged*Value calls not parsed into a literal section and key:\n"
                        + "\n".join("  " + u for u in unparsed))
    if len(fw) < MIN_KEYS:
        problems.append("FAIL: parser found only %d distinct firmware keys (floor %d)" % (len(fw), MIN_KEYS))
    if not files:
        problems.append("FAIL: no default-settings-*.json files in " + CONFIG_DIR)
    for path in files:
        problems += check_file(path, fw)
    if problems:
        print("\n".join(problems))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
