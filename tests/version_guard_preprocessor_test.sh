#!/bin/zsh
set -euo pipefail

# WO-2026-11-03-003 Amendment 4/5: the release workflow's source-version guard asks the
# C++ preprocessor for the ACTIVE FIRMWARE_PRODUCT_VERSION, so a commented-out or
# indented decoy cannot fool it. Runs the workflow's own `got=` line (extracted, not copied)
# against fixtures; Stage 7 round 3 (Codex) supplied the decoy.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WF="$ROOT/.github/workflows/build.yml"
fail=0

line="$(grep -F 'got=$(echo FIRMWARE_PRODUCT_VERSION' "$WF" | sed -e 's/^ *//' -e 's/  # .*$//')"
[ -n "$line" ] || { echo "FAIL: guard line not found in build.yml (the sed form must be gone)"; exit 1; }
[ "$(grep -c 'got=$(sed' "$WF")" = 0 ] || { echo "FAIL: the old sed parse is still in build.yml"; fail=1; }

# got <fixture header contents> -> prints what the guard would read
got() {
  local d; d="$(mktemp -d)"; mkdir -p "$d/src"
  printf '%s\n' "$1" > "$d/src/FirmwareVersion.h"
  bash -c "set -euo pipefail; cd '$d'; $line; printf '%s' \"\$got\""
  rm -rf "$d"
}

expect() { # <label> <want> <header>
  local g; g="$(got "$3")"
  [ "$g" = "$2" ] || { echo "FAIL: $1: read '$g', want '$2'"; fail=1; }
}

expect "plain define" 4890 '#define FIRMWARE_PRODUCT_VERSION 4890'
expect "indented define" 4890 ' #define FIRMWARE_PRODUCT_VERSION 4890'
expect "trailing comment" 4890 '#define FIRMWARE_PRODUCT_VERSION 4890 // v48.90'
expect "Codex decoy: commented canonical, indented active" 1 '/*
#define FIRMWARE_PRODUCT_VERSION 4890
*/
 #define FIRMWARE_PRODUCT_VERSION 1'
expect "line-commented canonical, active below" 7 '// #define FIRMWARE_PRODUCT_VERSION 4890
#define FIRMWARE_PRODUCT_VERSION 7'
expect "the repository file" "$(sed -n 's/^#define FIRMWARE_PRODUCT_VERSION *//p' "$ROOT/src/FirmwareVersion.h")" "$(cat "$ROOT/src/FirmwareVersion.h")"

# The decoy must NOT equal the tag's value: that is what makes the guard fail closed.
[ "$(got '/*
#define FIRMWARE_PRODUCT_VERSION 4890
*/
 #define FIRMWARE_PRODUCT_VERSION 1')" != 4890 ] || { echo "FAIL: decoy read as the commented value"; fail=1; }

[ "$fail" = 0 ] && echo "PASS: version_guard_preprocessor_test" || exit 1
