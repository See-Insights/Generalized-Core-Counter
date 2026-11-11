#!/usr/bin/env python3
"""WO-2026-11-11-002: the product default ledger file lists every key the firmware reads.

Compares the (section path, key) pairs read through getMerged*Value in
src/cloud/ConfigApply.cpp with the leaf key paths of
docs/config/default-settings-42131.json. Presence only, not values.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.environ.get("DEFAULTS_COVERAGE_SRC", os.path.join(ROOT, "src", "cloud", "ConfigApply.cpp"))
JSON_PATH = os.environ.get("DEFAULTS_COVERAGE_JSON", os.path.join(ROOT, "docs", "config", "default-settings-42131.json"))
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


def main():
    with open(SRC, encoding="utf-8") as f:
        fw = firmware_keys(f.read())
    with open(JSON_PATH, encoding="utf-8") as f:
        file_keys = leaf_paths(json.load(f))

    failed = False
    if len(fw) < MIN_KEYS:
        print("FAIL: parser found only %d distinct firmware keys (floor %d)" % (len(fw), MIN_KEYS))
        failed = True
    missing = sorted(fw - file_keys)
    dead = sorted(file_keys - fw)
    if missing:
        print("FAIL: incomplete default, read by firmware but missing from the file:")
        print("\n".join("  " + k for k in missing))
        failed = True
    if dead:
        print("FAIL: dead field, in the file but not read by firmware:")
        print("\n".join("  " + k for k in dead))
        failed = True
    if failed:
        return 1
    print("PASS: %d firmware keys == %d file keys" % (len(fw), len(file_keys)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
