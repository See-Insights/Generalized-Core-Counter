#!/usr/bin/env python3
"""WO-2026-11-11-002: every product default ledger file lists every key the firmware reads.

Compares the (section path, key) pairs read through getMerged*Value in
src/cloud/ConfigApply.cpp with the leaf key paths of each
docs/config/default-settings-*.json (at least one must exist). Presence only,
not values. DEFAULTS_COVERAGE_DIR / DEFAULTS_COVERAGE_SRC override the paths.
"""
import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.environ.get("DEFAULTS_COVERAGE_SRC", os.path.join(ROOT, "src", "cloud", "ConfigApply.cpp"))
CONFIG_DIR = os.environ.get("DEFAULTS_COVERAGE_DIR", os.path.join(ROOT, "docs", "config"))
MIN_KEYS = 25

TOP = re.compile(r'getTopLevelMap\(\s*defaults\s*,\s*"(\w+)"\s*,\s*(\w+)\s*\)')
NESTED = re.compile(r'getNestedMap\(\s*defaults\s*,\s*"(\w+)"\s*,\s*"(\w+)"\s*,\s*(\w+)\s*\)')
MERGED = re.compile(r'getMerged(?:Int|Bool|String|Float)Value\(\s*(\w+)\s*,\s*\w+\s*,\s*"(\w+)"')


def firmware_keys(text):
    sections = {m.group(2): m.group(1) for m in TOP.finditer(text)}
    sections.update({m.group(3): m.group(1) + "." + m.group(2) for m in NESTED.finditer(text)})
    keys = set()
    for m in MERGED.finditer(text):
        if m.group(1) in sections:
            keys.add(sections[m.group(1)] + "." + m.group(2))
    return keys


def leaf_paths(node, prefix=""):
    if not isinstance(node, dict):
        return {prefix}
    out = set()
    for k, v in node.items():
        out |= leaf_paths(v, prefix + "." + k if prefix else k)
    return out


def check_file(path, fw):
    with open(path, encoding="utf-8") as f:
        file_keys = leaf_paths(json.load(f))
    name = os.path.basename(path)
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
    with open(SRC, encoding="utf-8") as f:
        fw = firmware_keys(f.read())
    files = sorted(glob.glob(os.path.join(CONFIG_DIR, "default-settings-*.json")))

    problems = []
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
